#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <unordered_map>

// Represents a single road connection between two locations
struct Edge {
    int destination;
    double weight; // distance or travel-time cost

    Edge(int dest, double w) : destination(dest), weight(w) {}
};

// Graph class represents the road network as a weighted adjacency list.
// Nodes = locations (hospitals, ambulance positions, emergency sites)
// Edges = roads connecting them, weighted by distance/traffic cost
class Graph {
private:
    std::unordered_map<int, std::vector<Edge>> adjacencyList;
    int numNodes;

public:
    Graph();
    explicit Graph(int nodes);

    // Add a bidirectional or directed road between two locations
    void addEdge(int source, int destination, double weight, bool bidirectional = true);

    // Update weight of an existing edge (e.g., traffic congestion change)
    void updateEdgeWeight(int source, int destination, double newWeight);

    // Get all neighbors of a given node
    std::vector<Edge> getNeighbors(int node) const;

    int getNumNodes() const;
};

#endif // GRAPH_H
