#include "weather_system.hpp"

#include <algorithm>
#include <cmath>

/****************************************************************
*
* unsure if i should lerp a day / night temperature relative to
* tile's height in the world. shouldn't stone heavy tiles 'hold on'
* to temperature more? can we track this?
*
****************************************************************/
void Weather_System::update_temperatures(float delta) {
	// full pass over every tile, every sim tick — including row 0,
	// which previously never updated and became a one-way humidity trap.
	for (int x = 0; x < static_cast<int>(world_reference.size()); x++) {
		for (int y = static_cast<int>(world_reference.at(x).size()) - 1; y >= 0; y--) {
			Tile& self = world_reference.at(x).at(y);
			Tile *left, *right, *down, *up;
			left = nullptr;
			right = nullptr;
			down = nullptr;
			up = nullptr;
			float up_weight = 1.35f;
			float down_weight = 0.85f;
			float side_weight = 1.0f;

			float base_alpha = 0.998f;
			float permeability_factor = 1 - (self.permeability / 10000.0f);
			if (permeability_factor == 0) permeability_factor = 0.15f;
			float saturation_factor;
			if (self.max_saturation > 0) {
				saturation_factor = self.saturation / float(self.max_saturation);
			}
			else {
				saturation_factor = 0.25f;
			}
			float saturation_penalty = 1.0f - 0.01f * saturation_factor;
			float alpha = base_alpha * permeability_factor * saturation_penalty;
			alpha = std::max(0.2f, alpha);
			float sum_deltas = 0.0f;
			float total_weight = 0.0f;
			if (x > 0)  left = &world_reference.at(x - 1).at(y);
			if (x < static_cast<int>(world_reference.size()) - 1) right = &world_reference.at(x + 1).at(y);
			if (y < static_cast<int>(world_reference.at(x).size()) - 1) down = &world_reference.at(x).at(y + 1);
			if (y > 0) up = &world_reference.at(x).at(y - 1);
			// laplace sorta? i guess...
			if (up) {
				float d = static_cast<float>(up->temperature) - static_cast<float>(self.temperature);
				sum_deltas += up_weight * d;
				total_weight += up_weight;
			}
			if (down) {
				float d = static_cast<float>(down->temperature) - static_cast<float>(self.temperature);
				sum_deltas += down_weight * d;
				total_weight += down_weight;
			}
			if (left) {
				float d = static_cast<float>(left->temperature) - static_cast<float>(self.temperature);
				sum_deltas += side_weight * d;
				total_weight += side_weight;
			}
			if (right) {
				float d = static_cast<float>(right->temperature) - static_cast<float>(self.temperature);
				sum_deltas += side_weight * d;
				total_weight += side_weight;
			}
			// signed math + clamp: the old Uint8 cast turned negative deltas into +250ish
			int new_temperature = static_cast<int>(self.temperature) + static_cast<int>(std::lround(alpha * (sum_deltas / total_weight)));
			self.temperature = static_cast<Sint8>(std::clamp(new_temperature, -125, 125));
			if (up) {
				if (up->temperature <= self.temperature) {
					int avg = static_cast<int>(up->temperature) + static_cast<int>(self.temperature);
					avg /= 2;
					up->temperature = static_cast<Sint8>(avg);
				}
			}
			humidity_handling(x, y, delta);
		}
	}
	evaporations(delta);
}

Uint64 Weather_System::water_check() {
	Uint64 total_water = 0;
	for (size_t x = 0; x < world_reference.size(); x++) {
		for (size_t y = 0; y < world_reference.at(x).size(); y++) {
			total_water += world_reference.at(x).at(y).humidity + world_reference.at(x).at(y).saturation;
		}
	}
	return total_water;
}

void Weather_System::sun_temperature_update() {
	float day_temp = get_day_temperature();
	float night_temp = day_temp * 0.6f;
	float temperature_scalar = std::cos(2 * M_PI * time_system.get_day_pct());
	temperature_scalar = 0.5f * (1.0f - temperature_scalar);
	temperature_scalar = -0.5f * temperature_scalar + 0.5f;
	float current_temperature = night_temp + (day_temp - night_temp) * temperature_scalar;
	int sea_level = 125;
	for (auto& s : surface_tiles) {
		Tile& tile = world_reference.at(s.x).at(s.y);
		float altitude = sea_level - static_cast<float>(s.y);
		float altitude_factor = std::clamp(1.0f - (altitude / 100.0f), 0.1f, 1.0f);
		float permeability_factor = 1 + (0.15f * tile.permeability / 10000);
		float target = current_temperature * altitude_factor * permeability_factor;

		// thermal inertia: wet surfaces chase the sun slowly, dry land quickly.
		// the lake lagging hours behind land temperature is what drives the
		// day/night breeze cycle in the wind system.
		float wetness = 0.0f;
		if (tile.max_saturation > 0) {
			wetness = std::clamp(tile.saturation / static_cast<float>(tile.max_saturation), 0.0f, 1.0f);
		}
		float rate = Zen::LAND_HEAT_RATE + (Zen::WATER_HEAT_RATE - Zen::LAND_HEAT_RATE) * wetness;
		float new_temperature = tile.temperature + (target - tile.temperature) * rate;

		tile.temperature = static_cast<Sint8>(std::clamp(static_cast<int>(new_temperature), -125, 125));
	}
}

