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
#include "utils.hpp"

class Garden {
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
	SDL_Texture* terrain_atlas;

	std::vector<std::vector<Tile>> world; // 2d array of 8x8px 'blocks' in the garden
	bool running;
	int screen_width;
	int screen_height;
	SDL_Rect camera;
	std::string window_title;
};