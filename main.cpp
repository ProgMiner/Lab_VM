#include <iostream>
#include <cstdint>
#include <vector>
#include <chrono>


#ifdef _WIN32

#include "windows.h"
#include "psapi.h"


static int64_t memory_usage(void) {
    PROCESS_MEMORY_COUNTERS_EX pmc;

    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*) &pmc, sizeof(pmc));
    return pmc.PrivateUsage;
}

#else

#include <fstream>


static uint64_t memory_usage(void) {
    enum { PAGE_SIZE = 4096 };

    std::ifstream ifs("/proc/self/statm");
    uint64_t m;

    ifs >> m;
    return m * PAGE_SIZE;
}

#endif


struct Edge {

    Edge * next;
    std::size_t node_id;
};

class Graph {

    std::vector<Edge *> nodes;

public:

    Graph(std::size_t n): nodes(n, nullptr) {}

    ~Graph() noexcept {
        for (const auto & node : nodes) {
            for (Edge * p = node; p; ) {
                Edge * const next = p->next;

                delete p;
                p = next;
            }
        }
    }

    void connect(Edge * & from, std::size_t to) {
        from = new Edge { from, to };
    }

    void connect(std::size_t from, std::size_t to) {
        connect(nodes[from], to);
    }

    void build_complete_digraph() {
        const std::size_t n = nodes.size();

        for (std::size_t i = 0; i < n; i++) {
            for (std::size_t j = 0; j < n; j++) {
                if (i != j) {
                    connect(i, j);
                }
            }
        }
    }
};

static inline int64_t test(void) {
    const auto start = memory_usage();

    Graph g(10000);
    g.build_complete_digraph();
    return memory_usage() - start;
}

int main() {
    const auto start = std::chrono::high_resolution_clock::now();

    const auto memory_used = test();

    const auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Memory used: " << memory_used << " bytes\n";

    std::cout << "Time used: "
              << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()
              << " ns\n";

    return 0;
}
