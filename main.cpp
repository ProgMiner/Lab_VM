#include <sys/wait.h>
#include <unistd.h>
#include <optional>
#include <iostream>
#include <sstream>
#include <cstdint>


std::optional<uint8_t> safe_read_uint8(const uint8_t * p) {
    const int pid = fork();

    if (pid == 0) {
        const volatile uint8_t x = *p;
        (void) x;
        exit(0);
    }

    if (pid < 0) {
        throw std::system_error { errno, std::system_category(), "unable to fork" };
    }

    int wstatus;
    if (wait(&wstatus) < 0) {
        throw std::system_error { errno, std::system_category(), "unable to wait fork" };
    }

    if (!WIFEXITED(wstatus)) {
        return std::nullopt;
    }

    return *p;
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
