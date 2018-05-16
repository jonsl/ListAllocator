
#include "list_arena.h"
#include "list.h"

namespace via {

template<size_t _Align>
list_arena<_Align>::list_arena(size_t size)
        :
        base_(nullptr),

        size_(0),

        free_list_(0) {

    size = align_up(size);

    size_t blocks = size_to_blocks(size);

    size_t const total_size = sizeof(freelist_t) * blocks;

    base_ = (uint8 *) ::aligned_alloc(alignment, total_size);

    if (!base_) {

        /*
         * no space
         */
        throw std::bad_alloc();

    }
    ::memset(base_, 0, total_size);

    size_ = total_size;

    slist_init(&free_list_);
    free_list_.size_ = size_;

    /*
     * base_ is aligned
     */
    auto node = new(base_) freelist_t(blocks);

    slist_insert_after(&free_list_, node);

#ifdef LIST_ARENA_PRINT_DEBUG
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
#ifdef LIST_ARENA_PRINT_DEBUG
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
list_arena<_Align>::deallocate(void *p, size_t size) _VIA_NOEXCEPT {

    add_free(p, size);

    assert(check_integrity());

#ifdef LIST_ARENA_PRINT_DEBUG
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

    size = align_up(size);

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
list_arena<_Align>::add_free(void *p, size_t size) _VIA_NOEXCEPT {

    assert(check_valid_pointer(p, size) && "invalid pointer passed to add_free");

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

#ifdef LIST_ARENA_PRINT_DEBUG
        std::cout << "merge prev" << std::endl;
#endif

    } else {

        /*
         * cannot be merged, add new
         */

        auto ins = new(p) freelist_t(blocks);
        slist_insert_after(prev, ins);

        prev = ins;

#ifdef LIST_ARENA_PRINT_DEBUG
        std::cout << "not merge prev" << std::endl;
#endif
    }

    if (node != &free_list_ && prev + prev->size_ == node) {

        /*
         * merge next into previous
         */

        prev->size_ += node->size_;

        slist_dequeue(node, prev);

#ifdef LIST_ARENA_PRINT_DEBUG
        std::cout << "merge next" << std::endl;
#endif
    }

    free_list_.size_ += blocks * sizeof(freelist_t);
}

template<size_t _Align>
size_t const
list_arena<_Align>::size_to_blocks(size_t size) const _VIA_NOEXCEPT {
    return (size + sizeof(freelist_t) - 1) / sizeof(freelist_t);
}

template<size_t _Align>
size_t
list_arena<_Align>::align_up(size_t n) const _VIA_NOEXCEPT {
    return (n + (alignment - 1)) & ~(alignment - 1);
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
