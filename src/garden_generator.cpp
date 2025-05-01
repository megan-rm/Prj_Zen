#include "garden_generator.hpp"

bool Garden_Generator::generate_tilemap(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], SDL_Renderer* renderer) {
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, Zen::TERRAIN_WIDTH, Zen::TERRAIN_HEIGHT);
	SDL_SetRenderTarget(renderer, texture);
	SDL_RenderClear(renderer);
	std::unordered_map<std::string, tilemap_tile*> unique_tiles;

	SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, Zen::TERRAIN_WIDTH, Zen::TERRAIN_HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);
	if (!surface) {
		std::cout << "couldn't create surface for tilemap..." << std::endl;
		return false;
	}
	if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32, surface->pixels, surface->pitch) != 0) {
		std::cout << "couldn't read the renderer into the surface..." << std::endl;
		return false;
	}
	Uint32* pixels = (Uint32*)surface->pixels;
	SDL_Color color{ 0,0,0,0 };

	if (SDL_MUSTLOCK(surface)) {
		SDL_LockSurface(surface);
	}

	for (int x = 0; x < Zen::TERRAIN_WIDTH; x++) {
		for (int y = 0; y < Zen::TERRAIN_HEIGHT; y++)
		{
			switch (cells[x][y]) {
			case Zen::PIXEL_TYPE::EMPTY:
				color = Zen::MAGENTA_COLOR;
				break;
			case Zen::PIXEL_TYPE::DIRT:
				color = Zen::DIRT_COLOR;
				break;
			case Zen::PIXEL_TYPE::CLAY:
				color = Zen::CLAY_COLOR;
				break;
			case Zen::PIXEL_TYPE::STONE:
				color = Zen::STONE_COLOR;
				break;

			}
			pixels[(y * surface->w) + x] = SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a);
		}
	}
	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}

	std::ofstream file;
	int buffer_size = 131072;
	std::vector<char> buffer;
	buffer.resize(buffer_size);
	file.rdbuf()->pubsetbuf(&buffer[0], buffer_size);
	file.open("world_info/world.zen");
	file << "[WORLD_PROPERTIES]" << std::endl;
	file << Zen::mountain_end_x << "," << Zen::mountain_end_y << std::endl;
	file << Zen::river_start_x << "," << Zen::river_end_x << std::endl;
	file << Zen::lake_start_x << "," << Zen::lake_end_x << std::endl;
	file << "[WORLD_TILES]" << std::endl;
	// generate tilemap
	for (int y = 0; y < Zen::TERRAIN_HEIGHT - 1; y += Zen::TILE_SIZE) {
		for (int x = 0; x < Zen::TERRAIN_WIDTH - 1; x += Zen::TILE_SIZE) {
			std::string hash = "";
			int permeability = 0;
			int max_saturation = 0;
			int id = 0;
			for (int tx = 0; tx < Zen::TILE_SIZE; tx++) {
				for (int ty = 0; ty < Zen::TILE_SIZE; ty++) {
					hash += '0' + cells[x + tx][y + ty];
					switch (cells[x + tx][y + ty]) {
					case Zen::PIXEL_TYPE::EMPTY:
						max_saturation += ((100.0 / 64.0) * 100);
						permeability += ((100.0 / 64.0) * 100);
						break;
					case Zen::PIXEL_TYPE::DIRT:
						permeability += Zen::DIRT_PERMIABILITY;
						max_saturation += (100 - Zen::DIRT_PERMIABILITY);
						break;
					case Zen::PIXEL_TYPE::CLAY:
						permeability += Zen::CLAY_PERMIABILITY;
						max_saturation += (100 - Zen::CLAY_PERMIABILITY);
						break;
					case Zen::PIXEL_TYPE::STONE:
						permeability += Zen::STONE_PERMIABILITY;
						max_saturation += 0;
						break;
					}
				}
			}

			if (auto i = unique_tiles.find(hash) == unique_tiles.end()) {
				tilemap_tile* new_tile = new tilemap_tile;
				new_tile->id = unique_tiles.size();
				id = unique_tiles.size();
				new_tile->tile = SDL_CreateRGBSurfaceWithFormat(0, Zen::TILE_SIZE, Zen::TILE_SIZE, 32, SDL_PIXELFORMAT_RGBA32);
				SDL_Rect src;
				src.x = x;
				src.y = y;
				src.w = Zen::TILE_SIZE;
				src.h = Zen::TILE_SIZE;
				SDL_BlitSurface(surface, &src, new_tile->tile, NULL);
				unique_tiles[hash] = new_tile;
			}
			else {
				id = unique_tiles.find(hash)->second->id;
			}
			if (max_saturation > 0) {
				max_saturation += 16;
			}
			if (permeability > 0) {
				permeability += 16;
			}
			file << id << "," << permeability << "," << max_saturation << "," << "0" << "|";
		}
		file << std::endl;
	}
	SDL_Surface* tilemap = SDL_CreateRGBSurfaceWithFormat(0, 2048, 2048, 32, SDL_PIXELFORMAT_RGBA32);
	//copy the unique tiles over to tilemap
	const int tiles_per_row = tilemap->w / Zen::TILE_SIZE;

	for (auto i : unique_tiles) {
		SDL_Rect dst;
		dst.x = (i.second->id % tiles_per_row) * Zen::TILE_SIZE;
		dst.y = (i.second->id / tiles_per_row) * Zen::TILE_SIZE;
		dst.w = Zen::TILE_SIZE;
		dst.h = Zen::TILE_SIZE;
		SDL_BlitSurface(i.second->tile, NULL, tilemap, &dst);
	}
	//SDL_RenderPresent(renderer);
	IMG_SavePNG(tilemap, "assets/images/tilemap.png");
	file.close();
	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);
	SDL_FreeSurface(tilemap);
	std::cout << std::endl << SDL_GetTicks() << std::endl;
	for (auto i : unique_tiles) {
		SDL_FreeSurface(i.second->tile);
	}
	return true;
}


