#ifndef HASH_TABLE_CPP
#define HASH_TABLE_CPP

#include "HashTable.h"

template <typename KeyType, typename ValueType>
HashTable<KeyType, ValueType>::HashTable(int cap) : capacity(cap), count(0) {
    buckets.resize(capacity);
}

template <typename KeyType, typename ValueType>
int HashTable<KeyType, ValueType>::hashFunction(const KeyType& key) const {
    std::size_t h = std::hash<KeyType>{}(key);
    return static_cast<int>(h % static_cast<std::size_t>(capacity));
}

template <typename KeyType, typename ValueType>
void HashTable<KeyType, ValueType>::insert(const KeyType& key, const ValueType& value) {
    int idx = hashFunction(key);
    for (auto& entry : buckets[idx]) {
        if (entry.first == key) {
            entry.second = value; // key already exists -> update in place
            return;
        }
    }
    buckets[idx].push_back({key, value});
    count++;
}

template <typename KeyType, typename ValueType>
bool HashTable<KeyType, ValueType>::find(const KeyType& key, ValueType& outValue) const {
    int idx = hashFunction(key);
    for (const auto& entry : buckets[idx]) {
        if (entry.first == key) {
            outValue = entry.second;
            return true;
        }
    }
    return false;
}

template <typename KeyType, typename ValueType>
bool HashTable<KeyType, ValueType>::remove(const KeyType& key) {
    int idx = hashFunction(key);
    auto& chain = buckets[idx];
    for (auto it = chain.begin(); it != chain.end(); ++it) {
        if (it->first == key) {
            chain.erase(it);
            count--;
            return true;
        }
    }
    return false;
}

template <typename KeyType, typename ValueType>
bool HashTable<KeyType, ValueType>::contains(const KeyType& key) const {
    int idx = hashFunction(key);
    for (const auto& entry : buckets[idx]) {
        if (entry.first == key) return true;
    }
    return false;
}

template <typename KeyType, typename ValueType>
int HashTable<KeyType, ValueType>::size() const {
    return count;
}

#endif // HASH_TABLE_CPP
