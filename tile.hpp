#pragma once
#include <SDL.h>

class Tile
{
public:
	Tile();
	~Tile();
	void register_image(SDL_Texture* texture);
	void set_pos(int x, int y);
	void set_permeability(int perm);
	void set_max_saturation(int max_sat);
	void set_saturation(int sat);
	void set_tile_id(int img_id);
private:
	SDL_Rect img_src;
	SDL_Rect tile;
	SDL_Rect mask; // for darkening according to moisture
	SDL_Texture* texture;

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