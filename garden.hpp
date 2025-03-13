#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <SDL.h>
#include <algorithm>
#include <random>

namespace Zen {
	const int TERRAIN_WIDTH = 640;
	const int TERRAIN_HEIGHT = 200;
	const int TOTAL_PIXELS = TERRAIN_WIDTH * TERRAIN_HEIGHT * 0.8;
	const float DIRT_RATIO = 0.7f;
	const float CLAY_RATIO = 0.2f;
	const float STONE_RATIO = 0.1f;
	const int DIRT_PIXELS = TOTAL_PIXELS * DIRT_RATIO;
	const int CLAY_PIXELS = TOTAL_PIXELS * CLAY_RATIO;
	const int STONE_PIXELS = TOTAL_PIXELS * STONE_RATIO;
	enum PIXEL_TYPE{EMPTY=0, DIRT, CLAY, STONE};
	
	SDL_Color DIRT_COLOR = { 140, 70, 20, 255 };
	SDL_Color CLAY_COLOR = { 210, 180, 140, 255 };
	SDL_Color STONE_COLOR = { 128, 128, 128, 255 };
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