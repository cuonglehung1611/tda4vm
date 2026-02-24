#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

static constexpr const char* kSockPath = "/tmp/temperature_service.sock";

static bool write_all(int fd, const std::string& s) {
    const char* p = s.data();
    size_t left = s.size();
    while (left > 0) {
        ssize_t n = ::write(fd, p, left);
        if (n > 0) {
            p += n;
            left -= static_cast<size_t>(n);
        } else if (n == -1 && errno == EINTR) {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

static bool read_line(int fd, std::string& out) {
    out.clear();
    char c;
    while (true) {
        ssize_t n = ::read(fd, &c, 1);
        if (n == 1) {
            out.push_back(c);
            if (c == '\n') return true;
        } else if (n == 0) {
            return false; // closed
        } else if (errno == EINTR) {
            continue;
        } else {
            return false;
        }
    }
}

int main() {
    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "socket() failed: " << std::strerror(errno) << "\n";
        return 1;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", kSockPath);

    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "connect() failed: " << std::strerror(errno) << "\n";
        ::close(fd);
        return 1;
    }

    std::cout << "Connected to TemperatureService.\n";
    std::cout << "Listening for EVENT messages and calling GET every 3 seconds.\n";

    auto next_get = std::chrono::steady_clock::now();

    while (true) {
        // 1) Non-blocking-ish loop using select with timeout to interleave GET and reading
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 200 * 1000; // 200ms

        int r = ::select(fd + 1, &rfds, nullptr, nullptr, &tv);
        if (r < 0 && errno != EINTR) {
            std::cerr << "select() failed: " << std::strerror(errno) << "\n";
            break;
        }

        if (r > 0 && FD_ISSET(fd, &rfds)) {
            std::string line;
            if (!read_line(fd, line)) {
                std::cout << "Server closed connection.\n";
                break;
            }
            // Could be EVENT or OK response
            std::cout << "RX: " << line;
        }

        // 2) Periodic method call
        auto now = std::chrono::steady_clock::now();
        if (now >= next_get) {
            if (!write_all(fd, "GET\n")) {
                std::cerr << "Failed to send GET\n";
                break;
            }
            next_get = now + std::chrono::seconds(3);
        }
    }

    ::close(fd);
    return 0;
}