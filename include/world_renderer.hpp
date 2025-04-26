#pragma once
#include "SDL.h"
#include "SDL_image.h"
#include "tile.hpp"
#include "texture_manager.hpp"
#include "time_system.hpp"

#include <iostream>
#include <string>
#include <vector>

class World_Renderer {
public:
	World_Renderer(SDL_Renderer* ren, Texture_Manager& tm, SDL_Rect& cam) : renderer(ren), texture_manager(tm), camera(cam) {
		moon = nullptr;
		sun = nullptr;
		sky_gradient = nullptr;
		tile_atlas = nullptr;
		SDL_QueryTexture(texture_manager.get_texture("tilemap"), nullptr, nullptr, &tile_atlas_width, &tile_atlas_height);
		tile_size = Zen::TILE_SIZE;
		tile_atlas = texture_manager.get_texture("tilemap");
		sky_gradient = texture_manager.get_texture("sky_gradient");
		sun = texture_manager.get_texture("sun");
	};
	~World_Renderer();
	void render_sky(Time_System& ts);
	void render_tiles(const std::vector<std::vector<Tile>>& world); // let's just hold a reference... world renderer cant render a world it doesn't know about?

private:
	SDL_Renderer* renderer;
	SDL_Rect& camera;
	Texture_Manager& texture_manager;
	SDL_Texture* tile_atlas;
	SDL_Texture* sky_gradient;
	SDL_Texture* sun;
	SDL_Texture* moon;

	int tile_atlas_width;
	int tile_atlas_height;
	int tile_size;
	SDL_Rect tile_src_rect(int tile_id);
};