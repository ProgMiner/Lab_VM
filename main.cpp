#include <optional>
#include <iostream>
#include <sstream>
#include <cstdint>
#include <csignal>
#include <csetjmp>


static sigjmp_buf safe_read_uint8_env;

static void safe_read_uint8_handler(int) {
    siglongjmp(safe_read_uint8_env, 1);
}

std::optional<uint8_t> safe_read_uint8(const uint8_t * p) {
    // FIXME: add sigprocmask + pthread_sigmask to ensure thread-safety

    struct sigaction old_sa;

    struct sigaction sa {};
    sa.sa_handler = safe_read_uint8_handler;
    if (sigaction(SIGSEGV, &sa, &old_sa)) {
        throw std::system_error { errno, std::system_category(), "unable to setup sigaction" };
    }

    std::optional<uint8_t> result;
    if (!sigsetjmp(safe_read_uint8_env, SIGSEGV)) {
        result = *p;
    } else {
        // ATTENTION: it seems like compiler optimizes construction of std::optional so we
        //            actually dereferencing p AFTER non-empty std::optional was constructed
        result = std::nullopt;
    }

    if (sigaction(SIGSEGV, &old_sa, nullptr)) {
        throw std::system_error { errno, std::system_category(), "unable to restore sigaction" };
    }

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
    check(reinterpret_cast<const uint8_t *>(x));
    check(reinterpret_cast<const uint8_t *>(argv));
    check(reinterpret_cast<const uint8_t *>(x));
    check(reinterpret_cast<const uint8_t *>(x));
    check(reinterpret_cast<const uint8_t *>(argv));
    check(reinterpret_cast<const uint8_t *>(argv));
    check(reinterpret_cast<const uint8_t *>(x));
    check(reinterpret_cast<const uint8_t *>(argv));
}
