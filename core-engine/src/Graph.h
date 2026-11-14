#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <unordered_map>

// One road leaving a node: where it goes and what it costs.
struct Edge {
    int destination;
    double weight;

    Edge(int dest, double w) : destination(dest), weight(w) {}
};

// Road network as a weighted adjacency list.
// Nodes = locations, edges = roads weighted by distance/travel cost.
class Graph {
private:
    std::unordered_map<int, std::vector<Edge>> adjacencyList;

public:
    Graph();
    explicit Graph(int expectedNodes); // size hint only; the graph grows as needed

    // Adds a road (and the reverse road when bidirectional). Both end nodes
    // become known to the graph, even if they have no outgoing roads.
    void addEdge(int source, int destination, double weight, bool bidirectional = true);

    // Changes the cost of the road source -> destination (e.g. traffic).
    // Two-way roads need a call in each direction.
    void updateEdgeWeight(int source, int destination, double newWeight);

    std::vector<Edge> getNeighbors(int node) const;
    bool hasNode(int node) const;
    bool hasEdge(int source, int destination) const;       // is there a road source -> destination?
    double getEdgeWeight(int source, int destination) const; // -1 if there is no such road
    int getNumNodes() const; // number of distinct nodes the graph knows about
};

#endif // GRAPH_H
