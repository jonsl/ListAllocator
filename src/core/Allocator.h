//
// Created by jslater on 15/03/18.
//

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <cstddef>
#include <cstdlib>
#include <new>
#include <cstring>
#include <cassert>
#include <iostream>
#include "Config.h"
#include "Queue.h"

#define freemap_set(p) { \
    std::size_t i = (uint8 *) (p) - base_; \
    assert(i % Size_ == 0); \
    assert(freeMap_[i / Size_] == nullptr); \
    freeMap_[i / Size_] = (free_node_t *) (p); \
}

#define freemap_clear(p) { \
    std::size_t i = (uint8 *) (p) - base_; \
    assert(i % Size_ == 0); \
    assert(freeMap_[i / Size_] != nullptr); \
    freeMap_[i / Size_] = nullptr; \
}


namespace magn {

template<typename T>
class Allocator {

    struct free_node_t {
        free_node_t() : prev_(nullptr), next_(nullptr), size_(0) {}

        explicit free_node_t(std::size_t size) : prev_(nullptr), next_(nullptr), size_(size) {}

        free_node_t *prev_;
        free_node_t *next_;
        std::size_t size_;
    };

public:

    explicit Allocator(std::size_t num)
            :
#ifndef NDEBUG
            nonfree_(nullptr),
#endif
            base_(nullptr),

            num_(0),

            freeList_(0),

            freeMap_(nullptr) {

        std::size_t total = Size_ * num;
        total += sizeof(free_node_t *) * num;
#ifndef NDEBUG
        total += sizeof(char) * num;
#endif

        base_ = (uint8 *) ::malloc(total);
        if (!base_) {

            /*
             * no space
             */
            throw std::bad_alloc();

        }
        ::memset(base_, 0, total);

        num_ = num;

        queue_init(&freeList_);

        uint8 *p = base_;
        p += Size_ * num_;

        freeMap_ = (free_node_t **) p;
        p += sizeof(free_node_t *) * num_;

#ifndef NDEBUG
        nonfree_ = (char *) p;
        p += sizeof(char) * num_;
        assert(p == base_ + total);
#endif

        initFree();
    }

    Allocator(Allocator const &) = delete;

    Allocator &operator=(Allocator const &) = delete;

    ~Allocator() noexcept {
        ::free(base_);
        base_ = nullptr;
    }

    void *
    allocate(std::size_t n) {
        void *ret = removeFree(n);

#ifndef NDEBUG
        std::cout << "allocated " << n << " element(s)"
                  << " of size " << Size_
                  << " at: [" << (void *) ret << "]"
                  << " dump:" << std::endl;
        printFree();
#endif

        return ret;
    }

    void
    deallocate(void *p, std::size_t n) {
        addFree(p, n);

#ifndef NDEBUG
        std::cout << "deallocated " << n << " element(s)"
                  << " of size " << Size_
                  << " at: [" << (void *) p << "]"
                  << " dump:" << std::endl;
        printFree();
#endif

        coalesceFree();
    }

    size_t getNum() const {
        return num_;
    }

private:

#ifndef NDEBUG
    char *nonfree_;
#endif

    uint8 *base_;

    std::size_t num_;

    free_node_t freeList_;

    free_node_t **freeMap_;


    void initFree() {
        auto newNode = new(base_) free_node_t(Size_ * num_);

#ifndef NDEBUG
        std::cout << "=> inserting into freeList: " << (void *) newNode << ", of size: " << newNode->size_ << std::endl;
#endif
        queue_insert_after(&freeList_, newNode);

        freemap_set(newNode);

        assert((uint8 *) freeList_.next_ == base_);

#ifndef NDEBUG
        printFree();
#endif
    }

