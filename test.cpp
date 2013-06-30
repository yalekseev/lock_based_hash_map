#include "hash_map.h"

#include <iostream>

int main() {
    lock_based::hash_map<std::string, int> hash;

    hash.insert("Yury", 29);
    hash.insert("Vasya", 34);

    int val;
    hash.get("Yury", val);
    std::cout << val << std::endl;

    hash.get("Vasya", val);
    std::cout << val << std::endl;

    return 0;
}
