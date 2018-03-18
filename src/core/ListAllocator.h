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

    // rebind allocator to type U
    template<class U>
    struct rebind {
        typedef ListAllocator<U> other;
    };

    // return address of values
    pointer address(reference value) const {
        return &value;
    }

    const_pointer address(const_reference value) const {
        return &value;
    }

    /* constructors and destructor
     * - nothing to do because the allocator has no state
     */
    ListAllocator(const ListAllocator &) throw() {


    }

    template<class U>
    ListAllocator(const ListAllocator<U> &) throw() {
    }


    ListAllocator() throw()
//            :
//#ifndef NDEBUG
//            nonfree_(nullptr),
//#endif
//            base_(nullptr),
//
//            freeList_(0),
//
//            freeMap_(nullptr)
    {


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

        queue_init(&freeList_);

        uint8 *p = base_;
        p += Size_ * Num_;

        freeMap_ = (free_node_t **) p;
        p += sizeof(free_node_t *) * Num_;

#ifndef NDEBUG
        nonfree_ = (char *) p;
        p += sizeof(char) * Num_;
        assert(p == base_ + size);
#endif

        initFree();

    }

    ~ListAllocator() throw() {
        ::free(base_);
        base_ = nullptr;
    }

    // return maximum number of elements that can be allocated
    size_type max_size() const throw() {
        return Num_;
    }

    // allocate but don't initialize num elements of type T
    pointer allocate(size_type num, const void * = nullptr) {

        pointer ret = removeFree(num);

#ifndef NDEBUG
        std::cout << "allocated " << num << " element(s)"
                  << " of size " << sizeof(T)
                  << " at: [" << (void *) ret << "]"
                  << " dump:" << std::endl;
        printFree();
#endif

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

#ifndef NDEBUG
        std::cout << "deallocated " << num << " element(s)"
                  << " of size " << sizeof(T)
                  << " at: [" << (void *) p << "]"
                  << " dump:" << std::endl;
        printFree();
#endif

        coalesceFree();
    }

private:

#ifndef NDEBUG
    static char *nonfree_;
#endif

    static uint8 *base_;

    static free_node_t freeList_;

    static free_node_t **freeMap_;


    void initFree() {
        auto newNode = new(base_) free_node_t(Size_ * Num_);

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

    pointer removeFree(size_type num, const void * = nullptr) {

        if (queue_empty(&freeList_)) {

            /*
             * no space
             */

            throw std::bad_alloc();
        }

        /*
         * find suitable space
         */

        size_type sizeRequested = Size_ * num;

        free_node_t const *const q = &freeList_;

        free_node_t *node = freeList_.next_;
        free_node_t *last = nullptr;

        size_type i = 0;

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
        assert(i < Num_);
        ++nonfree_[i];
        assert(nonfree_[i] == 1);
#endif

        return (pointer) remove;
    }

    void addFree(pointer p, size_type num) {

        if ((uint8 *) p < base_ || (uint8 *) p >= base_ + Size_ * Num_) {

            /*
             * unknown memory
             */

            throw std::bad_alloc();
        }

        size_type const sizeReturned = Size_ * num;

        /*
         * insert new after before
         */

        auto newNode = new(p)free_node_t(sizeReturned);

        queue_insert_after(&freeList_, newNode);

        freemap_set(p);

#ifndef NDEBUG
        size_type i = ((uint8 *) p - base_) / Size_;
        assert(i < Num_);
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
            if (next < base_ + Size_ * Num_) {

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

        size_type totalFree = 0;

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

    static constexpr size_type Num_ = 4096;

    static constexpr size_type Size_ = sizeof(T) > sizeof(free_node_t) ? sizeof(T) : sizeof(free_node_t);
};

// return that all specializations of this allocator are interchangeable
template<class T1, class T2>
bool operator==(const ListAllocator<T1> &, const ListAllocator<T2> &) throw() {
    return true;
}

template<class T1, class T2>
bool operator!=(const ListAllocator<T1> &, const ListAllocator<T2> &) throw() {
    return false;
}

#ifndef NDEBUG
template<typename T>
char *ListAllocator<T>::nonfree_ = nullptr;
#endif

template<typename T>
uint8 *ListAllocator<T>::base_ = nullptr;

template<typename T>
typename ListAllocator<T>::free_node_t ListAllocator<T>::freeList_;

template<typename T>
typename ListAllocator<T>::free_node_t **ListAllocator<T>::freeMap_ = nullptr;

}


#endif //MAGN_TREEALLOCATOR_H
