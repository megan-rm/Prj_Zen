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
	std::cout << "Generating world's tilemap..." << std::endl;
	std::ofstream file;
	int buffer_size = 131072;
	std::vector<char> buffer;
	buffer.resize(buffer_size);
	file.rdbuf()->pubsetbuf(&buffer[0], buffer_size);
	file.open(Zen::data_path("world_info/world.zen"));
	file << "[WORLD_PROPERTIES]" << std::endl;
	file << Zen::mountain_peak_x << "," << Zen::mountain_end_y << std::endl;
	file << "LAKES," << Zen::lakes.size() << std::endl;
	for (const auto& L : Zen::lakes) {
		file << L.start_x << "," << L.end_x << "," << L.surface_y << std::endl;
	}
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
			file << id << "," << permeability << "," << max_saturation << ",0,0,0" << "|"; // saturation, humidity, snow
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
	if (IMG_SavePNG(tilemap, Zen::data_path("assets/images/tilemap.png").c_str()) != 0) {
		std::cerr << "Failed to save tilemap.png: " << IMG_GetError() << std::endl;
	}
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
	std::cout << "Generating base terrain..." << std::endl;
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

int Garden_Generator::surface_at(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], int x) {
	if (x < 0 || x >= Zen::TERRAIN_WIDTH) return Zen::TERRAIN_HEIGHT - 1;
	for (int y = 0; y < Zen::TERRAIN_HEIGHT; y++) {
		if (cells[x][y] != Zen::PIXEL_TYPE::EMPTY) return y;
	}
	return Zen::TERRAIN_HEIGHT - 1;
}

bool Garden_Generator::place_lake(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], int center_x, int waterline_y) {
	std::uniform_real_distribution<float> vary(1.0f - Zen::LAKE_VARIATION_PCT / 100.0f,
	                                           1.0f + Zen::LAKE_VARIATION_PCT / 100.0f);
	const int width = static_cast<int>(Zen::LAKE_WIDTH * vary(rng));
	const int depth = std::max(20, static_cast<int>(Zen::LAKE_DEPTH * vary(rng)));
	int start_x = std::clamp(center_x - width / 2, 1, Zen::TERRAIN_WIDTH - 2);
	int end_x = std::clamp(center_x + width / 2, 1, Zen::TERRAIN_WIDTH - 2);
	if (end_x - start_x < 20) return false;
	center_x = (start_x + end_x) / 2;
	const int half_w = std::max(1, (end_x - start_x) / 2);

	std::cout << "Digging a lake basin..." << std::endl;
	// the water surface sits at the river's mouth (ordinary grade); the bowl is
	// dug BELOW it so the lake collects in a depression
	const int water_y = std::clamp(waterline_y, 121, Zen::TERRAIN_HEIGHT - 8);

	// BOTTOM half of an ellipse, floor deepest at center. Crucially each column
	// is dug from ITS OWN surface downward, so no higher ground is ever left
	// roofing the water — the lake is fully open to the sky.
	for (int x = start_x; x < end_x; x++) {
		if (x < 1 || x >= Zen::TERRAIN_WIDTH - 1) continue;
		const double dx = static_cast<double>(x - center_x) / static_cast<double>(half_w);
		if (std::abs(dx) > 1.0) continue;
		const int s = surface_at(cells, x);
		const int floor_y = std::min(water_y + static_cast<int>(depth * std::sqrt(1.0 - dx * dx)),
		                             Zen::TERRAIN_HEIGHT - 2);
		const int dig_top = std::max(1, std::min(s, water_y)); // open from the ground surface down
		for (int y = dig_top; y <= floor_y; y++) cells[x][y] = Zen::PIXEL_TYPE::EMPTY;
		// clay directly beneath the floor so the basin retains its water
		if (floor_y + 1 < Zen::TERRAIN_HEIGHT) cells[x][floor_y + 1] = Zen::PIXEL_TYPE::CLAY;
	}

	Zen::Lake_Region L;
	L.start_x = start_x;
	L.end_x = end_x;
	L.surface_y = water_y;
	Zen::lakes.push_back(L);
	return true;
}

