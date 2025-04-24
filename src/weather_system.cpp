#include "weather_system.hpp"
#include "utils.hpp"
/****************************************************************
*
* unsure if i should lerp a day / night temperature relative to
* tile's height in the world. shouldn't stone heavy tiles 'hold on'
* to temperature more? can we track this?
*
****************************************************************/
void Weather_System::update_temperatures(int tile_x, int tile_y) {
	Tile& self = world_reference.at(tile_x).at(tile_y);
	Tile* down = nullptr;
	Tile* up = nullptr;
	Tile* right = nullptr;
	Tile* left = nullptr;
	int valid_neighbors = 0;
	int temp_sum = 0;
	if (tile_y < Zen::TERRAIN_HEIGHT){
		temp_sum += world_reference.at(tile_x).at(tile_y + 1).temperature;
		valid_neighbors++;
	}

	if (tile_y > 0){
		temp_sum += world_reference.at(tile_x).at(tile_y - 1).temperature;
		valid_neighbors++;
	}
	
	if (tile_x < Zen::TERRAIN_WIDTH){
		temp_sum += world_reference.at(tile_x + 1).at(tile_y).temperature;
		valid_neighbors++;
	}

	if (tile_x > 0){
		temp_sum += world_reference.at(tile_x - 1).at(tile_y).temperature;
		valid_neighbors++;
	}
	self.temperature = (temp_sum / valid_neighbors) + (tile_y / world_top);
}