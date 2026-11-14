#include "HopCountPlanner.h"
#include <algorithm>
#include <queue>
#include <unordered_map>

PathResult HopCountPlanner::path(const Graph& graph, int source, int destination) const {
    PathResult result;
    std::unordered_map<int, int> previous;   // node -> the node we reached it from
    std::queue<int> frontier;
    previous[source] = source;
    frontier.push(source);

    while (!frontier.empty()) {
        int node = frontier.front();
        frontier.pop();
        if (node == destination) break;
        for (const Edge& edge : graph.getNeighbors(node)) {
            if (previous.find(edge.destination) == previous.end()) {
                previous[edge.destination] = node;
                frontier.push(edge.destination);
            }
        }
    }

    if (previous.find(destination) == previous.end()) return result; // unreachable

    std::vector<int> route;
    for (int node = destination; ; node = previous[node]) {
        route.push_back(node);
        if (node == source) break;
    }
    std::reverse(route.begin(), route.end());

    result.path = route;
    result.totalDistance = static_cast<double>(route.size() - 1); // number of roads used
    result.reachable = true;
    return result;
}

double HopCountPlanner::distance(const Graph& graph, int source, int destination) const {
    PathResult r = path(graph, source, destination);
    return r.reachable ? r.totalDistance : -1.0;
}

std::string HopCountPlanner::name() const {
    return "Hop count (fewest roads)";
}
