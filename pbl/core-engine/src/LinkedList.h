#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <string>

// A single dispatch record: which ambulance went to which hospital, and when
struct DispatchRecord {
    int emergencyId;
    int ambulanceId;
    int hospitalId;
    std::string timestamp;

    DispatchRecord(int eId, int aId, int hId, const std::string& ts)
        : emergencyId(eId), ambulanceId(aId), hospitalId(hId), timestamp(ts) {}
};

struct Node {
    DispatchRecord data;
    Node* next;
    Node(const DispatchRecord& record) : data(record), next(nullptr) {}
};

// Singly Linked List used to store dispatch history in chronological order,
// and to maintain waitlists of emergencies when no ambulance is free.
class LinkedList {
private:
    Node* head;
    Node* tail;
    int count;

public:
    LinkedList();
    ~LinkedList();

    void append(const DispatchRecord& record);
    void printAll() const;
    int size() const;
};

#endif // LINKED_LIST_H
