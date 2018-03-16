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
#include "Error.h"

//#define NDEBUG

namespace magn {

template<typename T>
class ListAllocator {
public:
    // type definitions
    typedef T value_type;
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef T &reference;
    typedef const T &const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    struct node_t {
        node_t() : prev_(nullptr), next_(nullptr), size_(0) {}

        explicit node_t(size_type size) : prev_(nullptr), next_(nullptr), size_(size) {}

        node_t *prev_;
        node_t *next_;
        size_type size_;
    };

#define queue_init(q) \
    (q)->prev_ = (q); \
    (q)->next_= (q)

#define queue_empty(q) \
    (q) == (q)->prev_

#define queue_replace(n, i) \
    (i)->prev_ = (n)->prev_; \
    (i)->prev_->next_ = (i); \
    (i)->next_ = (n)->next_; \
    (i)->next_->prev_ = (i)

#define queue_dequeue(n) \
    (n)->prev_->next_ = (n)->next_; \
    (n)->next_->prev_ = (n)->prev_;

#define queue_insert_after(n, i) \
    (i)->next_ = (n)->next_; \
    (i)->next_->prev_ = (i); \
    (i)->prev_ = (n); \
    (n)->next_ = (i)

#define queue_insert_before(n, i) \
    (i)->next_ = (n); \
    (i)->next_->prev_ = (i); \
    (i)->prev_ = (n)->prev_; \
    (i)->prev_->next_ = (n)

#define queue_swap(n1, n2) {\
    node_t* temp = (n1); \
    (n1)->prev_ = (n2)->prev_; \
    (n1)->next_ = (n2)->next_; \
    (n2)->prev_ = temp->prev_; \
    (n2)->next_ = temp->next_; \
}

#define freemap_store(p) { \
    size_type i = (uint8 *) p - base_; \
    assert(i % sizeof(T) == 0); \
    freeMap_[i / sizeof(T)] = (node_t *) p; \
}

    ListAllocator()
            : freeList_(nullptr),
              freeMap_(nullptr),
#ifndef NDEBUG
              nonfree_(nullptr),
#endif
              freeCount_(0) {

        size_type size = sizeof(T) * Num;
        size += sizeof(node_t *) * Num;
#ifndef NDEBUG
        size += sizeof(char) * Num;
#endif
        base_ = (uint8 *) malloc(size);
        memset(base_, 0, size);

        uint8 *p = base_;

        freeList_ = new(p) node_t(sizeof(T) * Num);
        queue_init(freeList_);
        ++freeCount_;
        p += sizeof(T) * Num;

        freeMap_ = (node_t **) p;
        p += sizeof(node_t *) * Num;

#ifndef NDEBUG
        nonfree_ = (char *) p;
        p += sizeof(char) * Num;
        assert(p == base_ + size);
#endif
    };

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
    }

private:

    uint8 *base_;

    node_t *freeList_;
    uint32 freeCount_;

    node_t **freeMap_;

#ifndef NDEBUG
    char *nonfree_;
#endif

    pointer removeFree(size_type num, const void * = nullptr) {

        if (!freeList_) {

            throw ListAllocatorUnderflowError();

        } else {

            size_type sizeRequested = sizeof(T) * num;

            node_t *node = freeList_;
            node_t *last = nullptr;

            for (int i = 0; i < freeCount_
                            && std::greater_equal<size_type>()(sizeRequested, node->size_); ++i) {
                last = node;
                node = node->next_;
            }

            node_t *removed = nullptr;

            if (last) {
                // remove

                queue_dequeue(last);
                --freeCount_;

                removed = last;

                if (removed->size_ - sizeRequested >= sizeof(node_t)) {

                    // split

                    uint8 *p = (uint8 *) removed + sizeRequested;
                    auto newNode = new(p)node_t(removed->size_ - sizeRequested);

                    queue_replace(removed, newNode);
                    ++freeCount_;

                    freemap_store(p);
                }

            } else {
                // head

                // remove

                queue_dequeue(freeList_);
                --freeCount_;

                removed = freeList_;
                freeList_ = nullptr;

                if (removed->size_ - sizeRequested >= sizeof(node_t)) {

                    // split

                    uint8 *p = (uint8 *) removed + sizeRequested;
                    auto newNode = new(p)node_t(removed->size_ - sizeRequested);

                    queue_replace(removed, newNode);
                    ++freeCount_;

                    freemap_store(p);

                    freeList_ = newNode;
                }

                if (!freeList_) {
                    throw ListAllocatorUnderflowError();
                }
            }
#ifndef NDEBUG
            size_type i = (uint8 *) removed - base_;
            assert(i < Num);
            ++nonfree_[i];
            assert(nonfree_[i] == 1);
#endif

            return (pointer) removed;
        }
    }

    void addFree(pointer p, size_type num) {

        if (!freeList_) {

            throw ListAllocatorUnderflowError();

        } else {

            size_type sizeReturned = sizeof(T) * num;

            node_t *node = freeList_;
            node_t *last = nullptr;
            for (int i = 0; i < freeCount_
                            && std::greater_equal<size_type>()(sizeReturned, node->size_); ++i) {
                last = node;
                node = node->next_;
            }

            if (last) {
                // insert

                node = last;
                auto newNode = new(p)node_t(sizeReturned);

                queue_insert_after(node, newNode);
                ++freeCount_;

                freemap_store(p);

            } else {
                // head

                // insert

                node = freeList_;
                auto newNode = new(p)node_t(sizeReturned);

                queue_insert_before(node, newNode);
                ++freeCount_;

                freemap_store(p);

                freeList_ = newNode;
            }

#ifndef NDEBUG
            size_type i = (uint8 *) p - base_;
            assert(i < Num);
            --nonfree_[i];
            assert(nonfree_[i] == 0);
#endif
        }
    }

    void defrag() {
        size_type const size = Num * sizeof(T);
        node_t *node = freeList_;
        node_t *last = nullptr;
        for (int i = 0; i < freeCount_; ++i) {
            uint8 *next = (uint8 *) node + node->size_;
            if (next < base_ + size) {
            }
            last = node;
            node = node->next_;
        }
    }

    void printFree() {
        node_t *node = freeList_;
        for (int i = 0; i < freeCount_; ++i) {
            std::cout << "\t[" << (void *) node << "]:" << "l:" << (void *) node->prev_ << "|r:" << (void *) node->next_
                      << "|s:" << node->size_ << " " << std::endl;
            node = node->next_;
        }
        std::cout << std::endl;
    }

    static constexpr size_type Num = 4096;
};

}


#endif //MAGN_TREEALLOCATOR_H
