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
			Tile *left, *right, *down, *up;
			left = nullptr;
			right = nullptr;
			down = nullptr;
			up = nullptr;
			float up_weight = 1.35;
			float down_weight = 0.85f;
			float side_weight = 1.0f;

			float base_alpha = 0.998f;
			float permeability_factor = 1 - (self.permeability / 10000.0f);
			if (permeability_factor == 0) permeability_factor = 0.15;
			float saturation_factor;
			if (self.max_saturation > 0) {
				saturation_factor = self.saturation / float(self.max_saturation);
			}
			else {
				saturation_factor = 0.25f;
			}
			float saturation_penalty = 1.0f - 0.01f * saturation_factor;
			float alpha = base_alpha * permeability_factor * saturation_penalty;//  0.5;
			alpha = std::max(0.2f, alpha);
			float sum_deltas = 0.0f;
			float total_weight = 0.0f;
			if (x > 0)  left = &world_reference.at(x - 1).at(y);
			if (x < world_reference.size() - 1) right = &world_reference.at(x + 1).at(y);
			if (y < world_reference.at(x).size() - 1) down = &world_reference.at(x).at(y + 1);
			if (y > 0) up = &world_reference.at(x).at(y - 1);
			if (self.saturation == 10000) {
				float gamma = 0.1;
				int weight = pow(gamma, sum_deltas);
			}
			// laplace sorta? i guess...
			if (up) {
				float delta = static_cast<float>(up->temperature) - static_cast<float>(self.temperature);
				sum_deltas += up_weight * delta;
				total_weight += up_weight;
			}
			if (down) {
				float delta = static_cast<float>(down->temperature) - static_cast<float>(self.temperature);
				sum_deltas += down_weight * delta;
				total_weight += down_weight;
			}
			if (left) {
				float delta = static_cast<float>(left->temperature) - static_cast<float>(self.temperature);
				sum_deltas += side_weight * delta;
				total_weight += side_weight;
			}
			if (right) {
				float delta = static_cast<float>(right->temperature) - static_cast<float>(self.temperature);
				sum_deltas += side_weight * delta;
				total_weight += side_weight;
			}
			self.temperature += (Uint8)round(alpha * (sum_deltas / total_weight));
			if (up) {
				if (up->temperature <= self.temperature) {
					int avg = up->temperature + self.temperature;
					avg /= 2;
					up->temperature = avg;
				}
			}
		}
	}
	update_count += 1;
	update_count = update_count % update_mod;
}

void Weather_System::sun_temperature_update() {
	float day_temp = get_day_temperature();
	float night_temp = day_temp * 0.6f;
	float temperature_scalar = std::cos(2 * M_PI * (time_system.get_day_pct() - 0.5f));
	temperature_scalar = -0.5f * temperature_scalar + 0.5f;
	float current_temperature = night_temp + (day_temp - night_temp) * temperature_scalar;
	int sea_level = 130;
	for (auto i : surface_tiles) {
		float altitude = sea_level - static_cast<float>(i.second);
		float altitude_factor = std::clamp(1.0f - (altitude / 100.0f), 0.1f, 1.0f);
		float permeability_factor = 1 + (0.15 * i.first.get().permeability / 10000);

		i.first.get().temperature = (current_temperature * altitude_factor * permeability_factor);
	}
}

float Weather_System::get_day_temperature() {
	int day_of_year = time_system.get_month_days_now() + time_system.get_time().day;

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

void Weather_System::evaporations(float delta) {
	const Uint8 evaporation_temperature = 40;
	const float evaporation_rate = 0.02f;

	int x = 0;
	for (auto& pair : surface_tiles) {
		Tile& tile = pair.first.get();
		int y = pair.second;
		x++;

		if (tile.temperature < evaporation_temperature || tile.saturation == 0)
			continue;

		if (y == 0) continue;
		Tile& above = world_reference.at(x).at(y - 1);

		if (above.permeability < 10000 && above.humidity >= 10000) continue;

		float temp_factor = (tile.temperature - evaporation_temperature) / 30.0f;
		float permeability_penalty = 1.0f - (tile.permeability / 10000.0f);
		float evap_amount = tile.saturation * evaporation_rate * temp_factor * permeability_penalty * delta;

		int evap_units = static_cast<int>(evap_amount);
		if (evap_units <= 0) continue;

		evap_units = std::min(evap_units, static_cast<int>(tile.saturation));
		int humidity_space = above.max_saturation - above.saturation;
		int humidity_added = std::min(evap_units, humidity_space);

		tile.saturation -= humidity_added;
		above.humidity += humidity_added;
	}
}

void Weather_System::find_surface_tiles() {
	for (int x = 0; x < world_reference.size(); x++) {
		for (int y = 0; y < world_reference.at(x).size(); y++) {
			if (world_reference.at(x).at(y).max_saturation != 10000 ||
				world_reference.at(x).at(y).max_saturation == 10000 && world_reference.at(x).at(y).saturation > 0) {
				surface_tiles.push_back(std::make_pair(std::ref(world_reference.at(x).at(y)), y));
				break;
			}
		}
	}
}