int Garden_Generator::carve_river(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT], int foot_x, int foot_y, int dir, int length) {
	(void)foot_y;
	const int end_x = foot_x + dir * length;
	if (end_x < Zen::MOUNTAIN_EDGE_MARGIN || end_x >= Zen::TERRAIN_WIDTH - Zen::MOUNTAIN_EDGE_MARGIN) return -1;
	std::cout << "Carving a riverbed with a pond at the mountain base..." << std::endl;

	// elevation profile along the river (larger y = lower ground):
	//   pond floor welling against the mountain -> a rim -> a small berm/mound
	//   -> long descent to ordinary grade at the lake mouth
	const int pond_w = std::clamp(length / 144, 3, 5); // a tiny puddle against the mountain
	const int mound_w = std::clamp(length / 8, 30, 80);
	const int cs[4] = { 0, pond_w, pond_w + mound_w, length };
	const int cy[4] = {
		avg_surface_y + 1,   // pond floor, right against the mountain (barely a dip)
		avg_surface_y,       // pond's outer rim
		avg_surface_y - 28,  // the berm crest (slight rise)
		avg_surface_y        // lake mouth at grade
	};
	auto bed_at = [&](int step) -> int {
		if (step <= cs[0]) return cy[0];
		for (int i = 1; i < 4; i++) {
			if (step <= cs[i]) {
				const float t = static_cast<float>(step - cs[i - 1]) / static_cast<float>(std::max(1, cs[i] - cs[i - 1]));
				return cy[i - 1] + static_cast<int>((cy[i] - cy[i - 1]) * t);
			}
		}
		return cy[3];
	};

	std::uniform_int_distribution<int> mat(0, 99);
	std::uniform_int_distribution<int> jit(-3, 3);
	std::uniform_int_distribution<int> chance(0, 99);
	auto pick = [&]() { const int r = mat(rng); return (r < 55) ? Zen::PIXEL_TYPE::CLAY
	                                                : (r < 95) ? Zen::PIXEL_TYPE::STONE
	                                                           : Zen::PIXEL_TYPE::DIRT; };

	for (int step = 0; step <= length; step++) {
		const int x = foot_x + dir * step;
		if (x < 2 || x >= Zen::TERRAIN_WIDTH - 2) break;
		const int jitter = (step <= cs[1]) ? 0 : jit(rng); // keep the tiny well crisp; roughen the rest
		const int bed_y = std::clamp(bed_at(step) + jitter, 1, Zen::TERRAIN_HEIGHT - 3);
		const int s = surface_at(cells, x);
		if (s > bed_y) { // build the ground up (pond rim, berm, foothill toward mountain)
			for (int y = bed_y; y < s && y < Zen::TERRAIN_HEIGHT; y++) cells[x][y] = pick();
		} else if (s < bed_y) { // cut terrain that stands above the bed line
			for (int y = s; y < bed_y; y++) cells[x][y] = Zen::PIXEL_TYPE::EMPTY;
		}
		// thick, rough clay-and-rock bed lining (2-3 tiles)
		const int thick = 2 + (chance(rng) < 40 ? 1 : 0);
		for (int y = bed_y; y <= bed_y + thick && y < Zen::TERRAIN_HEIGHT; y++) cells[x][y] = pick();
		// peppered bumps poking up out of the bed
		if (chance(rng) < 22 && bed_y - 1 >= 1) cells[x][bed_y - 1] = pick();
	}

	// the small pond welling up against the mountain (fills with water)
	{
		Zen::Lake_Region pond;
		pond.start_x = std::min(foot_x, foot_x + dir * pond_w);
		pond.end_x = std::max(foot_x, foot_x + dir * pond_w);
		pond.surface_y = avg_surface_y;
		if (pond.end_x - pond.start_x > 4) Zen::lakes.push_back(pond);
	}

	river_mouth_y = avg_surface_y; // the low point sits at ordinary grade
	Zen::river_start_x = std::min(foot_x, end_x);
	Zen::river_end_x = std::max(foot_x, end_x);
	return std::clamp(end_x, Zen::MOUNTAIN_EDGE_MARGIN, Zen::TERRAIN_WIDTH - Zen::MOUNTAIN_EDGE_MARGIN - 1);
}

