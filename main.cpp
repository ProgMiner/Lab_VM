#include <sys/wait.h>
#include <unistd.h>
#include <optional>
#include <iostream>
#include <sstream>
#include <cstdint>


std::optional<uint8_t> safe_read_uint8(const uint8_t * p) {
    int io[2];

    if (pipe(io)) {
        throw std::system_error { errno, std::system_category(), "unable to create pipe" };
    }

    std::optional<uint8_t> result;
    if (write(io[1], p, 1) > 0) {
        result = *p;
    }

    close(io[0]);
    close(io[1]);

    return result;
}

static void check(const uint8_t * p) {
    std::cout << "Address: " << static_cast<const void *>(p) << std::endl;

    std::optional<uint8_t> x = safe_read_uint8(p);

    if (x) {
        std::cout << "Result: " << static_cast<uint32_t>(*x) << std::endl;
    } else {
        std::cout << "Result: std::nullopt" << std::endl;
    }
}

int main(int argc, char ** argv) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <unsigned integer>" << std::endl;
        return 0;
    }

    uint64_t x;
    if (!(std::istringstream(argv[1]) >> x)) {
        std::cerr << "Not a number passed" << std::endl;
        return 1;
    }

    check(reinterpret_cast<const uint8_t *>(x));
    check(reinterpret_cast<const uint8_t *>(argv));
}
