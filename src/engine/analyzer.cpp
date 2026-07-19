#include "buildsim/engine/analyzer.h"
#include "buildsim/algorithms/dijkstra.h"
#include <iostream>
#include <iomanip>
#include <cmath>

namespace buildsim {

CoverageResult Analyzer::analyzeCoverage(const Graph& g, 
                                         const std::vector<Facility>& facilities,
                                         const std::vector<DemandPoint>& demands) {
    Timer t;
    CoverageResult result;
    if (demands.empty() || facilities.empty()) return result;

    std::vector<int> sources;
    for (const auto& f : facilities) {
        sources.push_back(f.node_id);
    }

    auto dists = multiSourceDijkstra(g, sources);

    double sum_dist = 0;
    double max_dist = 0;
    int c1=0, c3=0, c5=0, c10=0;
    
    std::vector<double> valid_dists;

    for (const auto& d : demands) {
        double dist = dists[d.node_id];
        if (dist == INF) {
            result.underserved_count++;
            continue;
        }
        
        valid_dists.push_back(dist);
        sum_dist += dist;
        if (dist > max_dist) max_dist = dist;
        
        if (dist <= 1000.0) c1++;
        if (dist <= 3000.0) c3++;
        if (dist <= 5000.0) c5++;
        if (dist <= 10000.0) c10++;
    }

    int n_valid = valid_dists.size();
    if (n_valid > 0) {
        result.avg_distance = sum_dist / n_valid;
        result.max_distance = max_dist;
        
        double variance_sum = 0;
        for (double d : valid_dists) {
            variance_sum += (d - result.avg_distance) * (d - result.avg_distance);
        }
        result.std_deviation = std::sqrt(variance_sum / n_valid);
    }

    int n_total = demands.size();
    result.coverage_1km = (double)c1 / n_total * 100.0;
    result.coverage_3km = (double)c3 / n_total * 100.0;
    result.coverage_5km = (double)c5 / n_total * 100.0;
    result.coverage_10km = (double)c10 / n_total * 100.0;
    
    result.solve_time_ms = t.elapsedMs();
    return result;
}

void Analyzer::printComparison(const CoverageResult& before, const CoverageResult& after) {
    printHeader("Coverage Comparison");
    auto printRow = [](const std::string& label, double b, double a, const std::string& unit) {
        std::cout << std::left << std::setw(25) << label 
                  << std::right << std::setw(10) << std::fixed << std::setprecision(2) << b << unit
                  << " -> " 
                  << std::right << std::setw(10) << std::fixed << std::setprecision(2) << a << unit;
        
        double diff = a - b;
        std::cout << "  (" << (diff > 0 ? "+" : "") << diff << unit << ")\n";
    };

    printRow("Avg Distance", before.avg_distance, after.avg_distance, "m");
    printRow("Max Distance", before.max_distance, after.max_distance, "m");
    printRow("Coverage (1km)", before.coverage_1km, after.coverage_1km, "%");
    printRow("Coverage (3km)", before.coverage_3km, after.coverage_3km, "%");
    printRow("Coverage (5km)", before.coverage_5km, after.coverage_5km, "%");
    printRow("Coverage (10km)", before.coverage_10km, after.coverage_10km, "%");
    printSeparator('-');
}

void Analyzer::printCoverageReport(const CoverageResult& result) {
    printHeader("Coverage Report");
    std::cout << "  Analysis Time:     " << std::fixed << std::setprecision(2) << result.solve_time_ms << " ms\n";
    std::cout << "  Avg Distance:      " << result.avg_distance << " m\n";
    std::cout << "  Max Distance:      " << result.max_distance << " m\n";
    std::cout << "  Std Deviation:     " << result.std_deviation << " m\n";
    std::cout << "  Underserved Nodes: " << result.underserved_count << "\n\n";
    std::cout << "  Coverage thresholds:\n";
    std::cout << "    <= 1 km:  " << result.coverage_1km << "%\n";
    std::cout << "    <= 3 km:  " << result.coverage_3km << "%\n";
    std::cout << "    <= 5 km:  " << result.coverage_5km << "%\n";
    std::cout << "    <= 10 km: " << result.coverage_10km << "%\n";
    printSeparator('-');
}

} // namespace buildsim
