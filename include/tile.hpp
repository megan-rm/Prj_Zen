#pragma once
#include <SDL.h>

struct Tile {
	Uint16 saturation;
	Uint16 max_saturation;
	Uint16 permeability;
	Uint16 humidity; // we have to separate standing water to render, vs humidity in the air :(
	Uint16 snow = 0;  // snow-water-equivalent resting on this surface tile; 1 unit = 1 water
	Uint8 algae = 0;  // aquatic food in a submerged water tile (fish graze it); grows from sunlight
	Sint8 temperature;
	int img_id; // used in tile_renderer to extract which image in the atlas
};

struct tilemap_tile {
	int id = 0;
	SDL_Surface* tile = nullptr;
};