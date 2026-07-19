/// @file isochrone.cpp
/// @brief Isochrone (reachability) analysis implementation for BuildSim.
///
/// Isochrone computation answers: "Which parts of the city can be reached
/// from a facility within X meters (or minutes)?" This is critical for
/// urban planning — e.g., ensuring all residents are within 5km of a hospital.
///
/// Implementation uses modified Dijkstra with a distance cutoff for efficiency:
/// nodes beyond the budget are never enqueued, pruning the search space.

#include "buildsim/algorithms/isochrone.h"

namespace buildsim {

// ======================== Forward Isochrone ========================

IsochroneResult computeIsochrone(const Graph& g, int source, double max_distance) {
    IsochroneResult result;
    Timer timer;

    result.max_time_or_dist = max_distance;

    int n = g.numNodes();
    if (!g.isValidNode(source) || n == 0) {
        result.solve_time_ms = timer.elapsedMs();
        return result;
    }

    // Distance array
    std::vector<double> dist(n, INF);
    dist[source] = 0.0;

    // Min-heap: (distance, node_id)
    using PQEntry = std::pair<double, int>;
    std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<>> pq;
    pq.emplace(0.0, source);

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();

        // Skip stale entries
        if (d > dist[u]) continue;

        // This node is within budget — add to reachable set
        result.reachable_nodes.push_back(u);
        result.distances[u] = dist[u];

        // Relax neighbors, but only enqueue those within the distance budget
        for (const auto& edge : g.neighbors(u)) {
            double new_dist = dist[u] + edge.weight;

            // Distance cutoff: don't explore beyond the budget
            if (new_dist <= max_distance && new_dist < dist[edge.to]) {
                dist[edge.to] = new_dist;
                pq.emplace(new_dist, edge.to);
            }
        }
    }

    // Compute coverage as percentage of total graph nodes
    if (n > 0) {
        result.coverage_percent = (static_cast<double>(result.reachable_nodes.size()) / n) * 100.0;
    }

    result.solve_time_ms = timer.elapsedMs();
    return result;
}

// ======================== Reverse Isochrone ========================

IsochroneResult computeReverseIsochrone(const Graph& g, int target, double max_distance) {
    IsochroneResult result;
    Timer timer;

    result.max_time_or_dist = max_distance;

    int n = g.numNodes();
    if (!g.isValidNode(target) || n == 0) {
        result.solve_time_ms = timer.elapsedMs();
        return result;
    }

    // Distance array (distances on reverse graph from target)
    std::vector<double> dist(n, INF);
    dist[target] = 0.0;

    using PQEntry = std::pair<double, int>;
    std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<>> pq;
    pq.emplace(0.0, target);

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();

        if (d > dist[u]) continue;

        // This node can reach the target within budget
        result.reachable_nodes.push_back(u);
        result.distances[u] = dist[u];

        // Traverse REVERSE edges: who can reach u?
        // reverseNeighbors(u) gives edges v → u in the original graph,
        // stored as (v, weight) — so we're effectively walking backwards.
        for (const auto& edge : g.reverseNeighbors(u)) {
            double new_dist = dist[u] + edge.weight;

            if (new_dist <= max_distance && new_dist < dist[edge.to]) {
                dist[edge.to] = new_dist;
                pq.emplace(new_dist, edge.to);
            }
        }
    }

    if (n > 0) {
        result.coverage_percent = (static_cast<double>(result.reachable_nodes.size()) / n) * 100.0;
    }

    result.solve_time_ms = timer.elapsedMs();
    return result;
}

// ======================== Coverage Percentage ========================

double computeCoveragePercent(const Graph& g, int source, double max_distance) {
    // Delegate to computeIsochrone and extract the coverage statistic.
    // This avoids duplicating the Dijkstra logic while providing a
    // convenient single-value API for quick coverage checks.
    IsochroneResult iso = computeIsochrone(g, source, max_distance);
    return iso.coverage_percent;
}

} // namespace buildsim
