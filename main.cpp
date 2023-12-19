#include <iostream>
#include <cstdint>
#include <cstring>
#include <chrono>


template<std::size_t S>
struct unit {

    unsigned char data[S];

    unit() = default;

    unit(uint64_t v) {
        *reinterpret_cast<uint64_t*>(data) = v;
    }
};


static constexpr const uint64_t PAGE_SIZE = 4096; // bytes


static __inline__ unsigned long long rdtsc(void) {
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((unsigned long long) lo) | (((unsigned long long) hi) << 32);
}


template<typename W>
static uint64_t measure_work(W work, uint64_t repeats = 1000000) {
    const uint64_t start = rdtsc();

    for (uint64_t i = 0; i < repeats; ++i) {
        work();
    }

    const uint64_t end = rdtsc();

    return (end - start) / repeats;
}

template<std::size_t SIZE, std::size_t N>
static uint64_t do_measure_cache_line_size() {
    unit<SIZE> * bufs[128];

    for (uint64_t i = 0; i < 128; ++i) {
        bufs[i] = new unit<SIZE>[PAGE_SIZE / SIZE];
    }

    uint64_t buf_i = 0;
    const uint64_t result = measure_work([bufs, &buf_i] {
        unit<SIZE> * buf = bufs[buf_i]; 

        uint64_t gen = 0;
        for (uint64_t i = 0; i < N; ++i) {
            buf[0].data[gen % SIZE] = buf_i;

            gen = gen * 73129 + 95121;
        }

        buf_i = (buf_i + 1) % 128;
    });

    for (uint64_t i = 0; i < 128; ++i) {
        delete[] bufs[i];
    }

    return result;
}

template<std::size_t SIZE, std::size_t N>
static void measure_cache_line_size() {
    std::cout << "Size " << SIZE
              << " bytes: " << do_measure_cache_line_size<SIZE, N>()
              << " ticks" << std::endl;
}

static void measure_cache_line_size() {
    measure_cache_line_size<8, 64>();
    measure_cache_line_size<16, 64>();
    measure_cache_line_size<32, 64>();
    measure_cache_line_size<64, 64>();
    measure_cache_line_size<128, 64>();
    measure_cache_line_size<256, 64>();
    measure_cache_line_size<512, 64>();
    measure_cache_line_size<1024, 64>();
    measure_cache_line_size<2048, 64>();
    measure_cache_line_size<4096, 64>();
}

int main() {
    measure_cache_line_size();
}
