#include "tile.hpp"
#include "garden.hpp"

Tile::Tile() {
	tile.x = 0;
	tile.y = 0;
	tile.w = Zen::TILE_SIZE;
	tile.h = Zen::TILE_SIZE;

	mask.x = 0;
	mask.y = 0;
	mask.w = Zen::TILE_SIZE;
	mask.h = Zen::TILE_SIZE;

	max_saturation = 0;
	saturation = 0;
	permeability = 0;
}

Tile::~Tile() {

}
