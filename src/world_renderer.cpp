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
		t = day_pct / blend_start;
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
		flip = SDL_FLIP_HORIZONTAL;
	}
	else if (moon_phase == Moon_Phase::FIRST_QUARTER) {
		src.x = 32;
		src.y = 0;
		flip = SDL_FLIP_HORIZONTAL;
	}
	else if (moon_phase == Moon_Phase::WAXING_GIBBOUS) {
		src.x = 16;
		src.y = 0;
		flip = SDL_FLIP_HORIZONTAL;
	}
	else if (moon_phase == Moon_Phase::FULL_MOON) {
		src.x = 0;
		src.y = 0;
		flip = SDL_FLIP_NONE;
	}
	else if (moon_phase == Moon_Phase::WANING_GIBBOUS) {
		src.x = 16;
		src.y = 0;
		flip = SDL_FLIP_NONE;
	}
	else if (moon_phase == Moon_Phase::LAST_QUARTER) {
		src.x = 32;
		src.y = 0;
		flip = SDL_FLIP_NONE;
	}
	else if (moon_phase == Moon_Phase::WANING_CRESCENT) {
		src.x = 16;
		src.y = 0;
		flip = SDL_FLIP_NONE;
	}
	SDL_RenderCopyEx(renderer, texture_manager.get_texture("celestial_bodies"), &src, &dst, NULL, NULL, flip);
	SDL_SetTextureAlphaMod(texture_manager.get_texture("celestial_bodies"), 255);
}

void World_Renderer::render_stars(Time_System& time_system) {
	auto now = time_system.get_time();
}

void World_Renderer::render_clouds(int humidity, SDL_Rect dst) {
	//float current_time = SDL_GetTicks() / 1000.0f;
	//cloud_manager->update(current_time);
	//cloud_manager->draw(current_time);
	float humidity_pct = (humidity - 80) / 20.0f;
	humidity_pct = std::clamp(humidity_pct, 0.0f, 1.0f);
	Uint8 alpha = static_cast<Uint8>(humidity_pct * 200.0f);
	SDL_SetTextureAlphaMod(texture_manager.get_texture("celestial_bodies"), alpha);
	SDL_Rect src = { 0, 0, 16, 16 };
	dst.x -= 2;
	dst.y -= 2;
	dst.w += 4;
	dst.h += 4;
	SDL_RenderCopy(renderer, texture_manager.get_texture("celestial_bodies"), &src, &dst);
	SDL_SetTextureAlphaMod(texture_manager.get_texture("celestial_bodies"), 255);
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
			//temperature view
			if (*garden_debug_mode == Zen::DEBUG_MODE::TEMPERATURE) {
				SDL_Color heat_mask_color;
				SDL_Rect temp_mask{ dst.x + 1, dst.y + 1, tile_size - 2, tile_size - 2};
				heat_mask_color = get_heatmap_color(tile.temperature);
				SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
				SDL_SetRenderDrawColor(renderer, heat_mask_color.r, heat_mask_color.g, heat_mask_color.b, heat_mask_color.a);
				SDL_RenderFillRect(renderer, &temp_mask);
			}
			else if (*garden_debug_mode == Zen::DEBUG_MODE::HUMIDITY) {
				if (tile.humidity > 0) {
					SDL_Color humidity_mask_color;
					SDL_Rect humidity_mask{ dst.x + 1, dst.y + 1, tile_size - 2, tile_size - 2 };
					humidity_mask_color = get_humidity_color(tile.humidity);
					SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
					SDL_SetRenderDrawColor(renderer, humidity_mask_color.r, humidity_mask_color.g, humidity_mask_color.b, humidity_mask_color.a);
					SDL_RenderFillRect(renderer, &humidity_mask);
				}
			}
			//normal view to see water
			else if (*garden_debug_mode == Zen::DEBUG_MODE::NONE) {
				if (tile.humidity >= 80) {
					render_clouds(tile.humidity, dst);
				}
				if (tile.max_saturation > 0 && tile.saturation > 10) {
					float ratio = static_cast<float>(tile.saturation) / tile.max_saturation;
					if (ratio < 0.09f) continue;
					SDL_Rect water_level{ dst.x, dst.y + tile_size, tile_size, -tile_size * ratio };
					SDL_RenderFillRect(renderer, &water_level);
				}
			}
		}
	}
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

SDL_Color World_Renderer::get_humidity_color(int humidity) {
	float humidity_pct = std::clamp(static_cast<float>(humidity) / 100.0f, 0.0f, 1.0f);

	SDL_Color dry_color = { 255, 150, 0, 127 };
	SDL_Color mid_color = { 255, 255, 0, 127 };
	SDL_Color green_color = { 0, 255, 100, 127 };
	SDL_Color wet_color = { 0, 100, 255, 127 };

	if (humidity_pct <= 0.25f) {
		return Zen::lerp_color(dry_color, mid_color, humidity_pct * 4.0f);
	}
	else if (humidity_pct <= 0.5f) {
		return Zen::lerp_color(mid_color, green_color, (humidity_pct - 0.25f) * 2.0f);
	}
	else {
		return Zen::lerp_color(green_color, wet_color, (humidity_pct - 0.5f) * 2.0f);
	}
}

SDL_Color World_Renderer::get_heatmap_color(int temperature) {
	const int min_temp = -19;
	const int max_temp = 30;
	const int celsius = static_cast<int>((temperature - 32) * 5.0 / 9.0);
	float heat_pct = std::clamp((celsius - min_temp) / float(max_temp - min_temp), 0.0f, 1.0f);

	SDL_Color blue = { 0, 255, 255, 128 };
	SDL_Color green = { 0, 255, 0, 128 };
	SDL_Color yellow = { 255, 255, 0, 128 };
	SDL_Color orange = { 255, 165, 0, 128 };
	SDL_Color red = { 255, 0, 0, 128 };
	SDL_Color purple = { 255, 0, 255, 128 };

	if (heat_pct < 0.2f) {
		return Zen::lerp_color(blue, green, heat_pct / 0.2f);
	}
	else if (heat_pct < 0.4f) {
		return Zen::lerp_color(green, yellow, (heat_pct - 0.2f) / 0.2f);
	}
	else if (heat_pct < 0.6f) {
		return Zen::lerp_color(yellow, orange, (heat_pct - 0.4f) / 0.2f);
	}
	else if (heat_pct < 0.8f) {
		return Zen::lerp_color(orange, red, (heat_pct - 0.6f) / 0.2f);
	}
	else {
		return Zen::lerp_color(red, purple, (heat_pct - 0.8f) / 0.2f);
	}
}