bool Garden_Generator::place_terrain(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], int appx_height, float dirt_pct, float clay_pct, float stone_pct) {
	int total_pixels = Zen::TERRAIN_WIDTH * appx_height;
	int dirt_pixels = total_pixels * dirt_pct;
	int clay_pixels = total_pixels * clay_pct;
	int stone_pixels = total_pixels * stone_pct;

	std::vector<Zen::PIXEL_TYPE> pixels(Zen::TERRAIN_WIDTH * appx_height);
	std::fill_n(pixels.begin(), dirt_pixels, Zen::PIXEL_TYPE::DIRT);
	std::fill_n(pixels.begin() + dirt_pixels, clay_pixels, Zen::PIXEL_TYPE::CLAY);
	std::fill_n(pixels.begin() + dirt_pixels + clay_pixels, stone_pixels, Zen::PIXEL_TYPE::STONE);

	std::random_device rd;
	std::mt19937 g(rd());
	std::uniform_int_distribution<int> dist(0, Zen::TERRAIN_WIDTH - 1);
	std::shuffle(pixels.begin(), pixels.end(), g);

	/*****************************************
	*		PLACE PIXEL SUB ROUTINE
	*****************************************/
	int temp = 0;
	for (auto i : pixels) {
		temp++;
		int x_val = dist(g);
		int y = 0;
		//what if cells[x][0] is full? pick new x (UPPER LIMIT OF TERRAIN)
		while (cells[x_val][0] != Zen::PIXEL_TYPE::EMPTY) {
			x_val = dist(g);
		}
		while (cells[x_val][y + 1] == Zen::PIXEL_TYPE::EMPTY) {
			y++;
		}

		// DIRT PIXEL PLACEMENT
		while (i == Zen::PIXEL_TYPE::DIRT) {
			const unsigned int bottom_left = std::max(x_val - 1, 0);
			const unsigned int bottom_right = std::min(x_val + 1, Zen::TERRAIN_WIDTH - 1);
			if (cells[x_val][y + 1] == Zen::PIXEL_TYPE::EMPTY) {
				y++;
				continue;
			}
			else if (cells[bottom_left][y + 1] == Zen::PIXEL_TYPE::EMPTY) {
				x_val--;
				continue;
			}
			else if (cells[bottom_right][y + 1] == Zen::PIXEL_TYPE::EMPTY) {
				x_val++;
				continue;
			}
			else {
				cells[x_val][y] = Zen::PIXEL_TYPE::DIRT;
				break;
			}
		}
		//CLAY PIXEL PLACEMENT -- if left & right neighbors are dirt, turn them to clay - we want clay 'paddies'
		if (i == Zen::PIXEL_TYPE::CLAY) {
			if (x_val != 0 && x_val != Zen::TERRAIN_WIDTH - 1) {
				if (cells[x_val - 1][y] == Zen::PIXEL_TYPE::DIRT && cells[x_val + 1][y] == Zen::PIXEL_TYPE::DIRT) {
					cells[x_val - 1][y] = cells[x_val + 1][y] = cells[x_val][y] = Zen::PIXEL_TYPE::CLAY;
				}
			}
		}
		//STONE PIXEL PLACEMENT;
		if (i == Zen::PIXEL_TYPE::STONE) {
			cells[x_val][y] = Zen::PIXEL_TYPE::STONE;
		}

	}
	return true;
}

