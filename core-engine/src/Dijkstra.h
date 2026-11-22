#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <string>
#include <unordered_map>
#include "Graph.h"
#include "RoutePlanner.h"

// Dijkstra's shortest-path algorithm over the Graph (PathResult now lives in
// RoutePlanner.h). Internally uses a min-heap (std::priority_queue) keyed by
// (distance, node). Complexity O((V + E) log V); needs non-negative weights.
class Dijkstra {
public:
    static std::unordered_map<int, double> shortestDistances(const Graph& graph, int source);
    static double shortestDistanceTo(const Graph& graph, int source, int destination); // -1 if unreachable
    static PathResult shortestPath(const Graph& graph, int source, int destination);
};

// Adapter that makes Dijkstra usable wherever a RoutePlanner is expected.
// This is the Dispatcher's default planner.
class DijkstraPlanner : public RoutePlanner {
public:
    double distance(const Graph& graph, int source, int destination) const override {
        return Dijkstra::shortestDistanceTo(graph, source, destination);
    }
    PathResult path(const Graph& graph, int source, int destination) const override {
        return Dijkstra::shortestPath(graph, source, destination);
    }
    std::string name() const override { return "Dijkstra (lowest total weight)"; }
};

#endif // DIJKSTRA_H
