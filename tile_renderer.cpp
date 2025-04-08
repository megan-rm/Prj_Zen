#include "tile_renderer.hpp"

Tile_Renderer::Tile_Renderer(SDL_Texture* atlas, int tile_size) {
	this->atlas = atlas;
	this->tile_size = tile_size;
	SDL_QueryTexture(atlas, nullptr, nullptr, &atlas_width, &atlas_height);

}

Tile_Renderer::~Tile_Renderer() {
	SDL_DestroyTexture(atlas);
}

SDL_Rect Tile_Renderer::tile_src_rect(int tile_id) {
	int tiles_per_row = atlas_width / tile_size;
	int x = (tile_id % tiles_per_row) * tile_size;
	int y = (tile_id / tiles_per_row) * tile_size;
	return SDL_Rect{ x, y, tile_size, tile_size };
}

void Tile_Renderer::render(int tile_id, SDL_Renderer* renderer, int x, int y) {
	SDL_Rect src = tile_src_rect(tile_id);
	SDL_Rect dst{ x * tile_size, y * tile_size, tile_size, tile_size };
	SDL_RenderCopy(renderer, atlas, &src, &dst);
}