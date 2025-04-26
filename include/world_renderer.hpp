#pragma once
#include "SDL.h"
#include "SDL_image.h"
#include "tile.hpp"
#include "time_system.hpp"

#include <iostream>
#include <string>
#include <vector>

class World_Renderer {
public:
	World_Renderer();
	~World_Renderer();
	void render_sky(Time_System& ts, SDL_Renderer* ren, const SDL_Rect& cam);
	void render_tiles(const std::vector<std::vector<Tile>>& world, SDL_Renderer* renderer, const SDL_Rect& camera);
	void register_tile_atlas(SDL_Renderer* renderer, std::string path);

private:
	SDL_Texture* tile_atlas;
	SDL_Texture* sky_gradient;
	SDL_Texture* sun;
	SDL_Texture* moon;

	int tile_atlas_width;
	int tile_atlas_height;
	int tile_size;
	SDL_Rect tile_src_rect(int tile_id);
};