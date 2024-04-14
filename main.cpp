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

#ifdef USE_POOL

#include <sys/resource.h>
#include <sys/mman.h>


struct pool_t {

    static constexpr const std::size_t INIT_ADDRESS = 0x7fffffff000;
    static constexpr const std::size_t INIT_SIZE = 4096;

    void * address = nullptr;

public:

    pool_t() {
        fix_stack_limit();

        void * const ptr = mmap(
            reinterpret_cast<void*>(INIT_ADDRESS),
            INIT_SIZE,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN | MAP_STACK,
            -1,
            0
        );

        if (ptr == MAP_FAILED) {
            throw std::system_error { errno, std::system_category() };
        }

        std::cout << "mmap result: " << ptr << std::endl;

        address = reinterpret_cast<uint8_t*>(ptr);
    }

    void * alloc(std::size_t size) {
        uint8_t * const ptr = reinterpret_cast<uint8_t*>(address);

        // std::cout << "allocated: " << reinterpret_cast<void*>(ptr) << std::endl;

        address = ptr - size;
        return ptr;
    }

    template<typename T>
    T * alloc(void) {
        return reinterpret_cast<T*>(alloc(sizeof(T)));
    }

private:

    static void fix_stack_limit(void) {
        struct rlimit stack_rlimit;

        if (getrlimit(RLIMIT_STACK, &stack_rlimit)) {
            throw std::system_error { errno, std::system_category(), "unable to get stack limit" };
        }

        stack_rlimit.rlim_cur = stack_rlimit.rlim_max;

        if (setrlimit(RLIMIT_STACK, &stack_rlimit)) {
            throw std::system_error { errno, std::system_category(), "unable to set stack limit" };
        }
    }

} pool;

#endif

#endif


struct Edge {

    Edge * next;
    std::size_t node_id;

#ifdef USE_POOL

    template<typename ... Args>
    static Edge * create(Args && ... args) {
        return new(pool.alloc<Edge>()) Edge { std::forward<Args>(args)... };
    }

    void destroy() noexcept {}

#else

    template<typename ... Args>
    static Edge * create(Args && ... args) {
        return new Edge { std::forward<Args>(args)... };
    }

    void destroy() noexcept {
        delete this;
    }

#endif

};

class Graph {

    std::vector<Edge *> nodes;

public:

    Graph(std::size_t n): nodes(n, nullptr) {}

    ~Graph() noexcept {
        for (const auto & node : nodes) {
            for (Edge * p = node; p; ) {
                Edge * const next = p->next;

                p->destroy();
                p = next;
            }
        }
    }

    void connect(Edge * & from, std::size_t to) {
        from = Edge::create(from, to);
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
