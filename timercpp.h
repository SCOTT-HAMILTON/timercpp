#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <utility>

// Thanks to https://cppatomic.blogspot.com/2018/05/modern-effective-c-make-stdthread.html
class ThreadContainer
{
    std::thread t;
public:
    ThreadContainer(std::thread&& otherTh) : t(std::move(otherTh)) {}
	template<typename _Callable, typename... _Args>
	ThreadContainer(_Callable&& __f, _Args&&... __args) :
		t(std::forward<_Callable>(__f), std::forward<_Args>(__args)...)
	{}
    ~ThreadContainer() {
        if (t.joinable()) {
			try {
				t.join();
			} catch(std::system_error &e) {
				std::cerr << "[fatal] failed to join thread: " << e.what() << ".\n";
			}
        }
    }
    // to support moving
    ThreadContainer(ThreadContainer&&) = default;
    ThreadContainer& operator=(ThreadContainer&&) = default;
	bool joinable() const noexcept { return t.joinable(); }
	void join() { t.join(); }
};

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
	std::unique_ptr<ThreadContainer> thread;
	bool sleep_for_ms(int ms);
};

// true on success, false when interrupted
bool Timer::sleep_for_ms(int ms) {
	if (ms > 1'000) {
		auto start = std::chrono::steady_clock::now();
		while(active.load()) {
			std::this_thread::sleep_for(
					std::chrono::milliseconds(500));
			auto current = std::chrono::steady_clock::now();
			auto durationMs =
				std::chrono::duration_cast<std::chrono::milliseconds>
					(current - start).count();
			auto diff = ms-durationMs;
			if (diff <= 0) {
				return true;
			} else if (diff <= 1'000){
				std::this_thread::sleep_for(
						std::chrono::milliseconds(diff));
			}
		}
		if (!active.load()) {
			return false;
		}
	} else {
		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
	}
	return true;
}

void Timer::setTimeout(std::function<void()> function, int delay) {
	stop();
    active.store(true);
    thread = std::make_unique<ThreadContainer>([=]() {
        if(!active.load()) {
			return;
		}
        if (!sleep_for_ms(delay)) {
			return;
		}
        if(!active.load()) {
			return;
		}
        function();
    });
}

void Timer::setInterval(std::function<void()> function, int interval) {
	stop();
    active.store(true);
	thread = std::make_unique<ThreadContainer>([=]() {
        while(active.load()) {
			if (!sleep_for_ms(interval)) return;
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
