// signal.cpp
#include <csignal>   // std::signal, std::raise, SIGINT, ...
#include <iostream>  // std::cout
#include <thread>    // std::this_thread
#include <chrono>    // std::chrono::milliseconds

// ---------------------------------------------------------------
// 1. Global flag that can be changed safely from a signal handler
// ---------------------------------------------------------------
volatile std::sig_atomic_t keep_running = 1;

// ---------------------------------------------------------------
// 2. Signal handler – only async-signal-safe operations!
// ---------------------------------------------------------------
void handler(int sig)
{
    // Printf is async-signal-safe; std::cout is *not* guaranteed.
    // For a demo we keep it simple with std::cout (works on most systems).
    std::cout << "\n[Signal " << sig << " caught – exiting...]\n";
    keep_running = 0;               // tell the main loop to stop
}

// ---------------------------------------------------------------
// 3. Main program
// ---------------------------------------------------------------
int main()
{
    // Register the same handler for Ctrl-C and for a polite kill
    std::signal(SIGINT,  handler);
    std::signal(SIGTERM, handler);

    std::cout << "Running – press Ctrl-C to quit gracefully.\n";

    while (keep_running) {
        // 100 ms sleep → ~0 % CPU while idle
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Clean shutdown.\n";
    return 0;
}
