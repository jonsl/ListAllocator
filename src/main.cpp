//
// Created by jslater on 10/03/18.
//

#define LIST_ARENA_PRINT_DEBUG

#include <random>
#include "container_types.h"


int main(int argc, char const *const *argv) {

// create custom allocator
    via::list_arena<> allocator_1(256 * 1024);

// define base and derived to allocate and store
    class Base {
    public:
        Base(int x, int y) : x_(x), y_(y) {}

        virtual ~Base() = default;

    private:
        int x_;
        int y_;
    };

    class Derived : public Base {
    public:
        Derived(int x, int y, int z) : Base(x, y), z_(z) {}

    private:
        int z_;
    };

// new list of base pointers
    via::list_alloc_vector<Base *> base_list(allocator_1);

// allocate 1 base object and store in list
    decltype(base_list.get_allocator())::rebind<Base>::other base_alloc(allocator_1);

    auto pBase = base_alloc.allocate(1);
    base_alloc.construct(pBase, 12, 15);
    base_list.push_back(pBase);

// allocate 1 derived object and store in list
    decltype(base_list.get_allocator())::rebind<Derived>::other derived_alloc(allocator_1);

    auto pDerived = derived_alloc.allocate(1);
    derived_alloc.construct(pDerived, 1, 2, 3);
    base_list.push_back(pDerived);

// print some results
    for (Base *pb : base_list) {
        std::cout << typeid(*pb).name() << " ";
    }
    std::cout << std::endl << std::flush;

// clean up

    derived_alloc.deallocate(pDerived, 1);

    base_alloc.deallocate(pBase, 1);

    base_list.clear();

// new list of random numbers with same allocator

    via::list_alloc_vector<int> int_list(300, allocator_1);

    std::random_device rd;
    std::mt19937 gen(rd()); //32-bit mersenne_twister_engine
    std::uniform_int_distribution<int> dis(1, 1000);    // 1->1000 inclusive

    // ..generate  a  list  of random  numbers  between  1  and  1000
    std::generate(int_list.begin(), int_list.end(), [&]() { return dis(gen); });

// delete some

    int i;

    auto it = int_list.begin();
    for (i = 0; it != int_list.end() && i < 20; ++i) {
        it = int_list.erase(it);
    }

// add some

    for (i = 0; i < 600; ++i) {
        auto priority = dis(gen);
        int_list.push_back(priority);
    }

    return 0;

// allocator defrag and deallocation of memory
// => back to initial state
}