bool Garden_Generator::place_lake(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], bool direction) {
	/*************************************************************************
	TODO:
		* Bleed clay into lake bed
	**************************************************************************/
	Zen::lake_start_x = Zen::river_end_x - 24;
	Zen::lake_end_x = Zen::lake_start_x + Zen::LAKE_WIDTH;

	int center_x = (Zen::lake_end_x + Zen::lake_start_x) / 2;
	int center_y = 0;
	for (int y = 0; y < Zen::TERRAIN_HEIGHT; y++) {
		if (cells[center_x][y] != Zen::PIXEL_TYPE::EMPTY) {
			center_y = y - 30;
			break;
		}
	}

	for (int x = Zen::lake_start_x; x < Zen::lake_end_x; x++) {
		for (int y = center_y - Zen::LAKE_DEPTH; y < center_y + Zen::LAKE_DEPTH; y++) {
			double ellipse_val = (pow(x - center_x, 2) / pow(Zen::LAKE_WIDTH / 2, 2)) + (pow(y - center_y, 2) / pow(Zen::LAKE_DEPTH, 2));
			if (ellipse_val <= 1.0 && y > 120) {
				cells[x][y] = Zen::PIXEL_TYPE::EMPTY;
			}
		}
	}
	int clay_pixels = Zen::LAKE_WIDTH * 7;
	int stone_pixels = Zen::LAKE_WIDTH * 2;

	std::vector<Zen::PIXEL_TYPE> pixels(clay_pixels + stone_pixels);
	std::fill_n(pixels.begin(), clay_pixels, Zen::PIXEL_TYPE::CLAY);
	std::fill_n(pixels.begin() + clay_pixels, stone_pixels, Zen::PIXEL_TYPE::STONE);

	std::random_device rd;
	std::mt19937 g(rd());
	float range_start = Zen::LAKE_WIDTH / 2.0f;
	float range_standard_deviation = Zen::LAKE_WIDTH / 3.0f;
	std::normal_distribution<double> dist(range_start, range_standard_deviation);
	std::shuffle(pixels.begin(), pixels.end(), g);
	for (auto i : pixels) {
		int x = dist(g) + Zen::lake_start_x;
		for (int y = center_y + 45; y < Zen::TERRAIN_HEIGHT; y++) {
			if (cells[x][y + 1] != Zen::PIXEL_TYPE::EMPTY) {
				if (cells[x - 1][y + 1] == Zen::PIXEL_TYPE::EMPTY) {
					x--;
					continue;
				}
				else if (cells[x + 1][y + 1] == Zen::PIXEL_TYPE::EMPTY) {
					x++;
					continue;
				}
			}
			if (cells[x][y + 1] != Zen::PIXEL_TYPE::EMPTY && cells[x][y] == Zen::PIXEL_TYPE::EMPTY) {
				cells[x][y] = i;
				break;
			}
		}
	}
	return true;
}

