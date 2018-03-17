//
// Created by jslater on 15/03/18.
//

#ifndef MAGN_TREEALLOCATOR_H
#define MAGN_TREEALLOCATOR_H

#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <cassert>
#include <iomanip>
#include <set>
#include <unordered_set>
#include "Queue.h"
#include "Error.h"

//#define NDEBUG

#define freemap_set(p) { \
    size_type i = (uint8 *) (p) - base_; \
    assert(i % Size_ == 0); \
    assert(freeMap_[i / Size_] == nullptr); \
    freeMap_[i / Size_] = (free_node_t *) (p); \
}

#define freemap_clear(p) { \
    size_type i = (uint8 *) (p) - base_; \
    assert(i % Size_ == 0); \
    assert(freeMap_[i / Size_] != nullptr); \
    freeMap_[i / Size_] = nullptr; \
}

namespace magn {

template<typename T>
class ListAllocator {

public:

    /*
     * type definitions
     */

    typedef T value_type;
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef T &reference;
    typedef const T &const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    struct free_node_t {
        free_node_t() : prev_(nullptr), next_(nullptr), size_(0) {}

        explicit free_node_t(size_type size) : prev_(nullptr), next_(nullptr), size_(size) {}

        free_node_t *prev_;
        free_node_t *next_;
        size_type size_;
    };

    ListAllocator()
            :
#ifndef NDEBUG
            nonfree_(nullptr),
#endif
            base_(nullptr),

            freeList_(0),
            freeCount_(0),

            freeMap_(nullptr) {

        size_type size = Size_ * Num_;
        size += sizeof(free_node_t *) * Num_;
#ifndef NDEBUG
        size += sizeof(char) * Num_;
#endif

        base_ = (uint8 *) ::malloc(size);
        if (!base_) {

            /*
             * no space
             */
            throw std::bad_alloc();

        }
        memset(base_, 0, size);

        uint8 *p = base_;
        queue_init(&freeList_);
        p += Size_ * Num_;

        freeMap_ = (free_node_t **) p;
        p += sizeof(free_node_t *) * Num_;

#ifndef NDEBUG
        nonfree_ = (char *) p;
        p += sizeof(char) * Num_;
        assert(p == base_ + size);
#endif
    };

    ~ListAllocator() {
        ::free(base_);
        base_ = nullptr;
    }

    // allocate but don't initialize num elements of type T
    pointer allocate(size_type num, const void * = nullptr) {

        pointer ret = removeFree(num);

        std::cout << "allocated " << num << " element(s)"
                  << " of size " << sizeof(T)
                  << " at: [" << (void *) ret << "]"
                  << " dump:" << std::endl;
        printFree();

        return ret;
    }

    // initialize elements of allocated storage p with value value
    void construct(pointer p, const T &value) {
        // initialize memory with placement new
        new((void *) p)T(value);
    }

    // destroy elements of initialized storage p
    void destroy(pointer p) {
        // destroy objects by calling their destructor
        p->~T();
    }

    // deallocate storage p of deleted elements
    void deallocate(pointer p, size_type num) {

        addFree(p, num);

        std::cout << "deallocated " << num << " element(s)"
                  << " of size " << sizeof(T)
                  << " at: [" << (void *) p << "]"
                  << " dump:" << std::endl;
        printFree();

//        defrag();
//
//        std::cout << "defrag dump:" << std::endl;
//        printFree();
    }

private:

#ifndef NDEBUG
    char *nonfree_;
#endif

    uint8 *base_;

    free_node_t freeList_;
    uint32 freeCount_;

    free_node_t **freeMap_;


