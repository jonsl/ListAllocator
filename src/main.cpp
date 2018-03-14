//
// Created by jslater on 10/03/18.
//

//#define RUN_TESTS

#ifndef RUN_TESTS

#include "Core.h"

//#pragma pack(push, 1)

struct A {
    A(int x, int y) : x_(x), y_(y) {}

    int x_, y_;
};

int main(int argc, char const *const *argv) {
    try {
        magn::master::Options serverOptions(argc, argv);
    } catch (magn::Error &error) {
        std::cout << "exception: " << error.what() << std::endl;
    }

    magn::ThreadPool threadPool("first", 4, 200);

    auto start = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(std::chrono::seconds(2));

//    magn::PoolAllocator allocator(32, 10, 2);
//
//    char *mem[300];
//    for (auto &i : mem) {
//        i = static_cast<char *>(allocator.allocate(32));
//        *i = 'n';
//    }
//    for (auto &i : mem) {
//        allocator.free(i);
//    }

    std::unique_ptr<A> a(new A(1, 2));
    std::unique_ptr<magn::TaskBase> task(new magn::Task<A>(std::move(a), [](std::unique_ptr<A> data) -> int {
        std::cout << "hello" << std::endl;
    }));
    threadPool.post(std::move(task));

    for (;;);

    return 0;
}

#else

#define CATCH_CONFIG_RUNNER

#include "Tests.h"

int main(int argc, char *argv[]) {
    return Catch::Session().run(argc, argv);
}

#endif
