#include "water_system.hpp"

void Water_System::update_saturation(int delta) {
	for (int i = update_count; i < world_reference.size(); i += update_mod) {
		for (int y = 0; y < world_reference.at(i).size(); y++){
			Tile& self = world_reference.at(i).at(y);
			Tile* left, *right, *down;
			left = nullptr;
			right = nullptr;
			down = nullptr;
			if (i > 0) left = &world_reference.at(i - 1).at(y);
			if (i < world_reference.size()) right = &world_reference.at(i + 1).at(y);
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
		}
	}
	update_count = (update_count + 1) % update_mod;
}

void Water_System::calculate_flow(Tile& self, Tile& tile, int delta) {
	int saturation_difference = std::abs(self.saturation - tile.saturation); /// what if we didn't abs, and just kept it as potential negative and had 2 variables; 1 to add into from, 1 to add into dest.
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
	int from_chance = self.permeability / 100; // 10,000 / 100 = 100, so we're taking the potential max divided by 100 to bring it down to a int representation of percentage
	float to_scale = tile.permeability / 100000.0f;

	std::random_device rd;
	std::mt19937 g(rd());
	std::uniform_real_distribution<float> dist(0, 1);
	float random = dist(g);
	if (from_chance > random) {
		return;
	}
	float rate_constant = 0.25; // fam, i'm gonna fine tune this over time. idk.
	float fraction = delta / rate_constant;
	int amount = saturation_difference * fraction * to_scale;
	int remainder = 0;
	tile.saturation += amount;
	if (tile.saturation >= tile.max_saturation) {
		remainder = tile.saturation - tile.max_saturation;
	}
	self.saturation += remainder;
	self.saturation -= amount;
	return;
}