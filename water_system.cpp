#include "water_system.hpp"

void Water_System::update_saturation() {
	for (int i = update_count; i < world_reference.size(); i += update_mod) {
		for (int tile = 0; i < world_reference.at(i).size(); tile++) {
			world_reference.at(i).at(tile).saturation;
		}
	}
	update_count = (update_count + 1) % update_mod;
}

void Water_System::check_neighbor(Tile& self, Tile& tile) {
	int saturation_difference = std::abs(self.saturation - tile.saturation);
	int tiles_ratio = tile.saturation / tile.max_saturation;
	
}