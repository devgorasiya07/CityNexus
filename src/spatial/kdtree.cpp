#include "buildsim/spatial/kdtree.h"
#include <algorithm>

namespace buildsim {

void KDTree::build(const std::vector<Node>& nodes) {
    std::vector<KDNode> pts;
    for (const auto& n : nodes) {
        pts.push_back({n.id, n.x, n.y, -1, -1});
    }
    
    if (pts.empty()) return;
    
    tree_ = pts; // copy
    root_ = buildRecursive(tree_, 0, tree_.size(), 0);
}

int KDTree::buildRecursive(std::vector<KDNode>& pts, int start, int end, int depth) {
    if (start >= end) return -1;
    
    int mid = start + (end - start) / 2;
    int axis = depth % 2;
    
    auto cmp = [axis](const KDNode& a, const KDNode& b) {
        if (axis == 0) return a.x < b.x;
        return a.y < b.y;
    };
    
    std::nth_element(pts.begin() + start, pts.begin() + mid, pts.begin() + end, cmp);
    
    pts[mid].left = buildRecursive(pts, start, mid, depth + 1);
    pts[mid].right = buildRecursive(pts, mid + 1, end, depth + 1);
    
    return mid;
}

int KDTree::nearestNeighbor(double x, double y) const {
    if (tree_.empty()) return -1;
    int best_id = -1;
    double best_dist = INF;
    nearestRecursive(root_, x, y, 0, best_id, best_dist);
    return best_id;
}

void KDTree::nearestRecursive(int node_idx, double x, double y, int depth, int& best_id, double& best_dist) const {
    if (node_idx == -1) return;
    
    const auto& node = tree_[node_idx];
    double dx = node.x - x;
    double dy = node.y - y;
    double dist = std::sqrt(dx*dx + dy*dy);
    
    if (dist < best_dist) {
        best_dist = dist;
        best_id = node.id;
    }
    
    int axis = depth % 2;
    double axis_dist = (axis == 0) ? (x - node.x) : (y - node.y);
    
    int first = (axis_dist < 0) ? node.left : node.right;
    int second = (axis_dist < 0) ? node.right : node.left;
    
    nearestRecursive(first, x, y, depth + 1, best_id, best_dist);
    if (std::abs(axis_dist) < best_dist) {
        nearestRecursive(second, x, y, depth + 1, best_id, best_dist);
    }
}

std::vector<int> KDTree::kNearestNeighbors(double x, double y, int k) const {
    std::vector<int> res;
    if (tree_.empty() || k <= 0) return res;
    
    std::priority_queue<std::pair<double, int>> pq;
    kNearestRecursive(root_, x, y, 0, k, pq);
    
    while (!pq.empty()) {
        res.push_back(pq.top().second);
        pq.pop();
    }
    std::reverse(res.begin(), res.end());
    return res;
}

void KDTree::kNearestRecursive(int node_idx, double x, double y, int depth, int k, std::priority_queue<std::pair<double, int>>& pq) const {
    if (node_idx == -1) return;
    
    const auto& node = tree_[node_idx];
    double dx = node.x - x;
    double dy = node.y - y;
    double dist = std::sqrt(dx*dx + dy*dy);
    
    if (pq.size() < (size_t)k) {
        pq.push({dist, node.id});
    } else if (dist < pq.top().first) {
        pq.pop();
        pq.push({dist, node.id});
    }
    
    int axis = depth % 2;
    double axis_dist = (axis == 0) ? (x - node.x) : (y - node.y);
    
    int first = (axis_dist < 0) ? node.left : node.right;
    int second = (axis_dist < 0) ? node.right : node.left;
    
    kNearestRecursive(first, x, y, depth + 1, k, pq);
    if (pq.size() < (size_t)k || std::abs(axis_dist) < pq.top().first) {
        kNearestRecursive(second, x, y, depth + 1, k, pq);
    }
}

std::vector<int> KDTree::rangeQuery(double x, double y, double radius) const {
    std::vector<int> res;
    if (tree_.empty()) return res;
    rangeRecursive(root_, x, y, radius, 0, res);
    return res;
}

void KDTree::rangeRecursive(int node_idx, double x, double y, double radius, int depth, std::vector<int>& result) const {
    if (node_idx == -1) return;
    
    const auto& node = tree_[node_idx];
    double dx = node.x - x;
    double dy = node.y - y;
    double dist = std::sqrt(dx*dx + dy*dy);
    
    if (dist <= radius) {
        result.push_back(node.id);
    }
    
    int axis = depth % 2;
    double axis_dist = (axis == 0) ? (x - node.x) : (y - node.y);
    
    int first = (axis_dist < 0) ? node.left : node.right;
    int second = (axis_dist < 0) ? node.right : node.left;
    
    rangeRecursive(first, x, y, radius, depth + 1, result);
    if (std::abs(axis_dist) <= radius) {
        rangeRecursive(second, x, y, radius, depth + 1, result);
    }
}

} // namespace buildsim
