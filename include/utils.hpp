#pragma once
#ifdef _DEBUG_
int drawn_tiles = 0;

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

	static SDL_Color lerp_color(SDL_Color start, SDL_Color end, float t) {
		SDL_Color new_color;
		new_color.r = static_cast<Uint8>(start.r + t * (end.r - start.r));
		new_color.g = static_cast<Uint8>(start.g + t * (end.g - start.g));
		new_color.b = static_cast<Uint8>(start.b + t * (end.b - start.b));
		new_color.a = static_cast<Uint8>(start.a + t * (end.a - start.a));
		return new_color;
	}
}