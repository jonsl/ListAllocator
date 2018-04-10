//
// Created by jslater on 15/03/18.
//

#ifndef LIST_ARENA_H
#define LIST_ARENA_H

#include <cstddef>
#include "config.h"


namespace via {

template<std::size_t alignment = alignof(std::max_align_t)>
class _VIA_DECL list_arena {

    struct freelist_t {
        explicit freelist_t(std::size_t size)
                : next_(nullptr), size_(size) {}

        freelist_t *next_;
        std::size_t size_;
    };

public:

    explicit
    list_arena(std::size_t size);

    list_arena(list_arena const &) = delete;

    list_arena &operator=(list_arena const &) = delete;

    ~list_arena() _VIA_NOEXCEPT;

    void *
    allocate(std::size_t size) _VIA_NOEXCEPT;

    void
    deallocate(void *p, std::size_t size);

    std::size_t
    get_free() const _VIA_NOEXCEPT;

    std::size_t
    get_largest_contiguous_free() const _VIA_NOEXCEPT;

private:
    void *
    remove_free(std::size_t size) _VIA_NOEXCEPT;

    void
    add_free(void *p, std::size_t size);

    std::size_t const
    size_to_blocks(std::size_t size) const _VIA_NOEXCEPT;

    uint8 *get_aligned(uint8 *base) const _VIA_NOEXCEPT;

    bool
    check_valid_pointer(void *p, std::size_t size) const _VIA_NOEXCEPT;

#ifndef NDEBUG

    bool
    check_integrity() const _VIA_NOEXCEPT;

    void
    printfree() _VIA_NOEXCEPT;

#endif

private:

    uint8 *base_;

    std::size_t size_;

    freelist_t free_list_;
};

}


#include "list_arena.ipp"

#endif //LIST_ARENA_H
