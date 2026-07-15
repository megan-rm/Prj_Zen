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

	up_key = down_key = left_key = right_key = h_key = t_key = left_mouse = false;
	running = true;
	existing_world = false;
	tick_count = 0;
	sim_accumulator = 0.0f;
	chronicle = nullptr;
	cloud_manager = nullptr;
	life_system = nullptr;
	texture_manager = nullptr;
	water_system = nullptr;
	wind_manager = nullptr;
	weather_system = nullptr;
	world_renderer = nullptr;

	window = SDL_CreateWindow(window_title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_SHOWN);	
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	SDL_RendererInfo info;
	SDL_GetRendererInfo(renderer, &info);
	printf("Renderer: %s\n", info.name);
	SDL_GL_SetSwapInterval(0); // 0 = no vsync

	debug_mode = Zen::DEBUG_MODE::NONE;
	show_hud = true;

	world.resize(Zen::TERRAIN_WIDTH / Zen::TILE_SIZE);
	for (int i = 0; i < Zen::TERRAIN_WIDTH / Zen::TILE_SIZE; i++) {
		world.at(i).resize(Zen::TERRAIN_HEIGHT / Zen::TILE_SIZE);
	}
}

Garden::~Garden() {
	delete chronicle;
	delete cloud_manager;
	delete life_system;
	delete weather_system;
	delete wind_manager;
	delete water_system;
	delete world_renderer;
	delete texture_manager;
	world.clear();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

bool Garden::save_world() {
	std::cout << "Saving world..." << std::endl;
	if (chronicle) chronicle->record_save(); // stamp the moment we stopped simulating
	if (life_system) {
		life_system->return_water_to_soil(); // world.zen alone carries the whole budget
		life_system->save_life();            // flora.zen: structure only (re-drinks on load)
	}
	std::ofstream file;
	file.open(Zen::data_path("world_info/world.zen"));
	if (file.is_open()) {
		file << "[WORLD_PROPERTIES]" << std::endl;
		file << Zen::mountain_peak_x << "," << Zen::mountain_end_y << std::endl;
		file << "LAKES," << Zen::lakes.size() << std::endl;
		for (const auto& L : Zen::lakes) {
			file << L.start_x << "," << L.end_x << "," << L.surface_y << std::endl;
		}
		file << "[WORLD_TILES]" << std::endl;
		for (int y = 0; y < world.at(0).size(); y++) {
			for (int x = 0; x < world.size(); x++) {
				const Tile& t = world.at(x).at(y);
				file << t.img_id << "," << t.permeability << "," << t.max_saturation << ","
				     << t.saturation << "," << t.humidity << "," << t.snow << "|";
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
	file.open(Zen::data_path("world_info/world.zen"));
	if (!file.is_open()) {
		return false;
	}
	while (!file.eof()) {
		std::string line;
		std::getline(file, line);
		if (line == "[WORLD_PROPERTIES]") {
			// line 1: mountain peak x, mountain foot y
			std::getline(file, line);
			std::stringstream property_stream(line);
			std::getline(property_stream, line, ',');
			try { Zen::mountain_peak_x = std::stoi(line); } catch (...) {}
			std::getline(property_stream, line, ',');
			try { Zen::mountain_end_y = std::stoi(line); } catch (...) {}
			// line 2: "LAKES,<count>" then <count> lines of start,end,surface_y
			std::getline(file, line);
			property_stream.clear();
			property_stream.str(line);
			std::string tag;
			std::getline(property_stream, tag, ',');
			int lake_count = 0;
			if (tag == "LAKES") { // tolerate pre-multi-lake save files
				std::getline(property_stream, line, ',');
				try { lake_count = std::stoi(line); } catch (...) { lake_count = 0; }
			}
			Zen::lakes.clear();
			for (int i = 0; i < lake_count; i++) {
				std::getline(file, line);
				std::stringstream ls(line);
				Zen::Lake_Region L;
				try {
					std::getline(ls, line, ','); L.start_x = std::stoi(line);
					std::getline(ls, line, ','); L.end_x = std::stoi(line);
					std::getline(ls, line, ','); L.surface_y = std::stoi(line);
					Zen::lakes.push_back(L);
				} catch (...) {}
			}
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
					std::getline(tile_stream, line, ',');
					int humidity = std::stoi(line);
					int snow = 0; // optional 6th field: tolerate worlds saved before snow existed
					if (std::getline(tile_stream, line, ',')) {
						try { snow = std::stoi(line); } catch (...) { snow = 0; }
					}
					world.at(x).at(y).img_id = tile_id;
					world.at(x).at(y).permeability = permeability;
					world.at(x).at(y).max_saturation = max_saturation;
					world.at(x).at(y).saturation = saturation;
					world.at(x).at(y).temperature = 45; // mild default; the sun cycle takes over within minutes
					world.at(x).at(y).humidity = humidity;
					world.at(x).at(y).snow = static_cast<Uint16>(snow);
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

/****************************************************************
*	Fixed-timestep sim: each tick is a COMPLETE pass over the
*	whole world (no more column staggering), and only after the
*	pass finishes do we copy into the render snapshot. The
*	renderer never sees a half-updated world.
****************************************************************/
void Garden::run_sim_tick() {
	water_system->update_saturation(Zen::SIM_DT);
	weather_system->update_temperatures(Zen::SIM_DT);
	wind_manager->update(Zen::SIM_DT);
	cloud_manager->sim_update();
	// raining storms splash cold outflow gusts across the surface wind
	for (const auto& cluster : cloud_manager->get_clusters()) {
		if (cluster.raining) wind_manager->add_outflow(cluster.cx, cluster.radius, Zen::SIM_DT);
	}
	if (tick_count % 10 == 0) weather_system->sun_temperature_update();
	life_system->update(Zen::SIM_DT);
	tick_count++;
}

void Garden::update(float delta) {
	sim_accumulator += delta;
	if (sim_accumulator > 4.0f * Zen::SIM_DT) sim_accumulator = 4.0f * Zen::SIM_DT; // don't spiral after a hitch

	while (sim_accumulator >= Zen::SIM_DT) {
		run_sim_tick();
		sim_accumulator -= Zen::SIM_DT;
	}
	cloud_manager->update_rain(delta); // raindrops animate every frame for smooth falling
	life_system->update_motion(delta); // bugs glide smoothly between sim ticks

	snapshot = world; // flip: publish the completed state for rendering
}

void Garden::render(float delta) {
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	SDL_RenderClear(renderer);
	world_renderer->render_sky(time_system);
	world_renderer->render_sun(time_system);
	world_renderer->render_moon(time_system);
	world_renderer->render_tiles(snapshot);
	life_system->render(renderer, camera);
	cloud_manager->render(renderer, texture_manager->get_texture("celestial_bodies"), camera, snapshot);
	wind_manager->render(renderer, camera);
	if (show_hud) hud.panel(renderer, 8, 8, 2, build_hud_lines());
	SDL_RenderPresent(renderer);
}

std::vector<std::string> Garden::build_hud_lines() {
	auto pad2 = [](int v) {
		std::string s = std::to_string(v);
		return s.size() < 2 ? "0" + s : s;
	};
	Time now = time_system.get_time();

	// northern-hemisphere season by month (Dec-Feb winter ... Sep-Nov autumn)
	static const char* season_by_month[] = { "WINTER","WINTER","SPRING","SPRING","SPRING","SUMMER",
	                                         "SUMMER","SUMMER","AUTUMN","AUTUMN","AUTUMN","WINTER" };
	const char* season = season_by_month[std::clamp(now.month - 1, 0, 11)];

	// sample surface temperature at the middle of the view
	int sample_x = std::clamp((camera.x + camera.w / 2) / Zen::TILE_SIZE, 0, static_cast<int>(snapshot.size()) - 1);
	int surface_temp = 0;
	for (int yy = 0; yy < static_cast<int>(snapshot.at(sample_x).size()); yy++) {
		const Tile& t = snapshot.at(sample_x).at(yy);
		if (!Zen::is_air(t) || t.saturation > 0) { surface_temp = t.temperature; break; }
	}

	int raining = 0, clouds = 0;
	for (const auto& c : cloud_manager->get_clusters()) {
		clouds++;
		if (c.raining) raining++;
	}

	std::vector<std::string> lines;
	lines.push_back(std::to_string(now.year) + "-" + pad2(now.month) + "-" + pad2(now.day)
		+ " " + pad2(now.hour) + ":" + pad2(now.minute));
	lines.push_back(std::string("SEASON: ") + season);
	lines.push_back("TEMP: " + std::to_string(surface_temp) + "F");
	lines.push_back("PLANTS: " + std::to_string(life_system->plant_count())
		+ " FISH: " + std::to_string(life_system->fish_count()));
	lines.push_back("BUGS: " + std::to_string(life_system->bug_count())
		+ " BIRDS: " + std::to_string(life_system->predator_count()));
	lines.push_back("CLOUDS: " + std::to_string(clouds)
		+ (raining > 0 ? "  RAINING" : ""));
	return lines;
}

void Garden::mouse_click(int x, int y) {
	x += camera.x;
	y += camera.y;
	int tile_x = x / Zen::TILE_SIZE;
	int tile_y = y / Zen::TILE_SIZE;
	if (tile_x >= world.size()) return;
	if (tile_y >= world.at(tile_x).size()) return;
	Tile& tile = world.at(tile_x).at(tile_y);
	std::cout << "Tile(" << tile_x << ", " << tile_y << "):" << std::endl;
	std::cout << "Permeability: " << tile.permeability << std::endl;
	std::cout << "Max Saturation: " << tile.max_saturation << std::endl;
	std::cout << "Saturation: " << tile.saturation << std::endl;
	std::cout << "Temperature: " << static_cast<int>(tile.temperature) << std::endl;
	std::cout << "Humidity: " << static_cast<int>(tile.humidity) << std::endl << "----------------------" << std::endl;
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
			case SDLK_i:
				show_hud = !show_hud; // toggle the stats readout
				break;
			case SDLK_END:
				camera.x = Zen::TERRAIN_WIDTH - camera.w;
				camera.y = Zen::TERRAIN_HEIGHT - camera.h;
				break;
			case SDLK_HOME:
				camera.x = 0;
				camera.y = Zen::TERRAIN_HEIGHT - camera.h;
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
				h_key = false;
				break;
			case SDLK_h:
				h_key = !h_key;
				t_key = false;
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
		h_key = false;
		debug_mode = Zen::DEBUG_MODE::TEMPERATURE;
	}
	else if (!t_key && !h_key) {
		debug_mode = Zen::DEBUG_MODE::NONE;
	}
	if (h_key) {
		t_key = false;
		debug_mode = Zen::DEBUG_MODE::HUMIDITY;
	}
	else if (!h_key && !t_key) {
		debug_mode = Zen::DEBUG_MODE::NONE;
	}
	//world_renderer->register_debug_mode(debug_mode);
}

void Garden::init() {
	std::filesystem::create_directories(Zen::data_path("world_info"));

	texture_manager = new Texture_Manager(renderer);
	texture_manager->load_texture("celestial_bodies");
	texture_manager->load_texture("sky_gradient");

	std::ifstream file(Zen::data_path("world_info/world.zen"));
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
	world_renderer->register_debug_mode(debug_mode);
	world_renderer->bake_terrain(world); // one-time: terrain is static at runtime

	water_system = new Water_System(world, 1); // mod 1: full pass every sim tick
	if (!existing_world) {
		Uint64 water = water_system->place_water(0.60f);
	}
	weather_system = new Weather_System(world, time_system);
	wind_manager = new Wind_Manager(world);
	cloud_manager = new Cloud_Manager(world);
	cloud_manager->register_wind(wind_manager);

	life_system = new Life_System(world);
	bool had_life = life_system->load_life(); // restore flora/fauna before offline catch-up

	// offline time: pseudo-simulate whatever happened while the app was closed
	chronicle = new Chronicle();
	long gap = chronicle->load();
	if (existing_world && gap > 0) {
		int replay_ticks = chronicle->catch_up(world, life_system, gap);
		for (int i = 0; i < replay_ticks; i++) {
			run_sim_tick();
		}
	}

	weather_system->sun_temperature_update(); // warm the surface BEFORE seeding
	if (!had_life) {
		life_system->scatter_seeds(400); // virgin world: first colonization
		life_system->scatter_fish(80);   // stock the lakes/rivers/ponds with fish
		// birds are NOT seeded here — there's no prey yet. They migrate in on
		// their own once plants mature and a bug population establishes.
	}

	// --- diagnostic: report the world's true horizontal extent -------------
	// if 'surfaced cols' is far below 'columns', terrain generated short and
	// that's why the scroll range feels tiny. Reads out in the terminal.
	{
		int leftmost = -1, rightmost = -1, surfaced = 0;
		for (int x = 0; x < static_cast<int>(world.size()); x++) {
			for (int y = 0; y < static_cast<int>(world.at(x).size()); y++) {
				const Tile& t = world.at(x).at(y);
				if (!Zen::is_air(t) || t.saturation > 0) {
					if (leftmost < 0) leftmost = x;
					rightmost = x;
					surfaced++;
					break;
				}
			}
		}
		std::cout << "[world] columns=" << world.size()
		          << " (" << world.size() * Zen::TILE_SIZE << "px wide)"
		          << ", terrain in cols " << leftmost << ".." << rightmost
		          << ", surfaced=" << surfaced
		          << ", camera max_x=" << (Zen::TERRAIN_WIDTH - camera.w)
		          << ", peak_x=" << Zen::mountain_peak_x
		          << ", lakes=" << Zen::lakes.size() << std::endl;
	}

	snapshot = world; // prime the render buffer before the first frame
}

void Garden::run()
{
	init();
	auto last_time = SDL_GetTicks();
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
		if (tick_time < frame_delay) {
			SDL_Delay(frame_delay - tick_time); // no more unsigned underflow when a frame runs long
		}
	}
	save_world();
}