#include "hash_map.h"

int main() {
    lock_based::hash_map<std::string, int> hash;

    hash.insert("Yury", 29);
    hash.insert("Vasya", 34);

    return 0;
}
