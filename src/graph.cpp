#include "buildsim/graph.h"
#include <queue>
#include <random>
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace buildsim {

// ======================== Graph Implementation ========================

Graph::Graph(int num_nodes) {
    init(num_nodes);
}

void Graph::init(int num_nodes) {
    nodes_.resize(num_nodes);
    adj_.resize(num_nodes);
    radj_.resize(num_nodes);
    ch_level_.assign(num_nodes, 0);
    for (int i = 0; i < num_nodes; ++i) {
        nodes_[i].id = i;
    }
    num_edges_ = 0;
}

int Graph::addNode(double x, double y) {
    int id = static_cast<int>(nodes_.size());
    nodes_.push_back(Node(id, x, y));
    adj_.emplace_back();
    radj_.emplace_back();
    ch_level_.push_back(0);
    return id;
}

void Graph::addEdge(int from, int to, double weight, RoadType type) {
    Edge e(to, weight, type);
    adj_[from].push_back(e);
    Edge re(from, weight, type);
    radj_[to].push_back(re);
    num_edges_++;
}

void Graph::addBidirectionalEdge(int from, int to, double weight, RoadType type) {
    addEdge(from, to, weight, type);
    addEdge(to, from, weight, type);
}

void Graph::addShortcutEdge(int from, int to, double weight, int mid_node) {
    Edge e(to, weight);
    e.is_shortcut = true;
    e.shortcut_mid = mid_node;
    adj_[from].push_back(e);

    Edge re(from, weight);
    re.is_shortcut = true;
    re.shortcut_mid = mid_node;
    radj_[to].push_back(re);
    num_edges_++;
}

int Graph::numConnectedComponents() const {
    int n = numNodes();
    std::vector<bool> visited(n, false);
    int components = 0;
    for (int i = 0; i < n; ++i) {
        if (!visited[i]) {
            components++;
            std::queue<int> q;
            q.push(i);
            visited[i] = true;
            while (!q.empty()) {
                int u = q.front(); q.pop();
                for (auto& e : adj_[u]) {
                    if (!visited[e.to]) {
                        visited[e.to] = true;
                        q.push(e.to);
                    }
                }
                for (auto& e : radj_[u]) {
                    if (!visited[e.to]) {
                        visited[e.to] = true;
                        q.push(e.to);
                    }
                }
            }
        }
    }
    return components;
}

std::vector<int> Graph::largestComponent() const {
    int n = numNodes();
    std::vector<bool> visited(n, false);
    std::vector<int> best_comp;
    for (int i = 0; i < n; ++i) {
        if (!visited[i]) {
            std::vector<int> comp;
            std::queue<int> q;
            q.push(i);
            visited[i] = true;
            while (!q.empty()) {
                int u = q.front(); q.pop();
                comp.push_back(u);
                for (auto& e : adj_[u]) {
                    if (!visited[e.to]) {
                        visited[e.to] = true;
                        q.push(e.to);
                    }
                }
                for (auto& e : radj_[u]) {
                    if (!visited[e.to]) {
                        visited[e.to] = true;
                        q.push(e.to);
                    }
                }
            }
            if (comp.size() > best_comp.size()) {
                best_comp = std::move(comp);
            }
        }
    }
    return best_comp;
}

double Graph::avgDegree() const {
    if (nodes_.empty()) return 0.0;
    double total = 0;
    for (int i = 0; i < numNodes(); ++i) {
        total += static_cast<double>(adj_[i].size());
    }
    return total / numNodes();
}

int Graph::maxDegree() const {
    int mx = 0;
    for (int i = 0; i < numNodes(); ++i) {
        mx = std::max(mx, static_cast<int>(adj_[i].size()));
    }
    return mx;
}

void Graph::printStats() const {
    printHeader("Graph Statistics");
    std::cout << "  Nodes:                " << numNodes() << "\n";
    std::cout << "  Edges:                " << numEdges() << "\n";
    std::cout << "  Avg Degree:           " << std::fixed << std::setprecision(2) << avgDegree() << "\n";
    std::cout << "  Max Degree:           " << maxDegree() << "\n";
    std::cout << "  Connected Components: " << numConnectedComponents() << "\n";
    printSeparator('-');
}

// ======================== Graph Generators ========================

Graph generateGridCity(int rows, int cols, double block_size, double perturbation, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> perturb(-perturbation, perturbation);
    std::uniform_real_distribution<double> weight_var(0.8, 1.2);

    int n = rows * cols;
    Graph g(n);

    // Place nodes on a grid with perturbation
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int id = r * cols + c;
            double x = c * block_size + perturb(rng);
            double y = r * block_size + perturb(rng);
            g.node(id) = Node(id, x, y);
        }
    }

    // Connect grid neighbors
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int id = r * cols + c;
            // Right neighbor
            if (c + 1 < cols) {
                int right = r * cols + (c + 1);
                double dist = euclideanDist(g.node(id), g.node(right)) * weight_var(rng);
                RoadType rt = (r % 5 == 0) ? RoadType::MAIN_ROAD : RoadType::RESIDENTIAL;
                g.addBidirectionalEdge(id, right, dist, rt);
            }
            // Down neighbor
            if (r + 1 < rows) {
                int down = (r + 1) * cols + c;
                double dist = euclideanDist(g.node(id), g.node(down)) * weight_var(rng);
                RoadType rt = (c % 5 == 0) ? RoadType::MAIN_ROAD : RoadType::RESIDENTIAL;
                g.addBidirectionalEdge(id, down, dist, rt);
            }
            // Diagonal (occasional, like shortcuts)
            if (r + 1 < rows && c + 1 < cols && (r + c) % 7 == 0) {
                int diag = (r + 1) * cols + (c + 1);
                double dist = euclideanDist(g.node(id), g.node(diag)) * weight_var(rng);
                g.addBidirectionalEdge(id, diag, dist, RoadType::RESIDENTIAL);
            }
        }
    }

    // Add a few highway-like long edges across the grid
    std::uniform_int_distribution<int> node_dist(0, n - 1);
    int num_highways = std::max(1, n / 200);
    for (int i = 0; i < num_highways; ++i) {
        int a = node_dist(rng);
        int b = node_dist(rng);
        if (a != b) {
            double dist = euclideanDist(g.node(a), g.node(b)) * 0.7; // Highways are faster
            g.addBidirectionalEdge(a, b, dist, RoadType::HIGHWAY);
        }
    }

    return g;
}

