#include "garden.hpp"

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

	Garden_Generator* garden_generator = new Garden_Generator();
	garden_generator->generate_world(renderer);

}

Garden::Garden(std::string st, int sw, int sh) {
	window_title = st;
	screen_width = sw;
	screen_height = sh;

	window = SDL_CreateWindow(window_title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_SHOWN);
	
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	running = true;

	int w = Zen::TERRAIN_WIDTH;
	int h = Zen::TERRAIN_HEIGHT;

	camera.w = screen_width;
	camera.h = screen_height;
	camera.x = 0;
	camera.y = Zen::TERRAIN_HEIGHT - camera.h;

	world.resize(w / Zen::TILE_SIZE);
	for (int i = 0; i < w / Zen::TILE_SIZE; i++) {
		world.at(i).resize(h / Zen::TILE_SIZE);
	}

	Garden_Generator* garden_generator = new Garden_Generator();
	garden_generator->generate_world(renderer);
	world_renderer = new World_Renderer(renderer);

	load_world();
	world_renderer->register_tile_atlas("tilemap.png");
}

Garden::~Garden() {
	world.clear();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

bool Garden::load_world() {
	int x = 0;
	int y = 0;
	std::ifstream file;
	file.open("world.zen");
	if (file.is_open()) {
		std::string line;
		std::getline(file, line);
		if (line == "[WORLD_TILES]") {
			std::getline(file, line);
			while (!file.eof() && line.at(0) != '[') {
				std::stringstream s_stream(line);
				while (std::getline(s_stream, line, '|')) {
					std::string value{};
					std::stringstream tile_stream(line);
					
					std::getline(tile_stream, line, ',');
					int tile_id = std::stoi(line);
					std::getline(tile_stream, line, ',');
					int permeability = std::stoi(line);
					std::getline(tile_stream, line, ',');
					int max_saturation = std::stoi(line);
					std::getline(tile_stream, line, ',');
					int saturation = std::stoi(line);

					//world.at(x).at(y).register_image(terrain_atlas);
					world.at(x).at(y).img_id = tile_id;
					world.at(x).at(y).permeability = permeability;
					world.at(x).at(y).max_saturation = max_saturation;
					world.at(x).at(y).saturation = saturation;
					//world.at(x).at(y).set_pos(x * Zen::TILE_SIZE, y * Zen::TILE_SIZE);

					x++;
					if (x >= Zen::TERRAIN_WIDTH / Zen::TILE_SIZE) {
						x = 0;
						y++;
					}
					if (y > Zen::TERRAIN_HEIGHT / Zen::TILE_SIZE) { // just a failsafe
						y = Zen::TERRAIN_HEIGHT;
					}
				}
				std::getline(file, line, '|');
			}
		}
		return true;
	}
	else {
		std::cout << "Error loading world.zen..." << std::endl;
		return false;
	}
}

void Garden::update() {

}

void Garden::render() {
	SDL_RenderClear(renderer);
	world_renderer->render_tiles(world, renderer, camera);
	SDL_RenderPresent(renderer);
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