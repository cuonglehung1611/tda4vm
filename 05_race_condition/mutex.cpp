#include <iostream>
#include <thread>
#define USE_THREAD_SYNC
#ifdef USE_THREAD_SYNC
#include <mutex>
std::mutex mtx;
#endif

int counter = 0;

void increment() {
    for (int i = 0; i < 100000; ++i) {
#ifdef USE_THREAD_SYNC
		std::lock_guard<std::mutex> lock(mtx); // Automatically unlockswhen out of scope
#endif
        counter++; // Not thread-safe
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);
    t1.join();
    t2.join();
    std::cout << "Counter: " << counter << std::endl;
    return 0;
}