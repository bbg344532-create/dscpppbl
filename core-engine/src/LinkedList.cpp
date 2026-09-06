#include "LinkedList.h"
#include <iostream>

LinkedList::LinkedList() : head(nullptr), tail(nullptr), count(0) {}

LinkedList::~LinkedList() {
    // TODO: traverse and delete all nodes to avoid memory leaks
}

void LinkedList::append(const DispatchRecord& record) {
    Node* newNode = new Node(record);
    if (!head) {
        head = tail = newNode;
    } else {
        tail->next = newNode;
        tail = newNode;
    }
    count++;
}

void LinkedList::printAll() const {
    Node* current = head;
    while (current) {
        std::cout << "Emergency " << current->data.emergencyId
                  << " -> Ambulance " << current->data.ambulanceId
                  << " -> Hospital " << current->data.hospitalId
                  << " @ " << current->data.timestamp << "\n";
        current = current->next;
    }
}

int LinkedList::size() const {
    return count;
}
