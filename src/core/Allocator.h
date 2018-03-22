//
// Created by jslater on 15/03/18.
//

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <cstddef>
#include <cstdlib>
#include <new>
#include <cstring>
#include <cassert>
#include <iostream>
#include <limits>
#include "Config.h"
#include "List.h"


namespace magn {

template<std::size_t alignment = alignof(std::max_align_t)>
class Allocator {

    struct free_node_t {
        explicit free_node_t(std::size_t size) : next_(nullptr), size_(size) {}

        free_node_t *next_;
        std::size_t size_;
    };

public:

    explicit Allocator(std::size_t size)
            :
            base_(nullptr),

            freeList_(0) {

        std::size_t const offset = alignment - 1;

        base_ = (uint8 *) ::malloc(size * sizeof(free_node_t) + offset);
        if (!base_) {

            /*
             * no space
             */
            throw std::bad_alloc();

        }
        ::memset(base_, 0, size);

        list_init(&freeList_);
        freeList_.size_ = size;

        /*
         * align free list
         */

        auto node = new((uint8 *) (((size_t) base_ + offset) & ~(alignment - 1))) free_node_t(size);

        list_insert_after(&freeList_, node);

#ifndef NDEBUG
        printFree();
#endif
    }

    Allocator(Allocator const &) = delete;

    Allocator &operator=(Allocator const &) = delete;

    ~Allocator() noexcept {
        ::free(base_);
        base_ = nullptr;
    }

    void *
    allocate(std::size_t size) noexcept {

        void *ret = removeFree(size);

#ifndef NDEBUG
        std::cout << "allocated size " << size
                  << " at: [" << (void *) ret << "]"
                  << " dump:" << std::endl;
        printFree();
#endif

        return ret;
    }

    void
    deallocate(void *p, std::size_t size) noexcept {

        addFree(p, size);

#ifndef NDEBUG
        std::cout << "deallocated size " << size
                  << " at: [" << (void *) p << "]"
                  << " dump:" << std::endl;
        printFree();
#endif
    }

private:

    uint8 *base_;

    free_node_t freeList_;

    void *
    removeFree(std::size_t size) noexcept {

        if (list_empty(&freeList_)) {

            /*
             * no space
             */

            return nullptr;
        }

        /*
         * find suitable space
         */

        free_node_t *prev = &freeList_;
        free_node_t *node = freeList_.next_;

        size_t blocks = sizeToBlocks(size);

        do {

            if (node->size_ >= blocks) {

                if (node->size_ == blocks) {

                    /*
                     * remove node
                     */

                    list_dequeue(node, prev);

                } else {

                    /*
                     * split node
                     */

                    node->size_ -= blocks;

                    node += node->size_;
                }

                freeList_.size_ -= blocks;

                return node;
            }

            prev = node;

            node = node->next_;

        } while (node != &freeList_);

        /*
         * no space
         */

        return nullptr;
    }

    void
    addFree(void *p, std::size_t size) noexcept {

        /*
         * insert new in address order
         */

        size_t blocks = sizeToBlocks(size);

        free_node_t *prev = &freeList_;
        free_node_t *node = freeList_.next_;

        while (p > node && node != &freeList_) {

            prev = node;
            node = node->next_;
        }

        if (prev != &freeList_ && prev + prev->size_ == p) {

            /*
             * merge new into previous
             */

            prev->size_ += blocks;

            std::cout << "merge prev" << std::endl;

        } else {

            /*
             * cannot be merged, add new
             */

            auto ins = new(p) free_node_t(blocks);
            list_insert_after(prev, ins);

            prev = ins;

            std::cout << "not merge prev" << std::endl;
        }

        if (node != &freeList_ && prev + prev->size_ == node) {

            /*
             * merge next into previous
             */

            prev->size_ += node->size_;

            list_dequeue(node, prev);

            std::cout << "merge next" << std::endl;
        }

        freeList_.size_ += blocks;
    }

    std::size_t const
    sizeToBlocks(std::size_t size) {
        return (size + sizeof(free_node_t) - 1) / sizeof(free_node_t);
    }

#ifndef NDEBUG

    void
    printFree() noexcept {

        free_node_t *node = &freeList_;
        do {

            std::cout << "\t[" << (void *) node << "->" << (void *) (node + node->size_) << "]:" << "n:"
                      << (void *) node->next_ << "|s:" << node->size_ << " " << std::endl;

            node = node->next_;

        } while (node != &freeList_);
    }

#endif

};

}


#endif //ALLOCATOR_H
