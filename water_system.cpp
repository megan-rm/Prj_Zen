#include "water_system.hpp"

void Water_System::update_saturation(float delta) {
	for (int i = update_count; i < world_reference.size(); i += update_mod) {
		for (int y = 0; y < world_reference.at(i).size() - 1; y++){
			Tile& self = world_reference.at(i).at(y);
			if (self.saturation == 0) {
				continue;
			}
			Tile* left, *right, *down;
			left = nullptr;
			right = nullptr;
			down = nullptr;
			if (i > 0) left = &world_reference.at(i - 1).at(y);
			if (i < world_reference.size() - 1) right = &world_reference.at(i + 1).at(y);
			if (y < world_reference.at(i).size()) down = &world_reference.at(i).at(y + 1);
			//share down first, if we can
			if (down != nullptr) {
				if (down->saturation < down->max_saturation) {
					calculate_flow(self, *down, delta);
				}
			}
			//share left
			if (left != nullptr) {
				if (self.saturation < left->saturation && left->saturation < left->max_saturation) {
					calculate_flow(self, *left, delta);
				}
			}
			//share left
			if (right != nullptr) {
				if (self.saturation < right->saturation && right->saturation < right->max_saturation) {
					calculate_flow(self, *right, delta);
				}
			}
			water_update_total += self.saturation;
		}
	}
	update_count += 1;
	if (update_count >= update_mod) {
		std::cout << "Test" << std::endl;
		update_count = 0;
		std::cout << water_update_total << "/" << Zen::water_budget << std::endl;
		water_update_total = 0;
	}
}

void Water_System::calculate_flow(Tile& self, Tile& tile, float delta) {
	//delta = 16.0/1000.0f;
	int saturation_difference = self.saturation - tile.saturation; /// what if we didn't abs, and just kept it as potential negative and had 2 variables; 1 to add into from, 1 to add into dest.
	if (saturation_difference < 0) {
		return;
	}
	int tiles_ratio = tile.saturation / tile.max_saturation;
	/**************************************************************
	*
	*	so like... what i think we're going to do is like...
	*   source tile's permeability is the probability that water
	*   leaves the tile at all. permeability / 100 = chance water
	*	can leave the source tile. Assuming it does, we now look at
	*   the difference between source->destination, absolute value
	*   then we times that by some... ratios or some such? idk. 
	*
	**************************************************************/
	float from_chance = self.permeability / 10000.0f; // 10,000 / 100 = 100, so we're taking the potential max divided by 100 to bring it down to a int representation of percentage
	float to_scale = tile.permeability / 10000.0f;

	std::random_device rd;
	std::mt19937 g(rd());
	std::uniform_real_distribution<float> dist(0, 1);
	float random = dist(g);
	if (random > from_chance) {
		return;
	}
	float rate_constant = 0.25; // fam, i'm gonna fine tune this over time. idk.
	float fraction = delta / rate_constant;
	int amount = saturation_difference * fraction * to_scale;
	amount = std::max(amount, 1);
	
	int remainder = 0;
	tile.saturation += amount;
	if (tile.saturation >= tile.max_saturation) {
		remainder = tile.saturation - tile.max_saturation;
		tile.saturation = tile.max_saturation;
	}

	self.saturation += remainder;
	self.saturation -= amount;
	if (self.saturation > self.max_saturation) {
		std::cout << "ERROR" << std::endl;
	}
	return;
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
	Uint64 total_water = 0;
	std::random_device rd;
	std::mt19937 g(rd());
	// so if we set relative_pct to 0.5, on average you'd expect a tile to hold 50% of its maximum. this random distr
	// allows a +/- 30% to the relative_pct - meaning we'd have 35~65% of our maximum_saturation in each tile;
	std::uniform_real_distribution<float> dist(0.7, 1.3);
	for (int x = 0; x < world_reference.size(); x++) {
		for (int y = 0; y < world_reference.at(x).size(); y++) {
			if (world_reference.at(x).at(y).max_saturation == 10000) {
				continue;
			}
			float sat_pct_modifier = dist(g);
			world_reference.at(x).at(y).saturation = ((sat_pct_modifier * relative_pct) * world_reference.at(x).at(y).max_saturation);
			total_water += world_reference.at(x).at(y).saturation;
		}
	}
	Zen::water_budget = total_water;
	water_total = total_water;
	return total_water;
}