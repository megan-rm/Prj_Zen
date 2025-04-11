#pragma once
#include "tile.hpp"

#include <vector>

class Water_System {
public:
	Water_System() = default;
	Water_System(std::vector<std::vector<Tile>>& world_reference, int mod)
		: world_reference(world_reference), update_count(0), update_mod(mod) {
		
	}
	~Water_System() = default;
	void update_saturation();
private:
	std::vector<std::vector<Tile>>& world_reference;
	int update_count;
	int update_mod;
	void check_neighbor(Tile& self, Tile& tile);
};