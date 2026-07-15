#pragma once
#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <unordered_map>
#include <algorithm>

#include <SDL.h>
#include <SDL_image.h>
#include "tile.hpp"
#include "utils.hpp"

class Garden_Generator {
public:
	Garden_Generator() = default;
	~Garden_Generator() = default;
	bool generate_world(SDL_Renderer* renderer);
	bool generate_tilemap(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], SDL_Renderer* renderer);
	bool place_terrain(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], int appx_height, float dirt_pct, float clay_pct, float stone_pct);
	// symmetric mountain at a random x; records peak + both feet
	bool place_mountain(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT]);
	// carve a downhill clay/rock riverbed from a mountain foot outward; returns
	// the river-end x (or -1 if no room) and sets river_mouth_y to its low end
	int carve_river(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], int foot_x, int foot_y, int dir, int length);
	// dig + line a lake basin at center_x; waterline_y sets the fill level so it
	// sits in a depression instead of brimming over. Appends to Zen::lakes.
	bool place_lake(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], int center_x, int waterline_y);

private:
	int peak_x = 0;
	int left_foot_x = 0, left_foot_y = 0;
	int right_foot_x = 0, right_foot_y = 0;
	int river_mouth_y = 0;  // elevation where the most recent river empties
	int avg_surface_y = 0;  // rough average surface of the normal (non-mountain) terrain
	std::mt19937 rng{ std::random_device{}() };

	int surface_at(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], int x); // first non-empty y in a column
};