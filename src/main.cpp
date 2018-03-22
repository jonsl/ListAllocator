//
// Created by jslater on 10/03/18.
//

#include <core/ListAllocator.h>
#include <vector>


int main(int argc, char const *const *argv) {

    magn::Allocator<> allocator_1(256 * 1024);

//    magn::Allocator<> allocator_2(2048);

    std::vector<int, magn::SA<int>> v(&allocator_1);


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

