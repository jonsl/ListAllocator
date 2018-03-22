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
#include <limits>
#include "Config.h"
#include "List.h"


namespace magn {

template<std::size_t alignment = alignof(std::max_align_t)>
class Allocator {

    struct free_node_t {
        explicit free_node_t(std::size_t size) : next_(nullptr), size_(size) {}

        free_node_t *next_;
        std::size_t size_;
    };

public:

    explicit Allocator(std::size_t size)
            :
            base_(nullptr),

            size_(0),

            freeList_(0) {

        std::size_t const offset = alignment - 1;

        std::size_t blocks = sizeToBlocks(size);

        base_ = (uint8 *) ::malloc(blocks * sizeof(free_node_t) + offset);
        if (!base_) {

            /*
             * no space
             */
            throw std::bad_alloc();

        }
        ::memset(base_, 0, blocks * sizeof(free_node_t) + offset);

        size_ = blocks * sizeof(free_node_t);

        list_init(&freeList_);
        freeList_.size_ = size_;

        /*
         * align free list
         */
        auto alignedBase = getAlignedBase();

        auto node = new(alignedBase) free_node_t(blocks);

        list_insert_after(&freeList_, node);

#ifndef NDEBUG
        printFree();
#endif
        assert(checkIntegrity());
    }

    Allocator(Allocator const &) = delete;

    Allocator &operator=(Allocator const &) = delete;

    ~Allocator() noexcept {
        ::free(base_);
        base_ = nullptr;
    }

    void *
    allocate(std::size_t size) noexcept {

        void *ret = removeFree(size);

        assert(checkIntegrity());

#ifndef NDEBUG
        std::cout << "allocated size " << size
                  << " at: [" << (void *) ret << "]"
                  << " dump:" << std::endl;
        printFree();
#endif

        return ret;
    }

    void
    deallocate(void *p, std::size_t size) noexcept {

        try {

            addFree(p, size);

        } catch (std::bad_alloc &ex) {

            std::cout << "exception: " << ex.what() << std::endl;

        }

        assert(checkIntegrity());

#ifndef NDEBUG
        std::cout << "deallocated size " << size
                  << " at: [" << (void *) p << "]"
                  << " dump:" << std::endl;
        printFree();
#endif
    }

    std::size_t
    getFree() {
        return freeList_.size_;
    }

    std::size_t
    getLargestContiguousFree() {

        std::size_t largestContiguousFree = 0;

        free_node_t *node = freeList_.next_;

        while (node != &freeList_) {

            if (node->size_ > largestContiguousFree) {
                largestContiguousFree = node->size_;
            }
        }

        return largestContiguousFree * sizeof(free_node_t);
    }

private:

    uint8 *base_;

    std::size_t size_;

    free_node_t freeList_;

    void *
    removeFree(std::size_t size) noexcept {

        if (list_empty(&freeList_)) {

            /*
             * no space
             */

            return nullptr;
        }

        /*
         * find suitable space
         */

        free_node_t *prev = &freeList_;
        free_node_t *node = freeList_.next_;

        size_t blocks = sizeToBlocks(size);

        do {

            if (node->size_ >= blocks) {

                if (node->size_ == blocks) {

                    /*
                     * remove node
                     */

                    list_dequeue(node, prev);

                } else {

                    /*
                     * split node
                     */

                    node->size_ -= blocks;

                    node += node->size_;
                }

                freeList_.size_ -= blocks * sizeof(free_node_t);

                return node;
            }

            prev = node;

            node = node->next_;

        } while (node != &freeList_);

        /*
         * no space
         */

        return nullptr;
    }

    void
    addFree(void *p, std::size_t size) {

        if (!checkValidPointer(p, size)) {

            /*
             * returned memory invalid
             */

            throw std::bad_alloc();
        }

        /*
         * insert new in address order
         */

        size_t blocks = sizeToBlocks(size);

        free_node_t *prev = &freeList_;
        free_node_t *node = freeList_.next_;

        while (p > node && node != &freeList_) {

            prev = node;
            node = node->next_;
        }

        if (prev != &freeList_ && prev + prev->size_ == p) {

            /*
             * merge new into previous
             */

            prev->size_ += blocks;

            std::cout << "merge prev" << std::endl;

        } else {

            /*
             * cannot be merged, add new
             */

            auto ins = new(p) free_node_t(blocks);
            list_insert_after(prev, ins);

            prev = ins;

            std::cout << "not merge prev" << std::endl;
        }

        if (node != &freeList_ && prev + prev->size_ == node) {

            /*
             * merge next into previous
             */

            prev->size_ += node->size_;

            list_dequeue(node, prev);

            std::cout << "merge next" << std::endl;
        }

        freeList_.size_ += blocks * sizeof(free_node_t);
    }

    std::size_t const
    sizeToBlocks(std::size_t size) {
        return (size + sizeof(free_node_t) - 1) / sizeof(free_node_t);
    }

    uint8 *getAlignedBase() {
        return (uint8 *) (((size_t) base_ + (alignment - 1)) & ~(alignment - 1));
    }

    bool
    checkValidPointer(void *p, std::size_t size) {

        auto alignedBase = getAlignedBase();

        return (alignedBase <= (uint8 *) p && (uint8 *) p + size <= alignedBase + size_);
    }

#ifndef NDEBUG

    bool
    checkIntegrity() {

        std::size_t total = 0;
        free_node_t *node = freeList_.next_;

        while (node != &freeList_) {

            free_node_t *next = node->next_;

            if ((next != &freeList_ && node >= next) || !checkValidPointer(node, node->size_)) {

                return false;
            }

            total += node->size_ * sizeof(free_node_t);

            node = next;
        }

        return (total == freeList_.size_);
    }

    void
    printFree() noexcept {

        free_node_t *node = &freeList_;
        do {

            std::cout << "\t[" << (void *) node << "->" << (void *) (node + node->size_) << "]:" << "n:"
                      << (void *) node->next_ << "|s:" << node->size_ << " " << std::endl;

            node = node->next_;

        } while (node != &freeList_);
    }

#endif

};

}


#endif //ALLOCATOR_H
