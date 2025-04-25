#pragma once
#include <chrono>
#include <ctime>

#include "date.h"

/******************************************************************
*
* using Howard Hinnant's date.h file:
* https://github.com/HowardHinnant/date
*
******************************************************************/
enum class Phase { MORNING, NOON, EVENING, NIGHT };
enum class Moon_Phase { NEW_MOON, WAXING_CRESCENT, FIRST_QUARTER, WAXING_GIBBOUS, FULL_MOON, WANING_GIBBOUS, THIRD_QUARTER, WANING_CRESCENT };

class Time_System {
public:
	Time_System() = default;
	~Time_System() = default;

	tm get_time() {
		auto now = std::chrono::system_clock::now();
		std::time_t t = std::chrono::system_clock::to_time_t(now);
		std::tm local = *std::localtime(&t);
		return local;
		auto now = std::chrono::system_clock::now();
		auto t_c = std::chrono::system_clock::to_time_t(now);
		
		/*auto now = std::chrono::system_clock::now();
		auto time_zone = std::chrono::current_zone();
		auto local_time = time_zone->to_local(now);
		auto time_point = std::chrono::time_point_cast<std::chrono::days>(local_time);
		auto time = std::chrono::min*/
	}

private:
};