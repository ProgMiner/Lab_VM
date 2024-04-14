#include <chrono>
#include <iostream>
#include <stdint.h>
#include <vector>


#ifdef _WIN32
  #include "windows.h"
  #include "psapi.h"
  static int64_t memory_usage(void) {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    return pmc.PrivateUsage;
  }
#else
  #include <fstream>
  using namespace std;
  static uint64_t memory_usage(void) {
    enum { PAGE_SIZE = 4096 };
    ifstream ifs("/proc/self/statm");
    uint64_t m;
    ifs >> m;
    return m * PAGE_SIZE;
  }
#endif

using namespace std;

struct Edge {
  Edge* next;
  int node_id;
};

class Graph {
  vector<Edge*> nodes;
public:
  Graph(int n) : nodes(n, nullptr) {}
  ~Graph() {
    for (const auto& node : nodes) {
      for (Edge* p = node; p;) {
        Edge* next = p->next;
        delete p;
        p = next;
      }
    }
  }
  void connect(Edge*& from, int to) {
    from = new Edge{from, to};
  }
  void connect(int from, int to) {
    connect(nodes[from], to);
  }
  void build_complete_digraph() {
    const auto n = nodes.size();
    for (size_t i = 0; i < n; i++)
      for (size_t j = 0; j < n; j++)
        if (i != j) connect(i, j);
  }
};

static inline int64_t test(void) {
  const auto start = memory_usage();
  Graph g(10000);
  g.build_complete_digraph();
  return memory_usage() - start;
}

int main(const int argc, const char* argv[]) {
  const auto start = chrono::high_resolution_clock::now();
  const auto memory_used = test();
  const auto end = chrono::high_resolution_clock::now();
  cout << "Memory used: " << memory_used << " bytes\n";
  cout << "Time used: " << chrono::duration_cast<chrono::nanoseconds>(end - start).count() << " ns\n";
  return 0;
}
