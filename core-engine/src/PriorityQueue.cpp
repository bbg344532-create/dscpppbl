#include "PriorityQueue.h"
#include <stdexcept>
#include <algorithm>

PriorityQueue::PriorityQueue() {}

bool PriorityQueue::hasHigherPriority(int a, int b) const {
    if (heap[a].severity != heap[b].severity) {
        return heap[a].severity > heap[b].severity;   // higher severity wins
    }
    return heap[a].waitTimeStamp < heap[b].waitTimeStamp; // earlier timestamp wins on tie
}

void PriorityQueue::heapifyUp(int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (hasHigherPriority(index, parent)) {
            std::swap(heap[index], heap[parent]);
            index = parent;
        } else {
            break;
        }
    }
}

void PriorityQueue::heapifyDown(int index) {
    int n = static_cast<int>(heap.size());
    while (true) {
        int left = 2 * index + 1;
        int right = 2 * index + 2;
        int best = index;

        if (left < n && hasHigherPriority(left, best)) best = left;
        if (right < n && hasHigherPriority(right, best)) best = right;

        if (best != index) {
            std::swap(heap[index], heap[best]);
            index = best;
        } else {
            break;
        }
    }
}

void PriorityQueue::removeAt(int index) {
    int lastIndex = static_cast<int>(heap.size()) - 1;
    if (index < 0 || index > lastIndex) return;

    std::swap(heap[index], heap[lastIndex]);
    heap.pop_back();

    if (index < static_cast<int>(heap.size())) {
        // The swapped-in element might need to move either direction to restore heap order
        heapifyUp(index);
        heapifyDown(index);
    }
}

int PriorityQueue::findIndexById(int emergencyId) const {
    for (int i = 0; i < static_cast<int>(heap.size()); ++i) {
        if (heap[i].emergencyId == emergencyId) return i;
    }
    return -1;
}

void PriorityQueue::push(const EmergencyEntry& entry) {
    heap.push_back(entry);
    heapifyUp(static_cast<int>(heap.size()) - 1);
}

EmergencyEntry PriorityQueue::pop() {
    if (heap.empty()) {
        throw std::runtime_error("PriorityQueue is empty - no emergencies pending");
    }
    EmergencyEntry top = heap.front();
    removeAt(0);
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

std::vector<EmergencyEntry> PriorityQueue::peekTopK(int k) const {
    std::vector<EmergencyEntry> copy(heap);
    std::sort(copy.begin(), copy.end(), [this](const EmergencyEntry& x, const EmergencyEntry& y) {
        if (x.severity != y.severity) return x.severity > y.severity;
        return x.waitTimeStamp < y.waitTimeStamp;
    });
    if (k < static_cast<int>(copy.size())) {
        copy.erase(copy.begin() + k, copy.end()); // trim to top-k without needing a default ctor
    }
    return copy;
}

bool PriorityQueue::removeById(int emergencyId) {
    int idx = findIndexById(emergencyId);
    if (idx == -1) return false;
    removeAt(idx);
    return true;
}
