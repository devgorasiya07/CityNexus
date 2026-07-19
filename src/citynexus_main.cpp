#include "buildsim/common.h"
#include "buildsim/graph.h"
#include "buildsim/algorithms/dijkstra.h"
#include "buildsim/algorithms/astar.h"
#include "buildsim/algorithms/contraction.h"
#include "buildsim/algorithms/mst.h"
#include "buildsim/algorithms/facility.h"
#include "buildsim/algorithms/steiner.h"
#include "buildsim/algorithms/isochrone.h"
#include "buildsim/spatial/kdtree.h"
#include "buildsim/engine/analyzer.h"
#include "buildsim/engine/planner.h"

#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <limits>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <windows.h>

using namespace buildsim;

// ====================== UI HELPERS ======================

void cls() {
#ifdef _WIN32
    std::system("cls");
#else
    std::system("clear");
#endif
}

void line(char ch = '=', int width = 60) {
    std::cout << "  ";
    for (int i = 0; i < width; ++i) std::cout << ch;
    std::cout << "\n";
}

void banner(const std::string& title) {
    std::cout << "\n";
    line('=');
    int pad = (60 - (int)title.size()) / 2;
    std::cout << "  ";
    for (int i = 0; i < pad; ++i) std::cout << " ";
    std::cout << title << "\n";
    line('=');
}

void sectionTitle(const std::string& title) {
    std::cout << "\n";
    line('-');
    std::cout << "    " << title << "\n";
    line('-');
}

void explain(const std::string& text) {
    std::cout << "    [INFO] " << text << "\n";
}

void menuItem(int num, const std::string& label) {
    std::cout << "    [" << num << "]  " << label << "\n";
}

void menuBack() {
    std::cout << "\n    [0]  << Back to Main Menu\n";
}

void statusBox(const std::string& status) {
    std::cout << "\n  +----------------------------------------------------------+\n";
    int pad = (58 - (int)status.size()) / 2;
    std::cout << "  |";
    for (int i = 0; i < pad; ++i) std::cout << " ";
    std::cout << status;
    for (int i = 0; i < 58 - pad - (int)status.size(); ++i) std::cout << " ";
    std::cout << "|\n";
    std::cout << "  +----------------------------------------------------------+\n";
}

void resultRow(const std::string& label, const std::string& value) {
    std::cout << "    " << std::left << std::setw(28) << label << ": " << value << "\n";
}

void resultRow(const std::string& label, double value, const std::string& unit = "") {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << value;
    if (!unit.empty()) ss << " " << unit;
    resultRow(label, ss.str());
}

void resultRow(const std::string& label, int value) {
    resultRow(label, std::to_string(value));
}

void successMsg(const std::string& msg) {
    std::cout << "\n    [OK] " << msg << "\n";
}

void errorMsg(const std::string& msg) {
    std::cout << "\n    [!!] " << msg << "\n";
}

int getChoice() {
    int choice;
    std::cout << "\n  >> Enter choice: ";
    if (!(std::cin >> choice)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return -1;
    }
    return choice;
}

int getInt(const std::string& prompt, int default_val = -1) {
    int val;
    if (default_val >= 0) {
        std::cout << "    " << prompt << " [default: " << default_val << "]: ";
    } else {
        std::cout << "    " << prompt << ": ";
    }
    if (!(std::cin >> val)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return default_val;
    }
    return val;
}

double getDouble(const std::string& prompt, double default_val = -1) {
    double val;
    if (default_val >= 0) {
        std::cout << "    " << prompt << " [default: " << default_val << "]: ";
    } else {
        std::cout << "    " << prompt << ": ";
    }
    if (!(std::cin >> val)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return default_val;
    }
    return val;
}

void pause() {
    std::cout << "\n  ";
    line('-');
    std::cout << "    Press ENTER to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

std::vector<DemandPoint> generateRandomDemands(int n, int num_demands) {
    std::vector<DemandPoint> demands;
    if (n == 0) return demands;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, n - 1);
    for (int i = 0; i < num_demands; ++i) {
        demands.push_back(DemandPoint(dist(rng), 1.0));
    }
    return demands;
}

