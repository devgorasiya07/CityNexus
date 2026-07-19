/// @file dijkstra.cpp
/// @brief Dijkstra's shortest-path algorithm implementations for BuildSim.
///
/// All implementations use std::priority_queue with std::greater<> for
/// min-heap ordering, which is the standard efficient approach for Dijkstra.

#include "buildsim/algorithms/dijkstra.h"

namespace buildsim {

// ======================== Path Reconstruction ========================

std::vector<int> reconstructPath(const std::vector<int>& pred, int source, int target) {
    // If target has no predecessor and isn't the source, no path exists
    if (pred[target] == -1 && target != source) {
        return {};
    }

    std::vector<int> path;
    int current = target;

    // Walk backwards from target to source using predecessor links
    while (current != source) {
        path.push_back(current);
        current = pred[current];

        // Safety check: if we hit -1 before reaching source, path is broken
        if (current == -1) {
            return {};
        }
    }
    path.push_back(source);

    // Reverse to get source → target order
    std::reverse(path.begin(), path.end());
    return path;
}

// ======================== Single-Source Single-Target ========================

PathResult dijkstra(const Graph& g, int source, int target) {
    PathResult result;
    Timer timer;

    int n = g.numNodes();

    // Validate input nodes
    if (!g.isValidNode(source) || !g.isValidNode(target)) {
        result.time_ms = timer.elapsedMs();
        return result;
    }

    // Trivial case: source == target
    if (source == target) {
        result.path = {source};
        result.distance = 0.0;
        result.found = true;
        result.nodes_explored = 1;
        result.time_ms = timer.elapsedMs();
        return result;
    }

    // Distance array and predecessor array
    std::vector<double> dist(n, INF);
    std::vector<int> pred(n, -1);
    dist[source] = 0.0;

    // Min-heap: (distance, node_id)
    // std::greater<> ensures smallest distance is at the top
    using PQEntry = std::pair<double, int>;
    std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<>> pq;
    pq.emplace(0.0, source);

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();

        // Skip stale entries (node already settled with shorter distance)
        if (d > dist[u]) continue;

        result.nodes_explored++;

        // Early termination: target settled — we have the optimal path
        if (u == target) {
            result.distance = dist[target];
            result.path = reconstructPath(pred, source, target);
            result.found = true;
            result.time_ms = timer.elapsedMs();
            return result;
        }

        // Relax all outgoing edges from u
        for (const auto& edge : g.neighbors(u)) {
            double new_dist = dist[u] + edge.weight;
            if (new_dist < dist[edge.to]) {
                dist[edge.to] = new_dist;
                pred[edge.to] = u;
                pq.emplace(new_dist, edge.to);
            }
        }
    }

    // Target was never reached — no path exists
    result.time_ms = timer.elapsedMs();
    return result;
}

// ======================== Single-Source All-Targets ========================

std::vector<double> dijkstraAll(const Graph& g, int source) {
    int n = g.numNodes();
    std::vector<double> dist(n, INF);

    if (!g.isValidNode(source)) {
        return dist;
    }

    dist[source] = 0.0;

    using PQEntry = std::pair<double, int>;
    std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<>> pq;
    pq.emplace(0.0, source);

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();

        // Skip stale entries
        if (d > dist[u]) continue;

        // Relax all outgoing edges — no early termination since we need all distances
        for (const auto& edge : g.neighbors(u)) {
            double new_dist = dist[u] + edge.weight;
            if (new_dist < dist[edge.to]) {
                dist[edge.to] = new_dist;
                pq.emplace(new_dist, edge.to);
            }
        }
    }

    return dist;
}

// ======================== Multi-Source Dijkstra ========================

std::vector<double> multiSourceDijkstra(const Graph& g, const std::vector<int>& sources) {
    int n = g.numNodes();
    std::vector<double> dist(n, INF);

    using PQEntry = std::pair<double, int>;
    std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<>> pq;

    // Initialize all source nodes at distance 0 — conceptually equivalent
    // to adding a virtual super-source connected to all real sources with
    // zero-weight edges, but more efficient.
    for (int s : sources) {
        if (g.isValidNode(s)) {
            dist[s] = 0.0;
            pq.emplace(0.0, s);
        }
    }

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();

        if (d > dist[u]) continue;

        for (const auto& edge : g.neighbors(u)) {
            double new_dist = dist[u] + edge.weight;
            if (new_dist < dist[edge.to]) {
                dist[edge.to] = new_dist;
                pq.emplace(new_dist, edge.to);
            }
        }
    }

    return dist;
}

} // namespace buildsim
