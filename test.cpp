#include "hash_map.h"

#include <iostream>
#include <cassert>
#include <cstdlib>
#include <string>
#include <set>

int main() {
    std::srand(123);

    for (size_t i = 0; i < 10; ++i) {
        lock_based::hash_map<std::string, int> hash;
        std::set<int> expected_values;

        size_t size = std::rand() % 100000 + 20;

        for (size_t j = 0; j < size; ++j) {
            int val = std::rand();
            expected_values.insert(val);
            hash.insert(std::to_string(val), val);
        }

        assert(hash.size() == expected_values.size());
        for (int expected_val : expected_values) {
            int val;
            assert(hash.get(std::to_string(expected_val), val) == true);
            assert(expected_val == val);
        }
    }


    return 0;
}
