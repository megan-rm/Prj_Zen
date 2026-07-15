#include "world_renderer.hpp"

World_Renderer::~World_Renderer() {
	for (auto* chunk : terrain_chunks) SDL_DestroyTexture(chunk);
	if (overlay) SDL_DestroyTexture(overlay);
}

/****************************************************************
*	One-time bake: draw every tile of the (static) terrain into
*	big chunk textures so the per-frame cost is a few copies.
****************************************************************/
void World_Renderer::bake_terrain(const std::vector<std::vector<Tile>>& world) {
	const int grid_w = static_cast<int>(world.size());
	const int grid_h = grid_w > 0 ? static_cast<int>(world.front().size()) : 0;

	overlay_w = grid_w;
	overlay_h = grid_h;
	overlay = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, overlay_w, overlay_h);
	SDL_SetTextureBlendMode(overlay, SDL_BLENDMODE_BLEND);
#if SDL_VERSION_ATLEAST(2, 0, 12)
	SDL_SetTextureScaleMode(overlay, SDL_ScaleModeNearest); // crisp 8x pixels, no smearing
#endif

	const int chunk_count = (grid_w + CHUNK_TILES - 1) / CHUNK_TILES;
	for (int c = 0; c < chunk_count; c++) {
		const int w_tiles = std::min(CHUNK_TILES, grid_w - c * CHUNK_TILES);
		SDL_Texture* chunk = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET,
			w_tiles * tile_size, grid_h * tile_size);
		SDL_SetTextureBlendMode(chunk, SDL_BLENDMODE_BLEND);
		SDL_SetRenderTarget(renderer, chunk);
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderClear(renderer);
		for (int x = 0; x < w_tiles; x++) {
			for (int y = 0; y < grid_h; y++) {
				SDL_Rect src = tile_src_rect(world.at(c * CHUNK_TILES + x).at(y).img_id);
				SDL_Rect dst{ x * tile_size, y * tile_size, tile_size, tile_size };
				SDL_RenderCopy(renderer, tile_atlas, &src, &dst);
			}
		}
		terrain_chunks.push_back(chunk);
	}
	SDL_SetRenderTarget(renderer, nullptr);
	std::cout << "terrain baked into " << chunk_count << " chunk textures" << std::endl;
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
		t = day_pct / blend_start;
		alpha = static_cast<Uint8>((1.0f - t) * 255.0f);
	}
	else if (day_pct < sunrise_pct && day_pct < blend_start) {
		alpha = 255;
	}
	else if (day_pct > sunset_pct) {
		t = (day_pct - sunset_pct) / (1.0f - sunset_pct);
		alpha = static_cast<Uint8>(t * 255.0f);
	}
	else {
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
		src.x = 48; // crescent sprite (16 was the gibbous — waning crescent rendered as gibbous)
		src.y = 0;
		flip = SDL_FLIP_NONE;
	}
	SDL_RenderCopyEx(renderer, texture_manager.get_texture("celestial_bodies"), &src, &dst, NULL, NULL, flip);
	SDL_SetTextureAlphaMod(texture_manager.get_texture("celestial_bodies"), 255);
}

void World_Renderer::render_stars(Time_System& time_system) {
	auto now = time_system.get_time();
}

