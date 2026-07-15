#pragma once
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#include "tile.hpp"
#include "time_system.hpp"
#include "utils.hpp"

class Weather_System {
public:
	Weather_System(std::vector<std::vector<Tile>>& world, Time_System& ts) : world_reference(world), time_system(ts), rand((std::random_device())()) {
		update_forecasts();
		find_surface_tiles();
	};
	~Weather_System() = default;
	void update_temperatures(float delta);
	void sun_temperature_update();
	Uint64 water_check(); // total humidity + saturation in the world, for budget debugging
private:
	std::vector<std::vector<Tile>>& world_reference;

	struct Surface_Tile {
		int x;
		int y;
	};

	float get_day_temperature();
	void evaporations(float delta);
	void melt_snow(float delta);
	void humidity_handling(int x, int y, float delta);
	void humidity_share(Tile& self, Tile& neighbor, float weight);
	void update_forecasts(); // called in constructor, handles week.forecast and month.forecast
	Time_System& time_system;
	std::mt19937 rand;
	//Monthly_Forecast monthly_forecast;
	std::vector<Surface_Tile> surface_tiles;
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
