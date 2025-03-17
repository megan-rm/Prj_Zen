#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <SDL.h>
#include <algorithm>
#include <random>

namespace Zen {
	static const int TERRAIN_WIDTH = 10000;
	static const int TERRAIN_HEIGHT = 200;
	static const int TOTAL_PIXELS = TERRAIN_WIDTH * TERRAIN_HEIGHT * 0.8;
	static const float DIRT_RATIO = 0.7f;
	static const float CLAY_RATIO = 0.2f;
	static const float STONE_RATIO = 0.1f;
	static const int DIRT_PIXELS = TOTAL_PIXELS * DIRT_RATIO;
	static const int CLAY_PIXELS = TOTAL_PIXELS * CLAY_RATIO;
	static const int STONE_PIXELS = TOTAL_PIXELS * STONE_RATIO;
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
	void place_lake(Zen::PIXEL_TYPE** cells, int center, int h_radius, int w_radius);
	void place_mountain(Zen::PIXEL_TYPE** cells, int center, int height);

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