#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <string>
#include <vector>
#include <list>
#include <utility>

// Generic Hash Table with chaining for collision resolution.
// Used for fast O(1) average lookup of ambulances/hospitals by ID.
template <typename ValueType>
class HashTable {
private:
    std::vector<std::list<std::pair<int, ValueType>>> buckets;
    int capacity;
    int count;

    int hashFunction(int key) const;

public:
    explicit HashTable(int cap = 101);

    void insert(int key, const ValueType& value);
    bool find(int key, ValueType& outValue) const;
    void remove(int key);
    int size() const;
};

#include "HashTable.cpp" // template implementation included here (common pattern for template classes)

#endif // HASH_TABLE_H
