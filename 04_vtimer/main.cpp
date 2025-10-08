#include "vtimer.h"
#include <iostream>
#include <iomanip>
#include <thread>

void runTask(const std::string& name, int iterations) {
	VTimer timer(name);
	for (volatile int i = 0; i < iterations; ++i) {}
	VTimer::addTiming(timer.stop());
}

void printTimings() {
	for (const auto& record : VTimer::getTimingsCopy()) {
		std::cout << "Timer" << record.name << ": " << std::fixed << std::setprecision(2)
				<< record.duration_ms << " ms\n";
	}
}

int main()
{
	// Clear timings to start fresh
	VTimer::clearTimings();

	// Launch multiple threads
	std::thread t1(runTask, "Task1", 1000000);
	std::thread t2(runTask, "Task2", 2000000);
	std::thread t3(runTask, "Task3", 1500000);

	// Wait for threads to finish
	t1.join();
	t2.join();
	t3.join();

	// print all timings
	printTimings();
	return 0;
}