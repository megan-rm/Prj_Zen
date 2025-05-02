#pragma once
#include <fstream>
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
};