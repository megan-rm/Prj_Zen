#pragma once
#include "SDL.h"
#include "SDL_image.h"
#include "tile.hpp"

#include <vector>

class World_Renderer {
public:
	World_Renderer(SDL_Renderer* renderer);
	~World_Renderer();

	void render_tiles(const std::vector<std::vector<Tile>>& world, SDL_Renderer* renderer, const SDL_Rect& camera);

private:
	SDL_Texture* tile_atlas;
	int tile_atlas_width;
	int tile_atlas_height;
	int tile_size;
	SDL_Rect tile_src_rect(int tile_id);

};