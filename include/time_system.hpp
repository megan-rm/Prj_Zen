#pragma once
#include "utils.hpp"

#include <chrono>
#include <ctime>

enum class Day_Phase { MORNING, NOON, EVENING, NIGHT };
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
		static const int days_in_months[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
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
		day_pct = current_time.hour * 60 + current_time.second;
		year_pct = ((current_time.month-1) * 30 + current_time.day) / 365; // is it elegant? no. but for now
		
	}
	//to be honest this is mostly for the sake of knowing where to draw the entities
	Zen::Vector2D get_sun_pos(SDL_Rect& camera) {
		update_time();
		int sun_height = Zen::TERRAIN_HEIGHT * 0.8;
		sun_position.x = day_pct * camera.w;
		sun_position.y = camera.h - sinf(day_pct * M_PI) * (camera.h * 0.8);

		return sun_position;
	}

	Zen::Vector2D get_moon_pos(SDL_Rect& camera) {
		update_time();

		return moon_position;
	}

	Moon_Phase get_moon_phase(SDL_Rect& camera) {
		update_time();

		return moon_phase;
	}

private:
	Zen::Vector2D sun_position;
	Zen::Vector2D moon_position;

	Time current_time;
	Day_Phase day_phase;
	Moon_Phase moon_phase;
	float day_pct;
	float year_pct;
	const static int seconds_in_day = 60 * 60 * 24;
	const static int days_in_year = 365; // more often than not. we're not going to recalculate this over and over.
	
};