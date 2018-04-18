//
// Created by jslater on 19/03/18.
//

#ifndef VIA_LIST_ALLOC_H
#define VIA_LIST_ALLOC_H

#include "list_arena.h"

namespace via {

/// An stl-ready allocator that uses a single-linked structure to manage memory.
/// \tparam T type of base object
/// \tparam _Align alignment of the allocations
template<typename T, size_t _Align = alignof(std::max_align_t)>
class list_alloc {

public:

    using value_type = T;
    using reference = T &;
    using const_reference = T const &;
    using pointer = T *;
    using const_pointer = T const *;

    static auto constexpr alignment = _Align;
    using arena_type = list_arena<alignment>;

private:

    arena_type &a_;

public:

    list_alloc(const list_alloc &) = default;

    list_alloc &operator=(const list_alloc &) = delete;

    list_alloc(arena_type &a) _VIA_NOEXCEPT//NOLINT
            : a_(a) {

    }

    template<class U>
    list_alloc(const list_alloc<U, alignment> &a) _VIA_NOEXCEPT//NOLINT
            : a_(a.a_) {}

    template<class _Up>
    struct rebind {
        using other = list_alloc<_Up, alignment>;
    };

    value_type *allocate(size_t n) {

        auto mem = static_cast<pointer>(a_.allocate(n * sizeof(T)));

        if (!mem) {
            throw std::bad_alloc();
        }

        return mem;
    }

    void deallocate(pointer p, size_t n) _VIA_NOEXCEPT {
        a_.deallocate(p, n * sizeof(T));
    }

    template<class _Up>
    void
    destroy(_Up *p) _VIA_NOEXCEPT {
        p->~_Up();
    }

    template<class T1, size_t A1,
            class U, size_t A2>
    friend
    bool
    operator==(const list_alloc<T1, A1> &x, const list_alloc<U, A2> &y) _VIA_NOEXCEPT;

    template<class U, size_t A> friend
    class list_alloc;

};

template<class T, size_t A1, class U, size_t A2>
inline
bool
operator==(const list_alloc<T, A1> &x, const list_alloc<U, A2> &y) _VIA_NOEXCEPT {
    return A1 == A2 && &x.a_ == &y.a_;
}

template<class T, size_t A1, class U, size_t A2>
inline
bool
operator!=(const list_alloc<T, A1> &x, const list_alloc<U, A2> &y) _VIA_NOEXCEPT {
    return !(x == y);
}

/// use scoped allocator to ensure that all of the elements of a
/// container get their memory from the same source as the container itself
template<typename T, size_t _Align = alignof(std::max_align_t)>
using scoped_list_alloc = std::scoped_allocator_adaptor<list_alloc<T, _Align>>;

}

#endif //VIA_LIST_ALLOC_H