    pointer removeFree(size_type num, const void * = nullptr) {

        if (queue_empty(&freeList_)) {

            /*
             * add base_ to queue
             */
            auto newNode = new(base_) free_node_t(Size_ * Num_);

            queue_insert_after(&freeList_, newNode);

            freemap_set(freeList_.next_);

            ++freeCount_;
        }

        /*
         * find suitable space
         */

        size_type sizeRequested = Size_ * num;

        free_node_t *node = freeList_.next_;
        free_node_t *last = nullptr;

        size_type i = 0;

        bool found = false;

        while (i++ < freeCount_) {

            if (std::less_equal<size_type>()(sizeRequested, node->size_)) {

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

        free_node_t **ppRemove = nullptr;

        if (last) {

            /*
             * space at node other than head
             */

            ppRemove = &last;

        } else {

            /*
             * space at head
             */

            ppRemove = &freeList_.next_;
        }

        free_node_t *space = *ppRemove;

        if ((*ppRemove)->size_ - sizeRequested < sizeof(free_node_t)) {

            /*
             * space cannot be split up
             */

            queue_dequeue(space);

            if (freeCount_ == 1) {

                /*
                 * used all possible space
                 */

                queue_init(&freeList_);
            }

            --freeCount_;

            freemap_clear(space);

            return (pointer) space;
        }

        /*
         * space can be split up
         */

        uint8 *p = (uint8 *) space + sizeRequested;
        auto newNode = new(p)free_node_t(space->size_ - sizeRequested);

        queue_insert_after(space, newNode);
        ++freeCount_;
        freemap_set(p);

        queue_dequeue(space);
        --freeCount_;
        freemap_clear(space);

        *ppRemove = newNode;

#ifndef NDEBUG
        i = ((uint8 *) space - base_) / Size_;
        assert(i < Num_);
        ++nonfree_[i];
        assert(nonfree_[i] == 1);
#endif

        return (pointer) space;
    }

    void addFree(pointer p, size_type num) {

        if ((uint8 *) p < base_ || (uint8 *) p >= base_ + Size_ * Num_) {

            /*
             * unknown memory
             */

            throw std::bad_alloc();
        }

        size_type sizeReturned = Size_ * num;

        free_node_t *node = freeList_.next_;
        free_node_t *last = nullptr;

        size_type i = 0;

        while (i++ < freeCount_) {

            if (std::less_equal<size_type>()(sizeReturned, node->size_)) {

                break;
            }

            last = node;
            node = node->next_;
        }

        free_node_t **ppNode = nullptr;

        auto newNode = new(p)free_node_t(sizeReturned);

        if (last) {

            /*
             * space at node other than head
             */

            ppNode = &last;

            queue_insert_after(last, newNode);

        } else {

            /*
             * space at head
             */

            ppNode = &freeList_.next_;

            queue_insert_before(freeList_.next_, newNode);
        }

        ++freeCount_;

        freemap_set(p);

        *ppNode = newNode;

#ifndef NDEBUG
        i = ((uint8 *) p - base_) / Size_;
        assert(i < Num_);
        --nonfree_[i];
        assert(nonfree_[i] == 0);
#endif
    }

    void defrag() {
        size_type const size = Size_ * Num_;
        free_node_t *node = freeList_.next_;
        for (int i = 0; i < freeCount_; ++i) {
            uint8 *next = (uint8 *) node + node->size_;
            if (next < base_ + size) {
                size_type mi = (next - base_) / Size_;
                auto n = freeMap_[mi];
                if (n) {

                    // merge

                    queue_dequeue(n);
                    --freeCount_;

                    queue_dequeue(node);
                    --freeCount_;

                    if (freeCount_ == 0) {
                        queue_init(&freeList_);
                    }

                    node->size_ += n->size_;

                    addFree((T *) node, node->size_ / Size_);

                    return;
                }
            }
            node = node->next_;
        }
    }

    void printFree() {
        free_node_t *node = freeList_.next_;
        for (int i = 0; i < freeCount_; ++i) {
            std::cout << "\t[" << (void *) node << "]:" << "l:" << (void *) node->prev_ << "|r:" << (void *) node->next_
                      << "|s:" << node->size_ << " " << std::endl;
            node = node->next_;
        }
        std::cout << std::endl;
    }

    static constexpr size_type Num_ = 4096;

    static constexpr size_type Size_ = sizeof(T);
};

}


#endif //MAGN_TREEALLOCATOR_H
