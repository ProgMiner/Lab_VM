#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <random>


static constexpr const std::size_t PAGE_SIZE = 4096; // bytes
static constexpr const std::size_t HEAT_PATTERN_SIZE = 512 * 1024 / sizeof(void *);
static constexpr const uint64_t REPEATS = 10000000;

static void ** heat_pattern = nullptr;
uint64_t counter = 0;

// to disable optimizer, do not change
volatile std::size_t one = 1;

static std::ofstream log_file { "./log.txt" };


enum ord { NA = 0, LT, EQ, GT };


static std::ostream & operator<<(std::ostream & os, const ord & o) {
    switch (o) {
    case NA:
        os << "NA";
        break;

    case LT:
        os << "LT";
        break;

    case EQ:
        os << "EQ";
        break;

    case GT:
        os << "GT";
        break;
    }

    return os;
}

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

static std::vector<std::pair<std::size_t, uint64_t>> measure_cache_capacity_and_associativity(
    std::size_t max_memory,
    uint64_t max_associativity
) {
    std::unordered_map<uint64_t, std::vector<std::size_t>> assoc_to_stride;
    std::vector<uint64_t> prev_jumps;

    for (std::size_t set_size = 8; set_size * max_associativity <= max_memory; set_size *= 2) {
        std::vector<uint64_t> commit_jumps;

        while (true) {
            std::vector<uint64_t> jumps;

            uint64_t prev_time = measure_pattern(set_size, one);

            log_file << "Set size:\t" << set_size
                     << "\tInitial time:\t" << prev_time
                     << std::endl;

            for (uint64_t associativity = 2; associativity <= max_associativity; ++associativity) {
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
            log_file << "Retry" << std::endl;
        }

        for (uint64_t j : commit_jumps) {
            assoc_to_stride[j].push_back(set_size);
        }

        if (!commit_jumps.empty() && prev_jumps == commit_jumps) {
            break;
        }

        prev_jumps = std::move(commit_jumps);
    }

    log_file << "Associativity to stride table:" << std::endl;

    for (const auto & [assoc, strides] : assoc_to_stride) {
        log_file << assoc << ":\t";

        for (std::size_t stride : strides) {
            log_file << stride << ", ";
        }

        log_file << std::endl;
    }

    std::vector<std::pair<std::size_t, uint64_t>> result;

    for (uint64_t j : prev_jumps) {
        result.emplace_back(assoc_to_stride[j][0], j - 1);
    }

    return result;
}

static std::size_t find_greater_time_spots(std::size_t stride, std::size_t max_spots) {
    std::size_t l = 1, r = max_spots + 1;

    const uint64_t first_time = measure_pattern(stride, one);
    log_file << "First time: " << first_time << std::endl;

    while (l + 1 < r) {
        const std::size_t m = l + (r - l) / 2;

        const uint64_t current_time = measure_pattern(stride, m);

        log_file << "Spots:\t" << m
                 << "\tTime:\t" << current_time
                 << std::endl;

        if (current_time < first_time + REPEATS) {
            l = m;
        } else {
            r = m;
        }
    }

    return r;
}

static std::size_t measure_cache_line_size(std::size_t max_line_size, std::size_t cache_size) {
    bool eq_lt_reached = false;

    for (std::size_t high_stride = 8; high_stride <= max_line_size; high_stride *= 2) {
        log_file << "High stride: " << high_stride << std::endl;

        const std::size_t expected_spots = cache_size / high_stride;
        log_file << "Expected spots: " << expected_spots << std::endl;

        const std::size_t max_spots = expected_spots + expected_spots / 2;

        std::vector<ord> commit_ords;

        while (true) {
            std::vector<ord> ords;

            for (std::size_t low_stride = 1; low_stride < high_stride; low_stride *= 2) {
                log_file << "Low stride: " << low_stride << std::endl;

                const std::size_t spots = find_greater_time_spots(high_stride + low_stride, max_spots);
                const long double frac = static_cast<long double>(spots) / expected_spots;

                log_file << "Fraction: " << frac << std::endl;

                const ord result =
                    spots > max_spots ? NA :
                    frac < 0.9 ? LT :
                    frac > 1.1 ? GT :
                    EQ;

                if (result == NA) {
                    log_file << "Wasn't reached expected" << std::endl;
                }

                ords.push_back(result);
            }

            if (ords == commit_ords) {
                break;
            }

            commit_ords = std::move(ords);
            log_file << "Retry" << std::endl;
        }

        log_file << "Orderings: ";

        for (ord o : commit_ords) {
            log_file << o << ", ";
        }

        log_file << std::endl;

        if (eq_lt_reached) {
            bool gt = false;

            for (ord o : commit_ords) {
                if (o == NA || o == GT) {
                    gt = true;
                }
            }

            if (gt) {
                return high_stride / 2;
            }
        } else {
            for (ord o : commit_ords) {
                if (o == EQ || o == LT) {
                    eq_lt_reached = true;
                    break;
                }
            }
        }
    }

    throw std::logic_error { "cannot determine cache line size" };
}

int main() {
    heat_pattern = new(std::align_val_t(PAGE_SIZE)) void *[HEAT_PATTERN_SIZE];
    generate_pattern(heat_pattern, HEAT_PATTERN_SIZE);

    log_file << std::boolalpha;

    auto detected = measure_cache_capacity_and_associativity(32 * 1024 * 1024, 24);

    std::cout << "Detected entities:" << std::endl;

    for (const auto & [stride, assoc] : detected) {
        const std::size_t cache_size = stride * assoc;

        std::cout << " -\tStride: " << stride << std::endl
                  << "\tAssociativity: " << assoc << "-way" << std::endl
                  << "\tSize: " << stride * assoc << std::endl;

        const std::size_t cache_line_size = measure_cache_line_size(stride, cache_size);

        std::cout << "\tCache line size: " << cache_line_size << std::endl;
    }
}
