#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <vector>

// Represents one emergency waiting to be served
struct EmergencyEntry {
    int emergencyId;
    int severity;    // lower number = more critical (or define your own convention)
    long waitTimeStamp;

    EmergencyEntry(int id, int sev, long ts)
        : emergencyId(id), severity(sev), waitTimeStamp(ts) {}
};

// Min-Heap based Priority Queue.
// Orders pending emergencies by severity (and wait time as tiebreaker)
// so the most critical emergency is always served first.
class PriorityQueue {
private:
    std::vector<EmergencyEntry> heap;

    void heapifyUp(int index);
    void heapifyDown(int index);

public:
    PriorityQueue();

    void push(const EmergencyEntry& entry);
    EmergencyEntry pop();       // removes and returns highest-priority emergency
    EmergencyEntry peek() const;
    bool isEmpty() const;
    int size() const;
};

#endif // PRIORITY_QUEUE_H
