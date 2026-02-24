#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>

static constexpr const char* kSockPath = "/tmp/temperature_service.sock";

// Make fd non-blocking (for accept loop)
static bool set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0);
}

static bool write_all(int fd, const std::string& s) {
    const char* p = s.data();
    size_t left = s.size();
    while (left > 0) {
        ssize_t n = ::write(fd, p, left);
        if (n > 0) {
            p += n;
            left -= static_cast<size_t>(n);
        } else if (n == -1 && (errno == EINTR)) {
            continue;
        } else if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // Best effort for demo: treat as failure if socket buffer full.
            return false;
        } else {
            return false;
        }
    }
    return true;
}

// Read a single line ending in '\n' (blocking)
static bool read_line(int fd, std::string& out) {
    out.clear();
    char c;
    while (true) {
        ssize_t n = ::read(fd, &c, 1);
        if (n == 1) {
            out.push_back(c);
            if (c == '\n') return true;
        } else if (n == 0) {
            return false; // peer closed
        } else if (errno == EINTR) {
            continue;
        } else {
            return false;
        }
    }
}

int main() {
    // Remove old socket file if exists
    ::unlink(kSockPath);

    int listen_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        std::cerr << "socket() failed: " << std::strerror(errno) << "\n";
        return 1;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", kSockPath);

    if (::bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "bind() failed: " << std::strerror(errno) << "\n";
        ::close(listen_fd);
        return 1;
    }

    if (::listen(listen_fd, 16) < 0) {
        std::cerr << "listen() failed: " << std::strerror(errno) << "\n";
        ::close(listen_fd);
        return 1;
    }

    if (!set_nonblocking(listen_fd)) {
        std::cerr << "set_nonblocking() failed\n";
        ::close(listen_fd);
        return 1;
    }

    std::cout << "TemperatureService provider started.\n";
    std::cout << "Socket: " << kSockPath << "\n";
    std::cout << "Protocol:\n";
    std::cout << "  Client -> Server: GET\\n\n";
    std::cout << "  Server -> Client: OK <temp>\\n\n";
    std::cout << "  Server -> Client: EVENT <temp>\\n (periodic)\n\n";

    std::vector<int> clients;

    // Fake temperature generator
    std::default_random_engine eng{std::random_device{}()};
    std::uniform_real_distribution<float> dist(20.0f, 35.0f);
    float current_temp = 25.0f;

    auto next_event = std::chrono::steady_clock::now();

    while (true) {
        // 1) Accept new clients (non-blocking)
        while (true) {
            int cfd = ::accept(listen_fd, nullptr, nullptr);
            if (cfd >= 0) {
                std::cout << "Client connected (fd=" << cfd << ")\n";
                clients.push_back(cfd);
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) break; // no more
                if (errno == EINTR) continue;
                std::cerr << "accept() failed: " << std::strerror(errno) << "\n";
                break;
            }
        }

        // 2) Handle client requests (simple: poll each client with MSG_PEEK-like approach)
        // For demo simplicity, we do a non-blocking read check using select()
        fd_set rfds;
        FD_ZERO(&rfds);
        int maxfd = -1;
        for (int fd : clients) {
            FD_SET(fd, &rfds);
            maxfd = std::max(maxfd, fd);
        }

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000; // 100ms

        int ready = 0;
        if (maxfd >= 0) {
            ready = ::select(maxfd + 1, &rfds, nullptr, nullptr, &tv);
            if (ready < 0 && errno != EINTR) {
                std::cerr << "select() failed: " << std::strerror(errno) << "\n";
            }
        } else {
            // No clients: sleep a bit to avoid busy loop
            ::usleep(100 * 1000);
        }

        if (ready > 0) {
            // iterate clients and read one line if available
            std::vector<int> to_remove;
            for (int fd : clients) {
                if (!FD_ISSET(fd, &rfds)) continue;

                std::string line;
                if (!read_line(fd, line)) {
                    std::cout << "Client disconnected (fd=" << fd << ")\n";
                    ::close(fd);
                    to_remove.push_back(fd);
                    continue;
                }

                if (line == "GET\n") {
                    std::string resp = "OK " + std::to_string(current_temp) + "\n";
                    if (!write_all(fd, resp)) {
                        std::cout << "Write failed, dropping client (fd=" << fd << ")\n";
                        ::close(fd);
                        to_remove.push_back(fd);
                    }
                } else {
                    // unknown command
                    std::string resp = "ERR unknown_command\n";
                    (void)write_all(fd, resp);
                }
            }

            if (!to_remove.empty()) {
                clients.erase(std::remove_if(clients.begin(), clients.end(),
                                             [&](int fd){ return std::find(to_remove.begin(), to_remove.end(), fd) != to_remove.end(); }),
                              clients.end());
            }
        }

        // 3) Periodic event push (every 2 seconds)
        auto now = std::chrono::steady_clock::now();
        if (now >= next_event) {
            current_temp = dist(eng);
            std::string evt = "EVENT " + std::to_string(current_temp) + "\n";

            std::vector<int> to_remove;
            for (int fd : clients) {
                if (!write_all(fd, evt)) {
                    std::cout << "Event write failed, dropping client (fd=" << fd << ")\n";
                    ::close(fd);
                    to_remove.push_back(fd);
                }
            }
            if (!to_remove.empty()) {
                clients.erase(std::remove_if(clients.begin(), clients.end(),
                                             [&](int fd){ return std::find(to_remove.begin(), to_remove.end(), fd) != to_remove.end(); }),
                              clients.end());
            }

            std::cout << "Broadcast: " << evt; // already contains '\n'
            next_event = now + std::chrono::seconds(2);
        }
    }

    // never reached in demo
    for (int fd : clients) ::close(fd);
    ::close(listen_fd);
    ::unlink(kSockPath);
    return 0;
}