#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

std::mutex mtx;
std::condition_variable cv;
bool ready = false;

void consumer() {
	std::unique_lock<std::mutex> lock(mtx); // Must use unique_lock
	std::cout << "Consumer: Waiting for resource...\n";

	// ready is false, unlocks the mutex and puts the thread to sleep
	cv.wait(lock, [] { // Wait release mutex, then relocks when woken
		return ready; // wait until ready is true
	}); 

	if (ready) { // Just want to confirm the ready is always true
		std::cout << "Consumer: Resource is ready!" << std::endl;
	} else {
		std::cout << "Consumer: Resource is not ready!" << std::endl;
	}
	// Mutex is automatically unlocked when lock goes out of scope
}

void producer() {
	std::this_thread::sleep_for(std::chrono::seconds(1));
	{
		std::unique_lock<std::mutex> lock(mtx); // Lock acquired
		// std::lock_guard<std::mutex> lock(mtx);
		std::cout << "Producer: Acquired mutex, setting ready to true\n";
		ready=true;
		// Mutex is automatically unlocked when lock goes out of scope
	}
	cv.notify_one(); // Notify waiting thread
	std::cout << "Producer: Notified consumer\n";
}

int main() {
	std::thread t1(consumer);
	std::thread t2(producer);
	t1.join();
	t2.join();

    return 0;
}