#include "world_renderer.hpp"

World_Renderer::~World_Renderer() {
	SDL_DestroyTexture(tile_atlas);
}
SDL_Rect World_Renderer::tile_src_rect(int tile_id) {
	int tiles_per_row = tile_atlas_width / tile_size;
	int x = (tile_id % tiles_per_row) * tile_size;
	int y = (tile_id / tiles_per_row) * tile_size;
	return SDL_Rect{ x, y, tile_size, tile_size };
}

void World_Renderer::render_sky(Time_System& time_system) {
	auto now = time_system.get_time();
	SDL_Rect dst{ 0, -camera.y , camera.w, Zen::TERRAIN_HEIGHT };
	SDL_SetTextureBlendMode(sky_gradient, SDL_BLENDMODE_BLEND);
	SDL_SetTextureAlphaMod(sky_gradient, 225);
	if (now.hour >= 0 && now.hour < 6) {
		SDL_SetTextureAlphaMod(sky_gradient, 225);
		SDL_SetTextureColorMod(sky_gradient, 25, 25, 115);
		SDL_RenderCopy(renderer, sky_gradient, nullptr, &dst);
	}
	else if (now.hour >= 7 && now.hour < 11) {
		SDL_SetTextureAlphaMod(sky_gradient, 110);
		SDL_SetTextureColorMod(sky_gradient, 180, 220, 255);
		SDL_RenderCopy(renderer, sky_gradient, nullptr, &dst);;
	}
	else if (now.hour >= 11 && now.hour < 17) {
		SDL_SetTextureAlphaMod(sky_gradient, 225);
		SDL_SetTextureColorMod(sky_gradient, 85, 145, 255);
		SDL_RenderCopy(renderer, sky_gradient, nullptr, &dst);
	}
	else if (now.hour >= 17 && now.hour < 20) {
		SDL_SetTextureAlphaMod(sky_gradient, 180);
		SDL_SetTextureColorMod(sky_gradient, 255, 190, 80);
		SDL_RenderCopy(renderer, sky_gradient, nullptr, &dst);
	}
	else if (now.hour >= 20 && now.hour <= 23) {
		SDL_SetTextureAlphaMod(sky_gradient, 225);
		SDL_SetTextureColorMod(sky_gradient, 15, 15, 50);
		SDL_RenderCopy(renderer, sky_gradient, nullptr, &dst);
	}
}

void World_Renderer::render_sun(Time_System& time_system) {
	auto now = time_system.get_time();
	auto sun_pos = time_system.get_sun_pos(camera);
	SDL_Rect src = { 0, 0, 16, 16 };
	SDL_Rect dst = { sun_pos.x, sun_pos.y, 48, 48 };
	SDL_SetTextureColorMod(texture_manager.get_texture("celestial_bodies"), 255, 255, 0);
	SDL_RenderCopy(renderer, texture_manager.get_texture("celestial_bodies"), &src, &dst);
}

void World_Renderer::render_moon(Time_System& time_system) {
	auto now = time_system.get_time();
}

void World_Renderer::render_stars(Time_System& time_system) {
	auto now = time_system.get_time();
}

void World_Renderer::render_clouds() {

}

void World_Renderer::render_tiles(const std::vector<std::vector<Tile>>& world) {
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