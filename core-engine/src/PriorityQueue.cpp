#include "PriorityQueue.h"
#include <stdexcept>

PriorityQueue::PriorityQueue() {}

void PriorityQueue::heapifyUp(int index) {
    // TODO: bubble the element at 'index' up until heap property (by severity) is restored
}

void PriorityQueue::heapifyDown(int index) {
    // TODO: bubble the element at 'index' down until heap property is restored
}

void PriorityQueue::push(const EmergencyEntry& entry) {
    heap.push_back(entry);
    heapifyUp(heap.size() - 1);
}

EmergencyEntry PriorityQueue::pop() {
    if (heap.empty()) {
        throw std::runtime_error("PriorityQueue is empty - no emergencies pending");
    }
    EmergencyEntry top = heap.front();
    heap[0] = heap.back();
    heap.pop_back();
    if (!heap.empty()) {
        heapifyDown(0);
    }
    return top;
}

EmergencyEntry PriorityQueue::peek() const {
    if (heap.empty()) {
        throw std::runtime_error("PriorityQueue is empty");
    }
    return heap.front();
}

bool PriorityQueue::isEmpty() const {
    return heap.empty();
}

int PriorityQueue::size() const {
    return static_cast<int>(heap.size());
}
