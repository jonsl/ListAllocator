//
// Created by jslater on 19/03/18.
//

#ifndef LIST_ALLOC_H
#define LIST_ALLOC_H

#include <scoped_allocator>
#include <unordered_map>
#include <vector>
#include <queue>
#include "list_arena.h"


namespace via {

template<class T, std::size_t Align = alignof(std::max_align_t)>
class list_alloc {

public:
    using value_type = T;
    using pointer = T *;
    static auto constexpr alignment = Align;
    using arena_type = list_arena<alignment>;

private:
    arena_type &a_;

public:

    list_alloc(const list_alloc &) = default;

    list_alloc &operator=(const list_alloc &) = delete;

    list_alloc(arena_type &a) noexcept : a_(a) {

    }

    template<class U>
    list_alloc(const list_alloc<U, alignment> &a) noexcept
            : a_(a.a_) {}

    template<class _Up>
    struct rebind {
        using other = list_alloc<_Up, alignment>;
    };

    pointer allocate(std::size_t n) {

        auto mem = static_cast<pointer>(a_.allocate(n * sizeof(T)));

        if (!mem) {
            throw std::bad_alloc();
        }

#ifndef NDEBUG
        std::cout << "allocated: " << n << ", free: " << a_.get_free() << std::endl;
#endif

        return mem;
    }

    void deallocate(pointer p, std::size_t n) {
        a_.deallocate(p, n * sizeof(T));
#ifndef NDEBUG
        std::cout << "deallocated: " << n << ", free: " << a_.get_free() << std::endl;
#endif
    }


    template<class T1, std::size_t A1,
            class U, std::size_t A2>
    friend
    bool
    operator==(const list_alloc<T1, A1> &x, const list_alloc<U, A2> &y) noexcept;

    template<class U, std::size_t A> friend
    class list_alloc;

};

template<class T, std::size_t A1, class U, std::size_t A2>
inline
bool
operator==(const list_alloc<T, A1> &x, const list_alloc<U, A2> &y) noexcept {
    return A1 == A2 && &x.a_ == &y.a_;
}

template<class T, std::size_t A1, class U, std::size_t A2>
inline
bool
operator!=(const list_alloc<T, A1> &x, const list_alloc<U, A2> &y) noexcept {
    return !(x == y);
}

template<typename T>
using scoped_list_alloc = std::scoped_allocator_adaptor<list_alloc<T>>;

template<typename T>
using list_alloc_vector = std::vector<T, scoped_list_alloc<T>>;

template<typename T>
using list_alloc_priority_queue = std::priority_queue<T, list_alloc_vector<T>>;

template<typename T, typename U>
using list_alloc_unordered_map = std::unordered_map<T, U,
        std::hash<T>, std::equal_to<T>, scoped_list_alloc<std::pair<T, U>>>;

}


#endif //LIST_ALLOC_H