#include "buildsim/algorithms/mst.h"
#include <queue>
#include <algorithm>

namespace buildsim {

// Simplified MST implementation using std::priority_queue (Fibonacci Heap alternative)
TreeResult primMST(const Graph& g) {
    int n = g.numNodes();
    TreeResult result;
    if (n == 0) return result;

    Timer t;
    std::vector<bool> inMST(n, false);
    std::vector<double> minWeight(n, INF);
    std::vector<int> parent(n, -1);

    using pq_t = std::pair<double, int>;
    std::priority_queue<pq_t, std::vector<pq_t>, std::greater<pq_t>> pq;

    int start_node = 0;
    pq.push({0.0, start_node});
    minWeight[start_node] = 0.0;

    while (!pq.empty()) {
        auto [weight, u] = pq.top();
        pq.pop();

        if (inMST[u]) continue;
        inMST[u] = true;

        if (parent[u] != -1) {
            result.edges.push_back({parent[u], u});
            result.total_cost += weight;
            result.num_nodes++;
        } else {
            result.num_nodes++; // Start node
        }

        for (const auto& edge : g.neighbors(u)) {
            int v = edge.to;
            if (!inMST[v] && edge.weight < minWeight[v]) {
                minWeight[v] = edge.weight;
                parent[v] = u;
                pq.push({minWeight[v], v});
            }
        }
    }

    result.solve_time_ms = t.elapsedMs();
    return result;
}

TreeResult primMST(const Graph& g, const std::vector<int>& subset) {
    int n = g.numNodes();
    TreeResult result;
    if (subset.empty()) return result;

    Timer t;
    std::vector<bool> validNode(n, false);
    for (int node : subset) validNode[node] = true;

    std::vector<bool> inMST(n, false);
    std::vector<double> minWeight(n, INF);
    std::vector<int> parent(n, -1);

    using pq_t = std::pair<double, int>;
    std::priority_queue<pq_t, std::vector<pq_t>, std::greater<pq_t>> pq;

    int start_node = subset[0];
    pq.push({0.0, start_node});
    minWeight[start_node] = 0.0;

    while (!pq.empty()) {
        auto [weight, u] = pq.top();
        pq.pop();

        if (inMST[u]) continue;
        inMST[u] = true;

        if (parent[u] != -1) {
            result.edges.push_back({parent[u], u});
            result.total_cost += weight;
            result.num_nodes++;
        } else {
            result.num_nodes++;
        }

        for (const auto& edge : g.neighbors(u)) {
            int v = edge.to;
            if (validNode[v] && !inMST[v] && edge.weight < minWeight[v]) {
                minWeight[v] = edge.weight;
                parent[v] = u;
                pq.push({minWeight[v], v});
            }
        }
    }

    result.solve_time_ms = t.elapsedMs();
    return result;
}

// Union-Find (Disjoint Set)
struct UnionFind {
    std::vector<int> parent, rank;
    UnionFind(int n) : parent(n), rank(n, 0) {
        for (int i = 0; i < n; ++i) parent[i] = i;
    }
    int find(int i) {
        if (parent[i] == i) return i;
        return parent[i] = find(parent[i]);
    }
    bool unite(int i, int j) {
        int root_i = find(i);
        int root_j = find(j);
        if (root_i != root_j) {
            if (rank[root_i] < rank[root_j]) {
                parent[root_i] = root_j;
            } else if (rank[root_i] > rank[root_j]) {
                parent[root_j] = root_i;
            } else {
                parent[root_j] = root_i;
                rank[root_i]++;
            }
            return true;
        }
        return false;
    }
};

TreeResult kruskalMST(const Graph& g) {
    Timer t;
    TreeResult result;
    int n = g.numNodes();

    struct EdgeData { int u, v; double weight; };
    std::vector<EdgeData> edges;
    for (int u = 0; u < n; ++u) {
        for (const auto& edge : g.neighbors(u)) {
            if (u < edge.to) { // Only add once for undirected graph
                edges.push_back({u, edge.to, edge.weight});
            }
        }
    }

    std::sort(edges.begin(), edges.end(), [](const EdgeData& a, const EdgeData& b) {
        return a.weight < b.weight;
    });

    UnionFind uf(n);
    for (const auto& e : edges) {
        if (uf.unite(e.u, e.v)) {
            result.edges.push_back({e.u, e.v});
            result.total_cost += e.weight;
        }
    }
    result.num_nodes = n;
    result.solve_time_ms = t.elapsedMs();
    return result;
}

} // namespace buildsim
