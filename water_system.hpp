#pragma once
#include "tile.hpp"

#include <iostream>
#include <random>
#include <vector>

class Water_System {
public:
	Water_System() = default;
	Water_System(std::vector<std::vector<Tile>>& world_reference, int mod)
		: world_reference(world_reference), update_mod(mod), rand((std::random_device())()) {
		update_count = 0;
		water_total = 0;
		water_update_total = 0;
		water_line = 0;
	}
	~Water_System() = default;
	void update_saturation(float delta);
	Uint64 place_water(float relative_pct);
private:
	std::vector<std::vector<Tile>>& world_reference;
	int update_count;
	int update_mod;
	void calculate_flow(Tile& self, Tile& tile, float delta);
	
	std::mt19937 rand;

	Uint64 water_total;
	Uint64 water_update_total; // we'll calculate this every full update, and if it differs than water_total, we've made a oopsie
	Uint8 water_line;

};