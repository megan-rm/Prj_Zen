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
	SDL_Color current_sky_color;
	float day_pct = time_system.get_day_pct();
	float sunrise_pct = time_system.get_sunrise_pct();
	float sunset_pct = time_system.get_sunset_pct();
	float midday_pct = time_system.get_midday_pct();
	if (day_pct < sunrise_pct) {
		float sunrise_duration_pct = 1.5f / 24.0f;
		float blend_start = sunrise_pct - sunrise_duration_pct;

		if (day_pct >= blend_start) {
			float t = (day_pct - blend_start) / (sunset_pct - blend_start);
			current_sky_color = Zen::lerp_color(Zen::NIGHT_COLOR, Zen::DAWN_COLOR, t);
			SDL_SetTextureColorMod(sky_gradient, current_sky_color.r, current_sky_color.g, current_sky_color.b);
			SDL_SetTextureAlphaMod(sky_gradient, current_sky_color.a);
		}
		else {
			current_sky_color = Zen::NIGHT_COLOR;
			SDL_SetTextureColorMod(sky_gradient, current_sky_color.r, current_sky_color.g, current_sky_color.b);
			SDL_SetTextureAlphaMod(sky_gradient, current_sky_color.a);
		}
		SDL_RenderCopy(renderer, sky_gradient, nullptr, nullptr);
	}
	else if (day_pct >= sunrise_pct && day_pct < midday_pct) {
		float t = day_pct / midday_pct;
		current_sky_color = Zen::lerp_color(Zen::DAWN_COLOR, Zen::MIDDAY_COLOR, t);
		SDL_SetTextureColorMod(sky_gradient, current_sky_color.r, current_sky_color.g, current_sky_color.b);
		SDL_SetTextureAlphaMod(sky_gradient, current_sky_color.a);
		SDL_RenderCopy(renderer, sky_gradient, nullptr, nullptr);
	}
	else if (day_pct >= midday_pct && day_pct < sunset_pct) {
		float sunset_duration_pct = 2.0f / 24.0f;
		float blend_start = sunset_pct - sunset_duration_pct;

		if (day_pct >= blend_start) {
			float t = (day_pct - blend_start) / (sunset_pct - blend_start);
			current_sky_color = Zen::lerp_color(Zen::MIDDAY_COLOR, Zen::EVENING_COLOR, t);
			SDL_SetTextureColorMod(sky_gradient, current_sky_color.r, current_sky_color.g, current_sky_color.b);
			SDL_SetTextureAlphaMod(sky_gradient, current_sky_color.a);
		}
		else {
			current_sky_color = Zen::MIDDAY_COLOR;
			SDL_SetTextureColorMod(sky_gradient, current_sky_color.r, current_sky_color.g, current_sky_color.b);
			SDL_SetTextureAlphaMod(sky_gradient, current_sky_color.a);
		}
		SDL_RenderCopy(renderer, sky_gradient, nullptr, nullptr);
	}
	else if (day_pct >= sunset_pct) {
		float t = day_pct / 1;
		current_sky_color = Zen::lerp_color(Zen::EVENING_COLOR, Zen::NIGHT_COLOR, t);
		SDL_SetTextureColorMod(sky_gradient, current_sky_color.r, current_sky_color.g, current_sky_color.b);
		SDL_SetTextureAlphaMod(sky_gradient, current_sky_color.a);
		SDL_RenderCopy(renderer, sky_gradient, nullptr, nullptr);
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
	Moon_Phase moon_phase = time_system.get_moon_phase();
	SDL_Rect src{ 0,0,16,16 };
	SDL_RendererFlip flip = SDL_FLIP_NONE;
	auto moon_pos= time_system.get_moon_pos(camera);
	SDL_Rect dst{ moon_pos.x, moon_pos.y, 48, 48 };
	SDL_SetTextureColorMod(texture_manager.get_texture("celestial_bodies"), 255, 255, 255);
	float day_pct = time_system.get_day_pct();
	float sunset_pct = time_system.get_sunset_pct();
	float sunrise_pct = time_system.get_sunrise_pct();
	float sunrise_duration_pct = 1.5f / 24.0f;
	float blend_start = sunrise_pct - sunrise_duration_pct;
	float t = 0.0f;
	Uint8 alpha = 0;
	if (day_pct < sunrise_pct && day_pct > blend_start) {
		// Midnight to sunrise: fade out
		t = day_pct / sunrise_pct;
		alpha = static_cast<Uint8>((1.0f - t) * 255.0f);
	}
	else if (day_pct < sunrise_pct && day_pct < blend_start) {
		alpha = 255;
	}
	else if (day_pct > sunset_pct) {
		// Sunset to midnight: fade in
		t = (day_pct - sunset_pct) / (1.0f - sunset_pct);
		alpha = static_cast<Uint8>(t * 255.0f);
	}
	else {
		// Daytime: invisible
		alpha = 0;
	}
	SDL_SetTextureAlphaMod(texture_manager.get_texture("celestial_bodies"), alpha);
	if (moon_phase == Moon_Phase::NEW_MOON) {
		src.x = 64;
		src.y = 0;
		flip = SDL_FLIP_NONE;
	}
	else if (moon_phase == Moon_Phase::WAXING_CRESCENT) {
		src.x = 48;
		src.y = 0;
		flip = SDL_FLIP_NONE;
	}
	else if (moon_phase == Moon_Phase::FIRST_QUARTER) {
		src.x = 32;
		src.y = 0;
		flip = SDL_FLIP_NONE;
	}
	else if (moon_phase == Moon_Phase::WAXING_GIBBOUS) {
		src.x = 16;
		src.y = 0;
		flip = SDL_FLIP_NONE;
	}
	else if (moon_phase == Moon_Phase::FULL_MOON) {
		src.x = 0;
		src.y = 0;
		flip = SDL_FLIP_NONE;
	}
	else if (moon_phase == Moon_Phase::WANING_GIBBOUS) {
		src.x = 16;
		src.y = 0;
		flip = SDL_FLIP_VERTICAL;
	}
	else if (moon_phase == Moon_Phase::LAST_QUARTER) {
		src.x = 32;
		src.y = 0;
		flip = SDL_FLIP_VERTICAL;
	}
	else if (moon_phase == Moon_Phase::WANING_CRESCENT) {
		src.x = 16;
		src.y = 0;
		flip = SDL_FLIP_VERTICAL;
	}
	SDL_RenderCopyEx(renderer, texture_manager.get_texture("celestial_bodies"), &src, &dst, NULL, NULL, flip);
	SDL_SetTextureAlphaMod(texture_manager.get_texture("celestial_bodies"), 255);
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