//
// Created by jslater on 14/04/18.
//

#ifndef VIA_CONTAINER_TYPES_H
#define VIA_CONTAINER_TYPES_H

#include "list_alloc.h"

namespace via {

/// align conatiner memory to cache line size
using via_list_arena = list_arena<alignof(std::max_align_t)>;

template<typename T>
using via_scoped_list_alloc = scoped_list_alloc<T, alignof(std::max_align_t)>;

/// stl vector using this allocator
template<typename T>
using list_alloc_vector = std::vector<T, via_scoped_list_alloc<T>>;

/// stl deque using this allocator
template<typename T>
using list_alloc_deque = std::deque<T, via_scoped_list_alloc<T>>;

/// stl set using this allocator
template<typename T, typename _Comp = std::less<T>>
using list_alloc_set = std::set<T, _Comp, via_scoped_list_alloc<T>>;

/// stl unordered_map using this allocator
template<typename T, typename U>
using list_alloc_unordered_map = std::unordered_map<T, U,
        std::hash<T>, std::equal_to<T>, via_scoped_list_alloc<std::pair<const T, U>>>;

}

#endif //VIA_CONTAINER_TYPES_H
