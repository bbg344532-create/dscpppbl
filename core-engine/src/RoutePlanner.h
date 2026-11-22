#ifndef ROUTE_PLANNER_H
#define ROUTE_PLANNER_H

#include <string>
#include <vector>
#include "Graph.h"

// Result of a shortest-path query: the ordered node IDs from source to
// destination and the total cost. An unreachable destination gives an empty
// path, totalDistance -1 and reachable == false.
struct PathResult {
    std::vector<int> path;
    double totalDistance;
    bool reachable;

    PathResult() : totalDistance(-1), reachable(false) {}
};

// Abstract base class (an interface): "something that can plan a route on a
// Graph". All three functions are PURE VIRTUAL, so RoutePlanner itself cannot
// be instantiated; every concrete planner must provide them.
//
// The Dispatcher only ever talks to a RoutePlanner pointer. Which algorithm
// runs (Dijkstra, hop count, ...) is decided at run time by virtual dispatch,
// so the Dispatcher's code does not change when the algorithm does.
class RoutePlanner {
public:
    virtual ~RoutePlanner() {}   // virtual destructor: safe deletion through a base pointer

    virtual double distance(const Graph& graph, int source, int destination) const = 0; // -1 if unreachable
    virtual PathResult path(const Graph& graph, int source, int destination) const = 0;
    virtual std::string name() const = 0;
};

#endif // ROUTE_PLANNER_H
