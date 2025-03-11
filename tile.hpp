#pragma once


enum TILE_TYPE
{
	AIR, WATER, DIRT, CLAY, STONE
};

class Tile
{
public:
	Tile();
	~Tile();
private:
	TILE_TYPE type;
	int porosity; // higher porosity = faster water absorption% and faster evaporation%, and shares with neighbors faster?
	//stone = 0, clay = .5, dirt = 1? possibly also water saturation?
	int humidity; // 0-100%, water content. 
	int max_humidity; // = 100+porosity?
};