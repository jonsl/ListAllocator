//
// Created by jslater on 10/03/18.
//

#include "Options.h"
#include "Error.h"
#include "ThreadPool.h"
#include "PoolAllocator.h"

//#pragma pack(push, 1)

int main(int argc, char const *const *argv) {
    try {
        magn::master::Options serverOptions(argc, argv);
    } catch (magn::Error &error) {
        std::cout << "exception: " << error.what() << std::endl;
    }

    magn::ThreadPool threadPool("first", 4, 200);

    auto start = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(std::chrono::seconds(2));

    magn::PoolAllocator allocator(32, 10, 2);

    char *mem[300];
    for (auto &i : mem) {
        i = static_cast<char *>(allocator.allocate(32));
        *i = 'n';
    }
    for (auto &i : mem) {
        allocator.free(i);
    }


//    std::shared_ptr<char*> data(new char*);
//    std::unique_ptr<magn::TaskBase> task(new magn::Task<std::unique_ptr<char*> >(data, [](std::unique_ptr<char*>) -> int {
//        std::cout << "hello" << std::endl;
//    }));
//    threadPool.post(std::move(task));
//
//    for (;;);

    return 0;
}
