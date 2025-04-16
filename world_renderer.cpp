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
	for (int x = 0; x < world.size(); x++){
		for (int y = 0; y < world.at(x).size(); y++) {
			int global_x = x * tile_size;
			int global_y = y * tile_size;
			if (global_x + tile_size < camera.x || global_x > camera.x + camera.w ||
				global_y + tile_size < camera.y || global_y > camera.y + camera.h) {
				continue;
			}
			SDL_Rect src = tile_src_rect(world.at(x).at(y).img_id);
			SDL_Rect dst{ global_x - camera.x, global_y - camera.y, tile_size, tile_size };
			SDL_RenderCopy(renderer, tile_atlas, &src, &dst);
			if (world.at(x).at(y).max_saturation > 0 && world.at(x).at(y).saturation > 10) {
				float ratio = static_cast<float>(world.at(x).at(y).saturation) / world.at(x).at(y).max_saturation;
				if (ratio < 0.09f) continue;
				SDL_Rect water_level{ dst.x, dst.y + 8, tile_size, -tile_size* ratio };
				//SDL_SetRenderDrawColor(renderer, 0, 80, 200, 125);
				SDL_RenderFillRect(renderer, &water_level);
				/*if (world.at(x).at(y).saturation > 0 && world.at(x).at(y).saturation < world.at(x).at(y).max_saturation) {
					std::cout << "DEBUG" << std::endl;
				}*/
			}

			//SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		}
	}
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}