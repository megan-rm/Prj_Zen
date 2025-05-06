#pragma once
#include <SDL.h>
#include "utils.hpp"

struct Tile {
	Uint16 saturation;
	Uint16 max_saturation;
	Uint16 permeability;
	Uint16 humidity; // we have to separate standing water to render, vs humidity in the air :(
	Sint8 temperature;
	int img_id; // used in tile_renderer to extract which image in the atlas
};

struct tilemap_tile {
	int id = 0;
	SDL_Surface* tile = nullptr;
};