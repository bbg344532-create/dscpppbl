#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdexcept>
#include <string>
#include <sstream>

// MedRoute's own exception hierarchy. Every error derives from MedRouteError,
// which derives from std::runtime_error, so callers can catch as specifically
// or as broadly as they like. what() is a virtual function inherited from
// std::exception, so catching by base-class reference still prints the
// message of the actual (derived) error.
//
//   std::exception
//     -> std::runtime_error
//          -> MedRouteError
//               -> EmptyQueueError
//               -> UnknownLocationError
//               -> DuplicateIdError
class MedRouteError : public std::runtime_error {
public:
    explicit MedRouteError(const std::string& message) : std::runtime_error(message) {}
};

// Thrown when pop() or peek() is called on an empty PriorityQueue.
class EmptyQueueError : public MedRouteError {
public:
    explicit EmptyQueueError(const std::string& message = "PriorityQueue is empty - no emergencies pending")
        : MedRouteError(message) {}
};

// Thrown when an ambulance, hospital or emergency refers to a node that is not in the road network.
class UnknownLocationError : public MedRouteError {
private:
    int node;
    static std::string build(int n) {
        std::ostringstream os;
        os << "Unknown location: node " << n << " does not exist in the road network";
        return os.str();
    }
public:
    explicit UnknownLocationError(int n) : MedRouteError(build(n)), node(n) {}
    int getNode() const { return node; }
};

// Thrown when an ID (ambulance, hospital or emergency) is registered twice.
class DuplicateIdError : public MedRouteError {
private:
    static std::string build(const std::string& kind, int id) {
        std::ostringstream os;
        os << "Duplicate " << kind << " ID " << id;
        return os.str();
    }
public:
    DuplicateIdError(const std::string& kind, int id) : MedRouteError(build(kind, id)) {}
};

// Thrown when an ID is looked up but was never registered.
class UnknownIdError : public MedRouteError {
private:
    static std::string build(const std::string& kind, int id) {
        std::ostringstream os;
        os << "Unknown " << kind << " ID " << id;
        return os.str();
    }
public:
    UnknownIdError(const std::string& kind, int id) : MedRouteError(build(kind, id)) {}
};

// Thrown when a traffic event names a road that does not exist.
class UnknownRoadError : public MedRouteError {
private:
    static std::string build(int a, int b) {
        std::ostringstream os;
        os << "No road between node " << a << " and node " << b;
        return os.str();
    }
public:
    UnknownRoadError(int a, int b) : MedRouteError(build(a, b)) {}
};

// Thrown for an invalid argument value (for example a non-positive road cost).
class InvalidArgumentError : public MedRouteError {
public:
    explicit InvalidArgumentError(const std::string& message) : MedRouteError(message) {}
};

// Thrown by the service program when its input cannot be understood.
class InputError : public MedRouteError {
public:
    explicit InputError(const std::string& message) : MedRouteError(message) {}
};

#endif // EXCEPTIONS_H
