#include "hash_map.h"

#include <iostream>
#include <cassert>

int main() {
    lock_based::hash_map<std::string, int> hash;

    hash.insert("One", 1);
    hash.insert("Two", 2);

    assert(hash.size() == 2);

    int val;
    assert(hash.get("One", val) == true);
    assert(val == 1);

    assert(hash.get("Two", val) == true);
    assert(val == 2);


    hash.insert("Three", 3);
    hash.insert("Four", 4);

    assert(hash.size() == 4);

    assert(hash.get("One", val) == true);
    assert(val == 1);

    assert(hash.get("Two", val) == true);
    assert(val == 2);

    assert(hash.get("Three", val) == true);
    assert(val == 3);

    assert(hash.get("Four", val) == true);
    assert(val == 4);


    hash.insert("Five", 5);
    hash.insert("Six", 6);

    assert(hash.size() == 6);

    assert(hash.get("One", val) == true);
    assert(val == 1);

    assert(hash.get("Two", val) == true);
    assert(val == 2);

    assert(hash.get("Three", val) == true);
    assert(val == 3);

    assert(hash.get("Four", val) == true);
    assert(val == 4);

    assert(hash.get("Five", val) == true);
    assert(val == 5);

    assert(hash.get("Six", val) == true);
    assert(val == 6);

    return 0;
}
