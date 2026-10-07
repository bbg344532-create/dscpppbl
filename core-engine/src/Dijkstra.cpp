#include "Dijkstra.h"
#include <queue>
#include <limits>
#include <algorithm>

std::unordered_map<int, double> Dijkstra::shortestDistances(const Graph& graph, int source) {
    std::unordered_map<int, double> dist;
    dist[source] = 0.0;

    // Min-heap of (distance, node), smallest distance popped first.
    using DistNodePair = std::pair<double, int>;
    std::priority_queue<DistNodePair, std::vector<DistNodePair>, std::greater<DistNodePair>> pq;
    pq.push({0.0, source});

    while (!pq.empty()) {
        auto [currentDist, node] = pq.top();
        pq.pop();

        // Stale entry check: we may have pushed this node multiple times with
        // different distances; skip if we've already found something better.
        auto it = dist.find(node);
        if (it != dist.end() && currentDist > it->second) {
            continue;
        }

        for (const Edge& edge : graph.getNeighbors(node)) {
            double newDist = currentDist + edge.weight;
            auto neighborIt = dist.find(edge.destination);
            if (neighborIt == dist.end() || newDist < neighborIt->second) {
                dist[edge.destination] = newDist;
                pq.push({newDist, edge.destination});
            }
        }
    }

    return dist;
}

double Dijkstra::shortestDistanceTo(const Graph& graph, int source, int destination) {
    if (source == destination) return 0.0;
    auto distances = shortestDistances(graph, source);
    auto it = distances.find(destination);
    if (it == distances.end()) return -1.0; // unreachable
    return it->second;
}

PathResult Dijkstra::shortestPath(const Graph& graph, int source, int destination) {
    PathResult result;

    std::unordered_map<int, double> dist;
    std::unordered_map<int, int> prev; // for reconstructing the path
    dist[source] = 0.0;

    using DistNodePair = std::pair<double, int>;
    std::priority_queue<DistNodePair, std::vector<DistNodePair>, std::greater<DistNodePair>> pq;
    pq.push({0.0, source});

    while (!pq.empty()) {
        auto [currentDist, node] = pq.top();
        pq.pop();

        auto it = dist.find(node);
        if (it != dist.end() && currentDist > it->second) {
            continue;
        }

        if (node == destination) break; // shortest path to destination finalized

        for (const Edge& edge : graph.getNeighbors(node)) {
            double newDist = currentDist + edge.weight;
            auto neighborIt = dist.find(edge.destination);
            if (neighborIt == dist.end() || newDist < neighborIt->second) {
                dist[edge.destination] = newDist;
                prev[edge.destination] = node;
                pq.push({newDist, edge.destination});
            }
        }
    }

    auto destIt = dist.find(destination);
    if (destIt == dist.end()) {
        result.reachable = false;
        return result; // unreachable - empty path, distance -1
    }

    // Reconstruct path by walking backwards through 'prev', then reverse it.
    std::vector<int> path;
    int current = destination;
    path.push_back(current);
    while (current != source) {
        auto prevIt = prev.find(current);
        if (prevIt == prev.end()) break; // safety guard, shouldn't happen if reachable
        current = prevIt->second;
        path.push_back(current);
    }
    std::reverse(path.begin(), path.end());

    result.path = path;
    result.totalDistance = destIt->second;
    result.reachable = true;
    return result;
}
