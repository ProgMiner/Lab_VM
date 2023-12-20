#include <algorithm>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <random>


static constexpr const uint64_t PAGE_SIZE = 4096; // bytes
static constexpr const uint64_t HEAT_PATTERN_SIZE = 512 * 1024 / sizeof(void *);
static constexpr const uint64_t REPEATS = 10000000;

static void ** heat_pattern = nullptr;
uint64_t counter = 0;


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

template<std::size_t SIZE>
static uint64_t measure_cache_size() {
    const std::size_t size = SIZE / sizeof(void *);

    void ** const pattern = new(std::align_val_t(PAGE_SIZE)) void *[size];
    generate_pattern(pattern, size);

    heat();

    void * ptr = pattern;
    const uint64_t result = measure_work([&ptr] {
        ptr = *reinterpret_cast<void **>(ptr);
    });

    counter += *reinterpret_cast<uint64_t *>(ptr);

    return result;
}

static uint64_t measure_cache_size() {
#define CHECK(__x) do { \
    const uint64_t _m = measure_cache_size<(__x)>(); \
    if (_m > prev_measure + REPEATS) return __x; \
    prev_measure = _m; \
} while (0)
    uint64_t prev_measure = measure_cache_size<8>();
    CHECK(16);
    CHECK(16);
    CHECK(32);
    CHECK(64);
    CHECK(128);
    CHECK(256);
    CHECK(512);
    CHECK(1024);
    CHECK(2 * 1024);
    CHECK(4 * 1024);
    CHECK(8 * 1024);
    CHECK(16 * 1024);
    CHECK(32 * 1024);
    CHECK(64 * 1024);
    CHECK(128 * 1024);
    CHECK(256 * 1024);
    CHECK(512 * 1024);
    CHECK(1024 * 1024);
    CHECK(2 * 1024 * 1024);
    CHECK(4 * 1024 * 1024);
    CHECK(8 * 1024 * 1024);
    CHECK(16 * 1024 * 1024);
    CHECK(32 * 1024 * 1024);
    CHECK(128 * 1024 * 1024);
    return -1;
#undef CHECK
}

int main() {
    heat_pattern = new(std::align_val_t(PAGE_SIZE)) void *[HEAT_PATTERN_SIZE];
    generate_pattern(heat_pattern, HEAT_PATTERN_SIZE);

    std::cout << "Cache size: " << measure_cache_size() << " bytes" << std::endl;
}
