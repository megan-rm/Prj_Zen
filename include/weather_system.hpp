#pragma once
#include <fstream>
#include <functional>
#include <random>
#include <vector>

#include "tile.hpp"
#include "time_system.hpp"

class Weather_System {
public:
	Weather_System(std::vector<std::vector<Tile>>& world, Time_System& ts, int mod) : world_reference(world), time_system(ts), update_mod(mod), rand((std::random_device())()) {
		world_top = 0;
		world_bottom = world_bottom = world_reference.front().size();
		update_count = 0;
		update_forecasts();
		find_surface_tiles();
	};
	~Weather_System() = default;
	void update_temperatures(float delta);
	void sun_temperature_update();
private:
	std::vector<std::vector<Tile>>& world_reference;
	int update_count;
	int update_mod;
	int world_top;
	int world_bottom;

	float get_day_temperature();
	void update_forecasts(); // called in constructor, handles week.forecast and month.forecast
	Time_System& time_system;
	std::mt19937 rand;
	//Monthly_Forecast monthly_forecast;
	std::vector<std::reference_wrapper<Tile>> surface_tiles;
	void find_surface_tiles();
};

struct Hourly_Forecast {
	Uint8 start_temp;
	Uint8 end_temp;
};

struct Daily_Forecast {
	Hourly_Forecast hour_forecast[24];
};

struct Monthly_Forecast {
	Daily_Forecast day_forecast[31];
};