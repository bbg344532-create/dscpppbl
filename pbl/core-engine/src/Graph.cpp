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
    // TODO: locate the edge (source -> destination) in adjacencyList and update its weight
    // Used for simulating traffic/road-condition changes for dynamic re-routing
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
