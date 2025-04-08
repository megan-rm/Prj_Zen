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

void Tile_Renderer::render(const std::vector<std::vector<Tile>>& world, SDL_Renderer* renderer, const SDL_Rect& camera) {

	for (int x = 0; x < world.size(); x++){
		for (int y = 0; y < world.at(x).size(); y++) {
			world.at(x).at(y).
		}
	}
}