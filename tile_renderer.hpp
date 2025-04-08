#pragma once
#include "SDL.h"
#include "SDL_image.h"
#include "tile.hpp"

#include <vector>

class Tile_Renderer {
public:
	Tile_Renderer(SDL_Texture* atlas);
	~Tile_Renderer();

	void render(const std::vector<std::vector<Tile>>& world, SDL_Renderer* renderer, const SDL_Rect& camera);

private:
	SDL_Texture* atlas;
	int atlas_width;
	int atlas_height;
	int tile_size;
	SDL_Rect tile_src_rect(int tile_id);
};