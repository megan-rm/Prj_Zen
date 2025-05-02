#include "weather_system.hpp"
#include "utils.hpp"
/****************************************************************
*
* unsure if i should lerp a day / night temperature relative to
* tile's height in the world. shouldn't stone heavy tiles 'hold on'
* to temperature more? can we track this?
*
****************************************************************/
void Weather_System::update_temperatures(float delta) {
	for (int x = update_count; x < world_reference.size(); x += update_mod) {
		for (int y = world_reference.at(x).size() - 1; y > 0; y--) {
			Tile& self = world_reference.at(x).at(y);
			if (self.saturation == 0) {
				continue;
			}
			Tile* left, *right, *down, *up;
			left = nullptr;
			right = nullptr;
			down = nullptr;
			up = nullptr;
			if (x > 0) left = &world_reference.at(x - 1).at(y);
			if (x < world_reference.size() - 1) right = &world_reference.at(x + 1).at(y);
			if (y < world_reference.at(x).size() - 1) down = &world_reference.at(x).at(y + 1);
			if (y > 0) up = &world_reference.at(x).at(y - 1);
			//share down first, if we can
			if (down != nullptr) {
			}
			//share right
			if (right != nullptr) {
				if (self.saturation > right->saturation && right->saturation < right->max_saturation) {
				}
			}
			//share left
			if (left != nullptr) {
				if (self.saturation > left->saturation && left->saturation < left->max_saturation) {
				}
			}
		}
	}
	update_count += 1;
	update_count = update_count % update_mod;
}

void Weather_System::sun_temperature_update() {
	float day_temp = get_day_temperature();
	for (int x = 0; x < world_reference.size(); x++) {
		//find the first non-air tile
		for (int y = 0; y < world_reference.front().size(); y++) {
			if (world_reference.at(x).at(y).permeability == 10000 && world_reference.at(x).at(y).saturation > 0) {
				break; // skip this column, we found a body of water (like our lake) and dont need to update the surface under the water
			}
			if (world_reference.at(x).at(y).permeability == 10000) {
				break;
			}

		}
	}
}

float Weather_System::get_day_temperature() {
	int day_of_year = time_system.get_time().month * 30 + time_system.get_time().day;

	float min_temp = -40.0f;
	float max_temp = 40.0f;

	float seasonal_variation = (std::sin(2 * M_PI * (day_of_year - 81) / 365) + 1.0) / 2.0;
	float day_temp = min_temp + (max_temp - min_temp) * seasonal_variation;
	day_temp = day_temp * (9 / 5) + 32;
	return day_temp;
}

void Weather_System::update_forecasts() {
	std::ifstream file("world_info/weekly.zen");
	if (!file.is_open()) {

	}

	file.open("world_info/monthly.zen");
}
