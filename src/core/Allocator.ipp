
#include <cstring>
#include <cassert>
#include <iostream>
#include "List.h"

namespace via {


template<std::size_t alignment>
Allocator<alignment>::Allocator(std::size_t size)
        :
        base_(nullptr),

        size_(0),

        freeList_(0) {

    std::size_t const offset = alignment - 1;

    std::size_t blocks = sizeToBlocks(size);

    base_ = (uint8 *)
            ::malloc(blocks * sizeof(free_node_t) + offset);

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
     * align the free list
     */
    auto alignedBase = getAlignedBase();

    auto node = new(alignedBase) free_node_t(blocks);

    list_insert_after(&freeList_, node);

#ifndef NDEBUG
    printFree();
#endif
    assert(checkIntegrity());
}

template<std::size_t alignment>
Allocator<alignment>::~Allocator() noexcept {
    ::free(base_);
    base_ = nullptr;
}

template<std::size_t alignment>
void *
Allocator<alignment>::allocate(std::size_t size) noexcept {

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

template<std::size_t alignment>
void
Allocator<alignment>::deallocate(void *p, std::size_t size) noexcept {

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

template<std::size_t alignment>
std::size_t
Allocator<alignment>::getFree() const noexcept {
    return freeList_.size_;
}

template<std::size_t alignment>
std::size_t
Allocator<alignment>::getLargestContiguousFree() const noexcept {

    std::size_t largestContiguousFree = 0;

    free_node_t *node = freeList_.next_;

    while (node != &freeList_) {

        if (node->size_ > largestContiguousFree) {
            largestContiguousFree = node->size_;
        }
    }

    return largestContiguousFree * sizeof(free_node_t);
}

template<std::size_t alignment>
void *
Allocator<alignment>::removeFree(std::size_t size) noexcept {

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

template<std::size_t alignment>
void
Allocator<alignment>::addFree(void *p, std::size_t size) {

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

template<std::size_t alignment>
std::size_t const
Allocator<alignment>::sizeToBlocks(std::size_t size) const noexcept {
    return (size + sizeof(free_node_t) - 1) / sizeof(free_node_t);
}

template<std::size_t alignment>
uint8 *
Allocator<alignment>::getAlignedBase() const noexcept {
    return (uint8 *) (((size_t) base_ + (alignment - 1)) & ~(alignment - 1));
}

template<std::size_t alignment>
bool
Allocator<alignment>::checkValidPointer(void *p, std::size_t size) const noexcept {

    auto alignedBase = getAlignedBase();

    return (alignedBase <= (uint8 *) p && (uint8 *) p + size <= alignedBase + size_);
}

#ifndef NDEBUG

template<std::size_t alignment>
bool
Allocator<alignment>::checkIntegrity() const noexcept {

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

template<std::size_t alignment>
void
Allocator<alignment>::printFree() noexcept {

    free_node_t *node = &freeList_;
    do {

        std::cout << "\t[" << (void *) node << "->" << (void *) (node + node->size_) << "]:" << "n:"
                  << (void *) node->next_ << "|s:" << node->size_ << std::endl;

        node = node->next_;

    } while (node != &freeList_);
}

#endif


} /*namespace via*/