float Weather_System::get_day_temperature() {
	int day_of_year = time_system.get_month_days_now() + time_system.get_time().day;

	float min_temp = -19.0f;
	float max_temp = 30.0f;

	float seasonal_variation = (std::sin(2 * M_PI * (day_of_year - 81) / 365) + 1.0) / 2.0;
	float day_temp = min_temp + (max_temp - min_temp) * seasonal_variation;
	day_temp = (day_temp * static_cast<float>(9.0f / 5.0f)) + 32;
	return day_temp;
}

void Weather_System::update_forecasts() {
	std::ifstream file(Zen::data_path("world_info/weekly.zen"));
	if (!file.is_open()) {

	}

	file.open(Zen::data_path("world_info/monthly.zen"));
}

void Weather_System::evaporations(float delta) {
	const Sint8 evaporation_temperature = 40;

	std::uniform_real_distribution<float> unit_dist(0.0f, 1.0f);
	for (auto& s : surface_tiles) {
		Tile& tile = world_reference.at(s.x).at(s.y);

		if (tile.temperature < evaporation_temperature || tile.saturation == 0)
			continue;

		if (s.y == 0) continue;
		Tile& above = world_reference.at(s.x).at(s.y - 1);

		if (!Zen::is_air(above)) continue;

		float temp_factor = (tile.temperature - evaporation_temperature) / 30.0f;
		// vapor pressure deficit: moist air above suppresses evaporation, so a
		// storm's own humid boundary layer throttles its resupply (breaks the
		// perpetual lake-storm feedback loop)
		float deficit = 1.0f - (static_cast<float>(above.humidity) / Zen::HUMIDITY_MAX);
		float evap_amount = tile.saturation * Zen::EVAPORATION_RATE * temp_factor * deficit * delta;

		// stochastic rounding: fractional amounts evaporate probabilistically,
		// so slow real-time rates still work instead of truncating to zero
		int evap_units = static_cast<int>(evap_amount);
		if (unit_dist(rand) < (evap_amount - evap_units)) evap_units += 1;
		if (evap_units <= 0) continue;

		evap_units = std::min(evap_units, static_cast<int>(tile.saturation));
		// air tiles hold at most HUMIDITY_MAX (100) humidity — the old code used
		// max_saturation (10000) here, which silently broke the humidity cap
		int humidity_space = Zen::HUMIDITY_MAX - static_cast<int>(above.humidity);
		int humidity_added = std::clamp(evap_units, 0, humidity_space);
		if (humidity_added <= 0) continue;

		tile.saturation -= humidity_added;
		above.humidity += humidity_added;
	}
}

