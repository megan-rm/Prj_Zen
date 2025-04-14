#pragma once
#include "tile.hpp"

#include <iostream>
#include <random>
#include <vector>

class Water_System {
public:
	Water_System() = default;
	Water_System(std::vector<std::vector<Tile>>& world_reference, int mod)
		: world_reference(world_reference), update_count(0), update_mod(mod) {
		
	}
	~Water_System() = default;
	void update_saturation(int delta);
	Uint64 place_water(float relative_pct);
private:
	std::vector<std::vector<Tile>>& world_reference;
	int update_count;
	int update_mod;
	void calculate_flow(Tile& self, Tile& tile, int delta);
	
};