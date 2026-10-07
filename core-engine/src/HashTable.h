#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <vector>
#include <list>
#include <utility>
#include <functional>

// Generic Hash Table with separate chaining for collision resolution.
// Templated on both KeyType and ValueType so the same class serves two
// different purposes in MedRoute:
//   - HashTable<int, Ambulance*>   -> fast ambulance lookup by ID
//   - HashTable<int, Hospital*>    -> fast hospital lookup by ID
//   - HashTable<std::string, int>  -> SeverityTable (emergency type -> severity score)
template <typename KeyType, typename ValueType>
class HashTable {
private:
    std::vector<std::list<std::pair<KeyType, ValueType>>> buckets;
    int capacity;
    int count;

    int hashFunction(const KeyType& key) const;

public:
    explicit HashTable(int cap = 101);

    void insert(const KeyType& key, const ValueType& value); // insert, or update if key exists
    bool find(const KeyType& key, ValueType& outValue) const;
    bool remove(const KeyType& key);
    bool contains(const KeyType& key) const;
    int size() const;
};

#include "HashTable.cpp" // template implementation included here (standard pattern for template classes)

#endif // HASH_TABLE_H
