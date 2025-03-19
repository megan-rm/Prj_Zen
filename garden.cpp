#include "Garden.hpp"

Garden::Garden() {
	window_title = "App";
	screen_width = 640;
	screen_height = 480;

	window = SDL_CreateWindow(window_title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_SHOWN);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	running = true;
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
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
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

void Garden::place_mountain(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], int center, int height) {

	//start our ascent. this is mountain left side screen->right side screen, however.
	// we'll need to alter to allow random far side of screen generation
	std::random_device rd;
	std::mt19937 g(rd());
	std::uniform_real_distribution<double> r_dist(0.95, 1.05);
	std::uniform_int_distribution<int> i_dist(0, 100);

	int width = (int)(Zen::MOUNTAIN_WIDTH * r_dist(g));
	center = width * 0.3333;
	int x = 0;
	int horizontal_chance = center / Zen::MOUNTAIN_HEIGHT;
	for (int y = Zen::TERRAIN_HEIGHT; y < (Zen::TERRAIN_HEIGHT - Zen::MOUNTAIN_HEIGHT); y--) { // change screen_height => Zen::TERRAIN_HEIGHT;
		int movement = i_dist(g);
		//place all stone below as stone
		for (int i = y; i < Zen::TERRAIN_HEIGHT; i++) {
			cells[x][i] = Zen::PIXEL_TYPE::STONE;
		}
		if (movement <= horizontal_chance) {
			
		}
		else {

		}
	}
}

void Garden::generate_world() {
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
	place_terrain(cells, 130, 0.4, 0.55, 0.05); // subsoil
	place_terrain(cells, 20, 0.8, 0.15, 0.05); //topsoil
	place_mountain(cells, 200, 0);
	SDL_RenderClear(renderer);
	//render the generated terrain to the screen
	for (int x = 0; x < Zen::TERRAIN_WIDTH; x++) {
		for (int y = 0; y < Zen::TERRAIN_HEIGHT; y++)
		{
			int tx = x;
			int ty = screen_height - Zen::TERRAIN_HEIGHT + y;
			set_pixel_color(cells[x][y]);
			SDL_RenderDrawPoint(renderer, tx, ty);
		}
	}
	SDL_RenderPresent(renderer);
	delete[] cells;
	//pixels.clear();
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