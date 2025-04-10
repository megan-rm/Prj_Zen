#pragma once
#include "tile.hpp"

#include <vector>

class Water_System {
public:
	Water_System() = default;
	Water_System(std::vector<std::vector<Tile>>& world_reference)
		: world_reference(world_reference) {

	}
	~Water_System();
	void update_saturation();
	void update_saturation(int cols);
	void update_saturation(int cols, int rows); // we use modulation to determine which cols & rows, not specific ones.
private:
	std::vector<std::vector<Tile>>& world_reference;
	int update_count;
};