#include "buildsim/engine/planner.h"
#include "buildsim/algorithms/dijkstra.h"
#include "buildsim/algorithms/astar.h"
#include "buildsim/algorithms/facility.h"
#include "buildsim/algorithms/steiner.h"
#include "buildsim/algorithms/isochrone.h"
#include <iostream>

namespace buildsim {

Planner::Planner(Graph& graph) : graph_(graph) {}

PlacementResult Planner::placeFacilities(FacilityType type, int count,
                                         const std::vector<DemandPoint>& demands,
                                         const std::string& method) {
    if (method == "pcenter") {
        return pCenter(graph_, demands, count);
    } else if (method == "pmedian") {
        return pMedian(graph_, demands, count);
    } else {
        return greedyPlacement(graph_, demands, count);
    }
}

TreeResult Planner::designNetwork(const std::vector<int>& facility_nodes) {
    return steinerTree(graph_, facility_nodes);
}

IsochroneResult Planner::computeReachability(int source, double max_distance) {
    return computeIsochrone(graph_, source, max_distance);
}

PathResult Planner::findShortestPath(int source, int target, const std::string& algo) {
    if (algo == "dijkstra") {
        return dijkstra(graph_, source, target);
    } else if (algo == "ch" && ch_ready_) {
        return ch_.query(graph_, source, target);
    } else {
        // default astar
        return astarDefault(graph_, source, target);
    }
}

void Planner::preprocessCH() {
    std::cout << "Preprocessing Contraction Hierarchies... Please wait.\n";
    ch_.preprocess(graph_);
    ch_ready_ = true;
    std::cout << "Preprocessing complete in " << ch_.preprocessTimeMs() << " ms.\n";
}

bool Planner::isCHReady() const {
    return ch_ready_;
}

} // namespace buildsim
