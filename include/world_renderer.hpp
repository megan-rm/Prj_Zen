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
#include <vector>

/****************************************************************
*	Rendering strategy (performance):
*	- terrain never changes at runtime, so it's baked ONCE into
*	  a few large chunk textures; drawing the world is a handful
*	  of texture copies instead of ~19k per-tile RenderCopies
*	- water/saturation (and the t/h debug views) render through
*	  a streaming overlay texture at 1 px per tile, scaled up 8x
*	  with nearest sampling — one texture update + one copy
*	  replaces thousands of per-tile FillRects. Soil alpha is
*	  proportional to saturation, so the water table reads as a
*	  gradient in the ground; standing water draws solid.
****************************************************************/
class World_Renderer {
public:
	World_Renderer(SDL_Renderer* ren, Texture_Manager& tm, SDL_Rect& cam) : renderer(ren), texture_manager(tm), camera(cam) {
		tile_size = Zen::TILE_SIZE;
		tile_atlas = texture_manager.get_texture("tilemap");
		SDL_QueryTexture(tile_atlas, nullptr, nullptr, &tile_atlas_width, &tile_atlas_height);
		sky_gradient = texture_manager.get_texture("sky_gradient");
		celestial_bodies = texture_manager.get_texture("celestial_bodies");
	};

	~World_Renderer();
	void bake_terrain(const std::vector<std::vector<Tile>>& world); // call once after world load
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

	std::vector<SDL_Texture*> terrain_chunks; // owned here (baked at init)
	SDL_Texture* overlay = nullptr;           // owned here (1 px per tile, streaming)
	int overlay_w = 0;
	int overlay_h = 0;
	static constexpr int CHUNK_TILES = 256;   // chunk width: 256 tiles = 2048 px

	Zen::DEBUG_MODE* garden_debug_mode;
	int tile_atlas_width;
	int tile_atlas_height;
	int tile_size;
	SDL_Rect tile_src_rect(int tile_id);
	void update_overlay(const std::vector<std::vector<Tile>>& world, int start_x, int start_y, int end_x, int end_y);
};
