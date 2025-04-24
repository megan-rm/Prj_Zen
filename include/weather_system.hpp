#pragma once

#include <vector>

#include "tile.hpp"
class Weather_System {
public:
	Weather_System(std::vector<std::vector<Tile>>& world) : world_reference(world) {
		world_top = 0;
		world_bottom = world_bottom = world_reference.front().size();
	};
	~Weather_System() = default;
	void update_temperatures(int tile_x, int tile_y);
private:
	std::vector<std::vector<Tile>>& world_reference;
	int world_top;
	int world_bottom;

};