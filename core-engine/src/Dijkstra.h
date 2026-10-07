#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <vector>
#include <unordered_map>
#include "Graph.h"

// Result of a shortest-path query: the ordered sequence of node IDs from
// source to destination, and the total path cost (distance/time).
struct PathResult {
    std::vector<int> path;   // empty if destination is unreachable from source
    double totalDistance;    // -1 if unreachable
    bool reachable;

    PathResult() : totalDistance(-1), reachable(false) {}
};

// Dijkstra's shortest-path algorithm over the Graph.
// Internally uses a min-heap (via std::priority_queue) keyed by (distance, node) -
// the same core idea as our own heap-based PriorityQueue, specialized here for
// Dijkstra's "always expand the currently-closest unvisited node" step.
//
// Used throughout MedRoute for:
//   - finding the nearest available ambulance to an emergency (distance only)
//   - computing the actual route: ambulance -> patient -> hospital (full path)
//   - re-routing after Graph::updateEdgeWeight() changes a road's cost
class Dijkstra {
public:
    // Returns the shortest distance from 'source' to every reachable node.
    static std::unordered_map<int, double> shortestDistances(const Graph& graph, int source);

    // Returns the shortest distance from 'source' to a single 'destination'.
    // Returns -1.0 if unreachable.
    static double shortestDistanceTo(const Graph& graph, int source, int destination);

    // Returns the full shortest path (sequence of node IDs) and its total cost.
    static PathResult shortestPath(const Graph& graph, int source, int destination);
};

#endif // DIJKSTRA_H
