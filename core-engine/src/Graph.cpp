#include "Graph.h"

Graph::Graph() {}

Graph::Graph(int expectedNodes) {
    if (expectedNodes > 0) adjacencyList.reserve(static_cast<std::size_t>(expectedNodes));
}

void Graph::addEdge(int source, int destination, double weight, bool bidirectional) {
    adjacencyList[source].emplace_back(destination, weight);
    (void)adjacencyList[destination]; // make sure the destination node is registered
    if (bidirectional) {
        adjacencyList[destination].emplace_back(source, weight);
    }
}

void Graph::updateEdgeWeight(int source, int destination, double newWeight) {
    auto it = adjacencyList.find(source);
    if (it != adjacencyList.end()) {
        for (auto& edge : it->second) {
            if (edge.destination == destination) {
                edge.weight = newWeight;
                return;
            }
        }
    }
    // The road did not exist yet: add it, so changing conditions can open new connections.
    adjacencyList[source].emplace_back(destination, newWeight);
    (void)adjacencyList[destination];
}

std::vector<Edge> Graph::getNeighbors(int node) const {
    auto it = adjacencyList.find(node);
    if (it != adjacencyList.end()) {
        return it->second;
    }
    return std::vector<Edge>();
}

bool Graph::hasNode(int node) const {
    return adjacencyList.find(node) != adjacencyList.end();
}

int Graph::getNumNodes() const {
    return static_cast<int>(adjacencyList.size());
}

bool Graph::hasEdge(int source, int destination) const {
    return getEdgeWeight(source, destination) >= 0;
}

double Graph::getEdgeWeight(int source, int destination) const {
    auto it = adjacencyList.find(source);
    if (it == adjacencyList.end()) return -1.0;
    for (const Edge& edge : it->second) {
        if (edge.destination == destination) return edge.weight;
    }
    return -1.0;
}
