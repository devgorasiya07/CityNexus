/// @file contraction.cpp
/// @brief Contraction Hierarchies implementation for BuildSim.
///
/// Contraction Hierarchies (CH) is a speed-up technique that preprocesses
/// the graph once, then answers shortest-path queries orders of magnitude
/// faster than plain Dijkstra. The key insight: contract unimportant nodes
/// first, adding shortcut edges to preserve distances, then run bidirectional
/// Dijkstra restricted to "upward" edges (toward more important nodes).
///
/// References:
///   Geisberger et al., "Contraction Hierarchies: Faster and Simpler
///   Hierarchical Routing in Road Networks" (2008)

#include "buildsim/algorithms/contraction.h"

namespace buildsim {

// ======================== Importance Heuristic ========================

int ContractionHierarchy::computeImportance(const Graph& g, int node) {
    // Edge difference: how many shortcuts are needed minus how many edges
    // are removed when contracting this node. Lower = better to contract early.
    //
    // Also penalise nodes with many already-contracted neighbors, since
    // contracting them would create a dense local cluster of shortcuts.

    const auto& in_edges = g.reverseNeighbors(node);
    const auto& out_edges = g.neighbors(node);

    // Count non-contracted in/out neighbors
    int in_count = 0;
    int out_count = 0;
    int contracted_neighbor_count = 0;

    for (const auto& e : in_edges) {
        if (!contracted_[e.to]) {
            in_count++;
        } else {
            contracted_neighbor_count++;
        }
    }
    for (const auto& e : out_edges) {
        if (!contracted_[e.to]) {
            out_count++;
        } else {
            contracted_neighbor_count++;
        }
    }

    // Upper bound on shortcuts needed: each (in, out) pair might need one.
    // In practice, many are suboptimal and won't need shortcuts.
    // We use a simulated contraction to get a tighter estimate.
    int shortcuts_needed = 0;

    for (const auto& in_e : in_edges) {
        if (contracted_[in_e.to]) continue;
        int u = in_e.to;

        for (const auto& out_e : out_edges) {
            if (contracted_[out_e.to]) continue;
            int w = out_e.to;
            if (u == w) continue;

            double shortcut_dist = in_e.weight + out_e.weight;

            // Check if there's a witness path u → w not going through node
            // that is at most as short as the shortcut. Use a limited local
            // Dijkstra search to find witness paths efficiently.
            bool witness_found = false;

            // Local search with hop limit for efficiency
            constexpr int MAX_SETTLE = 20;
            std::vector<double> local_dist(g.numNodes(), INF);
            local_dist[u] = 0.0;

            using PQEntry = std::pair<double, int>;
            std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<>> pq;
            pq.emplace(0.0, u);

            int settled = 0;
            while (!pq.empty() && settled < MAX_SETTLE) {
                auto [d, v] = pq.top();
                pq.pop();

                if (d > local_dist[v]) continue;
                settled++;

                if (v == w && d <= shortcut_dist) {
                    witness_found = true;
                    break;
                }

                // Only consider non-contracted neighbors (skip the node being evaluated)
                for (const auto& edge : g.neighbors(v)) {
                    if (contracted_[edge.to] || edge.to == node) continue;
                    double nd = d + edge.weight;
                    if (nd < local_dist[edge.to] && nd <= shortcut_dist) {
                        local_dist[edge.to] = nd;
                        pq.emplace(nd, edge.to);
                    }
                }
            }

            if (!witness_found) {
                shortcuts_needed++;
            }
        }
    }

    // Edge difference: shortcuts added minus edges removed
    int edges_removed = in_count + out_count;
    int edge_difference = shortcuts_needed - edges_removed;

    // Final importance: edge difference + penalty for contracted neighbors
    return edge_difference + contracted_neighbor_count;
}

// ======================== Node Contraction ========================

void ContractionHierarchy::contractNode(Graph& g, int node) {
    const auto& in_edges = g.reverseNeighbors(node);
    const auto& out_edges = g.neighbors(node);

    // For each pair (u → node → w), check if a shortcut u → w is needed
    for (const auto& in_e : in_edges) {
        if (contracted_[in_e.to]) continue;
        int u = in_e.to;

        for (const auto& out_e : out_edges) {
            if (contracted_[out_e.to]) continue;
            int w = out_e.to;
            if (u == w) continue;

            double shortcut_dist = in_e.weight + out_e.weight;

            // Witness search: is there a path u → w (avoiding node) that is
            // no longer than the shortcut? If yes, shortcut is unnecessary.
            bool witness_found = false;

            constexpr int MAX_SETTLE = 20;
            std::vector<double> local_dist(g.numNodes(), INF);
            local_dist[u] = 0.0;

            using PQEntry = std::pair<double, int>;
            std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<>> pq;
            pq.emplace(0.0, u);

            int settled = 0;
            while (!pq.empty() && settled < MAX_SETTLE) {
                auto [d, v] = pq.top();
                pq.pop();

                if (d > local_dist[v]) continue;
                settled++;

                if (v == w && d <= shortcut_dist) {
                    witness_found = true;
                    break;
                }

                for (const auto& edge : g.neighbors(v)) {
                    if (contracted_[edge.to] || edge.to == node) continue;
                    double nd = d + edge.weight;
                    if (nd < local_dist[edge.to] && nd <= shortcut_dist) {
                        local_dist[edge.to] = nd;
                        pq.emplace(nd, edge.to);
                    }
                }
            }

            if (!witness_found) {
                // No witness path found — we must add a shortcut edge u → w
                // through midpoint 'node' to preserve shortest-path distances
                g.addShortcutEdge(u, w, shortcut_dist, node);
            }
        }
    }

    // Mark node as contracted
    contracted_[node] = true;
}

// ======================== Preprocessing ========================

void ContractionHierarchy::preprocess(Graph& g) {
    Timer timer;
    int n = g.numNodes();

    contracted_.assign(n, false);
    node_order_.clear();
    node_order_.reserve(n);
    g.ch_level_.assign(n, 0);

    // Priority queue for node ordering: (importance, node_id)
    // Nodes with lowest importance are contracted first
    using PQEntry = std::pair<int, int>;
    std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<>> pq;

    // Initial importance computation for all nodes
    for (int i = 0; i < n; ++i) {
        int importance = computeImportance(g, i);
        pq.emplace(importance, i);
    }

    int level = 0;

    while (!pq.empty()) {
        auto [imp, node] = pq.top();
        pq.pop();

        // Skip already-contracted nodes
        if (contracted_[node]) continue;

        // Lazy update: recompute importance to account for neighbors that
        // were contracted since our last computation. If the node is no
        // longer the best candidate, re-insert it.
        int current_importance = computeImportance(g, node);
        if (current_importance > imp) {
            pq.emplace(current_importance, node);
            continue;
        }

        // Contract this node
        contractNode(g, node);
        g.ch_level_[node] = level;
        node_order_.push_back(node);
        level++;
    }

    preprocessed_ = true;
    preprocess_time_ms_ = timer.elapsedMs();
}

// ======================== Bidirectional CH Query ========================

PathResult ContractionHierarchy::query(const Graph& g, int source, int target) {
    PathResult result;
    Timer timer;

    if (!preprocessed_) {
        result.time_ms = timer.elapsedMs();
        return result;
    }

    if (!g.isValidNode(source) || !g.isValidNode(target)) {
        result.time_ms = timer.elapsedMs();
        return result;
    }

    if (source == target) {
        result.path = {source};
        result.distance = 0.0;
        result.found = true;
        result.nodes_explored = 1;
        result.time_ms = timer.elapsedMs();
        return result;
    }

    int n = g.numNodes();

    // Forward distances (from source, going UP the hierarchy)
    std::vector<double> dist_fwd(n, INF);
    // Backward distances (from target, going UP the hierarchy on reverse graph)
    std::vector<double> dist_bwd(n, INF);
    // Predecessor arrays for path reconstruction
    std::vector<int> pred_fwd(n, -1);
    std::vector<int> pred_bwd(n, -1);

    dist_fwd[source] = 0.0;
    dist_bwd[target] = 0.0;

    using PQEntry = std::pair<double, int>;
    std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<>> pq_fwd;
    std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<>> pq_bwd;

    pq_fwd.emplace(0.0, source);
    pq_bwd.emplace(0.0, target);

    // Best known distance through a meeting node
    double mu = INF;
    int meeting_node = -1;

    // Track settled nodes to avoid redundant processing
    std::vector<bool> settled_fwd(n, false);
    std::vector<bool> settled_bwd(n, false);

    // Alternating bidirectional search
    while (!pq_fwd.empty() || !pq_bwd.empty()) {

        // --- Forward step: expand from source going to HIGHER-level nodes ---
        if (!pq_fwd.empty()) {
            auto [d, u] = pq_fwd.top();
            pq_fwd.pop();

            if (d <= mu) {
                if (!settled_fwd[u] && d <= dist_fwd[u]) {
                    settled_fwd[u] = true;
                    result.nodes_explored++;

                    // Check if backward search already reached this node
                    if (dist_bwd[u] < INF) {
                        double candidate = dist_fwd[u] + dist_bwd[u];
                        if (candidate < mu) {
                            mu = candidate;
                            meeting_node = u;
                        }
                    }

                    // Relax edges going to higher-level nodes only
                    for (const auto& edge : g.neighbors(u)) {
                        if (g.ch_level_[edge.to] >= g.ch_level_[u]) {
                            double new_dist = dist_fwd[u] + edge.weight;
                            if (new_dist < dist_fwd[edge.to]) {
                                dist_fwd[edge.to] = new_dist;
                                pred_fwd[edge.to] = u;
                                pq_fwd.emplace(new_dist, edge.to);
                            }
                        }
                    }
                }
            }
        }

        // --- Backward step: expand from target going to HIGHER-level nodes ---
        if (!pq_bwd.empty()) {
            auto [d, u] = pq_bwd.top();
            pq_bwd.pop();

            if (d <= mu) {
                if (!settled_bwd[u] && d <= dist_bwd[u]) {
                    settled_bwd[u] = true;
                    result.nodes_explored++;

                    // Check if forward search already reached this node
                    if (dist_fwd[u] < INF) {
                        double candidate = dist_fwd[u] + dist_bwd[u];
                        if (candidate < mu) {
                            mu = candidate;
                            meeting_node = u;
                        }
                    }

                    // Relax reverse edges going to higher-level nodes only
                    for (const auto& edge : g.reverseNeighbors(u)) {
                        if (g.ch_level_[edge.to] >= g.ch_level_[u]) {
                            double new_dist = dist_bwd[u] + edge.weight;
                            if (new_dist < dist_bwd[edge.to]) {
                                dist_bwd[edge.to] = new_dist;
                                pred_bwd[edge.to] = u;
                                pq_bwd.emplace(new_dist, edge.to);
                            }
                        }
                    }
                }
            }
        }

        // Termination: both queues have minimum > mu
        double min_fwd = pq_fwd.empty() ? INF : pq_fwd.top().first;
        double min_bwd = pq_bwd.empty() ? INF : pq_bwd.top().first;
        if (min_fwd > mu && min_bwd > mu) {
            break;
        }
    }

    if (meeting_node == -1) {
        // No path found
        result.time_ms = timer.elapsedMs();
        return result;
    }

    // Reconstruct path: source → meeting_node → target
    // Forward part: trace pred_fwd from meeting_node back to source
    std::vector<int> fwd_path;
    {
        int cur = meeting_node;
        while (cur != source) {
            fwd_path.push_back(cur);
            cur = pred_fwd[cur];
            if (cur == -1) break;
        }
        if (cur == source) {
            fwd_path.push_back(source);
        }
        std::reverse(fwd_path.begin(), fwd_path.end());
    }

    // Backward part: trace pred_bwd from meeting_node back to target
    std::vector<int> bwd_path;
    {
        int cur = pred_bwd[meeting_node];
        while (cur != -1 && cur != target) {
            bwd_path.push_back(cur);
            cur = pred_bwd[cur];
        }
        if (cur == target) {
            bwd_path.push_back(target);
        }
    }

    // Combine: fwd_path already ends at meeting_node, bwd_path continues to target
    std::vector<int> packed_path = std::move(fwd_path);
    packed_path.insert(packed_path.end(), bwd_path.begin(), bwd_path.end());

    // Unpack shortcut edges to recover the full original path
    result.path = unpackPath(g, packed_path);
    result.distance = mu;
    result.found = true;
    result.time_ms = timer.elapsedMs();

    return result;
}

// ======================== Shortcut Unpacking ========================

std::vector<int> ContractionHierarchy::unpackPath(const Graph& g, const std::vector<int>& packed_path) {
    if (packed_path.size() <= 1) {
        return packed_path;
    }

    std::vector<int> unpacked;
    unpacked.push_back(packed_path[0]);

    // For each consecutive pair (u, v) in the packed path, check if the
    // edge u→v is a shortcut. If so, recursively unpack via the midpoint.
    for (size_t i = 0; i + 1 < packed_path.size(); ++i) {
        int u = packed_path[i];
        int v = packed_path[i + 1];

        // Find the edge u → v and check if it's a shortcut
        int mid = -1;
        bool found_shortcut = false;
        double best_weight = INF;

        for (const auto& edge : g.neighbors(u)) {
            if (edge.to == v && edge.is_shortcut) {
                // Among multiple shortcut edges u→v, pick the one with smallest weight
                if (edge.weight < best_weight) {
                    best_weight = edge.weight;
                    mid = edge.shortcut_mid;
                    found_shortcut = true;
                }
            }
        }

        if (found_shortcut && mid != -1) {
            // Recursively unpack: u → mid → v
            // The sub-paths u→mid and mid→v may themselves contain shortcuts
            std::vector<int> left = unpackPath(g, {u, mid});
            std::vector<int> right = unpackPath(g, {mid, v});

            // Append left (skip first since it's already in unpacked) and right
            for (size_t j = 1; j < left.size(); ++j) {
                unpacked.push_back(left[j]);
            }
            for (size_t j = 1; j < right.size(); ++j) {
                unpacked.push_back(right[j]);
            }
        } else {
            // Original edge — just append the destination
            unpacked.push_back(v);
        }
    }

    return unpacked;
}

} // namespace buildsim
