#include <algorithm>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <random>


static constexpr const uint64_t PAGE_SIZE = 4096; // bytes
static constexpr const uint64_t HEAT_PATTERN_SIZE = 128 * 1024 / sizeof(void *);

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
    }

    counter += *reinterpret_cast<uint64_t *>(ptr);
}

template<typename W>
static uint64_t measure_work(W work, uint64_t repeats = 10000000) {
    const uint64_t start = rdtsc();

    for (uint64_t i = 0; i < repeats; ++i) {
        work();
    }

    const uint64_t end = rdtsc();

    return (end - start);
}

template<std::size_t SIZE>
static uint64_t do_measure_cache_line_size() {
    const std::size_t size = SIZE / sizeof(void *);

    void ** const pattern = new void *[size];
    generate_pattern(pattern, size);

    heat();

    void * ptr = pattern;
    const uint64_t result = measure_work([&ptr] {
        ptr = *reinterpret_cast<void **>(ptr);
    });

    counter += *reinterpret_cast<uint64_t *>(ptr);

    return result;
}

template<std::size_t SIZE>
static void measure_cache_line_size() {
    std::cout << "Size\t" << SIZE
              << " bytes:\t" << do_measure_cache_line_size<SIZE>()
              << " ticks" << std::endl;
}

static void measure_cache_line_size() {
    measure_cache_line_size<8>();
    measure_cache_line_size<16>();
    measure_cache_line_size<32>();
    measure_cache_line_size<64>();
    measure_cache_line_size<128>();
    measure_cache_line_size<256>();
    measure_cache_line_size<512>();
    measure_cache_line_size<1024>();
    measure_cache_line_size<2048>();
    measure_cache_line_size<4096>();
}

int main() {
    heat_pattern = new void *[HEAT_PATTERN_SIZE];
    generate_pattern(heat_pattern, HEAT_PATTERN_SIZE);

    measure_cache_line_size();
}
