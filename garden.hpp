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
#include "world_renderer.hpp"
#include "garden_generator.hpp"

class Garden {
public:
	Garden();
	Garden(std::string st, int sw, int sh);
	~Garden();
	void run();
	void input(float delta);
	void render(float delta);
	void update(float delta	);
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
	World_Renderer* world_renderer;
	static constexpr float camera_speed = Zen::TERRAIN_WIDTH / 60.0f;
	bool up_key, down_key, left_key, right_key;
};