bool Garden_Generator::place_river(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], bool direction) {
	/*************************************************************************
	TODO:
		* River goes according to mountain gen (use bool direction)
	**************************************************************************/
	int ty = Zen::mountain_end_y - 10; // this is from where we'll iterate down
	std::random_device rd;
	std::mt19937 g(rd());
	std::uniform_int_distribution<int> rand(0, 69); // Darren I swear I asked Tyler what number to randomize between, he chose this.
	std::uniform_int_distribution<int> riverbed_dist(20, 30);
	
	//river moving -> right
	Zen::river_end_x = Zen::river_start_x + Zen::MOUNTAIN_WIDTH + rand(g);
	Zen::lake_start_x = Zen::river_end_x;
	int river_length = Zen::river_end_x - Zen::river_start_x;
	int river_slope_pixels = (river_length * 0.5f) * 32;
	int clay_pixels = river_slope_pixels * 0.85f;
	int stone_pixels = river_slope_pixels * 0.15f;

	std::vector<Zen::PIXEL_TYPE> pixels(clay_pixels + stone_pixels);
	std::fill_n(pixels.begin(), clay_pixels, Zen::PIXEL_TYPE::CLAY);
	std::fill_n(pixels.begin() + clay_pixels, stone_pixels, Zen::PIXEL_TYPE::STONE);
	std::shuffle(pixels.begin(), pixels.end(), g);

	for (int x = Zen::river_start_x; x < Zen::river_end_x; x++) {
		int tr = riverbed_dist(g);
		for (int y = ty; y < (ty + tr); y++) {
			if (cells[x][y] != Zen::PIXEL_TYPE::EMPTY && cells[x][y] != Zen::PIXEL_TYPE::STONE) {
				int r = rand(g);
				if (r > 34) {
					cells[x][y] = Zen::PIXEL_TYPE::CLAY;
				}
				else if (r < 10) {
					cells[x][y] = Zen::PIXEL_TYPE::STONE;
				}
			}
		}
	}

	std::default_random_engine rng;
	for (int tx = Zen::river_start_x - (Zen::TILE_SIZE * 3); tx < Zen::river_end_x; tx += Zen::TILE_SIZE) {
		float ratio = 1.0f - static_cast<float>(tx - Zen::river_start_x) / (Zen::river_end_x - Zen::river_start_x);
		int pixel_count = ((Zen::TILE_SIZE * Zen::TILE_SIZE) * 6) * ratio;
		int stone_pixels = pixel_count * 0.10;
		int clay_pixels = pixel_count * 0.70;
		int dirt_pixels = pixel_count - clay_pixels;
		dirt_pixels -= stone_pixels;
		std::vector<Zen::PIXEL_TYPE> pixels(pixel_count);
		std::fill_n(pixels.begin(), clay_pixels, Zen::PIXEL_TYPE::CLAY);
		std::fill_n(pixels.begin() + clay_pixels, stone_pixels, Zen::PIXEL_TYPE::STONE);
		std::fill_n(pixels.begin() + clay_pixels + stone_pixels, dirt_pixels, Zen::PIXEL_TYPE::DIRT);
		std::shuffle(pixels.begin(), pixels.end(), g);
		for (auto i : pixels) {
			int x = rand(g) % Zen::TILE_SIZE;
			for (int y = Zen::TERRAIN_HEIGHT / 1.5; y < Zen::TERRAIN_HEIGHT; y++) {
				if (cells[tx+x][y + 1] != Zen::PIXEL_TYPE::EMPTY) {
					// DIRT PIXEL PLACEMENT
					while (true) {
						const unsigned int bottom_left = tx+x-1;
						const unsigned int bottom_right = tx+x+1;
						if (cells[tx+x][y + 1] == Zen::PIXEL_TYPE::EMPTY) {
							y++;
							continue;
						}
						else if (cells[bottom_left][y + 1] == Zen::PIXEL_TYPE::EMPTY) {
							x--;
							continue;
						}
						else if (cells[bottom_right][y + 1] == Zen::PIXEL_TYPE::EMPTY) {
							x++;
							continue;
						}
						else {
							cells[tx+x][y] = i;
							break;
						}
					}
					//cells[tx+x][y] = i;
					break;
				}
			}
		}
	}
	return place_lake(cells, false);
}