// ====================== SUB-MENUS ======================

void handleGraphMenu(Graph& graph, bool& graph_loaded) {
    cls();
    banner("CITY GENERATION & ROAD NETWORK SETUP");
    std::cout << "\n";
    explain("This module generates the intersections (Nodes) and the");
    explain("roads connecting them (Edges). Road distances (Weights) are");
    explain("automatically calculated in meters based on real-world geometry.");
    std::cout << "\n";

    menuItem(1, "Build Grid-Layout City (e.g., Manhattan)");
    menuItem(2, "Build Radial-Layout City (e.g., Paris/New Delhi)");
    menuItem(3, "Build Random Sprawl City");
    if (graph_loaded) {
        std::cout << "\n";
        menuItem(4, "Show City Infrastructure Statistics");
    }
    menuBack();

    int c = getChoice();

    if (c == 1) {
        sectionTitle("Grid City Generator");
        int r = getInt("City Blocks (Rows)", 50);
        int cols = getInt("City Blocks (Columns)", 50);
        std::cout << "\n    Laying out concrete and paving roads...\n";
        graph = generateGridCity(r, cols);
        graph_loaded = true;

        sectionTitle("Construction Report");
        resultRow("City Type", "Grid Layout");
        resultRow("Intersections (Nodes)", graph.numNodes());
        resultRow("Road Segments (Edges)", graph.numEdges());
        successMsg("City successfully constructed!");

    } else if (c == 2) {
        sectionTitle("Radial City Generator");
        int rings = getInt("Number of Ring Roads", 20);
        int nper = getInt("Intersections per Ring", 50);
        std::cout << "\n    Laying out concrete and paving roads...\n";
        graph = generateRadialCity(rings, nper);
        graph_loaded = true;

        sectionTitle("Construction Report");
        resultRow("City Type", "Radial Layout");
        resultRow("Intersections (Nodes)", graph.numNodes());
        resultRow("Road Segments (Edges)", graph.numEdges());
        successMsg("City successfully constructed!");

    } else if (c == 3) {
        sectionTitle("Random Sprawl City Generator");
        int n = getInt("Total Intersections", 5000);
        std::cout << "\n    Laying out concrete and paving roads...\n";
        graph = generateRandomCity(n);
        graph_loaded = true;

        sectionTitle("Construction Report");
        resultRow("City Type", "Urban Sprawl (Random)");
        resultRow("Intersections (Nodes)", graph.numNodes());
        resultRow("Road Segments (Edges)", graph.numEdges());
        successMsg("City successfully constructed!");

    } else if (c == 4 && graph_loaded) {
        sectionTitle("City Infrastructure Statistics");
        resultRow("Total Intersections", graph.numNodes());
        resultRow("Total Road Segments", graph.numEdges());
        resultRow("Avg Roads per Intersection", graph.avgDegree());
        resultRow("Busiest Intersection Roads", graph.maxDegree());

    } else {
        return; // Back
    }
    pause();
}