    void *removeFree(std::size_t n) {

        if (queue_empty(&freeList_)) {

            /*
             * no space
             */

            throw std::bad_alloc();
        }

        /*
         * find suitable space
         */

        std::size_t sizeRequested = Size_ * n;

        free_node_t const *const q = &freeList_;

        free_node_t *node = freeList_.next_;
        free_node_t *last = nullptr;

        std::size_t i = 0;

        bool found = false;

        while (node != q) {

            if (sizeRequested <= node->size_) {

                found = true;

                break;
            }

            last = node;
            node = node->next_;
        }

        if (!found) {

            /*
             * no space
             */

            throw std::bad_alloc();
        }

        free_node_t *remove = node;

        if (node->size_ - sizeRequested >= sizeof(free_node_t)) {

            /*
             * space can be split up
             */

            free_node_t *prev = nullptr;

            if (last) {

                prev = last;

            } else {

                prev = &freeList_;

            }

            uint8 *p = (uint8 *) remove + sizeRequested;

            auto newNode = new(p)free_node_t(remove->size_ - sizeRequested);

#ifndef NDEBUG
            std::cout << "=> inserting: " << (void *) newNode << ", of size: " << newNode->size_ << std::endl;
#endif

            queue_insert_after(prev, newNode);

            freemap_set(p);
        }

#ifndef NDEBUG
        std::cout << "=> removing: " << (void *) remove << ", of size: " << sizeRequested << std::endl;
#endif

        queue_dequeue(remove);

        freemap_clear(remove);

#ifndef NDEBUG
        i = ((uint8 *) remove - base_) / Size_;
        assert(i < num_);
        ++nonfree_[i];
        assert(nonfree_[i] == 1);
#endif

        return (void *) remove;
    }

    void addFree(void *p, std::size_t num) {

        if ((uint8 *) p < base_ || (uint8 *) p >= base_ + Size_ * num_) {

            /*
             * unknown memory
             */

            throw std::bad_alloc();
        }

        std::size_t const sizeReturned = Size_ * num;

        /*
         * insert new after before
         */

        auto newNode = new(p)free_node_t(sizeReturned);

        queue_insert_after(&freeList_, newNode);

        freemap_set(p);

#ifndef NDEBUG
        std::size_t i = ((uint8 *) p - base_) / Size_;
        assert(i < num_);
        --nonfree_[i];
        assert(nonfree_[i] == 0);
#endif
    }

    void coalesceFree() {

        if (queue_empty(&freeList_)) {
            return;
        }

        free_node_t const *const q = &freeList_;

        /*
         * merge adjacent nodes
         */

        free_node_t *node = freeList_.next_;

        while (node != q) {

            uint8 *next = (uint8 *) node + node->size_;
            if (next < base_ + Size_ * num_) {

                auto n = freeMap_[(next - base_) / Size_];
                if (n) {

                    /*
                     * n is directly after node
                     * => abosorb n into node
                     */

                    node->size_ += n->size_;

                    queue_dequeue(n);

                    freemap_clear(n);

                }
            }
            node = node->next_;
        }

        /*
         * insertion sort
         */

        node = freeList_.next_->next_;

        while (node != q) {

            free_node_t *prev = node->prev_;

            queue_dequeue(node);

            do {
                if (prev->size_ <= node->size_) {
                    break;
                }

                prev = prev->prev_;

            } while (prev != q);

            queue_insert_after(prev, node);

            node = node->next_;
        }

#ifndef NDEBUG
        std::cout << "coalesceFree dump:" << std::endl;
        printFree();
#endif
    }

#ifndef NDEBUG

    void printFree() {

        std::size_t totalFree = 0;

        free_node_t *node = freeList_.next_;

        free_node_t const *const q = &freeList_;

        while (node != q) {

            std::cout << "\t[" << (void *) node << "]:" << "p:" << (void *) node->prev_ << "|n:" << (void *) node->next_
                      << "|s:" << node->size_ << " " << std::endl;

            totalFree += node->size_;

            node = node->next_;
        }

        std::cout << "\ttotal free: " << totalFree << std::endl;
    }

#endif

    static constexpr std::size_t Size_ = sizeof(T) > sizeof(free_node_t) ? sizeof(T) : sizeof(free_node_t);
};

}


#endif //ALLOCATOR_H
