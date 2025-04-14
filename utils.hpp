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

	static int mountain_start_x = 0;
	static int mountain_end_x = 0;
	static int mountain_end_y = 0; // for river formation, we only need the intercept at the end

	static int river_start_x = 0;
	static int river_end_x = 0;

	static int lake_start_x = 0;
	static int lake_end_x = 0;

	static Uint64 water_budget = 0;

	enum PIXEL_TYPE { EMPTY = 0, DIRT, CLAY, STONE };

	static SDL_Color DIRT_COLOR = { 140, 70, 20, 255 };
	static SDL_Color CLAY_COLOR = { 197, 95, 64, 255 };
	static SDL_Color STONE_COLOR = { 128, 128, 128, 255 };
	
	static SDL_Color MAGENTA_COLOR = { 255, 0, 255, 0 };

	struct Vector2D {
		int x, y;
	};
}