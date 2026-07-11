#pragma once
#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include "tile.hpp"
#include "utils.hpp"

/****************************************************************
*	Emergent wind. No scripted gusts: each column gets a
*	pressure proxy from its air temperature and humidity (warm,
*	moist air = low pressure), and two horizontal wind bands —
*	surface and aloft — accelerate down the pressure gradient.
*	The aloft band relaxes toward the opposite of the surface
*	flow, forming a convection cell with return flow.
*
*	What this produces without scripting:
*	  - lake breeze by day / land breeze by night (the lake's
*	    thermal inertia keeps it out of phase with land)
*	  - upslope mountain wind at sunrise
*	  - storm outflow gusts (cold downdraft splash, injected by
*	    Garden when a cluster rains) that decay over seconds
*	  - a small constant prevailing drift ("planetary rotation")
*	    so weather circumnavigates the cylinder world
*
*	The wind field advects humidity horizontally, with the same
*	air-only / 100-cap / conservation guards as diffusion.
****************************************************************/
class Wind_Manager {
public:
	Wind_Manager(std::vector<std::vector<Tile>>& world_ref) : world_reference(world_ref), rng((std::random_device())()) {
		grid_w = static_cast<int>(world_reference.size());
		grid_h = grid_w > 0 ? static_cast<int>(world_reference.front().size()) : 0;
		wind_surface.assign(grid_w, 0.0f);
		wind_aloft.assign(grid_w, 0.0f);
		gust.assign(grid_w, 0.0f);
		pressure_surface.assign(grid_w, 0.0f);
		pressure_aloft.assign(grid_w, 0.0f);
		scratch.assign(grid_w, 0.0f);
	}
	~Wind_Manager() = default;

	void update(float delta) {
		compute_pressures();
		smooth(pressure_surface);
		smooth(pressure_aloft);

		for (int x = 0; x < grid_w; x++) {
			const int xl = (x + grid_w - 1) % grid_w;
			const int xr = (x + 1) % grid_w;
			// accelerate toward low pressure (toward warm/moist columns)
			wind_surface[x] += (pressure_surface[xl] - pressure_surface[xr]) * Zen::WIND_COUPLING * delta;
			wind_aloft[x] += (pressure_aloft[xl] - pressure_aloft[xr]) * (Zen::WIND_COUPLING * 0.5f) * delta;
			// return flow: aloft branch of the convection cell
			wind_aloft[x] += ((-0.5f * wind_surface[x]) - wind_aloft[x]) * 0.1f * delta;
			// friction
			wind_surface[x] -= wind_surface[x] * Zen::WIND_DRAG * delta;
			wind_aloft[x] -= wind_aloft[x] * (Zen::WIND_DRAG * 0.5f) * delta;
			// storm outflow decays over ~6 seconds
			gust[x] -= gust[x] * std::min(1.0f, delta / 6.0f);

			wind_surface[x] = std::clamp(wind_surface[x], -Zen::WIND_MAX, Zen::WIND_MAX);
			wind_aloft[x] = std::clamp(wind_aloft[x], -Zen::WIND_MAX, Zen::WIND_MAX);
		}

		advect_humidity(delta);
	}

	// wind in px/s at a tile position (gusts + prevailing drift included)
	float wind_at(int tile_x, int tile_y) const {
		const int x = ((tile_x % grid_w) + grid_w) % grid_w;
		const float band = (tile_y <= Zen::CLOUD_BASE_ROW) ? wind_aloft[x] : wind_surface[x];
		return band + gust[x] + Zen::WIND_PREVAILING;
	}

	// cold downdraft splash from a raining storm: pushes surface air outward
	// from the storm's center. called by Garden once per sim tick per storm.
	void add_outflow(float center_tile_x, float radius_tiles, float delta) {
		const int reach = std::max(2, static_cast<int>(radius_tiles * 3.0f));
		const int cx = static_cast<int>(center_tile_x);
		for (int dx = -reach; dx <= reach; dx++) {
			if (dx == 0) continue;
			const int x = ((cx + dx) % grid_w + grid_w) % grid_w;
			const float falloff = 1.0f - std::abs(dx) / static_cast<float>(reach + 1);
			const float dir = dx > 0 ? 1.0f : -1.0f;
			gust[x] += dir * Zen::OUTFLOW_STRENGTH * falloff * delta;
		}
	}

