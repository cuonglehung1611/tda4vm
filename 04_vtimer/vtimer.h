#ifndef VTIMER_H
#define VTIMER_H

#include <chrono>
#include <vector>
#include <string>
#include <mutex>

class VTimer
{
public:
	struct Record {
		std::string name;
		double duration_ms;
	};

	VTimer(const std::string& name) : name_(name), start_(std::chrono::high_resolution_clock::now()){};
	~VTimer() = default;

	Record stop() {
		auto end = std::chrono::high_resolution_clock::now();
		double duration_ms = std::chrono::duration<double,std::milli>(end - start_).count();
		return {name_, duration_ms};
	}

	// Singleton access to timings
	static std::vector<Record>& getTimings() {
		static std::vector<Record> timings; // Static ensures one instance
		return timings;
	}

	// Thread-safe method to get a copy of timings
	static std::vector<Record> getTimingsCopy() {
		std::lock_guard<std::mutex> lock(getMutex());
		return getTimings();
	}

	// Thread-safe method to add a record
	static void addTiming(const Record& record) {
		std::lock_guard<std::mutex> lock(getMutex());
		getTimings().push_back(record);
	}

	// Thread-safe method to clear timings
	static void clearTimings() {
		std::lock_guard<std::mutex> lock(getMutex());
		getTimings().clear();
	}
	
private:
	std::string name_;
	std::chrono::high_resolution_clock::time_point start_;

	// Singleton access to mutex
	static std::mutex& getMutex() {
		static std::mutex mutex; // Single instance
		return mutex;
	}
};

#endif // VTIMER_H