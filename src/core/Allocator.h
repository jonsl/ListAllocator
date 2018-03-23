//
// Created by jslater on 15/03/18.
//

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <cstddef>
#include "Config.h"


namespace via {

template<std::size_t alignment = alignof(std::max_align_t)>
class Allocator {

    struct free_node_t {
        explicit free_node_t(std::size_t size) : next_(nullptr), size_(size) {}

        free_node_t *next_;
        std::size_t size_;
    };

public:

    explicit
    Allocator(std::size_t size);

    Allocator(Allocator const &) = delete;

    Allocator &operator=(Allocator const &) = delete;

    ~Allocator() noexcept;

    void *
    allocate(std::size_t size) noexcept;

    void
    deallocate(void *p, std::size_t size) noexcept;

    std::size_t
    getFree() const noexcept;

    std::size_t
    getLargestContiguousFree() const noexcept;

private:
    void *
    removeFree(std::size_t size) noexcept;

    void
    addFree(void *p, std::size_t size);

    std::size_t const
    sizeToBlocks(std::size_t size) const noexcept;

    uint8 *getAlignedBase() const noexcept;

    bool
    checkValidPointer(void *p, std::size_t size) const noexcept;

#ifndef NDEBUG

    bool
    checkIntegrity() const noexcept;

    void
    printFree() noexcept;

#endif

private:

    uint8 *base_;

    std::size_t size_;

    free_node_t freeList_;
};

}

#include "Allocator.ipp"

#endif //ALLOCATOR_H
