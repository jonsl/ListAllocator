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


//    magn::ListAllocator<std::string> stringAlloc;
////    std::cout << "stringAlloc.max_size(): " << stringAlloc.max_size() << std::endl;
//
//    std::string* myString = stringAlloc.allocate(3);
//
//    stringAlloc.construct(myString, "Hello");
//    stringAlloc.construct(myString + 1, "World");
//    stringAlloc.construct(myString + 2, "!");
//
//    std::cout << myString[0] << " " << myString[1] << " " << myString[2] << std::endl;
//
//    stringAlloc.destroy(myString);
//    stringAlloc.destroy(myString + 1);
//    stringAlloc.destroy(myString + 2);
//    stringAlloc.deallocate(myString, 3);


    magn::ListAllocator<magn::Task<int>> allocator;

    std::size_t sizeofAllocator = sizeof(allocator);

    std::vector<magn::Task<int>, magn::ListAllocator<magn::Task<int>>> c(allocator);

    // add some

    int i;
    for (i = 0; i < 300; ++i) {

        uint32 priority = (uint32) std::rand();

        magn::Task<int> t(priority, 25, [](int const &data) -> int {

            std::cout << "data: " << data << std::endl;
        });

        c.push_back(t);

    }

    // delete some

    auto it = c.begin();
    for (i = 0; it != c.end() && i < 20; ++i) {

        it = c.erase(it);

    }

    // add some

    for (i = 0; i < 600; ++i) {

        uint32 priority = (uint32) std::rand();

        magn::Task<int> t(priority, 25, [](int const &data) -> int {

            std::cout << "data: " << data << std::endl;
        });

        c.push_back(t);

    }




//    c.emplace_back(magn::Task<int>(100, 25, [](int const &data) -> int {
//        std::cout << "data: " << data << std::endl;
//    }));
//    c.emplace_back(magn::Task<int>(200, 10, [](int const &data) -> int {
//        std::cout << "data: " << data << std::endl;
//    }));
//    c.emplace_back(magn::Task<int>(50, 91, [](int const &data) -> int {
//        std::cout << "data: " << data << std::endl;
//    }));
//    c.emplace_back(magn::Task<int>(125, 307, [](int const &data) -> int {
//        std::cout << "data: " << data << std::endl;
//    }));



//    magn::TaskBase* first = allocator.allocate(300);
//    magn::TaskBase* second = allocator.allocate(100);
//    magn::TaskBase* third = allocator.allocate(200);
//
//    allocator.deallocate(first, 300);
//    allocator.deallocate(third, 200);
//    allocator.deallocate(second, 100);

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
#include <test/catch.hpp>

#include "test/unitTests/UnitTests.h"

int main(int argc, char *argv[]) {
    return Catch::Session().run(argc, argv);
}

#endif
