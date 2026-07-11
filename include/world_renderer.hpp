#pragma once
#include <SDL.h>
#include <SDL_image.h>

#include "texture_manager.hpp"
#include "tile.hpp"
#include "time_system.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class World_Renderer {
public:
	World_Renderer(SDL_Renderer* ren, Texture_Manager& tm, SDL_Rect& cam) : renderer(ren), texture_manager(tm), camera(cam) {
		tile_size = Zen::TILE_SIZE;
		tile_atlas = texture_manager.get_texture("tilemap");
		SDL_QueryTexture(tile_atlas, nullptr, nullptr, &tile_atlas_width, &tile_atlas_height);
		sky_gradient = texture_manager.get_texture("sky_gradient");
		celestial_bodies = texture_manager.get_texture("celestial_bodies");
	};

	~World_Renderer() = default; // textures are owned (and destroyed) by Texture_Manager
	void render_sky(Time_System& ts);
	void render_tiles(const std::vector<std::vector<Tile>>& world); // reads the completed sim snapshot
	void render_sun(Time_System& ts);
	void render_moon(Time_System& ts);
	void render_stars(Time_System& ts);
	SDL_Color get_heatmap_color(int temperature);
	SDL_Color get_humidity_color(int humidity);

	void register_debug_mode(Zen::DEBUG_MODE& mode) {
		garden_debug_mode = &mode;
	}

private:
	SDL_Renderer* renderer;
	SDL_Rect& camera;
	Texture_Manager& texture_manager;
	SDL_Texture* tile_atlas;
	SDL_Texture* sky_gradient;
	SDL_Texture* celestial_bodies;

	Zen::DEBUG_MODE* garden_debug_mode;
	int tile_atlas_width;
	int tile_atlas_height;
	int tile_size;
	SDL_Rect tile_src_rect(int tile_id);
};