void handlePathMenu(Planner& planner, Graph& graph) {
    cls();
    banner("NAVIGATION & TRAFFIC ROUTING");
    std::cout << "\n";
    explain("This module finds the absolute shortest driving distance");
    explain("between two intersections in your city. It calculates the");
    explain("actual distance in meters by driving along the road segments.");
    std::cout << "\n";

    menuItem(1, "Dijkstra's Algorithm (Thorough but slow)");
    menuItem(2, "A* Search (GPS-style, fast and directed)");
    menuItem(3, "Contraction Hierarchies (Enterprise-grade, instant)");
    std::cout << "\n";
    menuItem(4, "Benchmark: Compare all routing engines");
    menuItem(5, "Precompute Contraction Hierarchies (Required for CH)");
    menuBack();

    int c = getChoice();

    if (c >= 1 && c <= 4) {
        sectionTitle("GPS Input");
        explain("Valid Intersection IDs range from 0 to " + std::to_string(graph.numNodes() - 1));
        int u = getInt("Starting Intersection ID");
        int v = getInt("Destination Intersection ID");

        if (!graph.isValidNode(u) || !graph.isValidNode(v)) {
            errorMsg("Invalid intersection IDs!");
            pause();
            return;
        }

        if (c == 1) {
            PathResult res = planner.findShortestPath(u, v, "dijkstra");
            sectionTitle("Dijkstra Routing Result");
            resultRow("Route Found?", res.found ? "YES" : "NO");
            resultRow("Total Driving Distance", res.distance, "meters");
            resultRow("Intersections Scanned", res.nodes_explored);
            resultRow("Computation Time", res.time_ms, "ms");

        } else if (c == 2) {
            PathResult res = planner.findShortestPath(u, v, "astar");
            sectionTitle("A* Search Routing Result");
            resultRow("Route Found?", res.found ? "YES" : "NO");
            resultRow("Total Driving Distance", res.distance, "meters");
            resultRow("Intersections Scanned", res.nodes_explored);
            resultRow("Computation Time", res.time_ms, "ms");

        } else if (c == 3) {
            if (!planner.isCHReady()) {
                errorMsg("CH Engine is not ready! Please run Option 5 first.");
                pause();
                return;
            }
            PathResult res = planner.findShortestPath(u, v, "ch");
            sectionTitle("Contraction Hierarchies Result");
            resultRow("Route Found?", res.found ? "YES" : "NO");
            resultRow("Total Driving Distance", res.distance, "meters");
            resultRow("Intersections Scanned", res.nodes_explored);
            resultRow("Computation Time", res.time_ms, "ms");

        } else if (c == 4) {
            PathResult d = planner.findShortestPath(u, v, "dijkstra");
            PathResult a = planner.findShortestPath(u, v, "astar");

            sectionTitle("Routing Engine Benchmark");
            std::cout << "\n    " << std::left
                      << std::setw(14) << "Algorithm"
                      << std::setw(16) << "Total Distance"
                      << std::setw(14) << "Intersections Scanned"
                      << std::setw(14) << "Time (ms)" << "\n";
            line('-');
            auto printRow = [](const std::string& name, const PathResult& r) {
                std::cout << "    " << std::left << std::setw(14) << name
                          << std::setw(16) << std::fixed << std::setprecision(1) << r.distance
                          << std::setw(21) << r.nodes_explored
                          << std::setw(14) << std::fixed << std::setprecision(3) << r.time_ms << "\n";
            };
            printRow("Dijkstra", d);
            printRow("A* Search", a);
            if (planner.isCHReady()) {
                PathResult ch = planner.findShortestPath(u, v, "ch");
                printRow("CH Engine", ch);
                std::cout << "\n";
                double speedup = (ch.time_ms > 0) ? d.time_ms / ch.time_ms : 0;
                resultRow("CH Speedup vs Dijkstra", std::to_string((int)speedup) + "x faster");
            }
        }

    } else if (c == 5) {
        sectionTitle("Precomputing Contraction Hierarchies");
        std::cout << "    Building highway shortcuts and optimizing graph...\n";
        std::cout << "    This may take a moment for massive cities.\n\n";
        planner.preprocessCH();
        successMsg("CH Engine is now online and ready for instant routing!");

    } else {
        return;
    }
    pause();
}

