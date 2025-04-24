#pragma once

#include <vector>

#include "tile.hpp"
class Weather_System {
public:
	Weather_System(std::vector<std::vector<Tile>>& world) : world_reference(world) {
	};
	~Weather_System() = default;
	void update_temperatures(int tile_x, int tile_y);
private:
	std::vector<std::vector<Tile>>& world_reference;
};