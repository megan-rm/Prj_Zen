#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <math.h>
#include <fstream>
#include <unordered_map>
#include <sstream>

#include <SDL.h>
#include <SDL_image.H>

#include "tile.hpp"

namespace Zen {
	constexpr int TERRAIN_WIDTH = 10000;
	constexpr int TERRAIN_HEIGHT = 1200;
	
	constexpr int TILE_SIZE = 8;

	constexpr int MOUNTAIN_HEIGHT = 800; // 200 for terrain, 600 above terrain?
	constexpr int MOUNTAIN_WIDTH = 800; // we'll add some randomness to this? I'm unsure.
	
	constexpr int LAKE_WIDTH = 600;
	constexpr int LAKE_DEPTH = 120;

	//using ints below to prevent water loss with rounding errors in floats
	constexpr int DIRT_PERMIABILITY = 65; // divide by 100 aka 0.65;
	constexpr int CLAY_PERMIABILITY = 25; // divide by 100 ; aka  0.25 ; 10000 => 100.00 
	constexpr int STONE_PERMIABILITY = 0;

	static int mountain_start_x = 0;
	static int mountain_end_x = 0;
	static int mountain_end_y = 0; // for river formation, we only need the intercept at the end
	
	static int river_start_x = 0;
	static int river_end_x = 0;
	
	static int lake_start_x = 0;
	static int lake_end_x = 0;

	enum PIXEL_TYPE{EMPTY=0, DIRT, CLAY, STONE};
	
	static SDL_Color DIRT_COLOR = { 140, 70, 20, 255 };
	static SDL_Color CLAY_COLOR = { 197, 95, 64, 255 };
	static SDL_Color STONE_COLOR = { 128, 128, 128, 255 };
	static SDL_Color MAGENTA_COLOR = { 255, 0, 255, 255 };
}

class Garden
{
public:
	Garden();
	Garden(std::string st, int sw, int sh);
	~Garden();
	void run();
	void input();
	void render();
	void update();
	void generate_world();
	void generate_tilemap(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT]);
	void place_terrain(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], int appx_height, float dirt_pct, float clay_pct, float stone_pct);
	void place_lake(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], bool direction);
	void place_mountain(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], int center, int height);
	void place_river(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], bool direction);
	bool load_world();
private:
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Event events;
	std::vector<std::vector<Tile>> world; // 2d array of 8x8px 'blocks' in the garden
	bool running;
	int screen_width;
	int screen_height;
	SDL_Rect camera;
	std::string window_title;
};