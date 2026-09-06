#include "HashTable.h"

template <typename ValueType>
HashTable<ValueType>::HashTable(int cap) : capacity(cap), count(0) {
    buckets.resize(capacity);
}

template <typename ValueType>
int HashTable<ValueType>::hashFunction(int key) const {
    return key % capacity;
}

template <typename ValueType>
void HashTable<ValueType>::insert(int key, const ValueType& value) {
    // TODO: compute bucket index, check for existing key (update) else push_back new pair
}

template <typename ValueType>
bool HashTable<ValueType>::find(int key, ValueType& outValue) const {
    // TODO: compute bucket index, search chain for matching key, copy into outValue if found
    return false;
}

template <typename ValueType>
void HashTable<ValueType>::remove(int key) {
    // TODO: compute bucket index, erase matching entry from chain
}

template <typename ValueType>
int HashTable<ValueType>::size() const {
    return count;
}
