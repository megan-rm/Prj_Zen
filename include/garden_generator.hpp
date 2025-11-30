#pragma once
#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <unordered_map>
#include <algorithm>

#include "SDL.h"
#include "SDL_image.h"
#include "tile.hpp"
#include "utils.hpp"

class Garden_Generator {
public:
	Garden_Generator() = default;
	~Garden_Generator() = default;
	bool generate_world(SDL_Renderer* renderer);
	bool generate_tilemap(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], SDL_Renderer* renderer);
	bool place_terrain(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], int appx_height, float dirt_pct, float clay_pct, float stone_pct);
	bool place_lake(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], bool direction);
	bool place_mountain(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], int center, int height);
	bool place_river(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], bool direction);
	
private:

};