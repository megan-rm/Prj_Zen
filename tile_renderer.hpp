#pragma once
#include "SDL.h"
#include "SDL_image.h"
#include "tile.hpp"

class Tile_Renderer {
public:
	Tile_Renderer(SDL_Texture* atlas);
	~Tile_Renderer();

	void render(int tile_id, SDL_Renderer* renderer, int x, int y);

private:
	SDL_Texture* atlas;
	int atlas_width;
	int atlas_height;
	int tile_size;
	SDL_Rect tile_src_rect(int tile_id);
};