#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include <SDL2/SDL.h>

#include "cloud_manager.hpp"
#include "texture_manager.hpp"
#include "tile.hpp"
#include "time_system.hpp"

#include <algorithm>
#include <iostream>
#include <math.h>
#include <string>
#include <unordered_map>
#include <vector>

class World_Renderer {
public:
	World_Renderer(SDL_Renderer* ren, Texture_Manager& tm, SDL_Rect& cam) : renderer(ren), texture_manager(tm), camera(cam) {
		cloud_src_rect = { 0, 0, 16, 16 };
		tile_size = Zen::TILE_SIZE;
		tile_atlas = texture_manager.get_texture("tilemap");
		SDL_QueryTexture(tile_atlas, nullptr, nullptr, &tile_atlas_width, &tile_atlas_height);
		sky_gradient = texture_manager.get_texture("sky_gradient");
		celestial_bodies = texture_manager.get_texture("celestial_bodies");
	};

	~World_Renderer();
	void render_sky(Time_System& ts);
	void render_tiles(const std::vector<std::vector<Tile>>& world); // let's just hold a reference... world renderer cant render a world it doesn't know about?
	void render_sun(Time_System& ts);
	void render_moon(Time_System& ts);
	void render_stars(Time_System& ts);
	void render_clouds(int humidity, SDL_Rect dst);
	SDL_Color get_heatmap_color(int temperature);
	SDL_Color get_humidity_color(int humidity);

	void register_debug_mode(Zen::DEBUG_MODE& mode) {
		garden_debug_mode = &mode;
	}

	void register_cloud_manager(Cloud_Manager* cloud_mgr) {
		cloud_manager = cloud_mgr;
	}
	void register_tilemap(Texture_Manager& tx_mgr) {
		texture_manager = tx_mgr;
		tile_size = Zen::TILE_SIZE;
		tile_atlas = texture_manager.get_texture("tilemap");
		SDL_QueryTexture(tile_atlas, nullptr, nullptr, &tile_atlas_width, &tile_atlas_height);
	}

private:
	SDL_Renderer* renderer;
	SDL_Rect& camera;
	Texture_Manager& texture_manager;
	SDL_Texture* tile_atlas;
	SDL_Texture* sky_gradient;
	SDL_Texture* celestial_bodies;

	Cloud_Manager* cloud_manager;
	Zen::DEBUG_MODE* garden_debug_mode;
	int tile_atlas_width;
	int tile_atlas_height;
	int tile_size;
	SDL_Rect tile_src_rect(int tile_id);
	SDL_Rect cloud_src_rect;
};