bool Garden_Generator::place_mountain(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], int center, int height) {
	/********************************************************************
	TODO:
		* Randomize side of screen to generate mountain on
	********************************************************************/
	std::random_device rd;
	std::mt19937 g(rd());
	std::uniform_real_distribution<double> r_dist(0.95, 1.05);
	std::uniform_int_distribution<int> i_dist(0, 100);

	int width = (int)(Zen::MOUNTAIN_WIDTH * r_dist(g));
	int x = 0;
	int horizontal_chance = (center * 100) / Zen::MOUNTAIN_HEIGHT;
	/*for (int y = Zen::TERRAIN_HEIGHT; y > (Zen::TERRAIN_HEIGHT - Zen::MOUNTAIN_HEIGHT); y--) { // change screen_height => Zen::TERRAIN_HEIGHT;
		int movement = i_dist(g);
		//place all stone below as stone
		for (int i = y; i < Zen::TERRAIN_HEIGHT; i++) {
			cells[x][i] = Zen::PIXEL_TYPE::STONE;
		}
		if (movement <= horizontal_chance) {
			x++;
		}
		else {
			y--;
		}
	}*/

	//round out the top
	for (int y = 0; y < 50; y++) {
		std::uniform_real_distribution<double> r(0.99, 1.01);
		int dy = (Zen::TERRAIN_HEIGHT - Zen::MOUNTAIN_HEIGHT) * r(g);
		for (int i = dy; i < Zen::TERRAIN_HEIGHT; i++) {
			cells[x][i] = Zen::PIXEL_TYPE::STONE;
		}
		x++;
	}

	center = width;
	horizontal_chance = (center * 85) / Zen::MOUNTAIN_HEIGHT;
	for (int y = (Zen::TERRAIN_HEIGHT - Zen::MOUNTAIN_HEIGHT); y < Zen::TERRAIN_HEIGHT; y++) {
		int movement = i_dist(g);
		//place all stone below as stone
		for (int i = y; i < Zen::TERRAIN_HEIGHT; i++) {
			cells[x][i] = Zen::PIXEL_TYPE::STONE;
		}
		if (movement <= horizontal_chance) {
			x++;
		}
		else {
			y++;
		}
		if (cells[x + 1][y] != Zen::PIXEL_TYPE::EMPTY && Zen::mountain_end_y == 0) {
			Zen::mountain_end_x = x + 1;
			Zen::mountain_end_y = y;
			Zen::river_start_x = Zen::mountain_end_x;
		}
	}
	return place_river(cells, false);

}

bool Garden_Generator::generate_world(SDL_Renderer* renderer) {
	/***************************************************************
	TODO:
	***************************************************************/

	auto cells = new Zen::PIXEL_TYPE[Zen::TERRAIN_WIDTH][Zen::TERRAIN_HEIGHT];
	for (int i = 0; i < Zen::TERRAIN_WIDTH; i++) {
		for (int j = 0; j < Zen::TERRAIN_HEIGHT; j++) {
			cells[i][j] = Zen::PIXEL_TYPE::EMPTY;
		}
	}

	//base layer stone for water retention
	for (int i = Zen::TERRAIN_HEIGHT - Zen::TILE_SIZE; i < Zen::TERRAIN_HEIGHT; i++) {
		for (int j = 0; j < Zen::TERRAIN_WIDTH; j++) {
			cells[j][i] = Zen::PIXEL_TYPE::STONE;
		}
	}
	if ( !place_terrain(cells, 10, 0.0, 0.0, 1.0) ) { // bedrock bottom
		return false;
	}
	if ( !place_terrain(cells, 40, 0.3, 0.5, 0.2) ) { // bedrock top
		return false;
	}
	if ( !place_terrain(cells, 130, 0.45, 0.52, 0.03) ) { // subsoil
		return false;
	}
	if ( !place_terrain(cells, 20, 0.8, 0.15, 0.05) ) { //topsoil
		return false;
	}
	if (!place_mountain(cells, 500, 0)) {
		return false;
	}
	if (!generate_tilemap(cells, renderer)) {
		return false;
	}
	
	delete[] cells;
}