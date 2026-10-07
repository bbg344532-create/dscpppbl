#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <vector>

// Represents one emergency waiting to be served.
// Convention: HIGHER severity value = MORE critical (e.g. Cardiac Arrest = 9, Minor Injury = 2).
// This matches the SeverityTable lookup used when an Emergency is created.
struct EmergencyEntry {
    int emergencyId;
    int severity;
    long waitTimeStamp;   // tiebreaker: earlier timestamp wins when severity is equal

    EmergencyEntry(int id, int sev, long ts)
        : emergencyId(id), severity(sev), waitTimeStamp(ts) {}
};

// Max-Heap based Priority Queue (by severity, earliest-timestamp tiebreak).
// Used by the Dispatcher as follows:
//   - peekTopK(k)  -> look at the most urgent few emergencies without removing any of them
//   - removeById() -> once the Dispatcher picks the best FEASIBLE match (which may not be
//                     the single most severe one, if that one has no available hospital),
//                     remove exactly that entry from the heap
class PriorityQueue {
private:
    std::vector<EmergencyEntry> heap;

    bool hasHigherPriority(int a, int b) const;
    void heapifyUp(int index);
    void heapifyDown(int index);
    void removeAt(int index);
    int findIndexById(int emergencyId) const;

public:
    PriorityQueue();

    void push(const EmergencyEntry& entry);
    EmergencyEntry pop();
    EmergencyEntry peek() const;
    bool isEmpty() const;
    int size() const;

    std::vector<EmergencyEntry> peekTopK(int k) const;
    bool removeById(int emergencyId);
};

#endif // PRIORITY_QUEUE_H
