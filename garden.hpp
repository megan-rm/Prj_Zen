#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <SDL.h>
#include <algorithm>
#include <random>

namespace Zen {
	constexpr int TERRAIN_WIDTH = 10000;
	constexpr int TERRAIN_HEIGHT = 1200;
	constexpr int TOTAL_PIXELS = TERRAIN_WIDTH * TERRAIN_HEIGHT * 0.8;
	
	constexpr int MOUNTAIN_HEIGHT = 800; // 200 for terrain, 800 above terrain?
	constexpr int MOUNTAIN_WIDTH = 800; // we'll add some randomness to this? I'm unsure.
	static int mountain_start_x;
	static int mountain_end_x;
	static int mountain_end_y; // for river formation, we only need the intercept at the end
	static int river_start_x;
	static int river_end_x;
	static int lake_start_x;
	static int lake_end_x;

	enum PIXEL_TYPE{EMPTY=0, DIRT, CLAY, STONE};
	
	static SDL_Color DIRT_COLOR = { 140, 70, 20, 255 };
	static SDL_Color CLAY_COLOR = { 197, 95, 64, 255 };
	static SDL_Color STONE_COLOR = { 128, 128, 128, 255 };
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
	void generate_spritesheet();
	void place_terrain(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], int appx_height, float dirt_pct, float clay_pct, float stone_pct);
	void place_lake(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], int center, int h_radius, int w_radius);
	void place_mountain(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], int center, int height);

	void set_pixel_color(Zen::PIXEL_TYPE);
private:
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Event events;
	std::vector<std::vector<SDL_Rect>> grid; // 2d array of 8x8px 'blocks' in the garden
	bool running;
	int screen_width;
	int screen_height;
	std::string window_title;
};