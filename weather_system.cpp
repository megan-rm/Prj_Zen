#include "weather_system.hpp"
#include "utils.hpp"

void Weather_System::update_temperatures(int tile_x, int tile_y) {
	Tile& self = world_reference.at(tile_x).at(tile_y);
	Tile* down = nullptr;
	Tile* up = nullptr;
	Tile* right = nullptr;
	Tile* left = nullptr;
	int valid_neighbors = 0;
	int temp_sum;
	if (tile_y < Zen::TERRAIN_HEIGHT){
		temp_sum += world_reference.at(tile_x).at(tile_y + 1).temperature;
		valid_neighbors++;
	}

	if (tile_y > 0){
		temp_sum += world_reference.at(tile_x).at(tile_y - 1).temperature;
		valid_neighbors++;
	}
	
	if (tile_x < Zen::TERRAIN_WIDTH){
		valid_neighbors++;
	}

	if (tile_x > 0) left = &world_reference.at(tile_x + 1).at(tile_y) {
		valid_neighbors++;
	}

}