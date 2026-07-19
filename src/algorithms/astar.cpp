/// @file astar.cpp
/// @brief A* shortest-path search implementation with pluggable heuristics.
///
/// A* extends Dijkstra by adding a heuristic estimate h(n) to guide the search
/// toward the target. The exploration order is f(n) = g(n) + h(n), where g(n)
/// is the known cost from source and h(n) is the estimated cost to target.
/// With an admissible heuristic (never overestimates), A* guarantees optimality
/// while typically exploring far fewer nodes than plain Dijkstra.

#include "buildsim/algorithms/astar.h"

namespace buildsim {

// ======================== Heuristic Functions ========================

double euclideanHeuristic(const Node& a, const Node& b) {
    // Straight-line distance — the tightest admissible heuristic for
    // geometric graphs where edge weights are at least Euclidean distance.
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

double manhattanHeuristic(const Node& a, const Node& b) {
    // L1 distance — admissible for grid-aligned networks where travel
    // is restricted to axis-aligned roads (Manhattan-style city layouts).
    // Note: may overestimate if diagonal edges exist, but still useful
    // for grid cities where most roads are orthogonal.
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

double nullHeuristic(const Node& a, const Node& b) {
    // Zero heuristic: f(n) = g(n) + 0 = g(n), so A* degrades to Dijkstra.
    // Useful as a benchmark baseline to measure how much a real heuristic
    // reduces the number of explored nodes.
    (void)a;
    (void)b;
    return 0.0;
}

// ======================== A* Search ========================

PathResult astar(const Graph& g, int source, int target, Heuristic h) {
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

    // g_score[v] = best known cost from source to v
    std::vector<double> g_score(n, INF);
    g_score[source] = 0.0;

    // Predecessor array for path reconstruction
    std::vector<int> pred(n, -1);

    // Closed set to avoid re-expanding settled nodes
    std::vector<bool> closed(n, false);

    // Open set: min-heap ordered by f(n) = g(n) + h(n)
    // Entry: (f_score, node_id)
    using PQEntry = std::pair<double, int>;
    std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<>> open;

    const Node& target_node = g.node(target);
    double h_source = h(g.node(source), target_node);
    open.emplace(h_source, source);

    while (!open.empty()) {
        auto [f, u] = open.top();
        open.pop();

        // Skip if this node has already been settled
        if (closed[u]) continue;
        closed[u] = true;
        result.nodes_explored++;

        // Goal found — reconstruct path
        if (u == target) {
            result.distance = g_score[target];
            // Reconstruct path by walking predecessor chain
            std::vector<int> path;
            int current = target;
            while (current != source) {
                path.push_back(current);
                current = pred[current];
            }
            path.push_back(source);
            std::reverse(path.begin(), path.end());
            result.path = std::move(path);
            result.found = true;
            result.time_ms = timer.elapsedMs();
            return result;
        }

        // Expand neighbors
        for (const auto& edge : g.neighbors(u)) {
            if (closed[edge.to]) continue;

            double tentative_g = g_score[u] + edge.weight;
            if (tentative_g < g_score[edge.to]) {
                // Found a better path to this neighbor
                g_score[edge.to] = tentative_g;
                pred[edge.to] = u;

                // f(v) = g(v) + h(v)
                double f_score = tentative_g + h(g.node(edge.to), target_node);
                open.emplace(f_score, edge.to);
            }
        }
    }

    // Target was never reached
    result.time_ms = timer.elapsedMs();
    return result;
}

// ======================== Default A* (Euclidean) ========================

PathResult astarDefault(const Graph& g, int source, int target) {
    return astar(g, source, target, euclideanHeuristic);
}

} // namespace buildsim
