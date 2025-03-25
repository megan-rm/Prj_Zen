#pragma once
#include <SDL.h>

class Tile
{
public:
	Tile();
	~Tile();
private:
	SDL_Rect* tile;
	SDL_Rect* mask; // for darkening according to moisture

	int porosity; // higher porosity = faster water absorption% and faster evaporation%, and shares with neighbors faster?
	//stone = 0, clay = .5, dirt = 1? possibly also water saturation?
	int moisture; // 0-100%, water content. 
	int saturation_point; // = 100+porosity?
	int tile_id;
};