	// faint debug streaks showing wind speed/direction in each band
	void render(SDL_Renderer* renderer, SDL_Rect& camera) {
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 70);
		const int aloft_y = (Zen::CLOUD_TOP_ROW + 10) * Zen::TILE_SIZE - camera.y;
		const int surface_y = (Zen::CLOUD_BASE_ROW + 30) * Zen::TILE_SIZE - camera.y;
		for (int x = 0; x < grid_w; x += 25) {
			const int sx = x * Zen::TILE_SIZE - camera.x;
			if (sx < 0 || sx >= camera.w) continue;
			const int aloft_len = static_cast<int>(wind_at(x, Zen::CLOUD_TOP_ROW) * 0.25f);
			const int surface_len = static_cast<int>(wind_at(x, grid_h - 1) * 0.25f);
			if (aloft_y >= 0 && aloft_y < camera.h) SDL_RenderDrawLine(renderer, sx, aloft_y, sx + aloft_len, aloft_y);
			if (surface_y >= 0 && surface_y < camera.h) SDL_RenderDrawLine(renderer, sx, surface_y, sx + surface_len, surface_y);
		}
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
	}

private:
	std::vector<std::vector<Tile>>& world_reference;
	int grid_w;
	int grid_h;
	std::vector<float> wind_surface;    // px/s per column, near-terrain band
	std::vector<float> wind_aloft;      // px/s per column, cloud-deck band
	std::vector<float> gust;            // decaying storm-outflow component
	std::vector<float> pressure_surface;
	std::vector<float> pressure_aloft;
	std::vector<float> scratch;
	std::mt19937 rng;

	// pressure proxy: negative of (temperature + humidity bonus), averaged
	// over the band's air tiles. moist air really is lighter than dry air.
	void compute_pressures() {
		for (int x = 0; x < grid_w; x++) {
			float sum_aloft = 0.0f;
			int n_aloft = 0;
			for (int y = Zen::CLOUD_TOP_ROW; y <= Zen::CLOUD_BASE_ROW && y < grid_h; y++) {
				const Tile& t = world_reference.at(x).at(y);
				if (!Zen::is_air(t)) break;
				sum_aloft += t.temperature + 0.05f * t.humidity;
				n_aloft++;
			}
			pressure_aloft[x] = n_aloft > 0 ? -(sum_aloft / n_aloft) : pressure_aloft[x];

			float sum_surface = 0.0f;
			int n_surface = 0;
			for (int y = Zen::CLOUD_BASE_ROW + 1; y < grid_h; y++) {
				const Tile& t = world_reference.at(x).at(y);
				if (!Zen::is_air(t)) break; // stop at terrain: this band hugs the ground
				sum_surface += t.temperature + 0.05f * t.humidity;
				n_surface++;
			}
			pressure_surface[x] = n_surface > 0 ? -(sum_surface / n_surface) : pressure_surface[x];
		}
	}

	// wrapping box blur so wind responds to regional gradients, not tile noise
	void smooth(std::vector<float>& field) {
		const int radius = 4;
		for (int pass = 0; pass < 2; pass++) {
			for (int x = 0; x < grid_w; x++) {
				float sum = 0.0f;
				for (int dx = -radius; dx <= radius; dx++) {
					sum += field[((x + dx) % grid_w + grid_w) % grid_w];
				}
				scratch[x] = sum / (2 * radius + 1);
			}
			field.swap(scratch);
		}
	}

	/****************************************************************
	*	Move humidity downwind. Sweep direction opposes the flow
	*	so one unit can't chain across several columns in a single
	*	tick. Same guards as diffusion: air-to-air only, respects
	*	the 100 cap, stochastic rounding, conserves exactly.
	****************************************************************/
	void advect_humidity(float delta) {
		std::uniform_real_distribution<float> unit(0.0f, 1.0f);

		// pass 1 ascending: columns whose wind blows left (-x)
		for (int x = 0; x < grid_w; x++) advect_column(x, delta, false, unit);
		// pass 2 descending: columns whose wind blows right (+x)
		for (int x = grid_w - 1; x >= 0; x--) advect_column(x, delta, true, unit);
	}

	void advect_column(int x, float delta, bool rightward, std::uniform_real_distribution<float>& unit) {
		for (int y = 0; y < grid_h; y++) {
			Tile& self = world_reference.at(x).at(y);
			if (!Zen::is_air(self) || self.saturation > 0 || self.humidity == 0) continue;

			const float u = wind_at(x, y);
			if (rightward != (u > 0.0f)) continue;

			const int target_x = ((x + (u > 0.0f ? 1 : -1)) % grid_w + grid_w) % grid_w;
			Tile& target = world_reference.at(target_x).at(y);
			if (!Zen::is_air(target) || target.saturation > 0) continue; // mountains block wind-borne humidity

			const float p = std::clamp(std::abs(u) * delta / Zen::TILE_SIZE, 0.0f, 0.45f);
			const float amount = self.humidity * p;
			int units = static_cast<int>(amount);
			if (unit(rng) < (amount - units)) units += 1;

			units = std::min(units, static_cast<int>(self.humidity));
			units = std::min(units, Zen::HUMIDITY_MAX - static_cast<int>(target.humidity));
			if (units <= 0) continue;

			self.humidity -= units;
			target.humidity += units;
		}
	}
};
