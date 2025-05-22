#include <iostream>
#include <cassert>
#include "chargrid.hpp"

void test_initial_zero() {
    CharGrid g(2,3);
    for(size_t r = 0; r < 2; ++r)
        for(size_t c = 0; c < 3; ++c)
            assert(g(r,c) == 0 && "grid should be zero-init");
}

void test_set_get_and_to_string() {
    CharGrid g(2,4);
    g(0,0) = 'H';
    g(0,1) = 'i';
    g(1,3) = '!';
    assert(g(0,0) == 'H');
    assert(g(1,3) == '!');
    auto s = g.to_string();
    // first row "Hi  \n", second row "   !\n"
    assert(s == "Hi  \n   !\n");
}

void test_hash_invalidation() {
    CharGrid g(3,3);
    size_t h0 = g.get_hash();
    g(2,2) = 'A';
    size_t h1 = g.get_hash();
    assert(h0 != h1 && "hash must change when we mutate");
    // copy preserves both data and cache
    CharGrid g2 = g;
    assert(g2 == g);
    assert(g2.get_hash() == g.get_hash());
}

int main() {
    test_initial_zero();
    test_set_get_and_to_string();
    test_hash_invalidation();
    std::cout << "All CharGrid tests passed!\n";
    return 0;
}