void World_Renderer::render_tiles(const std::vector<std::vector<Tile>>& world) {
	// --- terrain: a few chunk copies instead of ~19k per-tile draws --------
	for (size_t c = 0; c < terrain_chunks.size(); c++) {
		const int chunk_px = static_cast<int>(c) * CHUNK_TILES * tile_size;
		int chunk_w_px;
		SDL_QueryTexture(terrain_chunks[c], nullptr, nullptr, &chunk_w_px, nullptr);
		const int inter_start = std::max(camera.x, chunk_px);
		const int inter_end = std::min(camera.x + camera.w, chunk_px + chunk_w_px);
		if (inter_start >= inter_end) continue;
		SDL_Rect src{ inter_start - chunk_px, camera.y, inter_end - inter_start, camera.h };
		SDL_Rect dst{ inter_start - camera.x, 0, src.w, src.h };
		SDL_RenderCopy(renderer, terrain_chunks[c], &src, &dst);
	}

	// --- water / debug overlays: one streamed texture, scaled 8x -----------
	int tile_start_x = std::max(0, camera.x / Zen::TILE_SIZE);
	int tile_start_y = std::max(0, camera.y / Zen::TILE_SIZE);
	int tile_end_x = std::min(overlay_w, (camera.x + camera.w) / Zen::TILE_SIZE + 1);
	int tile_end_y = std::min(overlay_h, (camera.y + camera.h) / Zen::TILE_SIZE + 1);
	if (tile_end_x <= tile_start_x || tile_end_y <= tile_start_y || !overlay) return;

	update_overlay(world, tile_start_x, tile_start_y, tile_end_x, tile_end_y);

	SDL_Rect src{ tile_start_x, tile_start_y, tile_end_x - tile_start_x, tile_end_y - tile_start_y };
	SDL_Rect dst{ tile_start_x * tile_size - camera.x, tile_start_y * tile_size - camera.y,
	              src.w * tile_size, src.h * tile_size };
	SDL_RenderCopy(renderer, overlay, &src, &dst);
}

/****************************************************************
*	Fill the visible region of the overlay, 1 texel per tile.
*	Normal view: standing water solid, soil saturation as an
*	alpha gradient (the water table reads as darkening ground).
*	t/h debug views reuse the same texture.
****************************************************************/
void World_Renderer::update_overlay(const std::vector<std::vector<Tile>>& world, int start_x, int start_y, int end_x, int end_y) {
	SDL_Rect lock_rect{ start_x, start_y, end_x - start_x, end_y - start_y };
	void* raw = nullptr;
	int pitch = 0;
	if (SDL_LockTexture(overlay, &lock_rect, &raw, &pitch) != 0) return;

	const Zen::DEBUG_MODE mode = *garden_debug_mode;
	for (int y = start_y; y < end_y; y++) {
		Uint8* p = static_cast<Uint8*>(raw) + static_cast<size_t>(y - start_y) * pitch;
		for (int x = start_x; x < end_x; x++, p += 4) {
			const Tile& tile = world.at(x).at(y);
			Uint8 r = 0, g = 0, b = 0, a = 0;

			if (mode == Zen::DEBUG_MODE::TEMPERATURE) {
				SDL_Color c = get_heatmap_color(tile.temperature);
				r = c.r; g = c.g; b = c.b; a = c.a;
			}
			else if (mode == Zen::DEBUG_MODE::HUMIDITY) {
				if (tile.humidity > 0) {
					SDL_Color c = get_humidity_color(tile.humidity);
					r = c.r; g = c.g; b = c.b; a = c.a;
				}
			}
			else if (tile.snow > 0) {
				// snowpack sits on top of everything: near-white, deeper = more opaque
				r = 236; g = 240; b = 250;
				a = static_cast<Uint8>(std::clamp(120 + tile.snow / 4, 0, 245));
			}
			else {
				if (Zen::is_frozen(tile)) {
					// ice: pale blue-white sheen over the water
					r = 200; g = 224; b = 235;
					a = 205;
				}
				else if (Zen::is_air(tile) && tile.saturation > 0) {
					// standing water: blue when clear, tinting green as algae builds up
					const float alg = std::clamp(tile.algae / static_cast<float>(Zen::ALGAE_MAX), 0.0f, 1.0f);
					r = static_cast<Uint8>(24 + alg * 22);
					g = static_cast<Uint8>(110 + alg * 90);
					b = static_cast<Uint8>(220 - alg * 135);
					a = 190;
				}
				else if (tile.max_saturation > 0 && tile.saturation > 10) {
					const float ratio = static_cast<float>(tile.saturation) / tile.max_saturation;
					if (ratio >= 0.09f) {
						// soil moisture: alpha gradient so the water table
						// shows as ground darkening with depth
						r = 10; g = 70; b = 190;
						a = static_cast<Uint8>(std::clamp(20.0f + 130.0f * ratio, 0.0f, 170.0f));
					}
				}
			}
			p[0] = r; p[1] = g; p[2] = b; p[3] = a;
		}
	}
	SDL_UnlockTexture(overlay);
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