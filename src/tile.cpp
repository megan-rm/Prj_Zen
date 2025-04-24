//#include "tile.hpp"
//#include "garden.hpp"
//
//Tile::Tile() {
//	tile.x = 0;
//	tile.y = 0;
//	tile.w = Zen::TILE_SIZE;
//	tile.h = Zen::TILE_SIZE;
//
//	mask.x = 0;
//	mask.y = 0;
//	mask.w = Zen::TILE_SIZE;
//	mask.h = Zen::TILE_SIZE;
//
//	img_src.x = 0;
//	img_src.y = 0;
//	img_src.w = Zen::TILE_SIZE;
//	img_src.h = Zen::TILE_SIZE;
//	max_saturation = 0;
//	saturation = 0;
//	permeability = 0;
//}
//
//Tile::~Tile() {
//
//}
//
//
//void Tile::set_pos(int x, int y) {
//	tile.x = x;
//	tile.y = y;
//
//	mask.x = x;
//	mask.y = y;
//}
//
//void Tile::set_permeability(int perm) {
//	permeability = perm;
//}
//
//void Tile::set_max_saturation(int max_sat) {
//	max_saturation = max_sat;
//}
//
//
//void Tile::set_saturation(int sat){
//	saturation = sat;
//}
//
//void Tile::set_tile_id(int img_id) {
//	tile_id = img_id;
//	const int img_width = 2048;
//	const int tiles_per_row = img_width / Zen::TILE_SIZE;
//	const int x_pos = img_id * 8 % tiles_per_row;
//	const int y_pos = img_id * 8 / tiles_per_row;
//	img_src.x = x_pos;
//	img_src.y = y_pos;
//
//}
//
//void Tile::register_image(SDL_Texture* tex) {
//	texture = tex;
//}