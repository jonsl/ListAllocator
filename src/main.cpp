//
// Created by jslater on 10/03/18.
//

//#define RUN_TESTS

#ifndef RUN_TESTS

#include "Core.h"

//#pragma pack(push, 1)

struct A {
    explicit A(int x) : x_(x), y_(0), z_(0) {}

    A(A const &) = default;

    bool operator<(A const &rhs) const {
        return x_ < rhs.x_;
    }

    friend std::ostream &operator<<(std::ostream &out, const A &a);

    long long x_, y_, z_;
};

std::ostream &operator<<(std::ostream &out, const A &a) {
    out << a.x_;
    return out;
}

int main(int argc, char const *const *argv) {
    try {
        magn::master::Options serverOptions(argc, argv);
    } catch (magn::Error &error) {
        std::cout << "exception: " << error.what() << std::endl;
    }

//    magn::ThreadPool threadPool("first", 4, 200);

//    auto start = std::chrono::high_resolution_clock::now();
//    std::this_thread::sleep_for(std::chrono::seconds(2));

//    magn::PoolAllocator allocator(32, 10, 2);
//
//    char *mem[300];
//    for (auto &i : mem) {
//        i = static_cast<char *>(allocator.allocate(32));
//        *i = 'n';
//    }
//    for (auto &i : mem) {
//        allocator.addFree(i);
//    }

    magn::ListAllocator<char> allocator;

    char* first = allocator.allocate(300);
    char* second = allocator.allocate(100);
    char* third = allocator.allocate(200);

    allocator.deallocate(first, 300);
    allocator.deallocate(third, 200);
    allocator.deallocate(second, 100);

//    magn::Queue<A> queue;
//
//    A a0(7);
//    A a1(6);
//    A a2(5);
//    A a3(3);
//    A a4(2);
//    A a5(6);
//
//    queue.insertTail(a0);
//    queue.insertTail(a1);
//    queue.insertTail(a2);
//    queue.insertTail(a3);
//    queue.insertTail(a4);
//    queue.insertCompare(a5);
//
//    queue.print();


//    std::unique_ptr<A> a(new A(1));
//    std::unique_ptr<magn::TaskBase> task(new magn::Task<A>(std::move(a), [](std::unique_ptr<A> data) -> int {
//        std::cout << "hello" << std::endl;
//    }));
//    threadPool.post(std::move(task));

//    for (;;);

    return 0;
}

#else

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include "Tests.h"

int main(int argc, char *argv[]) {
    return Catch::Session().run(argc, argv);
}

#endif
