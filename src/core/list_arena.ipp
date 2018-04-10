
#include <cstring>
#include <cassert>
#include <iostream>
#include "list.h"

namespace via {


template<std::size_t alignment>
list_arena<alignment>::list_arena(std::size_t size)
        :
        base_(nullptr),

        size_(0),

        free_list_(0) {

    std::size_t const offset = alignment - 1;

    std::size_t blocks = size_to_blocks(size);

    base_ = (uint8 *)
            ::malloc(blocks * sizeof(freelist_t) + offset);

    if (!base_) {

        /*
         * no space
         */
        throw std::bad_alloc();

    }
    ::memset(base_, 0, blocks * sizeof(freelist_t) + offset);

    size_ = blocks * sizeof(freelist_t);

    slist_init(&free_list_);
    free_list_.size_ = size_;

    /*
     * align the free list
     */
    auto alignedBase = get_aligned(base_);

    auto node = new(alignedBase) freelist_t(blocks);

    slist_insert_after(&free_list_, node);

#ifndef NDEBUG
    printfree();
#endif
    assert(check_integrity());
}

template<std::size_t alignment>
list_arena<alignment>::~list_arena() _VIA_NOEXCEPT {
    ::free(base_);
    base_ = nullptr;
}

template<std::size_t alignment>
void *
list_arena<alignment>::allocate(std::size_t size) _VIA_NOEXCEPT {

    void *ret = remove_free(size);

    assert(check_integrity());

    if (ret) {
#ifndef NDEBUG
        std::cout << "allocated size " << size
                  << " at: [" << (void *) ret << "]"
                  << " dump:" << std::endl;
        printfree();
#endif
    }

    return ret;
}

template<std::size_t alignment>
void
list_arena<alignment>::deallocate(void *p, std::size_t size) {

    try {

        add_free(p, size);

    } catch (std::bad_alloc &ex) {

        std::cout << "exception: " << ex.what() << std::endl;

    }

    assert(check_integrity());

#ifndef NDEBUG
    std::cout << "deallocated size " << size
              << " at: [" << (void *) p << "]"
              << " dump:" << std::endl;
    printfree();
#endif
}

template<std::size_t alignment>
std::size_t
list_arena<alignment>::get_free() const _VIA_NOEXCEPT {
    return free_list_.size_;
}

template<std::size_t alignment>
std::size_t
list_arena<alignment>::get_largest_contiguous_free() const _VIA_NOEXCEPT {

    std::size_t largestContiguousFree = 0;

    freelist_t *node = free_list_.next_;

    while (node != &free_list_) {

        if (node->size_ > largestContiguousFree) {
            largestContiguousFree = node->size_;
        }
    }

    return largestContiguousFree * sizeof(freelist_t);
}

template<std::size_t alignment>
void *
list_arena<alignment>::remove_free(std::size_t size) _VIA_NOEXCEPT {

    if (slist_empty(&free_list_)) {

        /*
         * no space
         */

        return nullptr;
    }

    /*
     * find suitable space
     */

    freelist_t *prev = &free_list_;
    freelist_t *node = free_list_.next_;

    size_t blocks = size_to_blocks(size);

    do {

        if (node->size_ >= blocks) {

            if (node->size_ == blocks) {

                /*
                 * remove node
                 */

                slist_dequeue(node, prev);

            } else {

                /*
                 * split node
                 */

                node->size_ -= blocks;

                node += node->size_;
            }

            free_list_.size_ -= blocks * sizeof(freelist_t);

            return node;
        }

        prev = node;

        node = node->next_;

    } while (node != &free_list_);

    /*
     * no space
     */

    return nullptr;
}

template<std::size_t alignment>
void
list_arena<alignment>::add_free(void *p, std::size_t size) {

    if (!check_valid_pointer(p, size)) {

        /*
         * returned memory invalid
         */

        throw std::bad_alloc();
    }

    /*
     * insert new in address order
     */

    size_t blocks = size_to_blocks(size);

    freelist_t *prev = &free_list_;
    freelist_t *node = free_list_.next_;

    while (p > node && node != &free_list_) {

        prev = node;
        node = node->next_;
    }

    if (prev != &free_list_ && prev + prev->size_ == p) {

        /*
         * merge new into previous
         */

        prev->size_ += blocks;

        std::cout << "merge prev" << std::endl;

    } else {

        /*
         * cannot be merged, add new
         */

        auto ins = new(p) freelist_t(blocks);
        slist_insert_after(prev, ins);

        prev = ins;

        std::cout << "not merge prev" << std::endl;
    }

    if (node != &free_list_ && prev + prev->size_ == node) {

        /*
         * merge next into previous
         */

        prev->size_ += node->size_;

        slist_dequeue(node, prev);

        std::cout << "merge next" << std::endl;
    }

    free_list_.size_ += blocks * sizeof(freelist_t);
}

template<std::size_t alignment>
std::size_t const
list_arena<alignment>::size_to_blocks(std::size_t size) const _VIA_NOEXCEPT {
    return (size + sizeof(freelist_t) - 1) / sizeof(freelist_t);
}

template<std::size_t alignment>
uint8 *
list_arena<alignment>::get_aligned(uint8 *base) const _VIA_NOEXCEPT {
    return (uint8 *) (((size_t) base + (alignment - 1)) & ~(alignment - 1));
}

template<std::size_t alignment>
bool
list_arena<alignment>::check_valid_pointer(void *p, std::size_t size) const _VIA_NOEXCEPT {

    auto alignedBase = get_aligned(base_);

    return (alignedBase <= (uint8 *) p && (uint8 *) p + size <= alignedBase + size_);
}

#ifndef NDEBUG

template<std::size_t alignment>
bool
list_arena<alignment>::check_integrity() const _VIA_NOEXCEPT {

    std::size_t total = 0;
    freelist_t *node = free_list_.next_;

    while (node != &free_list_) {

        freelist_t *next = node->next_;

        if ((next != &free_list_ && node >= next) || !check_valid_pointer(node, node->size_)) {

            return false;
        }

        total += node->size_ * sizeof(freelist_t);

        node = next;
    }

    return (total == free_list_.size_);
}

template<std::size_t alignment>
void
list_arena<alignment>::printfree() _VIA_NOEXCEPT {

    freelist_t *node = &free_list_;
    do {

        std::cout << "\t[" << (void *) node << "->" << (void *) (node + node->size_) << "]:" << "n:"
                  << (void *) node->next_ << "|s:" << node->size_ << std::endl;

        node = node->next_;

    } while (node != &free_list_);
}

#endif


} /*namespace via*/
