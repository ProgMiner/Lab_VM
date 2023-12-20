#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <random>


static constexpr const uint64_t PAGE_SIZE = 4096; // bytes
static constexpr const uint64_t HEAT_PATTERN_SIZE = 512 * 1024 / sizeof(void *);
static constexpr const uint64_t REPEATS = 10000000;

static void ** heat_pattern = nullptr;
uint64_t counter = 0;

static std::ofstream log_file { "./log.txt" };


static __inline__ unsigned long long rdtsc(void) {
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((unsigned long long) lo) | (((unsigned long long) hi) << 32);
}

static void generate_pattern(void ** pattern, std::size_t size) {
    std::vector<void *> ptrs;
    ptrs.reserve(size);

    for (uint64_t i = 1; i < size; ++i) {
        ptrs.push_back(&pattern[i]);
    }

    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(ptrs.begin(), ptrs.end(), g);

    ptrs.push_back(&pattern[0]);

    void * current_ptr = &pattern[0];
    for (void * ptr : ptrs) {
        *reinterpret_cast<void**>(current_ptr) = ptr;
        current_ptr = ptr;
    }
}

static void heat() {
    void * ptr = &heat_pattern[0];

    for (uint64_t i = 0; i < HEAT_PATTERN_SIZE; ++i) {
        ptr = *reinterpret_cast<void **>(ptr);
        counter += *reinterpret_cast<uint64_t *>(ptr);
    }
}

template<typename W>
static uint64_t measure_work(W work) {
    const uint64_t start = rdtsc();

    for (uint64_t i = 0; i < REPEATS; ++i) {
        work();
    }

    const uint64_t end = rdtsc();

    return end - start;
}

// static uint64_t measure_random_pattern(std::size_t size_bytes) {
//     const std::size_t size = size_bytes / sizeof(void *);
//
//     void ** const pattern = new(std::align_val_t(PAGE_SIZE)) void *[size];
//     generate_pattern(pattern, size);
//
//     heat();
//
//     void * ptr = pattern;
//     const uint64_t result = measure_work([&ptr] {
//         ptr = *reinterpret_cast<void **>(ptr);
//     });
//
//     counter += *reinterpret_cast<uint64_t *>(ptr);
//     return result;
// }

// static uint64_t measure_cache_size() {
//     uint64_t prev_measure = measure_random_pattern(8);
//
//     for (uint64_t size = 16; size < 64 * 1024 * 1024; size *= 2) {
//         const uint64_t m = measure_random_pattern(size);
//
//         if (m > prev_measure + REPEATS) {
//             return size / 2;
//         }
//
//         prev_measure = m;
//     }
//
//     throw std::logic_error { "cannot determine cache size" };
// }

static uint64_t measure_pattern(
    std::size_t stride_bytes,
    std::size_t spots
) {
    const std::size_t size_bytes = stride_bytes * spots;
    const std::size_t size = size_bytes / sizeof(void *);

    void ** const pattern = new(std::align_val_t(PAGE_SIZE)) void *[size];

    {
        const std::size_t stride = stride_bytes / sizeof(void *);
        void * ptr = &pattern[0];

        for (uint64_t i = 1; i < spots; ++i) {
            *reinterpret_cast<void **>(ptr) = reinterpret_cast<void **>(ptr) + stride;
            ptr = *reinterpret_cast<void **>(ptr);
        }

        *reinterpret_cast<void **>(ptr) = &pattern[0];
    }

    heat();

    void * ptr = pattern;
    const uint64_t result = measure_work([&ptr] {
        ptr = *reinterpret_cast<void **>(ptr);
    });

    counter += *reinterpret_cast<uint64_t *>(ptr);
    return result;
}

static std::pair<std::size_t, uint64_t> measure_cache_capacity_and_associativity(
    uint64_t max_memory,
    uint64_t max_associativity
) {
    std::vector<uint64_t> prev_jumps;

    for (std::size_t set_size = 16; set_size * max_associativity <= max_memory; set_size *= 2) {
        std::vector<uint64_t> commit_jumps;

        while (true) {
            std::vector<uint64_t> jumps;
            uint64_t prev_time = 0;

            log_file << "Set size: " << set_size << std::endl;

            for (uint64_t associativity = 1; associativity <= max_associativity; ++associativity) {
                const uint64_t current_time = measure_pattern(set_size, associativity);

                const bool jump = current_time > prev_time + REPEATS;

                log_file << "Associativity:\t" << associativity
                         << "\tTime:\t" << current_time
                         << "\tJump:\t" << jump
                         << std::endl;

                if (jump) {
                    jumps.push_back(associativity);
                }

                prev_time = current_time;
            }

            if (jumps == commit_jumps) {
                break;
            }

            commit_jumps = std::move(jumps);
        }

        if (commit_jumps == prev_jumps && commit_jumps.size() > 1) {
            return { set_size / 2, commit_jumps[1] - 1 };
        }

        prev_jumps = std::move(commit_jumps);
    }

    throw std::logic_error { "cannot determine cache set stride and associativity" };
}

int main() {
    heat_pattern = new(std::align_val_t(PAGE_SIZE)) void *[HEAT_PATTERN_SIZE];
    generate_pattern(heat_pattern, HEAT_PATTERN_SIZE);

    log_file << std::boolalpha;

    auto [cache_set_size, jump] = measure_cache_capacity_and_associativity(32 * 1024 * 1024, 16);

    std::cout << "Cache set size: " << cache_set_size << std::endl
              << "Associativity: " << jump << "-way" << std::endl
              << "Cache size: " << cache_set_size * jump << std::endl;
}
