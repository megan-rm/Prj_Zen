#pragma once
#include "SDL.h"
#include "SDL_image.h"
#include "tile.hpp"
#include "texture_manager.hpp"
#include "time_system.hpp"

#include <iostream>
#include <math.h>
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
		moon_phases["new_moon"] = { 0,0,16,16 };
		moon_phases["gibbous"] = { 16, 0, 16, 16 };
		moon_phases["quarter"] = { 32, 0, 16, 16 };
		moon_phases["crescent"] = { 48, 0, 16, 16 };
	};
	~World_Renderer();
	void render_sky(Time_System& ts);
	void render_tiles(const std::vector<std::vector<Tile>>& world); // let's just hold a reference... world renderer cant render a world it doesn't know about?
	void render_sun(Time_System& ts);
	void render_moon(Time_System& ts);
	void render_stars(Time_System& ts);
	void render_clouds(/*Temperature_System& tmp_s*/);

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

	int tile_atlas_width;
	int tile_atlas_height;
	int tile_size;
	SDL_Rect tile_src_rect(int tile_id);
	std::unordered_map<std::string, SDL_Rect> moon_phases;
};