Graph generateRadialCity(int num_rings, int nodes_per_ring, double ring_spacing, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> weight_var(0.8, 1.2);
    std::uniform_real_distribution<double> perturb(-ring_spacing * 0.1, ring_spacing * 0.1);

    // Center node + ring nodes
    int total_nodes = 1 + num_rings * nodes_per_ring;
    Graph g(total_nodes);

    // Center node
    g.node(0) = Node(0, 0.0, 0.0);

    // Ring nodes
    for (int ring = 0; ring < num_rings; ++ring) {
        double radius = (ring + 1) * ring_spacing;
        for (int i = 0; i < nodes_per_ring; ++i) {
            int id = 1 + ring * nodes_per_ring + i;
            double angle = 2.0 * PI * i / nodes_per_ring;
            double x = radius * std::cos(angle) + perturb(rng);
            double y = radius * std::sin(angle) + perturb(rng);
            g.node(id) = Node(id, x, y);
        }
    }

    // Connect within each ring (ring road)
    for (int ring = 0; ring < num_rings; ++ring) {
        RoadType rt = (ring % 3 == 0) ? RoadType::MAIN_ROAD : RoadType::RESIDENTIAL;
        for (int i = 0; i < nodes_per_ring; ++i) {
            int a = 1 + ring * nodes_per_ring + i;
            int b = 1 + ring * nodes_per_ring + ((i + 1) % nodes_per_ring);
            double dist = euclideanDist(g.node(a), g.node(b)) * weight_var(rng);
            g.addBidirectionalEdge(a, b, dist, rt);
        }
    }

    // Connect center to first ring (radial spokes)
    for (int i = 0; i < nodes_per_ring; ++i) {
        int id = 1 + i;
        double dist = euclideanDist(g.node(0), g.node(id)) * weight_var(rng);
        g.addBidirectionalEdge(0, id, dist, RoadType::MAIN_ROAD);
    }

    // Connect between adjacent rings (radial roads)
    for (int ring = 0; ring < num_rings - 1; ++ring) {
        for (int i = 0; i < nodes_per_ring; ++i) {
            int a = 1 + ring * nodes_per_ring + i;
            int b = 1 + (ring + 1) * nodes_per_ring + i;
            double dist = euclideanDist(g.node(a), g.node(b)) * weight_var(rng);
            RoadType rt = (i % 4 == 0) ? RoadType::MAIN_ROAD : RoadType::RESIDENTIAL;
            g.addBidirectionalEdge(a, b, dist, rt);
        }
    }

    return g;
}

Graph generateRandomCity(int num_nodes, double area_size, double connection_radius, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> pos_dist(0, area_size);
    std::uniform_real_distribution<double> weight_var(0.9, 1.1);

    Graph g(num_nodes);
    for (int i = 0; i < num_nodes; ++i) {
        g.node(i) = Node(i, pos_dist(rng), pos_dist(rng));
    }

    // Connect nearby nodes
    for (int i = 0; i < num_nodes; ++i) {
        for (int j = i + 1; j < num_nodes; ++j) {
            double dist = euclideanDist(g.node(i), g.node(j));
            if (dist <= connection_radius) {
                g.addBidirectionalEdge(i, j, dist * weight_var(rng));
            }
        }
    }

    return g;
}

Graph loadGraphFromFile(const std::string& filename) {
    std::ifstream fin(filename);
    if (!fin.is_open()) {
        std::cerr << "Error: Cannot open file: " << filename << "\n";
        return Graph(0);
    }

    int n, m;
    fin >> n >> m;

    Graph g(n);

    for (int i = 0; i < n; ++i) {
        int id;
        double x, y;
        fin >> id >> x >> y;
        g.node(id) = Node(id, x, y);
    }

    for (int i = 0; i < m; ++i) {
        int from, to;
        double weight;
        fin >> from >> to >> weight;
        g.addBidirectionalEdge(from, to, weight);
    }

    fin.close();
    return g;
}

void saveGraphToFile(const Graph& graph, const std::string& filename) {
    std::ofstream fout(filename);
    if (!fout.is_open()) {
        std::cerr << "Error: Cannot open file for writing: " << filename << "\n";
        return;
    }

    int n = graph.numNodes();
    int m = graph.numEdges();
    fout << n << " " << m << "\n";

    for (int i = 0; i < n; ++i) {
        const auto& nd = graph.node(i);
        fout << nd.id << " " << std::fixed << std::setprecision(4) << nd.x << " " << nd.y << "\n";
    }

    // Write each directed edge (undirected edges appear twice)
    for (int i = 0; i < n; ++i) {
        for (auto& e : graph.neighbors(i)) {
            if (i < e.to) { // Only write each undirected edge once
                fout << i << " " << e.to << " " << std::fixed << std::setprecision(4) << e.weight << "\n";
            }
        }
    }

    fout.close();
}

} // namespace buildsim
