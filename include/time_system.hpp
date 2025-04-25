#pragma once
#include <chrono>
#include <ctime>

enum class Phase { MORNING, NOON, EVENING, NIGHT };
enum class Moon_Phase { NEW_MOON, WAXING_CRESCENT, FIRST_QUARTER, WAXING_GIBBOUS, FULL_MOON, WANING_GIBBOUS, THIRD_QUARTER, WANING_CRESCENT };

struct Time {
	int year, month, day, hour, minute, second;
};
class Time_System {
public:
	Time_System() = default;
	~Time_System() = default;

	Time get_time() {
		update_time();
		return current_time;
	}

	void update_time() {
		auto now = std::chrono::system_clock::now();
		std::time_t time_now = std::chrono::system_clock::to_time_t(now);
		std::tm local_tm;
#ifdef _WIN32
		localtime_s(&local_tm, &time_now); // damn thread safe bullspit
#else
		local_time_r(&time_now, &local_tm);// but i get it, though.
#endif
		current_time.year = local_tm.tm_year + 1900;
		current_time.month = local_tm.tm_mon + 1;
		current_time.day = local_tm.tm_mday;
		current_time.hour = local_tm.tm_hour;
		current_time.minute = local_tm.tm_min;
		current_time.second = local_tm.tm_sec;
	}

private:
	Time current_time;
	Phase phase;
	Moon_Phase moon_phase;
};