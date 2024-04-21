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

#include <sys/mman.h>


struct pool_t {

    static constexpr const std::size_t PAGE_SIZE = 4096;

    uint8_t * start_address = nullptr;
    uint8_t * address = nullptr;

public:

    pool_t(std::size_t pool_size) {
        std::cerr << "pool size: " << pool_size << std::endl;

        address = start_address = allocate_pool(pool_size, PAGE_SIZE);

        std::cerr << "pool start: " << reinterpret_cast<void *>(address) << std::endl;
    }

    ~pool_t() noexcept(false) {
        std::size_t addr = reinterpret_cast<std::size_t>(address);
        addr = (addr / PAGE_SIZE) * PAGE_SIZE;

        address = reinterpret_cast<uint8_t *>(addr);
        if (munmap(address, start_address - address)) {
            throw std::system_error { errno, std::system_category(), "unable to do munmap" };
        }
    }

    void * alloc(std::size_t size) {
        void * const result = address -= size;

        // std::cerr << "allocated: " << result << std::endl;
        return result;
    }

    template<typename T>
    T * alloc(void) {
        return reinterpret_cast<T*>(alloc(sizeof(T)));
    }

private:

    // https://github.com/linux-test-project/ltp/blob/03333e6f8c2a7c9fa96fc5bdfbbaac0531aa750b/testcases/kernel/syscalls/mmap/mmap18.c

    static uint8_t * allocate_pool(std::size_t pool_size, std::size_t initial_size) {
	    const std::size_t reserved_size = 256 * PAGE_SIZE + pool_size;

	    uint8_t * const start = reinterpret_cast<uint8_t *>(mmap(nullptr, reserved_size,
            PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

        if (start == MAP_FAILED) {
            throw std::system_error { errno, std::system_category(), "unable to do mmap" };
        }

        std::cerr << "mmap result: " << reinterpret_cast<void *>(start) << std::endl;

	    if (munmap(start, reserved_size)) {
            throw std::system_error { errno, std::system_category(), "unable to do munmap" };
        }

	    if (mmap(start + reserved_size - initial_size, initial_size, PROT_READ | PROT_WRITE,
	            MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN, -1, 0) == MAP_FAILED) {
            throw std::system_error { errno, std::system_category(), "unable to do mmap" };
        }

	    return start + reserved_size;
    }
};

#endif

#endif


struct Edge {

    Edge * next;
    std::size_t node_id;

};

class Graph {

    std::vector<Edge *> nodes;

#ifdef USE_POOL

    pool_t pool;

#endif

public:

    Graph(std::size_t n)
        : nodes(n, nullptr)
#ifdef USE_POOL
        , pool(n * n * sizeof(Edge))
#endif
        {}

#ifdef USE_POOL

    ~Graph() = default;

#else

    ~Graph() noexcept {
        for (const auto & node : nodes) {
            for (Edge * p = node; p; ) {
                Edge * const next = p->next;

                delete p;
                p = next;
            }
        }
    }

#endif

    void connect(Edge * & from, std::size_t to) {
#ifdef USE_POOL

        from = new(pool.alloc<Edge>()) Edge { from, to };

#else

        from = new Edge { from, to };

#endif
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
