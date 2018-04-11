
#include <cstring>
#include <cassert>
#include <iostream>
#include "list.h"

namespace via {


template<size_t _Align>
list_arena<_Align>::list_arena(size_t size)
        :
        base_(nullptr),

        size_(0),

        free_list_(0) {

    size_t blocks = size_to_blocks(size);

    size_t const total_size = alignment * blocks_to_alignment(blocks);

    base_ = (uint8 *) ::aligned_alloc(alignment, total_size);

    if (!base_) {

        /*
         * no space
         */
        throw std::bad_alloc();

    }
    ::memset(base_, 0, total_size);

    /*
     * recompute number of blocks since we may have
     * more after alignment calculation
     */
    blocks = size_to_blocks(total_size);

    size_ = blocks * sizeof(freelist_t);

    slist_init(&free_list_);
    free_list_.size_ = size_;

    /*
     * base_ is aligned
     */
    auto node = new(base_) freelist_t(blocks);

    slist_insert_after(&free_list_, node);

#ifndef NDEBUG
    print_free();
#endif
    assert(check_integrity());
}

template<size_t _Align>
list_arena<_Align>::~list_arena() _VIA_NOEXCEPT {
    ::free(base_);
    base_ = nullptr;
}

template<size_t _Align>
void *
list_arena<_Align>::allocate(size_t size) _VIA_NOEXCEPT {

    void *ret = remove_free(size);

    assert(check_integrity());

    if (ret) {
#ifndef NDEBUG
        std::cout << "allocated size " << size
                  << " at: [" << (void *) ret << "]"
                  << " dump:" << std::endl;
        print_free();
#endif
    }

    return ret;
}

template<size_t _Align>
void
list_arena<_Align>::deallocate(void *p, size_t size) {

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
    print_free();
#endif
}

template<size_t _Align>
size_t
list_arena<_Align>::get_free() const _VIA_NOEXCEPT {
    return free_list_.size_;
}

template<size_t _Align>
size_t
list_arena<_Align>::get_largest_contiguous_free() const _VIA_NOEXCEPT {

    size_t largest_contiguous_free = 0;

    freelist_t *node = free_list_.next_;

    while (node != &free_list_) {

        if (node->size_ > largest_contiguous_free) {
            largest_contiguous_free = node->size_;
        }
    }

    return largest_contiguous_free * sizeof(freelist_t);
}

template<size_t _Align>
void *
list_arena<_Align>::remove_free(size_t size) _VIA_NOEXCEPT {

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

template<size_t _Align>
void
list_arena<_Align>::add_free(void *p, size_t size) {

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

template<size_t _Align>
size_t const
list_arena<_Align>::size_to_blocks(size_t size) const _VIA_NOEXCEPT {
    return (size + sizeof(freelist_t) - 1) / sizeof(freelist_t);
}

template<size_t _Align>
size_t const
list_arena<_Align>::blocks_to_alignment(size_t blocks) const _VIA_NOEXCEPT {
    return (blocks * sizeof(freelist_t) + alignment - 1) / alignment;
}

template<size_t _Align>
bool
list_arena<_Align>::check_valid_pointer(void *p, size_t size) const _VIA_NOEXCEPT {
    return (base_ <= (uint8 *) p && (uint8 *) p + size <= base_ + size_);
}

#ifndef NDEBUG

template<size_t _Align>
bool
list_arena<_Align>::check_integrity() const _VIA_NOEXCEPT {

    size_t total = 0;
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

template<size_t _Align>
void
list_arena<_Align>::print_free() _VIA_NOEXCEPT {

    freelist_t *node = &free_list_;
    do {

        std::cout << "\t[" << (void *) node << "->" << (void *) (node + node->size_) << "]:" << "n:"
                  << (void *) node->next_ << "|s:" << node->size_ << std::endl;

        node = node->next_;

    } while (node != &free_list_);
}

#endif


} /*namespace via*/