void handleFacilityMenu(Planner& planner, Graph& graph) {
    cls();
    banner("EMERGENCY SERVICES DEPLOYMENT");
    std::cout << "\n";
    explain("Deploy Hospitals across the city to minimize ambulance");
    explain("response times. The algorithms will calculate the optimal");
    explain("intersections to build hospitals to cover the most citizens.");
    std::cout << "\n";

    menuItem(1, "Greedy Deployment (Fast but suboptimal)");
    menuItem(2, "p-Center Deployment (Minimizes the MAXIMUM distance for any citizen)");
    menuItem(3, "p-Median Deployment (Minimizes the AVERAGE distance for all citizens)");
    std::cout << "\n";
    menuItem(4, "Ambulance Reachability Map (Isochrone)");
    menuBack();

    int c = getChoice();

    if (c >= 1 && c <= 3) {
        sectionTitle("Hospital Deployment Parameters");
        int count = getInt("Number of Hospitals to build", 5);
        auto demands = generateRandomDemands(graph.numNodes(), std::min(500, graph.numNodes()));
        std::string method = (c == 1) ? "greedy" : (c == 2) ? "pcenter" : "pmedian";

        std::cout << "\n    Simulating 500 residential neighborhoods as demand points...\n";
        std::cout << "    Calculating optimal intersections to build " << count << " Hospitals...\n";
        PlacementResult res = planner.placeFacilities(FacilityType::HOSPITAL, count, demands, method);

        sectionTitle("Deployment Results");
        resultRow("Strategy Used", method);
        resultRow("Hospitals Placed", (int)res.facility_nodes.size());
        resultRow("Worst-Case Ambulance Trip", res.max_distance, "meters");
        resultRow("Average Ambulance Trip", res.avg_distance, "meters");
        resultRow("Citizens within 5km", res.coverage_within_threshold, "%");
        resultRow("Calculation Time", res.solve_time_ms, "ms");

        std::cout << "\n    Build Hospitals at Intersection IDs: ";
        for (size_t i = 0; i < res.facility_nodes.size() && i < 10; ++i) {
            std::cout << res.facility_nodes[i];
            if (i + 1 < res.facility_nodes.size()) std::cout << ", ";
        }
        if (res.facility_nodes.size() > 10) std::cout << " ...";
        std::cout << "\n";

    } else if (c == 4) {
        sectionTitle("Ambulance Reachability Map");
        explain("Finds every single intersection an ambulance can reach");
        explain("from a specific hospital within a given driving distance.");
        
        int src = getInt("Hospital Intersection ID");
        double max_d = getDouble("Max driving distance (meters)", 5000);

        if (!graph.isValidNode(src)) {
            errorMsg("Invalid intersection ID.");
            pause();
            return;
        }

        std::cout << "\n    Mapping all reachable roads...\n";
        IsochroneResult res = planner.computeReachability(src, max_d);

        sectionTitle("Reachability Results");
        resultRow("Hospital Location (Node)", src);
        resultRow("Max Driving Distance", max_d, "meters");
        resultRow("Intersections in Range", (int)res.reachable_nodes.size());
        resultRow("Total City Coverage", res.coverage_percent, "%");

    } else {
        return;
    }
    pause();
}

void handleNetworkMenu(Planner& planner, Graph& graph) {
    cls();
    banner("CITY-WIDE CABLE LAYING (MIN-COST NETWORK)");
    std::cout << "\n";
    explain("This module designs a Fiber-Optic network that connects");
    explain("intersections while minimizing the total length of cable used.");
    explain("Minimum Spanning Trees (MST) connect EVERY intersection, while");
    explain("Steiner Trees connect only specific critical buildings.");
    std::cout << "\n";

    menuItem(1, "Connect ALL Intersections (Prim's MST)");
    menuItem(2, "Connect ALL Intersections (Kruskal's MST)");
    menuItem(3, "Connect only Government Buildings (Steiner Tree)");
    std::cout << "\n";
    menuItem(4, "Compare Prim's vs Kruskal's Performance");
    menuBack();

    int c = getChoice();

    if (c == 1 || c == 2) {
        std::string algo_name = (c == 1) ? "Prim's" : "Kruskal's";
        std::cout << "\n    Calculating the minimum cable required to connect the entire city...\n";

        TreeResult res = (c == 1) ? primMST(graph) : kruskalMST(graph);

        sectionTitle(algo_name + " Network Result");
        resultRow("Total Cable Required", res.total_cost, "meters");
        resultRow("Road Segments Dug Up", (int)res.edges.size());
        resultRow("Intersections Connected", res.num_nodes);
        resultRow("Calculation Time", res.solve_time_ms, "ms");

    } else if (c == 3) {
        sectionTitle("Government Fiber Network (Steiner Tree)");
        explain("A Steiner Tree finds the cheapest way to connect a subset");
        explain("of nodes (e.g. Government Buildings), using other roads only if necessary.");
        
        int t_count = getInt("Number of Government Buildings", 10);

        std::mt19937 rng(1337);
        std::uniform_int_distribution<int> dist(0, graph.numNodes() - 1);
        std::vector<int> terms;
        for (int i = 0; i < t_count; ++i) terms.push_back(dist(rng));

        std::cout << "\n    Designing custom fiber network for " << t_count << " buildings...\n";
        TreeResult res = planner.designNetwork(terms);

        sectionTitle("Steiner Tree Result");
        resultRow("Buildings Connected", t_count);
        resultRow("Total Cable Required", res.total_cost, "meters");
        resultRow("Calculation Time", res.solve_time_ms, "ms");

    } else if (c == 4) {
        std::cout << "\n    Racing Prim's and Kruskal's to find the minimum cable layout...\n";
        TreeResult p = primMST(graph);
        TreeResult k = kruskalMST(graph);

        sectionTitle("MST Algorithm Benchmark");
        std::cout << "\n    " << std::left
                  << std::setw(14) << "Algorithm"
                  << std::setw(22) << "Total Cable (meters)"
                  << std::setw(14) << "Roads Dug"
                  << std::setw(14) << "Time (ms)" << "\n";
        line('-');
        std::cout << "    " << std::left << std::setw(14) << "Prim's"
                  << std::setw(22) << std::fixed << std::setprecision(1) << p.total_cost
                  << std::setw(14) << p.edges.size()
                  << std::setw(14) << std::fixed << std::setprecision(3) << p.solve_time_ms << "\n";
        std::cout << "    " << std::left << std::setw(14) << "Kruskal's"
                  << std::setw(22) << std::fixed << std::setprecision(1) << k.total_cost
                  << std::setw(14) << k.edges.size()
                  << std::setw(14) << std::fixed << std::setprecision(3) << k.solve_time_ms << "\n";

    } else {
        return;
    }
    pause();
}

