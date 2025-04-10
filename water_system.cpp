#include "water_system.hpp"

void Water_System::update_saturation() {

}

void Water_System::update_saturation(int cols_mod) {
	if (update_count == cols_mod) {
		update_count = 0;
	}
	for (int i = 0; i < world_reference.size(); i++) {
		if (i % cols_mod == 0) {
			for (int tile = 0; i < world_reference.at(i).size(); tile++) {
				world_reference.at(i).at(tile).saturation;
			}
		}
	}
	update_count++;
}

void Water_System::update_saturation(int cols, int rows) {

}