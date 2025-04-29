#pragma once
#include "utils.hpp"

#include <chrono>
#include <ctime>

enum class Moon_Phase { NEW_MOON, WAXING_CRESCENT, FIRST_QUARTER, WAXING_GIBBOUS, FULL_MOON, WANING_GIBBOUS, LAST_QUARTER, WANING_CRESCENT };

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
		day_pct = ((current_time.hour * 60 * 60) + (current_time.minute * 60) + current_time.second) / static_cast<float>(seconds_in_day);
		year_pct = ((current_time.month-1) * 30 + current_time.day) / 365.0f; // is it elegant? no. but for now
		
	}
	//to be honest this is mostly for the sake of knowing where to draw the entities
	Zen::Vector2D get_sun_pos(SDL_Rect& camera) {
		update_time();
		float daylight_length = 12.0f + (4.0f * std::cosf((year_pct - 0.5f) * 2.0f * M_PI));

		float daylight_bias = 0.1f * std::cosf((year_pct - 0.25f) * 2.0f * M_PI);
		midday_pct = 0.5f - daylight_bias;

		sunrise_pct = midday_pct - (daylight_length / 48.0f); // I think I need to tweak this some. I'm kinda throwing magic numbers around with .125 and .166
		sunset_pct = midday_pct + (daylight_length / 48.0f);
		
		if (day_pct < sunrise_pct || day_pct > sunset_pct) {
			//return { 10000,10000 }; // out of sight, out of mind
		}
		float daylight_pct = (day_pct - sunrise_pct) / (sunset_pct - sunrise_pct);
		float peak_y = camera.h * 0.8;// 750.0f;

		sun_position.x = daylight_pct * (0.9 * camera.w);
		sun_position.y = peak_y - std::sinf(daylight_pct * M_PI) * peak_y;
		return sun_position;
	}

	Zen::Vector2D get_moon_pos(SDL_Rect& camera) {
		update_time();
		lunar_day = std::fmod((current_time.day + current_time.month * 30), lunar_length);
		lunar_pct = lunar_day / lunar_length;
		current_lunar_phase = (lunar_pct - 0.5f) * 2.0f; // we got two phases of moon to chew through

		float peak_y = 150.0f;
		float orbit_radius = 0.4f * camera.w;

		moon_position.x = (lunar_pct * orbit_radius) + (camera.w * 0.5f);
		moon_position.y = (0.5f * camera.h) - std::sinf(lunar_pct * M_PI) * peak_y;
		return moon_position;
	}

	float get_day_pct() {
		return day_pct;
	}

	float get_midday_pct() {
		return midday_pct;
	}

	float get_sunrise_pct() {
		return sunrise_pct;
	}

	float get_sunset_pct() {
		return sunset_pct;
	}

	Moon_Phase get_moon_phase() {
		update_time();

		if (current_lunar_phase >= -1.0f && current_lunar_phase < -0.75f) {
			moon_phase = Moon_Phase::NEW_MOON;
		}
		else if (current_lunar_phase >= -0.75f && current_lunar_phase < -0.5f) {
			moon_phase = Moon_Phase::WAXING_CRESCENT;
		}
		else if (current_lunar_phase >= -0.5f && current_lunar_phase < -0.25f) {
			moon_phase = Moon_Phase::FIRST_QUARTER;
		}
		else if (current_lunar_phase >= -0.25f && current_lunar_phase < 0.0f) {
			moon_phase = Moon_Phase::WAXING_GIBBOUS;
		}
		else if (current_lunar_phase >= 0.0f && current_lunar_phase < 0.25f) {
			moon_phase = Moon_Phase::FULL_MOON;
		}
		else if (current_lunar_phase >= 0.25f && current_lunar_phase < 0.5f) {
			moon_phase = Moon_Phase::WANING_GIBBOUS;
		}
		else if (current_lunar_phase >= 0.5f && current_lunar_phase < 0.75f) {
			moon_phase = Moon_Phase::LAST_QUARTER;
		}
		else if (current_lunar_phase >= 0.75f && current_lunar_phase < 0.95f) {
			moon_phase = Moon_Phase::WANING_CRESCENT;
		}
		else if (current_lunar_phase >= 0.95f) {
			moon_phase = Moon_Phase::NEW_MOON;
		}
		return moon_phase;
	}

private:
	Zen::Vector2D sun_position;
	Zen::Vector2D moon_position;
	const int reference_new_moon_year = 2025;
	const int reference_new_moon_month = 4;
	const int reference_new_moon_day = 27;
	const float lunar_length = 29.530575f; // idk i just googled lunar length and this is what came up, don't hurt me
	Time current_time;
	Moon_Phase moon_phase;
	float day_pct;
	float year_pct;
	float sunrise_pct;
	float midday_pct;
	float sunset_pct;

	float lunar_day;
	float lunar_pct;
	float current_lunar_phase;
	const static int seconds_in_day = 60 * 60 * 24;
	const static int days_in_year = 365; // more often than not. we're not going to recalculate this over and over.
	
};