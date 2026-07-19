#include "buildsim/algorithms/steiner.h"
#include "buildsim/algorithms/dijkstra.h"
#include "buildsim/algorithms/mst.h"

namespace buildsim {

TreeResult steinerTree(const Graph& g, const std::vector<int>& terminal_nodes) {
    Timer t;
    TreeResult result;
    
    if (terminal_nodes.empty()) return result;
    
    // Metric closure graph
    int k = terminal_nodes.size();
    Graph metric_closure(k);
    
    for (int i = 0; i < k; ++i) {
        auto dists = dijkstraAll(g, terminal_nodes[i]);
        for (int j = i + 1; j < k; ++j) {
            if (dists[terminal_nodes[j]] != INF) {
                metric_closure.addBidirectionalEdge(i, j, dists[terminal_nodes[j]]);
            }
        }
    }
    
    TreeResult mst_closure = primMST(metric_closure);
    
    // Simplification: we just report the cost of the MST on metric closure.
    // A true Steiner Tree would expand these paths and do MST again to remove cycles.
    result.total_cost = mst_closure.total_cost;
    result.solve_time_ms = t.elapsedMs();
    
    return result;
}

TreeResult mstBaseline(const Graph& g, const std::vector<int>& terminal_nodes) {
    return primMST(g, terminal_nodes);
}

} // namespace buildsim