// ====================== MAIN MENU ======================

extern "C" __declspec(dllexport) void CALLBACK RunApp(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow) {
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONIN$", "r", stdin);

    Graph graph;
    Planner planner(graph);
    bool graph_loaded = false;

    while (true) {
        cls();

        std::cout << "\n";
        std::cout << "    ____ _ _         _   _                     \n";
        std::cout << "   / ___(_) |_ _   _| \\ | | _____  ___   _ ___ \n";
        std::cout << "  | |   | | __| | | |  \\| |/ _ \\ \\/ / | | / __|\n";
        std::cout << "  | |___| | |_| |_| | |\\  |  __/>  <| |_| \\__ \\\n";
        std::cout << "   \\____|_|\\__|\\__, |_| \\_|\\___/_/\\_\\\\__,_|___/\n";
        std::cout << "               |___/                           \n";
        std::cout << "\n";
        line('=');
        std::cout << "         CityNexus v1.0 - Urban Infrastructure Simulator\n";
        line('=');

        if (graph_loaded) {
            statusBox("City Status: " + std::to_string(graph.numNodes()) + " Intersections | " + std::to_string(graph.numEdges()) + " Roads");
        } else {
            statusBox("Welcome Mayor! No City Generated Yet.");
        }

        std::cout << "\n";
        menuItem(1, "City Generation & Road Network Setup");
        menuItem(2, "Navigation & Traffic Routing (Shortest Paths)");
        menuItem(3, "Emergency Services Deployment (Hospitals/Facilities)");
        menuItem(4, "City-Wide Cable Laying (Min-Cost Networks)");
        std::cout << "\n";
        menuItem(0, "Quit Simulator");

        int choice = getChoice();

        if (choice == 0) {
            cls();
            std::cout << "\n    Shutting down CityNexus... Goodbye, Mayor!\n\n";
            break;
        }

        if (choice >= 2 && choice <= 4 && !graph_loaded) {
            errorMsg("You must build or load a city first! (Use Option 1)");
            pause();
            continue;
        }

        switch (choice) {
            case 1: handleGraphMenu(graph, graph_loaded); break;
            case 2: handlePathMenu(planner, graph); break;
            case 3: handleFacilityMenu(planner, graph); break;
            case 4: handleNetworkMenu(planner, graph); break;
            default: errorMsg("Invalid choice."); pause(); break;
        }
    }
    FreeConsole();
}
