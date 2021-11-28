#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <functional>
#include <memory>
#include <thread>

class Timer {
public:
	Timer() {}
	virtual ~Timer() {
		stop();
	}
	void setTimeout(std::function<void()> function, int delay);
	void setInterval(std::function<void()> function, int interval);
	bool isActive();
	void stop();
private:
	std::atomic_bool active{true};
	std::unique_ptr<std::thread> thread;

};

void Timer::setTimeout(std::function<void()> function, int delay) {
	if (active.load()) {
		stop();
	}
    active.store(true);
    thread = std::make_unique<std::thread>([=]() {
        if(!active.load()) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        if(!active.load()) return;
        function();
    });
}

void Timer::setInterval(std::function<void()> function, int interval) {
	if (active.load()) {
		stop();
	}
    active.store(true);
	thread = std::make_unique<std::thread>([=]() {
        while(active.load()) {
			if (interval > 1'000) {
				auto start = std::chrono::steady_clock::now();
				while(active.load()) {
					std::this_thread::sleep_for(
							std::chrono::milliseconds(1'000));
					auto current = std::chrono::steady_clock::now();
					auto durationMs =
						std::chrono::duration_cast<std::chrono::milliseconds>
							(current - start).count();
					auto diff = interval-durationMs;
					if (diff <= 0) {
						break;
					} else if (diff <= 1'000){
						std::this_thread::sleep_for(
								std::chrono::milliseconds(diff));
					}
				}
			} else {
				std::this_thread::sleep_for(std::chrono::milliseconds(interval));
			}
            if(!active.load()) return;
            function();
        }
    });
}

bool Timer::isActive() {
	return active.load();
}

void Timer::stop() {
    active.store(false);
	if (thread) {
		thread->join();
		thread.reset();
	}
}
