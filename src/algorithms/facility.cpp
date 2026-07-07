#include "buildsim/algorithms/facility.h"
#include "buildsim/algorithms/dijkstra.h"
#include <random>

namespace buildsim {

PlacementResult pCenter(const Graph& g, const std::vector<DemandPoint>& demands, int p) {
    Timer t;
    PlacementResult result;
    if (demands.empty() || p <= 0) return result;
    
    // Very simplified version for now: random placement, then calculate metrics
    // A true binary search + set cover is complex; using greedy approach instead.
    
    std::vector<int> demand_nodes;
    for (const auto& d : demands) demand_nodes.push_back(d.node_id);
    
    return greedyPlacement(g, demands, p);
}

PlacementResult pMedian(const Graph& g, const std::vector<DemandPoint>& demands, int p) {
    // Fall back to greedy for now, to ensure something runs
    return greedyPlacement(g, demands, p);
}

PlacementResult greedyPlacement(const Graph& g, const std::vector<DemandPoint>& demands, int p) {
    Timer t;
    PlacementResult result;
    int n = g.numNodes();
    
    std::vector<double> min_dist_to_facility(n, INF);
    std::vector<int> chosen_facilities;
    
    for (int step = 0; step < p; ++step) {
        int best_node = -1;
        double min_max_dist = INF;
        
        // Pick a small random sample of nodes to test as next facility to keep it fast
        std::vector<int> candidates;
        if (n <= 100) {
            for (int i=0; i<n; ++i) candidates.push_back(i);
        } else {
            for (int i=0; i<100; ++i) candidates.push_back(rand() % n);
        }
        
        for (int candidate : candidates) {
            double current_max_dist = 0;
            auto dists = dijkstraAll(g, candidate);
            
            for (const auto& d : demands) {
                double dist = std::min(min_dist_to_facility[d.node_id], dists[d.node_id]);
                if (dist > current_max_dist) {
                    current_max_dist = dist;
                }
            }
            
            if (current_max_dist < min_max_dist) {
                min_max_dist = current_max_dist;
                best_node = candidate;
            }
        }
        
        if (best_node != -1) {
            chosen_facilities.push_back(best_node);
            auto dists = dijkstraAll(g, best_node);
            for (int i=0; i<n; ++i) {
                min_dist_to_facility[i] = std::min(min_dist_to_facility[i], dists[i]);
            }
            result.max_distance = min_max_dist;
        }
    }
    
    result.facility_nodes = chosen_facilities;
    
    double sum_dist = 0;
    int covered_count = 0;
    double threshold = 5000.0;
    for (const auto& d : demands) {
        double dist = min_dist_to_facility[d.node_id];
        if (dist != INF) {
            sum_dist += dist;
            if (dist <= threshold) covered_count++;
        }
    }
    
    result.avg_distance = sum_dist / std::max(1.0, (double)demands.size());
    result.threshold_used = threshold;
    result.coverage_within_threshold = (double)covered_count / std::max(1.0, (double)demands.size()) * 100.0;
    result.solve_time_ms = t.elapsedMs();
    
    return result;
}

} // namespace buildsim