bool Garden_Generator::place_mountain(Zen::PIXEL_TYPE cells[][Zen::TERRAIN_HEIGHT]) {
	std::cout << "Raising a mountain..." << std::endl;
	std::uniform_real_distribution<double> r_dist(0.9, 1.1);
	std::uniform_int_distribution<int> noise(-6, 6);

	const int width = static_cast<int>(Zen::MOUNTAIN_WIDTH * r_dist(rng));
	const int half = std::max(1, width / 2);

	// random peak, clamped so the whole massif fits inside the world edges
	int lo = Zen::MOUNTAIN_EDGE_MARGIN + half;
	int hi = Zen::TERRAIN_WIDTH - Zen::MOUNTAIN_EDGE_MARGIN - half;
	if (hi < lo) lo = hi = Zen::TERRAIN_WIDTH / 2;
	std::uniform_int_distribution<int> px(lo, hi);
	peak_x = px(rng);

	// symmetric massif with a SLIGHTLY flattened summit (~10 tiles across) then
	// slopes down to the feet; small noise keeps the ridge from being razor-perfect
	const int flat_half = 5 * Zen::TILE_SIZE; // half of the ~10-tile flat top
	for (int d = -half; d <= half; d++) {
		const int x = peak_x + d;
		if (x < 1 || x >= Zen::TERRAIN_WIDTH - 1) continue;
		const int ad = std::abs(d);
		float frac;
		if (ad <= flat_half || half <= flat_half) frac = 1.0f; // flat plateau at full height
		else frac = 1.0f - static_cast<float>(ad - flat_half) / static_cast<float>(half - flat_half);
		frac = std::clamp(frac, 0.0f, 1.0f);
		int top = static_cast<int>(Zen::TERRAIN_HEIGHT - Zen::MOUNTAIN_HEIGHT * frac) + noise(rng);
		top = std::clamp(top, 1, Zen::TERRAIN_HEIGHT - 1);
		for (int i = top; i < Zen::TERRAIN_HEIGHT; i++) cells[x][i] = Zen::PIXEL_TYPE::STONE;
	}

	// feet sit just outside the base, on the surrounding terrain surface
	left_foot_x = std::clamp(peak_x - half - 1, 1, Zen::TERRAIN_WIDTH - 2);
	right_foot_x = std::clamp(peak_x + half + 1, 1, Zen::TERRAIN_WIDTH - 2);
	left_foot_y = surface_at(cells, left_foot_x);
	right_foot_y = surface_at(cells, right_foot_x);

	Zen::mountain_peak_x = peak_x;
	Zen::mountain_end_x = right_foot_x;
	Zen::mountain_end_y = right_foot_y;
	return true;
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
	Zen::lakes.clear();
	if (!place_mountain(cells)) {
		return false;
	}

	// rough average surface of the ordinary (non-mountain) terrain — the datum
	// the rivers descend to and the lakes fill to
	{
		long sum = 0; int n = 0;
		for (int x = Zen::MOUNTAIN_EDGE_MARGIN; x < Zen::TERRAIN_WIDTH - Zen::MOUNTAIN_EDGE_MARGIN; x += 50) {
			if (x >= left_foot_x - 60 && x <= right_foot_x + 60) continue; // skip the massif
			sum += surface_at(cells, x); n++;
		}
		avg_surface_y = n > 0 ? static_cast<int>(sum / n) : (Zen::TERRAIN_HEIGHT - 120);
	}

	// TRUE mountain feet: scan out from the peak to where the slope descends
	// back to grade. The earlier feet were the triangle's buried base corners
	// (out at the world floor), which left the river starting on flat ground
	// with a gap. This makes the river begin flush against the mountain.
	{
		const int grade = avg_surface_y - 4;
		right_foot_x = std::min(peak_x + 1, Zen::TERRAIN_WIDTH - 2);
		for (int x = peak_x; x < Zen::TERRAIN_WIDTH - Zen::MOUNTAIN_EDGE_MARGIN; x++) {
			if (surface_at(cells, x) >= grade) { right_foot_x = x; break; }
		}
		left_foot_x = std::max(peak_x - 1, 1);
		for (int x = peak_x; x > Zen::MOUNTAIN_EDGE_MARGIN; x--) {
			if (surface_at(cells, x) >= grade) { left_foot_x = x; break; }
		}
		right_foot_y = surface_at(cells, right_foot_x);
		left_foot_y = surface_at(cells, left_foot_x);
	}

	// a river descends each flank that has room for a river + lake; a mountain
	// jammed against an edge only sheds to the open side. Length varies +/- pct.
	std::uniform_real_distribution<float> rlen(1.0f - Zen::RIVER_VARIATION_PCT / 100.0f,
	                                           1.0f + Zen::RIVER_VARIATION_PCT / 100.0f);
	// a river ends AT the lake's near shore (offset by ~half a lake width, minus
	// a small overlap so the two connect) — it must not run across the open water
	const int shore_offset = Zen::LAKE_WIDTH / 2 - 50;
	// RIGHT flank
	{
		const int room = (Zen::TERRAIN_WIDTH - Zen::MOUNTAIN_EDGE_MARGIN - Zen::LAKE_WIDTH) - right_foot_x;
		const int len = std::min(static_cast<int>(Zen::RIVER_BASE_LENGTH * rlen(rng)), room);
		if (len > 200) {
			const int river_end = carve_river(cells, right_foot_x, right_foot_y, +1, len);
			if (river_end > 0) place_lake(cells, river_end + shore_offset, river_mouth_y);
		}
	}
	// LEFT flank — skipped when the mountain is essentially "fully left"
	{
		const int room = left_foot_x - (Zen::MOUNTAIN_EDGE_MARGIN + Zen::LAKE_WIDTH);
		const int len = std::min(static_cast<int>(Zen::RIVER_BASE_LENGTH * rlen(rng)), room);
		if (len > 200) {
			const int river_end = carve_river(cells, left_foot_x, left_foot_y, -1, len);
			if (river_end > 0) place_lake(cells, river_end - shore_offset, river_mouth_y);
		}
	}
	// guarantee at least one body of water no matter where the mountain landed
	if (Zen::lakes.empty()) {
		const int cx = std::clamp(right_foot_x + Zen::LAKE_WIDTH, Zen::LAKE_WIDTH, Zen::TERRAIN_WIDTH - Zen::LAKE_WIDTH);
		place_lake(cells, cx, surface_at(cells, cx) + 30); // modest depression
	}

	// per-tile-column ground surface in PIXELS, so plants/bugs rest exactly on
	// uneven ground instead of floating at the top of a partially-filled tile
	{
		std::ofstream sfile(Zen::data_path("world_info/surface.zen"));
		const int cols = Zen::TERRAIN_WIDTH / Zen::TILE_SIZE;
		for (int tx = 0; tx < cols; tx++) {
			int top = Zen::TERRAIN_HEIGHT - 1;
			for (int px = tx * Zen::TILE_SIZE; px < (tx + 1) * Zen::TILE_SIZE && px < Zen::TERRAIN_WIDTH; px++) {
				for (int y = 0; y < Zen::TERRAIN_HEIGHT; y++) {
					if (cells[px][y] != Zen::PIXEL_TYPE::EMPTY) { if (y < top) top = y; break; }
				}
			}
			sfile << top << "\n";
		}
	}

	if (!generate_tilemap(cells, renderer)) {
		return false;
	}

	delete[] cells;
	return true;
}