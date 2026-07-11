#pragma once
#include <cmath>
#include <string>

#include "tile.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Zen {
	constexpr int TERRAIN_WIDTH = 10000;
	constexpr int TERRAIN_HEIGHT = 1200;

	constexpr int TILE_SIZE = 8;

	constexpr int MOUNTAIN_HEIGHT = 800; // 200 for terrain, 600 above terrain?
	constexpr int MOUNTAIN_WIDTH = 800; // we'll add some randomness to this? I'm unsure.

	constexpr int LAKE_WIDTH = 600;
	constexpr int LAKE_DEPTH = 120;

	//using ints below to prevent water loss with rounding errors in floats
	constexpr int DIRT_PERMIABILITY = 65; // divide by 100 aka 0.65;
	constexpr int CLAY_PERMIABILITY = 25; // divide by 100 ; aka  0.25 ; 10000 => 100.00
	constexpr int STONE_PERMIABILITY = 0;

	/****************************************************************
	*	Simulation timing: the sim advances in fixed, complete
	*	ticks. Every tile updates every tick, then the finished
	*	state is snapshotted for the renderer (double buffering).
	****************************************************************/
	constexpr float SIM_HZ = 8.0f;
	constexpr float SIM_DT = 1.0f / SIM_HZ;

	/****************************************************************
	*	Air & humidity rules:
	*	- a tile with 10000 permeability is an air tile
	*	- an air tile holds at most 100 humidity
	*	- one raindrop carries exactly 1 humidity worth of water
	****************************************************************/
	constexpr int AIR_PERMEABILITY = 10000;
	constexpr int HUMIDITY_MAX = 100;

	// Fraction of a surface tile's saturation that evaporates per second at
	// full temperature factor. THE main knob for how fast clouds build in
	// real time — halve it for slower skies, double it for stormier ones.
	// (0.0005 ≈ a lake column supplies ~5 humidity/s, land ~1/s.)
	constexpr float EVAPORATION_RATE = 0.0005f;

	/****************************************************************
	*	Cloud tuning. A tile is a "cloud tile" at/above
	*	CLOUD_TILE_MIN_HUMIDITY. Touching (4-connected) cloud tiles
	*	form a cluster; a cluster rains once it has
	*	RAIN_CLUSTER_TILES members. Buoyancy fades to zero across
	*	the band CLOUD_BASE_ROW -> CLOUD_TOP_ROW so clouds hang in
	*	a deck instead of piling on row 0.
	****************************************************************/
	constexpr int CLOUD_TILE_MIN_HUMIDITY = 60;
	constexpr int RAIN_CLUSTER_TILES = 200;
	// hysteresis: once raining, a cluster keeps raining until it shrinks below
	// this — storms dump their water and clear instead of drizzling forever
	constexpr int RAIN_STOP_TILES = 120;
	constexpr int CLOUD_TOP_ROW = 12;
	constexpr int CLOUD_BASE_ROW = 60;
	constexpr int CLOUD_CONDENSE_RATE = 2;   // humidity pulled toward a cluster's core per tick
	// convective lift: humidity/second carried straight up per column by
	// thermals, regardless of gradient (diffusion alone can never reach the
	// deck — the share deadband caps climb at ~12 tiles above the surface)
	constexpr float CLOUD_LIFT_PER_SECOND = 16.0f;
	// per-cluster cap — must be high enough that huge storms out-rain the
	// evaporation supply, or a saturated sky can never drain itself
	constexpr int RAIN_MAX_DROPS_PER_TICK = 256;
	constexpr float RAIN_GRAVITY = 380.0f;   // px/s^2
	constexpr float RAIN_MAX_FALL = 240.0f;  // px/s terminal velocity

	/****************************************************************
	*	Wind: emergent from horizontal temperature/humidity
	*	gradients (warm moist columns = low pressure). Two bands —
	*	surface and aloft — form a convection cell with return
	*	flow. Storm downdrafts inject decaying outflow gusts.
	****************************************************************/
	constexpr float WIND_COUPLING = 40.0f;   // px/s^2 of acceleration per unit of pressure gradient
	constexpr float WIND_DRAG = 0.15f;       // 1/s, friction; terminal wind ~= accel / drag
	constexpr float WIND_PREVAILING = 3.0f;  // px/s constant "planetary rotation" drift
	constexpr float WIND_MAX = 90.0f;        // px/s hard clamp
	constexpr float OUTFLOW_STRENGTH = 20.0f;// px/s/s kick from a raining storm's cold downdraft

	// surface temperature response: saturated (water) tiles lag hours behind
	// land — this is what creates day/night lake breezes emergently
	constexpr float LAND_HEAT_RATE = 0.35f;
	constexpr float WATER_HEAT_RATE = 0.03f;

	inline bool is_air(const Tile& t) {
		return t.permeability == AIR_PERMEABILITY;
	}

	inline int mountain_start_x = 0;
	inline int mountain_end_x = 0;
	inline int mountain_end_y = 0; // for river formation, we only need the intercept at the end

	inline int river_start_x = 0;
	inline int river_end_x = 0;

	inline int lake_start_x = 0;
	inline int lake_end_x = 0;

	inline Uint64 water_update_total = 0;
	inline Uint64 water_budget = 0;

	enum PIXEL_TYPE { EMPTY = 0, DIRT, CLAY, STONE };
	enum DEBUG_MODE { NONE = 0, WATER, TEMPERATURE, HUMIDITY };
	static SDL_Color DIRT_COLOR = { 140, 70, 20, 255 };
	static SDL_Color CLAY_COLOR = { 197, 95, 64, 255 };
	static SDL_Color STONE_COLOR = { 128, 128, 128, 255 };

	static SDL_Color DAWN_COLOR = { 135, 205, 235, 155 };
	static SDL_Color MIDDAY_COLOR = { 0, 190, 255, 200 };
	static SDL_Color EVENING_COLOR = { 255, 140, 0, 215 };
	static SDL_Color NIGHT_COLOR = { 5, 5, 55, 245 };

	static SDL_Color MAGENTA_COLOR = { 255, 0, 255, 0 }; // deprecated from old rendering system

	struct Vector2D {
		int x, y;
	};

	// Resolve paths relative to the executable so the game runs from any cwd
	// on mac/linux/windows. Falls back to relative paths if SDL can't tell us.
	inline const std::string& base_path() {
		static const std::string path = [] {
			char* raw = SDL_GetBasePath();
			std::string result = raw ? raw : "";
			if (raw) SDL_free(raw);
			return result;
		}();
		return path;
	}

	inline std::string data_path(const std::string& relative) {
		return base_path() + relative;
	}

	static SDL_Color lerp_color(SDL_Color start, SDL_Color end, float t) {
		SDL_Color new_color;
		new_color.r = static_cast<Uint8>(start.r + t * (end.r - start.r));
		new_color.g = static_cast<Uint8>(start.g + t * (end.g - start.g));
		new_color.b = static_cast<Uint8>(start.b + t * (end.b - start.b));
		new_color.a = static_cast<Uint8>(start.a + t * (end.a - start.a));
		return new_color;
	}
}
