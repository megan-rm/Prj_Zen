#include "world_renderer.hpp"

World_Renderer::World_Renderer() {
	tile_atlas = nullptr;
	tile_atlas_width = 0;
	tile_atlas_height = 0;
	tile_size = 0;
}

World_Renderer::~World_Renderer() {
	SDL_DestroyTexture(tile_atlas);
}

void World_Renderer::register_tile_atlas(SDL_Renderer* renderer, std::string path) {
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
	SDL_SetRenderDrawColor(renderer, 0, 80, 200, 125);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_MUL);
	int tile_start_x = camera.x / Zen::TILE_SIZE;
	int tile_start_y = camera.y / Zen::TILE_SIZE;
	int tile_end_x = (camera.x + camera.w) / Zen::TILE_SIZE + 1;
	int tile_end_y = (camera.y + camera.h) / Zen::TILE_SIZE + 1;

	tile_start_x = std::max(0, tile_start_x);
	tile_start_y = std::max(0, tile_start_y);
	tile_end_x = std::min(Zen::TERRAIN_WIDTH/Zen::TILE_SIZE, tile_end_x);
	tile_end_y = std::min(Zen::TERRAIN_HEIGHT/Zen::TILE_SIZE, tile_end_y);

	for (int y = tile_start_y; y < tile_end_y; y++){
		for (int x = tile_start_x; x < tile_end_x; x++) {
			auto tile = world.at(x).at(y);
			SDL_Rect src = tile_src_rect(tile.img_id);
			SDL_Rect dst{ x * Zen::TILE_SIZE - camera.x,
						  y * Zen::TILE_SIZE - camera.y,
				          tile_size, tile_size
			};
			
			SDL_RenderCopy(renderer, tile_atlas, &src, &dst);
			if (tile.max_saturation > 0 && tile.saturation > 10) {
				float ratio = static_cast<float>(tile.saturation) / tile.max_saturation;
				if (ratio < 0.09f) continue;
				SDL_Rect water_level{ dst.x, dst.y + 8, tile_size, -tile_size* ratio };
				SDL_RenderFillRect(renderer, &water_level);
			}
		}
	}
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}