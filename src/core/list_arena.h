//
// Created by jslater on 15/03/18.
//

#ifndef LIST_ARENA_H
#define LIST_ARENA_H

#include "config.h"


namespace via {

/// An allocator that uses a single-linked list structure to manage memory.
/// Memory is allocated in multiples of blocks of sizeof(freelist_t) to the nearest alignment bytes
/// \tparam alignment alignment of the allocations
template<size_t _Align = alignof(std::max_align_t)>
class _VIA_DECL list_arena {

    struct freelist_t {
        explicit freelist_t(size_t size)
                : next_(nullptr), size_(size) {}

        freelist_t *next_;
        size_t size_;
    };

public:
    static auto constexpr alignment = _Align;

public:

    explicit
    list_arena(size_t size);

    list_arena(list_arena const &) = delete;

    list_arena &operator=(list_arena const &) = delete;

    ~list_arena() _VIA_NOEXCEPT;

    /// remove a block of memory from the free list
    /// \param size number of bytes to allocate
    /// \return
    void *
    allocate(size_t size) _VIA_NOEXCEPT;

    /// add a previously removed block of memory back to the free list
    /// \param p the pointer to the memory
    /// \param size the size of the memory in bytes
    void
    deallocate(void *p, size_t size);

    /// get the number of bytes remaining in the free list
    /// \return size in bytes
    size_t
    get_free() const _VIA_NOEXCEPT;

    /// get the largest contiguous free block
    /// \return size in bytes
    size_t
    get_largest_contiguous_free() const _VIA_NOEXCEPT;

private:
    void *
    remove_free(size_t size) _VIA_NOEXCEPT;

    void
    add_free(void *p, size_t size);

    size_t const
    size_to_blocks(size_t size) const _VIA_NOEXCEPT;

    size_t const
    blocks_to_alignment(size_t blocks) const _VIA_NOEXCEPT;

    bool
    check_valid_pointer(void *p, size_t size) const _VIA_NOEXCEPT;

#ifndef NDEBUG

    bool
    check_integrity() const _VIA_NOEXCEPT;

    void
    print_free() _VIA_NOEXCEPT;

#endif

private:

    uint8 *base_;

    size_t size_;

    freelist_t free_list_;
};

}

#include "list_arena.ipp"

#endif //LIST_ARENA_H
