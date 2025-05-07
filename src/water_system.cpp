#include "water_system.hpp"

void Water_System::update_saturation(float delta) {
	for (int x = update_count; x < world_reference.size(); x += update_mod) {
		for (int y = world_reference.at(x).size() - 1; y > 0; y--) {
			Tile& self = world_reference.at(x).at(y);
			if (self.saturation == 0) {
				continue;
			}
			Tile* left, *right, *down;
			left = nullptr;
			right = nullptr;
			down = nullptr;
			if (x > 0) left = &world_reference.at(x - 1).at(y);
			if (x < world_reference.size() - 1) right = &world_reference.at(x + 1).at(y);
			if (y < world_reference.at(x).size() - 1) down = &world_reference.at(x).at(y + 1);
			//share down first, if we can
			if (down != nullptr) {
				if (down->saturation < down->max_saturation) {
					calculate_flow(self, *down, delta, true);
				}
			}
			//share right
			if (right != nullptr) {
				if (self.saturation > right->saturation && right->saturation < right->max_saturation) {
					calculate_flow(self, *right, delta, false);
				}
			}
			//share left
			if (left != nullptr) {
				if (self.saturation > left->saturation && left->saturation < left->max_saturation) {
					calculate_flow(self, *left, delta, false);
				}
			}
			//water_update_total += self.saturation;
			if (down) {
				capillary_action(self, *down, delta);
			}
		}
	}
	update_count += 1;
	update_count = update_count % update_mod;
}

void Water_System::calculate_flow(Tile& self, Tile& tile, float delta, bool downward) {
	//delta = 16.0/1000.0f;
	int saturation_difference = self.saturation - tile.saturation; /// what if we didn't abs, and just kept it as potential negative and had 2 variables; 1 to add into from, 1 to add into dest.
	if (saturation_difference <= 0 && downward == false) {
		return;
	}
	int initial_sum = self.saturation + tile.saturation;
	int after_sum = 0;

	float from_chance = self.permeability / 10000.0f;
	float to_scale = tile.permeability / 10000.0f;

	std::uniform_real_distribution<float> dist(0, 1);
	float random = dist(rand);
	if (random > from_chance) {
		return;
	}
	float rate_constant = 0.25; // fam, i'm gonna fine tune this over time. idk.
	//float fraction = delta / rate_constant;
	int amount = saturation_difference * rate_constant * to_scale;
	if (downward) {
		amount = self.saturation * to_scale * std::pow(from_chance, 2);
	}
	amount = std::max(amount, 1);
	
	int remainder = 0;
	tile.saturation += amount;

	if (tile.saturation >= tile.max_saturation) {
		remainder = tile.saturation - tile.max_saturation;
		tile.saturation = tile.max_saturation;
	}

	self.saturation += remainder;
	self.saturation -= amount;
	after_sum = self.saturation + tile.saturation;
	if (after_sum != initial_sum) {
		std::cout << "ERROR" << std::endl;
	}
	if (self.saturation > self.max_saturation) {
		std::cout << "ERROR" << std::endl;
	}
	return;
}

void Water_System::capillary_action(Tile& self, Tile& below, float delta) {
	const Uint8 capillary_temp = 33;

	if (self.saturation >= below.saturation || self.temperature < capillary_temp || self.permeability == 10000) {
		return;
	}

	int saturation_delta = below.saturation - self.saturation;
	int capacity = self.max_saturation - self.saturation;

	// Prevent negative/zero transfer
	if (saturation_delta <= 0 || capacity <= 0) return;

	// Capillary effect strength scales with delta time and temperature difference
	float temp_factor = std::clamp((float)(self.temperature - capillary_temp) / 10.0f, 0.0f, 1.0f);
	float permeability_resistance = 1.0f - (self.permeability / 10000.0f);
	permeability_resistance = std::clamp(permeability_resistance, 0.1f, 1.0f);
	float alpha = 0.33f * delta * temp_factor * permeability_resistance;

	int transfer_amount = static_cast<int>(saturation_delta * alpha);
	transfer_amount = std::min(transfer_amount, capacity);
	transfer_amount = std::min(transfer_amount, static_cast<int>(below.saturation));

	if (transfer_amount <= 0) return;

	self.saturation += transfer_amount;
	below.saturation -= transfer_amount;
}

/*************************************************
*
*	ALRIGHT! so, this function should ideally only
*	ever be called probably once. We'll see. Maybe
*	affecting the world's total water budget on
*	a seasonal scale might be something to do, but
*	as of now - you set a relative pct (entries below)
*	and the world should maintain this amount
*	constantly, just in different forms.
*
**************************************************/

Uint64 Water_System::place_water(float relative_pct) {
	std::cout << "placing initial water..." << std::endl;
	Uint64 total_water = 0;
	// so if we set relative_pct to 0.5, on average you'd expect a tile to hold 50% of its maximum. this random distr
	// allows a +/- 30% to the relative_pct - meaning we'd have 35~65% of our maximum_saturation in each tile;
	std::uniform_real_distribution<float> dist(0.85, 1.15);
	for (int x = 0; x < world_reference.size(); x++) {
		for (int y = 0; y < world_reference.at(x).size(); y++) {
			if (world_reference.at(x).at(y).max_saturation == 10000) {
				continue;
			}
			float sat_pct_modifier = dist(rand);
			world_reference.at(x).at(y).saturation = ((sat_pct_modifier * relative_pct) * world_reference.at(x).at(y).max_saturation);
			total_water += world_reference.at(x).at(y).saturation;
		}
	}
	//lake fill
	for (int x = (Zen::lake_start_x / Zen::TILE_SIZE); x < (Zen::lake_end_x / Zen::TILE_SIZE); x++) {
		for (int y = (Zen::mountain_end_y / Zen::TILE_SIZE) + 1; y < Zen::TERRAIN_HEIGHT / Zen::TILE_SIZE; y++) {
			world_reference.at(x).at(y).saturation = world_reference.at(x).at(y).max_saturation;
			total_water += world_reference.at(x).at(y).saturation;
		}
	}
	Zen::water_budget = total_water;
	water_total = total_water;
	return total_water;
}