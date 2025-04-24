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

#include "garden_generator.hpp"
#include "tile.hpp"
#include "utils.hpp"
#include "water_system.hpp"
#include "world_renderer.hpp"


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
	void mouse_click(int x, int y);
private:
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Event events;

	std::vector<std::vector<Tile>> world; // 2d array of 8x8px 'blocks' in the garden
	std::vector<std::vector<Tile>> buffer; // I don't quite like how we have Tiles as buffers for just properly reading saturation states between updates.
	bool running;
	int screen_width;
	int screen_height;
	SDL_Rect camera;
	std::string window_title;
	World_Renderer* world_renderer;
	Water_System* water_system;
	static constexpr float camera_speed = Zen::TERRAIN_WIDTH / 60.0f;
	bool up_key, down_key, left_key, right_key, left_mouse;
};