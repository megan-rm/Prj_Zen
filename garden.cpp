#include "Garden.hpp"

Garden::Garden() {
	window_title = "App";
	screen_width = 640;
	screen_height = 480;

	window = SDL_CreateWindow(window_title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_SHOWN);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	running = true;

	camera.w = screen_width;
	camera.h = screen_height;
	camera.x = 0;
	camera.y = Zen::TERRAIN_HEIGHT - camera.h;
}

Garden::Garden(std::string st, int sw, int sh) {
	window_title = st;
	screen_width = sw;
	screen_height = sh;

	window = SDL_CreateWindow(window_title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_SHOWN);
	
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	running = true;

	// THIS NEED BE SET DIFFERENT
	int w = sw;
	int h = sh;
	camera.w = screen_width;
	camera.h = screen_height;
	camera.x = 0;
	camera.y = Zen::TERRAIN_HEIGHT - camera.h;

	grid.resize(w / 8);
	for (int i = 0; i < w / 8; i++) {
		grid.at(i).resize(h / 8);
	}
	for (int i = 0; i < w / 8; i++) {
		for (int j = 0; j < h / 8; j++) {
			grid.at(i).at(j).w = 8;
			grid.at(i).at(j).h = 8;
			grid.at(i).at(j).x = i * 8;
			grid.at(i).at(j).y = j * 8;
		}
	}
	generate_world();

}

Garden::~Garden() {
	grid.clear();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

void Garden::generate_tilemap(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT]) {
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, Zen::TERRAIN_WIDTH, Zen::TERRAIN_HEIGHT);
	SDL_SetRenderTarget(renderer, texture);
	SDL_RenderClear(renderer);
	std::unordered_map<std::string, SDL_Surface*> unique_tiles;
	/*for (int x = 0; x < Zen::TERRAIN_WIDTH; x += Zen::TERRAIN_WIDTH) {
		for (int y = 0; y < Zen::TERRAIN_HEIGHT; y += Zen::TERRAIN_HEIGHT) {
			std::string hash;
			SDL_Surface* tile;
			for (int tx = 0; tx < 8; tx++) {
				for (int ty = 0; ty < 8; ty++) {
					hash += cells[x * tx][y * ty];
					//tile->pixels = 
				}
			}
			//unique_tiles +=
		}
	}*/
	//render the generated terrain to the screen
	SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, Zen::TERRAIN_WIDTH, Zen::TERRAIN_HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);
	if (!surface) {
		std::cout << "couldn't create surface for tilemap..." << std::endl;
		//close SDL and free resources;
		return;
	}
	if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32, surface->pixels, surface->pitch) != 0) {
		std::cout << "couldn't read the renderer into the surface..." << std::endl;
		//close SDL and free resources;
		return;
	}
	Uint32* pixels = (Uint32*)surface->pixels;
	SDL_Color color;

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
			pixels[(y * surface->w) + x] = SDL_MapRGB(surface->format, color.r, color.g, color.b);
		}
	}
	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
	}
	//SDL_RenderPresent(renderer);
	IMG_SavePNG(surface, "test.png");
	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);
	std::cout << std::endl << SDL_GetTicks() << std::endl;
}

void Garden::place_terrain(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], int appx_height, float dirt_pct, float clay_pct, float stone_pct) {
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
			unsigned int bottom_left = std::max(x_val - 1, 0);
			unsigned int bottom_right = std::min(x_val + 1, Zen::TERRAIN_WIDTH - 1);
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
		//CLAY PIXEL PLACEMENT
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
}

void Garden::place_lake(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT]) {
	/*************************************************************************
	TODO:
		* Bleed clay into lake bed
		* Balance clay placed on top of lake bed
	**************************************************************************/
	Zen::lake_start_x = Zen::river_end_x;
	Zen::lake_end_x = Zen::lake_start_x + Zen::LAKE_WIDTH;
	//debug purposes
	Zen::lake_start_x = Zen::river_start_x + 20;
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
			if (ellipse_val <= 1.0) {
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
	float range_end = Zen::LAKE_WIDTH / 4.0f;
	std::normal_distribution<double> dist(range_start, range_end);
	std::shuffle(pixels.begin(), pixels.end(), g);
	for (auto i : pixels) {
		int x = dist(g) + Zen::lake_start_x;
		for (int y = center_y + 45; y < Zen::TERRAIN_HEIGHT; y++) {
			if (cells[x][y + 1] != Zen::PIXEL_TYPE::EMPTY && cells[x][y] == Zen::PIXEL_TYPE::EMPTY) {
				cells[x][y] = i;
				break;
			}
		}
	}
}

void Garden::place_river(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], bool direction) {
	/*************************************************************************
	TODO:
		* River goes according to mountain gen (use bool direction)
		* Bleed clay bed better
	**************************************************************************/
	int ty = Zen::mountain_end_y - 10; // this is from where we'll iterate down
	std::random_device rd;
	std::mt19937 g(rd());
	std::uniform_int_distribution<int> rand(0, 69); // Darren I swear I asked Tyler what number to randomize between, he chose this.
	std::uniform_int_distribution<int> riverbed_dist(20, 30);
	//river moving -> right
	Zen::river_end_x = Zen::river_start_x + Zen::MOUNTAIN_WIDTH + rand(g);
	Zen::lake_start_x = Zen::river_end_x;
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
	place_lake(cells);
}

