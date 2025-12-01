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

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "cloud_manager.hpp"
#include "garden_generator.hpp"
#include "texture_manager.hpp"
#include "tile.hpp"
#include "time_system.hpp"
#include "utils.hpp"
#include "water_system.hpp"
#include "weather_system.hpp"
#include "wind_manager.hpp"
#include "world_renderer.hpp"


class Garden {
public:
	Garden() = default;
	Garden(std::string st, int sw, int sh);
	~Garden();
	void run();
	void input(float delta);
	void init();
	void render(float delta);
	void update(float delta	);
	bool load_world();
	bool save_world();
	void mouse_click(int x, int y);
private:
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Event events;
	SDL_Rect camera;

	std::vector<std::vector<Tile>> world; // 2d array of 8x8px 'blocks' in the garden
	std::vector<std::vector<Tile>> buffer; // I don't quite like how we have Tiles as buffers for just properly reading saturation states between updates.
	bool running;
	bool existing_world;
	bool up_key, down_key, left_key, right_key, h_key, t_key, left_mouse;
	int screen_width;
	int screen_height;
	std::string window_title;
	Cloud_Manager* cloud_manager;
	Texture_Manager* texture_manager;
	Time_System time_system;
	Water_System* water_system;
	Wind_Manager* wind_manager;
	Weather_System* weather_system;
	World_Renderer* world_renderer;

	static constexpr float camera_speed = Zen::TERRAIN_WIDTH / 60.0f;
	Uint64 tick_count;
	Zen::DEBUG_MODE debug_mode;
};