/****************************************************************
*	Buoyancy band: below CLOUD_BASE_ROW moist air mostly rises.
*	Inside the band (CLOUD_BASE_ROW -> CLOUD_TOP_ROW) the lift
*	fades out and lateral spread takes over, so clouds billow
*	sideways into a deck. Above CLOUD_TOP_ROW there is no lift
*	at all and humidity gets pushed back down — nothing can
*	stack on row 0 anymore.
****************************************************************/
void Weather_System::humidity_handling(int x, int y, float delta) {
	Tile& self = world_reference.at(x).at(y);
	if (self.humidity == 0 || !Zen::is_air(self)) return;

	Tile *up, *down, *left, *right;
	up = down = nullptr;

	// the sky is a cylinder: x wraps at both edges. humidity_share only ever
	// deposits into air tiles, so at the seam the mountain blocks low-altitude
	// wrap like a real ridge, while sky-level humidity flows through.
	const int max_x = static_cast<int>(world_reference.size()) - 1;
	left = &world_reference.at(x > 0 ? x - 1 : max_x).at(y);
	right = &world_reference.at(x < max_x ? x + 1 : 0).at(y);
	if (y > 0) up = &world_reference.at(x).at(y - 1);
	if (y < static_cast<int>(world_reference.front().size()) - 1) down = &world_reference.at(x).at(y + 1);

	float lift_scale, down_weight, side_weight;
	if (y <= Zen::CLOUD_TOP_ROW) {
		// above the deck: no lift, drain back down
		lift_scale = 0.0f;
		down_weight = 0.9f;
		side_weight = 0.35f;
	}
	else if (y <= Zen::CLOUD_BASE_ROW) {
		// inside the deck: lift fades to zero at the top, spread widens
		lift_scale = static_cast<float>(y - Zen::CLOUD_TOP_ROW) / static_cast<float>(Zen::CLOUD_BASE_ROW - Zen::CLOUD_TOP_ROW);
		down_weight = 0.05f;
		side_weight = 0.45f;
	}
	else {
		// below the deck: full convective lift
		lift_scale = 1.0f;
		down_weight = 0.05f;
		side_weight = 0.2f;
	}

	/****************************************************************
	*	Convective lift: a fixed-rate conveyor, NOT gradient
	*	diffusion — thermals carry moisture up through drier air
	*	because temperature drives them. Warmth strengthens the
	*	updraft (with a floor, so cold nights pool moisture low:
	*	free morning fog). Stochastic rounding keeps slow rates
	*	alive. Conserves exactly.
	****************************************************************/
	if (lift_scale > 0.0f) {
		float temp_gate = 0.25f + 0.75f * std::clamp((static_cast<float>(self.temperature) - 32.0f) / 30.0f, 0.0f, 1.0f);
		float amount = Zen::CLOUD_LIFT_PER_SECOND * lift_scale * temp_gate * delta;
		int units = static_cast<int>(amount);
		std::uniform_real_distribution<float> unit_dist(0.0f, 1.0f);
		if (unit_dist(rand) < (amount - units)) units += 1;
		units = std::min(units, static_cast<int>(self.humidity));

		// rise as far as the tile above has room
		int lifted = 0;
		if (up && Zen::is_air(*up) && up->saturation == 0) {
			lifted = std::min(units, Zen::HUMIDITY_MAX - static_cast<int>(up->humidity));
			if (lifted > 0) {
				self.humidity -= lifted;
				up->humidity += lifted;
			}
		}

		/****************************************************************
		*	Anvil spreading: lift flux that can't rise diverts
		*	sideways to the drier horizontal neighbor. This is what
		*	makes clouds billow OUT along the deck instead of
		*	stacking downward from their own ceiling.
		****************************************************************/
		int blocked = units - lifted;
		if (blocked > 0 && self.humidity > 0) {
			Tile* side = nullptr;
			bool left_ok = left && Zen::is_air(*left) && left->saturation == 0 && left->humidity < self.humidity;
			bool right_ok = right && Zen::is_air(*right) && right->saturation == 0 && right->humidity < self.humidity;
			if (left_ok && right_ok) side = (left->humidity <= right->humidity) ? left : right;
			else if (left_ok) side = left;
			else if (right_ok) side = right;

			if (side) {
				int spread = std::min({ blocked, static_cast<int>(self.humidity), Zen::HUMIDITY_MAX - static_cast<int>(side->humidity) });
				if (spread > 0) {
					self.humidity -= spread;
					side->humidity += spread;
				}
			}
		}
	}

	if (down) humidity_share(self, *down, down_weight);
	if (left) humidity_share(self, *left, side_weight);
	if (right) humidity_share(self, *right, side_weight);
}

void Weather_System::humidity_share(Tile& self, Tile& neighbor, float weight) {
	if (!Zen::is_air(neighbor) || neighbor.saturation != 0) {
		return;
	}

	float self_humidity_pct = self.humidity / static_cast<float>(Zen::HUMIDITY_MAX);
	float neighbor_humidity_pct = neighbor.humidity / static_cast<float>(Zen::HUMIDITY_MAX);
	float delta_humidity = self_humidity_pct - neighbor_humidity_pct;

	if (std::abs(delta_humidity) > 0.08f) {
		int transfer = static_cast<int>(delta_humidity * Zen::HUMIDITY_MAX * weight);

		if (transfer > 0) {
			transfer = std::min(transfer, static_cast<int>(self.humidity));
			transfer = std::min(transfer, Zen::HUMIDITY_MAX - static_cast<int>(neighbor.humidity));
		}
		else {
			transfer = std::max(transfer, -static_cast<int>(neighbor.humidity));
			transfer = std::max(transfer, -(Zen::HUMIDITY_MAX - static_cast<int>(self.humidity)));
		}
		if (transfer == 0) return;

		self.humidity -= transfer;
		neighbor.humidity += transfer;
	}
}

void Weather_System::find_surface_tiles() {
	for (int x = 0; x < static_cast<int>(world_reference.size()); x++) {
		for (int y = 0; y < static_cast<int>(world_reference.at(x).size()); y++) {
			Tile& tile = world_reference.at(x).at(y);
			if (!Zen::is_air(tile) || tile.saturation > 0) {
				surface_tiles.push_back({ x, y });
				break;
			}
		}
	}
}
