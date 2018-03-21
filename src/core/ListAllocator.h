//
// Created by jslater on 19/03/18.
//

#ifndef LISTALLOCATOR_H
#define LISTALLOCATOR_H

#include <scoped_allocator>
#include "Allocator.h"

namespace magn {

template<typename T>
class ListAllocator {

public:

    template<typename U> friend
    struct ListAllocator;

    using value_type = T;
    using pointer = T *;

    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;

    ListAllocator() = delete;

    explicit ListAllocator(Allocator<> *a)
            :
            allocator_(a) {
    }

    template<typename U>
    ListAllocator(ListAllocator<U> const &rhs) // NOLINT
            :
            allocator_(rhs.allocator_) {}

    ListAllocator(ListAllocator<T> const &rhs)
            :
            allocator_(rhs.allocator_) {}

    pointer allocate(std::size_t n) {
        return static_cast<pointer>(allocator_->allocate(n));//, alignof(T)));
    }

    void deallocate(pointer p, std::size_t n) {
        allocator_->deallocate(p, n);
    }

    template<typename U>
    bool operator==(ListAllocator<U> const &rhs) const {
        return allocator_ == rhs.allocator_;
    }

    template<typename U>
    bool operator!=(ListAllocator<U> const &rhs) const {
        return allocator_ != rhs.allocator_;
    }

private:

    Allocator<> *allocator_;

};

template<typename T>
using SA = std::scoped_allocator_adaptor<ListAllocator<T>>;

}


#endif //LISTALLOCATOR_H
