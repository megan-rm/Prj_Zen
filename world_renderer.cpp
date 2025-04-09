#include "world_renderer.hpp"

World_Renderer::World_Renderer(SDL_Renderer* renderer) {
	this->renderer = renderer;

}

World_Renderer::~World_Renderer() {
	SDL_DestroyTexture(tile_atlas);
}

void World_Renderer::register_tile_atlas(std::string path) {
	tile_atlas = SDL_CreateTextureFromSurface(renderer, IMG_Load(path.c_str()));
	SDL_QueryTexture(tile_atlas, nullptr, nullptr, &tile_atlas_width, &tile_atlas_height);
	tile_size = 8;
}

SDL_Rect World_Renderer::tile_src_rect(int tile_id) {
	int tiles_per_row = tile_atlas_width / tile_size;
	int x = (tile_id % tiles_per_row) * tile_size;
	int y = (tile_id / tiles_per_row) * tile_size;
	return SDL_Rect{ x, y, tile_size, tile_size };
}

void World_Renderer::render_tiles(const std::vector<std::vector<Tile>>& world, SDL_Renderer* renderer, const SDL_Rect& camera) {

	for (int x = 0; x < world.size(); x++){
		for (int y = 0; y < world.at(x).size(); y++) {
			SDL_Rect src = tile_src_rect(world.at(x).at(y).img_id);
			SDL_Rect dst{ x * tile_size, y * tile_size, tile_size, tile_size };

			SDL_RenderCopy(renderer, tile_atlas, &src, &dst);
		}
	}
}