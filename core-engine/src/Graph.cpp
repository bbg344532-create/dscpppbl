#include "Graph.h"

Graph::Graph() : numNodes(0) {}

Graph::Graph(int nodes) : numNodes(nodes) {}

void Graph::addEdge(int source, int destination, double weight, bool bidirectional) {
    adjacencyList[source].emplace_back(destination, weight);
    if (bidirectional) {
        adjacencyList[destination].emplace_back(source, weight);
    }
}

void Graph::updateEdgeWeight(int source, int destination, double newWeight) {
    // Used to simulate traffic/road-condition changes for dynamic re-routing.
    // Updates the edge source -> destination if it exists. If the graph was built
    // bidirectionally, call this twice (once each direction) or extend as needed.
    auto it = adjacencyList.find(source);
    if (it == adjacencyList.end()) return;

    for (auto& edge : it->second) {
        if (edge.destination == destination) {
            edge.weight = newWeight;
            return;
        }
    }
    // Edge didn't exist yet - add it so dynamic conditions can introduce new connections
    adjacencyList[source].emplace_back(destination, newWeight);
}

std::vector<Edge> Graph::getNeighbors(int node) const {
    auto it = adjacencyList.find(node);
    if (it != adjacencyList.end()) {
        return it->second;
    }
    return {};
}

int Graph::getNumNodes() const {
    return numNodes;
}
