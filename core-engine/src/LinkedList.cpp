#include "LinkedList.h"
#include <iostream>

LinkedList::LinkedList() : head(nullptr), tail(nullptr), count(0) {}

LinkedList::~LinkedList() {
    Node* current = head;
    while (current) {
        Node* next = current->next;
        delete current;
        current = next;
    }
    head = tail = nullptr;
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
        std::cout << "  Emergency " << current->data.emergencyId
                  << " -> Ambulance " << current->data.ambulanceId
                  << " -> Hospital " << current->data.hospitalId
                  << " @ t=" << current->data.timestamp << "\n";
        current = current->next;
    }
}

int LinkedList::size() const {
    return count;
}