void Garden::place_mountain(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], int center, int height) {
	/********************************************************************
	TODO:
		* Randomize side of screen to generate mountain on
		* Fix width generation to make it appropriate
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

	center = width;// *0.6666;
	horizontal_chance = (center * 85) / Zen::MOUNTAIN_HEIGHT;
	for (int y = (Zen::TERRAIN_HEIGHT - Zen::MOUNTAIN_HEIGHT); y < Zen::TERRAIN_HEIGHT; y++) { // change screen_height => Zen::TERRAIN_HEIGHT;
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
		if (cells[x+1][y] != Zen::PIXEL_TYPE::EMPTY && Zen::mountain_end_y == 0) {
			Zen::mountain_end_x = x + 1;
			Zen::mountain_end_y = y;
			Zen::river_start_x = Zen::mountain_end_x;
		}
	}
	place_river(cells, false);

}

void Garden::generate_world() {
	/***************************************************************
	TODO:
		* Create a RGBA texture, render to that instead
		* Convert texture to surface, break surface into 8x8 chunks
		* Hash the 8x8 chunks into a tilemap to check for dupes
		* Generate a tilemap and save tile info on:
			* Moisture
			* Saturation Point
			* Porosity
			* Temperature
			* TileID (for tilemap purposes)
	***************************************************************/

	auto cells = new Zen::PIXEL_TYPE[Zen::TERRAIN_WIDTH][Zen::TERRAIN_HEIGHT];
	for (int i = 0; i < Zen::TERRAIN_WIDTH; i++) {
		for (int j = 0; j < Zen::TERRAIN_HEIGHT; j++) {
			cells[i][j] = Zen::PIXEL_TYPE::EMPTY;
		}
	}

	//base layer stone for water retention
	for (int i = Zen::TERRAIN_HEIGHT - 8; i < Zen::TERRAIN_HEIGHT; i++) {
		for (int j = 0; j < Zen::TERRAIN_WIDTH; j++) {
			cells[j][i] = Zen::PIXEL_TYPE::STONE;
		}
	}
	place_terrain(cells, 10, 0.0, 0.0, 1.0); // bedrock bottom
	place_terrain(cells, 40, 0.3, 0.5, 0.2); // bedrock top
	place_terrain(cells, 130, 0.45, 0.52, 0.03); // subsoil
	place_terrain(cells, 20, 0.8, 0.15, 0.05); //topsoil
	place_mountain(cells, 500, 0);

	std::cout << SDL_GetTicks();
	generate_tilemap(cells);
	delete[] cells;
	update();
}

void Garden::update() {

}

void Garden::set_pixel_color(Zen::PIXEL_TYPE p)
{
	if (p == Zen::PIXEL_TYPE::DIRT) {
		SDL_SetRenderDrawColor(renderer, Zen::DIRT_COLOR.r, Zen::DIRT_COLOR.g, Zen::DIRT_COLOR.b, 255);
	}
	else if (p == Zen::PIXEL_TYPE::CLAY) {
		SDL_SetRenderDrawColor(renderer, Zen::CLAY_COLOR.r, Zen::CLAY_COLOR.g, Zen::CLAY_COLOR.b, 255);
	}
	else if (p == Zen::PIXEL_TYPE::STONE) {
		SDL_SetRenderDrawColor(renderer, Zen::STONE_COLOR.r, Zen::STONE_COLOR.g, Zen::STONE_COLOR.b, 255);
	}
	else {
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	}
}

void Garden::render() {
	/*SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 136, 140, 141, 0);
	/*for (int i = 0; i < screen_width / 8; i++) {
		SDL_RenderFillRect(renderer, &grid.at(i).at((screen_height / 8) - 1));
	}
	
	SDL_RenderPresent(renderer);*/
}

void Garden::input() {
	while (SDL_PollEvent(&events)) {
		if (events.type == SDL_QUIT) {
			running = false;
			continue;
		}
		else if (events.type = SDL_KEYDOWN)
		{
			switch (events.key.keysym.sym) {
			case SDLK_ESCAPE:
				running = false;
				break;
			case SDLK_SPACE:
				break;
			case SDLK_RIGHT:
				break;
			case SDLK_LEFT:
				break;
			}
		}
	}
}

void Garden::run()
{
	while (running) {
		input();
		update();
		render();

	}
}