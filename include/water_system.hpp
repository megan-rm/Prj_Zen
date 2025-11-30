#pragma once
#include "tile.hpp"

#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
class Water_System {
public:
	Water_System() = default;
	Water_System(std::vector<std::vector<Tile>>& world_reference, int mod)
		: world_reference(world_reference), update_mod(mod), rand((std::random_device())()) {
		update_count = 0;
		water_total = 0;
	}
	~Water_System() = default;
	void update_saturation(float delta);
	Uint64 place_water(float relative_pct);
private:
	int update_count;
	int update_mod;
	std::vector<std::vector<Tile>>& world_reference;

	void calculate_flow(Tile& self, Tile& tile, float delta, bool downward);
	void capillary_action(Tile& self, Tile& below, float delta);
	std::mt19937 rand;

	Uint64 water_total;

};