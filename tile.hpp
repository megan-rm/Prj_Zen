#pragma once
#include <SDL.h>

class Tile
{
public:
	Tile();
	~Tile();
private:
	SDL_Rect tile;
	SDL_Rect mask; // for darkening according to moisture

	int permeability; // higher permeability = faster water absorption% and faster evaporation%, and shares with neighbors faster?
	//stone = 0, clay = .5, dirt = 1? possibly also water saturation?
	int saturation; // 0-100%, water content. 
	int max_saturation; // = 100+porosity?
	int tile_id;
};

/******************************************
*
*	This is used only for generating
*	a tilemap for the newly created world
*
******************************************/
struct tilemap_tile {
	int id = 0;
	SDL_Surface* tile = nullptr;
};