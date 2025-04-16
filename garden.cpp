#include "garden.hpp"

Garden::Garden() {
	window_title = "App";
	screen_width = 640;
	screen_height = 480;
	up_key = down_key = left_key = right_key = false;

	window = SDL_CreateWindow(window_title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_SHOWN);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	running = true;

	world_renderer = new World_Renderer();
	camera.w = screen_width;
	camera.h = screen_height;
	camera.x = 0;
	camera.y = Zen::TERRAIN_HEIGHT - camera.h;
	std::ifstream file("World.zen");
	if (!file.good()) {
		Garden_Generator* garden_generator = new Garden_Generator();
		garden_generator->generate_world(renderer);
	}
	else {
		load_world();
		world_renderer->register_tile_atlas(renderer, "tilemap.png");
	}
	file.close();
}

Garden::Garden(std::string st, int sw, int sh) {
	window_title = st;
	screen_width = sw;
	screen_height = sh;
	up_key = down_key = left_key = right_key = false;
	window = SDL_CreateWindow(window_title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_SHOWN);
	
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	running = true;

	world_renderer = new World_Renderer();

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

	std::ifstream file("World.zen");
	if ( !file.good() ) {
		Garden_Generator* garden_generator = new Garden_Generator();
		garden_generator->generate_world(renderer);
	}
	else {
		load_world();
		world_renderer->register_tile_atlas(renderer, "tilemap.png");
	}
	file.close();

}

Garden::~Garden() {
	delete world_renderer;
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

void Garden::update(float delta) {
	water_system->update_saturation(delta);
}

void Garden::render(float delta) {
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer);
	world_renderer->render_tiles(world, renderer, camera);
	SDL_RenderPresent(renderer);
}

void Garden::mouse_click(int x, int y) {
	x += camera.x;
	y += camera.y;
	int tile_x = x / Zen::TILE_SIZE;
	int tile_y = y / Zen::TILE_SIZE;
	Tile& tile = world.at(tile_x).at(tile_y);
	int addition = 3000;
	int open_space = tile.max_saturation - tile.saturation;
	int amt = std::min(addition, open_space);
	tile.saturation += amt;
}

void Garden::input(float delta) {
	while (SDL_PollEvent(&events)) {
		if (events.type == SDL_QUIT) {
			running = false;
			continue;
		}
		else if (events.type == SDL_KEYDOWN)
		{
			switch (events.key.keysym.sym) {
			case SDLK_ESCAPE:
				running = false;
				break;
			case SDLK_SPACE:
				break;
			case SDLK_RIGHT:
				right_key = true;
				break;
			case SDLK_LEFT:
				left_key = true;
				break;
			case SDLK_UP:
				up_key = true;
				break;
			case SDLK_DOWN:
				down_key = true;
				break;
			}
		}
		else if (events.type == SDL_KEYUP) {
			switch (events.key.keysym.sym) {
			case SDLK_RIGHT:
				right_key = false;
				break;
			case SDLK_LEFT:
				left_key = false;
				break;
			case SDLK_UP:
				up_key = false;
				break;
			case SDLK_DOWN:
				down_key = false;
				break;
			}
		}
		else if (events.type == SDL_MOUSEBUTTONDOWN && events.button.button == SDL_BUTTON_LEFT) {
			left_mouse = true;
		}
		else if (events.type == SDL_MOUSEBUTTONUP && events.button.button == SDL_BUTTON_LEFT) {
			left_mouse = false;
		}
	}
	
	/****************************************
	*	key press handling
	*
	****************************************/
	if (up_key) {
		camera.y -= camera_speed * delta;
		camera.y = std::max(camera.y, 0);
	}
	if (down_key) {
		camera.y += camera_speed * delta;
		if (camera.y + camera.h > Zen::TERRAIN_HEIGHT) {
			camera.y = Zen::TERRAIN_HEIGHT - camera.h;
		}
	}
	if (right_key) {
		camera.x += camera_speed * delta;
		if (camera.x + camera.w > Zen::TERRAIN_WIDTH) {
			camera.x = Zen::TERRAIN_WIDTH - camera.w;
		}
	}
	if (left_key) {
		camera.x -= camera_speed * delta;
		camera.x = std::max(camera.x, 0); // seriously? I don't know about this chief.
	}
	if (left_mouse) {
		int x, y;
		SDL_GetMouseState(&x, &y);
		mouse_click(x, y);
	}
}

void Garden::run()
{
	auto last_time = SDL_GetTicks();
	const int fps = 60;
	const int frame_delay = 1000 / fps;
	water_system = new Water_System(world, 80);
	Uint64 water = water_system->place_water(0.80f);
	std::string window_title;
	while (running) {
		auto current_time = SDL_GetTicks();
		float delta = (current_time - last_time) / 1000.0f;
		last_time = current_time;
		input(delta);
		update(delta);
		render(delta);
		auto tick_time = SDL_GetTicks() - last_time;
		window_title = "Project Zen: " + std::to_string(tick_time);
		SDL_SetWindowTitle(window, window_title.c_str());
		//std::cout << tick_time << std::endl;
		SDL_Delay(16);
	}
}