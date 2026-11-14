#ifndef HOP_COUNT_PLANNER_H
#define HOP_COUNT_PLANNER_H

#include "RoutePlanner.h"

// Second RoutePlanner: finds the route with the FEWEST ROADS (breadth-first
// search), ignoring road weights. "Distance" here is a count of roads.
// Used to show that planners are interchangeable behind the base class, and
// that a different strategy can pick a different route than Dijkstra.
class HopCountPlanner : public RoutePlanner {
public:
    double distance(const Graph& graph, int source, int destination) const override;
    PathResult path(const Graph& graph, int source, int destination) const override;
    std::string name() const override;
};

#endif // HOP_COUNT_PLANNER_H
