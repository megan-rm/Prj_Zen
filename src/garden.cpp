#include "garden.hpp"
#include <filesystem>

Garden::Garden(std::string st, int sw, int sh) {
	window_title = st;

	screen_width = sw;
	screen_height = sh;
	
	camera.w = screen_width;
	camera.h = screen_height;
	camera.x = 0;
	camera.y = Zen::TERRAIN_HEIGHT - camera.h;

	up_key = down_key = left_key = right_key = t_key = left_mouse = false;
	running = true;
	existing_world = false;
	tick_count = 0;

	window = SDL_CreateWindow(window_title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_SHOWN);	
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	debug_mode = Zen::DEBUG_MODE::NONE;

	world.resize(Zen::TERRAIN_WIDTH / Zen::TILE_SIZE);
	for (int i = 0; i < Zen::TERRAIN_WIDTH / Zen::TILE_SIZE; i++) {
		world.at(i).resize(Zen::TERRAIN_HEIGHT / Zen::TILE_SIZE);
	}
}

Garden::~Garden() {
	delete texture_manager;
	delete water_system;
	delete world_renderer;
	world.clear();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

bool Garden::save_world() {
	std::cout << "Saving world..." << std::endl;
	std::ofstream file;
	file.open("world_info/world.zen");
	if (file.is_open()) {
		file << "[WORLD_PROPERTIES]" << std::endl;
		file << Zen::mountain_end_x << "," << Zen::mountain_end_y << std::endl;
		file << Zen::river_start_x << "," << Zen::river_end_x << std::endl;
		file << Zen::lake_start_x << "," << Zen::lake_end_x << std::endl;
		file << "[WORLD_TILES]" << std::endl;
		for (int y = 0; y < world.at(0).size(); y++) {
			for (int x = 0; x < world.size(); x++) {
				int img_id, permeability, saturation, max_saturation;
				img_id = world.at(x).at(y).img_id;
				permeability = world.at(x).at(y).permeability;
				max_saturation = world.at(x).at(y).max_saturation;
				saturation = world.at(x).at(y).saturation;
				file << img_id << "," << permeability << "," << max_saturation << "," << saturation << "|";
			}
			file << std::endl;
		}
		file.close();
		return true;
	}
	file.close();
	return false;
}

bool Garden::load_world() {
	int x = 0;
	int y = 0;
	std::ifstream file;
	file.open("world_info/world.zen");
	if (!file.is_open()) {
		return false;
	}
	while (!file.eof()) {
		std::string line;
		std::getline(file, line);
		if (line == "[WORLD_PROPERTIES]") {
			std::getline(file, line);
			std::stringstream property_stream(line);
			//MOUNTAIN LOADING
			std::getline(property_stream, line, ',');
			Zen::mountain_end_x = std::stoi(line);
			std::getline(property_stream, line, ',');
			Zen::mountain_end_y = std::stoi(line);
			//RIVER LOADING
			std::getline(file, line);
			property_stream.clear();
			property_stream.str(line);
			std::getline(property_stream, line, ',');
			Zen::river_start_x = std::stoi(line);
			std::getline(property_stream, line, ',');
			Zen::river_end_x = std::stoi(line);
			//LAKE LOADING
			std::getline(file, line);
			property_stream.clear();
			property_stream.str(line);
			std::getline(property_stream, line, ',');
			Zen::lake_start_x = std::stoi(line);
			std::getline(property_stream, line, ',');
			Zen::lake_end_x = std::stoi(line);
		}
		else if (line == "[WORLD_TILES]") {
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

					world.at(x).at(y).img_id = tile_id;
					world.at(x).at(y).permeability = permeability;
					world.at(x).at(y).max_saturation = max_saturation;
					world.at(x).at(y).saturation = saturation;
					world.at(x).at(y).temperature = -20;
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
	}
	return false;
}

void Garden::update(float delta) {
	int tick_val = tick_count % 2; // maybe used to stagger updates between systems
	water_system->update_saturation(delta);
	weather_system->update_temperatures(delta);
	if (tick_count % 10 == 0) weather_system->sun_temperature_update();
	tick_count++; // maybe to stagger updates between systems?
}

void Garden::render(float delta) {
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	SDL_RenderClear(renderer);
	world_renderer->render_sky(time_system);
	world_renderer->render_sun(time_system);
	world_renderer->render_moon(time_system);
	world_renderer->render_tiles(world);
	SDL_RenderPresent(renderer);
}

void Garden::mouse_click(int x, int y) {
	x += camera.x;
	y += camera.y;
	int tile_x = x / Zen::TILE_SIZE;
	int tile_y = y / Zen::TILE_SIZE;
	Tile& tile = world.at(tile_x).at(tile_y);
	std::cout << "Tile(" << tile_x << ", " << tile_y << "):" << std::endl;
	std::cout << "Permeability: " << tile.permeability << std::endl;
	std::cout << "Max Saturation: " << tile.max_saturation << std::endl;
	std::cout << "Saturation: " << tile.saturation << std::endl;
	std::cout << "Temperature: " << static_cast<int>(tile.temperature) << std::endl << "----------------------" << std::endl;
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
			case SDLK_t:
				t_key = !t_key;
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
		camera.x = std::max(camera.x, 0);
	}
	if (left_mouse) {
		int x, y;
		SDL_GetMouseState(&x, &y);
		mouse_click(x, y);
	}
	if (t_key) {
		debug_mode = Zen::DEBUG_MODE::TEMPERATURE;
	}
	else if (!t_key) {
		debug_mode = Zen::DEBUG_MODE::NONE;
	}
	world_renderer->register_debug_mode(debug_mode);
}

void Garden::run()
{
	texture_manager = new Texture_Manager(renderer);
	texture_manager->load_texture("celestial_bodies");
	texture_manager->load_texture("sky_gradient");
	auto last_time = SDL_GetTicks();
	std::ifstream file("world_info/world.zen");
	if (!file.good()) {
		Garden_Generator* garden_generator = new Garden_Generator();
		garden_generator->generate_world(renderer);
		texture_manager->load_texture("tilemap");
		load_world();
	}
	else {
		load_world();
		existing_world = true;
		texture_manager->load_texture("tilemap");
	}
	file.close();
	world_renderer = new World_Renderer(renderer, *texture_manager, camera);
	//FOR DEBUG PURPOSES:
	world_renderer->register_debug_mode(debug_mode);

	water_system = new Water_System(world, 80);
	weather_system = new Weather_System(world, time_system, 80);
	if (!existing_world) {
		Uint64 water = water_system->place_water(0.60f);
	}

	const int fps = 60;
	const int frame_delay = 1000 / fps;
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
		if (tick_time > 16) tick_time = 15;
		SDL_Delay(frame_delay - tick_time);
	}
	save_world();
}