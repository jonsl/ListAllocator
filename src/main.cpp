//
// Created by jslater on 10/03/18.
//

#define LIST_ARENA_PRINT_DEBUG

#include "container_types.h"


int main(int argc, char const *const *argv) {

    via::list_arena<> allocator_1(256 * 1024);

    via::list_alloc_vector<int> v(allocator_1);


    int i;
    for (i = 0; i < 300; ++i) {

        auto priority = (int) std::rand();

        v.push_back(priority);

    }

    // delete some

    auto it = v.begin();
    for (i = 0; it != v.end() && i < 20; ++i) {

        it = v.erase(it);

    }

    // add some

    for (i = 0; i < 600; ++i) {

        auto priority = (int) std::rand();

        v.push_back(priority);

    }

    return 0;
}

