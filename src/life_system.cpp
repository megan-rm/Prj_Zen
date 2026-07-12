#include "life_system.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace {
	struct Species_Params {
		int max_growth;
		int grow_cost;          // water units consumed per stage
		int grow_sat;           // soil saturation needed to grow
		int wilt_sat;           // below this the plant stresses
		int min_temp;           // F
		int max_temp;
		float grow_seconds;     // cooldown between stages
		float stress_limit;     // seconds of continuous stress before death
		float transpire_rate;   // probability/sec of returning 1 water unit to the air
		float seed_rate;        // probability/sec a mature plant scatters a seed
	};
	constexpr Species_Params GRASS_P{ 4, 20, 150, 60, 38, 105, 20.0f, 90.0f, 0.05f, 0.010f };
	constexpr Species_Params SHRUB_P{ 8, 35, 250, 90, 34, 110, 45.0f, 150.0f, 0.03f, 0.004f };

	const Species_Params& params_of(Plant_Species s) {
		return s == Plant_Species::GRASS ? GRASS_P : SHRUB_P;
	}

	constexpr size_t MAX_PLANTS = 2500;
	constexpr size_t MAX_BUGS = 250;
	constexpr float BUG_HUNGER_PER_SECOND = 2.0f;
	constexpr float BUG_EAT_GAIN = 40.0f;
	constexpr float BUG_BREED_ENERGY = 160.0f;
	constexpr int BUG_FREEZE_TEMP = 35;
	constexpr float BUG_SPEED = 30.0f; // px/s
}

Life_System::Life_System(std::vector<std::vector<Tile>>& world_ref)
	: world(world_ref), rng((std::random_device())()) {
	grid_w = static_cast<int>(world.size());
	grid_h = grid_w > 0 ? static_cast<int>(world.front().size()) : 0;
	occupied.assign(grid_w, 0);
	surface_row.assign(grid_w, grid_h - 1);
	for (int x = 0; x < grid_w; x++) {
		for (int y = 0; y < grid_h; y++) {
			const Tile& t = world.at(x).at(y);
			if (!Zen::is_air(t) || t.saturation > 0) {
				surface_row[x] = y;
				break;
			}
		}
	}
}

bool Life_System::try_germinate(int x, Plant_Species species) {
	if (x < 0 || x >= grid_w) return false;
	if (occupied[x] || plants.size() >= MAX_PLANTS) return false;
	const int sy = surface_row[x];
	Tile& soil = world.at(x).at(sy);
	if (Zen::is_air(soil)) return false; // no rooting in open water
	const Species_Params& p = params_of(species);
	if (soil.saturation < p.grow_sat) return false;
	if (soil.temperature < p.min_temp || soil.temperature > p.max_temp) return false;
	if (soil.saturation < p.grow_cost) return false;

	Plant plant;
	plant.x = x;
	plant.y = sy;
	plant.species = species;
	plant.growth = 1;
	// germination itself drinks one stage's worth of water
	soil.saturation -= static_cast<Uint16>(p.grow_cost);
	plant.stored_water = p.grow_cost;
	plant.grow_cooldown = p.grow_seconds;
	plants.push_back(plant);
	occupied[x] = 1;
	return true;
}

void Life_System::scatter_seeds(int count) {
	std::uniform_int_distribution<int> col(0, grid_w - 1);
	std::uniform_real_distribution<float> roll(0.0f, 1.0f);
	for (int i = 0; i < count; i++) {
		try_germinate(col(rng), roll(rng) < 0.85f ? Plant_Species::GRASS : Plant_Species::SHRUB);
	}
}

void Life_System::deposit_water(int x, int y, int units) {
	// into the soil column first, overflow becomes humidity above the surface
	for (int yy = y; yy < grid_h && units > 0; yy++) {
		Tile& t = world.at(x).at(yy);
		if (Zen::is_air(t) && t.saturation == 0) continue;
		const int room = t.max_saturation - t.saturation;
		const int put = std::min(units, room);
		if (put > 0) { t.saturation += static_cast<Uint16>(put); units -= put; }
	}
	for (int yy = y - 1; yy >= 0 && units > 0; yy--) {
		Tile& t = world.at(x).at(yy);
		if (!Zen::is_air(t) || t.saturation > 0) continue;
		const int put = std::min(units, Zen::HUMIDITY_MAX - static_cast<int>(t.humidity));
		if (put > 0) { t.humidity += static_cast<Uint16>(put); units -= put; }
	}
}

void Life_System::kill_plant(Plant& plant) {
	plant.alive = false;
	occupied[plant.x] = 0;
	deposit_water(plant.x, plant.y, plant.stored_water);
	plant.stored_water = 0;
}

void Life_System::update(float delta) {
	// natural reseeding: a couple of germination attempts per tick, so life
	// emerges whenever and WHEREVER conditions allow (spring thaw, after a
	// die-off, freshly rained-on ground) instead of one launch-time lottery
	std::uniform_int_distribution<int> col(0, grid_w - 1);
	std::uniform_real_distribution<float> roll(0.0f, 1.0f);
	for (int i = 0; i < 2; i++) {
		try_germinate(col(rng), roll(rng) < 0.85f ? Plant_Species::GRASS : Plant_Species::SHRUB);
	}

	update_plants(delta);
	update_bugs(delta);

	// compact the dead
	plants.erase(std::remove_if(plants.begin(), plants.end(), [](const Plant& p) { return !p.alive; }), plants.end());
	bugs.erase(std::remove_if(bugs.begin(), bugs.end(), [](const Bug& b) { return !b.alive; }), bugs.end());
}

void Life_System::update_plants(float delta) {
	std::uniform_real_distribution<float> roll(0.0f, 1.0f);
	std::uniform_int_distribution<int> spread(-25, 25);

	for (auto& plant : plants) {
		if (!plant.alive) continue;
		Tile& soil = world.at(plant.x).at(plant.y);
		const Species_Params& p = params_of(plant.species);

		// stress from cold, heat, or drought; recovery when comfortable
		if (soil.temperature < p.min_temp || soil.temperature > p.max_temp) plant.stress += delta;
		else if (soil.saturation < p.wilt_sat) plant.stress += delta * 0.5f;
		else plant.stress = std::max(0.0f, plant.stress - delta);
		if (plant.stress > p.stress_limit) {
			kill_plant(plant);
			continue;
		}

		// growth: move water from soil into biomass
		plant.grow_cooldown -= delta;
		if (plant.growth < p.max_growth && plant.grow_cooldown <= 0.0f &&
			soil.saturation >= p.grow_sat && soil.saturation >= p.grow_cost) {
			soil.saturation -= static_cast<Uint16>(p.grow_cost);
			plant.stored_water += p.grow_cost;
			plant.growth++;
			plant.grow_cooldown = p.grow_seconds;
		}

		// transpiration: biomass water returns to the air — plants feed clouds
		if (plant.stored_water > plant.growth && roll(rng) < p.transpire_rate * delta) {
			if (plant.y > 0) {
				Tile& air = world.at(plant.x).at(plant.y - 1);
				if (Zen::is_air(air) && air.saturation == 0 && air.humidity < Zen::HUMIDITY_MAX) {
					plant.stored_water -= 1;
					air.humidity += 1;
				}
			}
		}

		// mature plants scatter seeds nearby
		if (plant.growth == p.max_growth && roll(rng) < p.seed_rate * delta) {
			try_germinate(plant.x + spread(rng), plant.species);
		}

		// mature plants occasionally attract a bug into existence
		if (plant.growth == p.max_growth && bugs.size() < MAX_BUGS && roll(rng) < 0.002f * delta * 60.0f) {
			Bug bug;
			bug.x = plant.x * Zen::TILE_SIZE + 4.0f;
			bug.y = (plant.y - 2) * Zen::TILE_SIZE;
			bugs.push_back(bug);
		}
	}
}

void Life_System::update_bugs(float delta) {
	std::uniform_real_distribution<float> roll(-1.0f, 1.0f);

	for (auto& bug : bugs) {
		if (!bug.alive) continue;
		bug.energy -= BUG_HUNGER_PER_SECOND * delta;
		bug.eat_cooldown -= delta;

		const int tx = std::clamp(static_cast<int>(bug.x) / Zen::TILE_SIZE, 0, grid_w - 1);
		const int sy = surface_row[tx];
		if (world.at(tx).at(sy).temperature < BUG_FREEZE_TEMP || bug.energy <= 0.0f) {
			bug.alive = false;
			continue;
		}

		// hungry: steer toward the nearest edible plant; otherwise wander
		Plant* meal = nullptr;
		if (bug.energy < 80.0f) {
			float best = 100.0f * 100.0f; // search radius^2 in px
			for (auto& plant : plants) {
				if (!plant.alive || plant.growth <= 1) continue;
				const float dx = plant.x * Zen::TILE_SIZE - bug.x;
				const float d2 = dx * dx;
				if (d2 < best) { best = d2; meal = &plant; }
			}
		}
		if (meal) {
			const float dx = meal->x * Zen::TILE_SIZE - bug.x;
			bug.vx += (dx > 0 ? 1.0f : -1.0f) * BUG_SPEED * delta;
			if (std::abs(dx) < 10.0f && bug.eat_cooldown <= 0.0f) {
				// eat one stage: its water returns to the soil, energy to the bug
				const Species_Params& p = params_of(meal->species);
				meal->growth--;
				const int released = std::min(meal->stored_water, p.grow_cost);
				meal->stored_water -= released;
				deposit_water(meal->x, meal->y, released);
				if (meal->growth <= 0) kill_plant(*meal);
				bug.energy += BUG_EAT_GAIN;
				bug.eat_cooldown = 5.0f;
			}
		}
		else {
			bug.vx += roll(rng) * BUG_SPEED * 2.0f * delta;
		}

		// well-fed bugs split
		if (bug.energy >= BUG_BREED_ENERGY && bugs.size() < MAX_BUGS) {
			bug.energy *= 0.5f;
			Bug child = bug;
			child.vx = -bug.vx;
			bugs.push_back(child);
			break; // vector may have reallocated; finish this tick here
		}
	}
}

void Life_System::update_motion(float delta) {
	const float world_px = static_cast<float>(grid_w) * Zen::TILE_SIZE;
	for (auto& bug : bugs) {
		bug.vx = std::clamp(bug.vx * (1.0f - 1.5f * delta), -BUG_SPEED, BUG_SPEED);
		bug.x += bug.vx * delta;
		if (bug.x < 0.0f) bug.x += world_px;
		if (bug.x >= world_px) bug.x -= world_px;
		// hover a couple tiles above the local surface
		const int tx = std::clamp(static_cast<int>(bug.x) / Zen::TILE_SIZE, 0, grid_w - 1);
		const float target_y = (surface_row[tx] - 2) * Zen::TILE_SIZE;
		bug.y += (target_y - bug.y) * std::min(1.0f, 2.0f * delta);
	}
}

void Life_System::render(SDL_Renderer* renderer, const SDL_Rect& camera) {
	for (const auto& plant : plants) {
		const int sx = plant.x * Zen::TILE_SIZE - camera.x;
		if (sx < -Zen::TILE_SIZE || sx >= camera.w) continue;
		const int height = plant.growth * 2;
		const int base_y = plant.y * Zen::TILE_SIZE - camera.y;
		if (plant.species == Plant_Species::GRASS) {
			SDL_SetRenderDrawColor(renderer, 52, 160, 60, 255);
			SDL_Rect blade{ sx + 3, base_y - height, 2, height };
			SDL_RenderFillRect(renderer, &blade);
		}
		else {
			SDL_SetRenderDrawColor(renderer, 32, 120, 48, 255);
			SDL_Rect bush{ sx + 1, base_y - height, 6, height };
			SDL_RenderFillRect(renderer, &bush);
		}
	}
	SDL_SetRenderDrawColor(renderer, 35, 30, 25, 255);
	for (const auto& bug : bugs) {
		const int sx = static_cast<int>(bug.x) - camera.x;
		const int sy = static_cast<int>(bug.y) - camera.y;
		if (sx < 0 || sx >= camera.w || sy < 0 || sy >= camera.h) continue;
		SDL_Rect dot{ sx, sy, 2, 2 };
		SDL_RenderFillRect(renderer, &dot);
	}
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
}

Uint64 Life_System::water_in_biomass() const {
	Uint64 total = 0;
	for (const auto& plant : plants) total += plant.stored_water;
	return total;
}

void Life_System::return_water_to_soil() {
	for (auto& plant : plants) {
		deposit_water(plant.x, plant.y, plant.stored_water);
		plant.stored_water = 0;
	}
}

/****************************************************************
*	flora.zen: structure only. Call return_water_to_soil()
*	FIRST so world.zen alone carries the whole water budget.
****************************************************************/
void Life_System::save_life() {
	std::ofstream file(Zen::data_path("world_info/flora.zen"));
	if (!file.is_open()) return;
	file << "[PLANTS]" << std::endl;
	for (const auto& p : plants) {
		file << p.x << "," << static_cast<int>(p.species) << "," << p.growth << "," << p.stress << std::endl;
	}
	file << "[BUGS]" << std::endl;
	for (const auto& b : bugs) {
		file << static_cast<int>(b.x) << "," << b.energy << std::endl;
	}
}

bool Life_System::load_life() {
	std::ifstream file(Zen::data_path("world_info/flora.zen"));
	if (!file.is_open()) return false;

	std::string line;
	bool in_plants = false, in_bugs = false;
	while (std::getline(file, line)) {
		if (line == "[PLANTS]") { in_plants = true; in_bugs = false; continue; }
		if (line == "[BUGS]") { in_plants = false; in_bugs = true; continue; }
		if (line.empty()) continue;
		std::stringstream ss(line);
		std::string field;

		if (in_plants && plants.size() < MAX_PLANTS) {
			Plant p;
			try {
				std::getline(ss, field, ','); p.x = std::stoi(field);
				std::getline(ss, field, ','); p.species = static_cast<Plant_Species>(std::stoi(field));
				std::getline(ss, field, ','); p.growth = std::stoi(field);
				std::getline(ss, field, ','); p.stress = std::stof(field);
			}
			catch (...) { continue; }
			if (p.x < 0 || p.x >= grid_w || occupied[p.x]) continue;
			p.y = surface_row[p.x];
			Tile& soil = world.at(p.x).at(p.y);
			if (Zen::is_air(soil)) continue;
			p.growth = std::clamp(p.growth, 1, params_of(p.species).max_growth);
			// re-drink: reclaim this plant's biomass water from its soil tile
			const int want = p.growth * params_of(p.species).grow_cost;
			const int got = std::min(want, static_cast<int>(soil.saturation));
			soil.saturation -= static_cast<Uint16>(got);
			p.stored_water = got;
			plants.push_back(p);
			occupied[p.x] = 1;
		}
		else if (in_bugs && bugs.size() < MAX_BUGS) {
			Bug b;
			try {
				std::getline(ss, field, ','); b.x = std::stof(field);
				std::getline(ss, field, ','); b.energy = std::stof(field);
			}
			catch (...) { continue; }
			const int tx = std::clamp(static_cast<int>(b.x) / Zen::TILE_SIZE, 0, grid_w - 1);
			b.y = (surface_row[tx] - 2) * Zen::TILE_SIZE;
			bugs.push_back(b);
		}
	}
	return !plants.empty() || !bugs.empty();
}

/****************************************************************
*	One coarse offline hour. An hour of hostile conditions
*	exceeds any live stress limit, so it kills outright; growth
*	compresses the hour's worth of stages; mature plants seed;
*	bugs graze statistically (water back to soil) or starve.
****************************************************************/
void Life_System::bulk_hour(float temp_f) {
	std::uniform_int_distribution<int> spread(-25, 25);
	std::uniform_real_distribution<float> roll(0.0f, 1.0f);

	// pioneer seeds: mirrors the live sim's continuous reseeding, so an
	// empty world can recover from extinction (e.g. spring after a hard
	// offline winter) without the app being open
	std::uniform_int_distribution<int> col(0, grid_w - 1);
	for (int i = 0; i < 8; i++) {
		try_germinate(col(rng), roll(rng) < 0.85f ? Plant_Species::GRASS : Plant_Species::SHRUB);
	}

	for (auto& plant : plants) {
		if (!plant.alive) continue;
		Tile& soil = world.at(plant.x).at(plant.y);
		const Species_Params& p = params_of(plant.species);
		if (temp_f < p.min_temp || temp_f > p.max_temp || soil.saturation < p.wilt_sat) {
			kill_plant(plant);
			continue;
		}
		int stages = static_cast<int>(3600.0f / p.grow_seconds);
		while (stages-- > 0 && plant.growth < p.max_growth &&
			soil.saturation >= p.grow_sat && soil.saturation >= p.grow_cost) {
			soil.saturation -= static_cast<Uint16>(p.grow_cost);
			plant.stored_water += p.grow_cost;
			plant.growth++;
		}
		if (plant.growth == p.max_growth) {
			for (int s = 0; s < 2; s++) {
				try_germinate(plant.x + spread(rng), plant.species);
			}
		}
	}

	int spawned = 0;
	for (auto& bug : bugs) {
		if (!bug.alive) continue;
		if (temp_f < BUG_FREEZE_TEMP) { bug.alive = false; continue; }
		// graze a few stages over the hour, from random living plants
		int meals = 0;
		for (int attempt = 0; attempt < 6 && meals < 3 && !plants.empty(); attempt++) {
			std::uniform_int_distribution<size_t> pick(0, plants.size() - 1);
			Plant& meal = plants[pick(rng)];
			if (!meal.alive || meal.growth <= 1) continue;
			const Species_Params& p = params_of(meal.species);
			meal.growth--;
			const int released = std::min(meal.stored_water, p.grow_cost);
			meal.stored_water -= released;
			deposit_water(meal.x, meal.y, released);
			if (meal.growth <= 0) kill_plant(meal);
			meals++;
		}
		if (meals == 0) { bug.alive = false; continue; } // nothing to eat all hour
		if (meals >= 3 && roll(rng) < 0.25f) spawned++;
	}
	// bug genesis, mirroring the live sim: mature plants can attract bugs
	// into existence even if the population hit zero (eggs hatching /
	// migration arriving where there's food)
	if (temp_f >= BUG_FREEZE_TEMP) {
		for (const auto& plant : plants) {
			if (!plant.alive || plant.growth < params_of(plant.species).max_growth) continue;
			if (roll(rng) < 0.02f) spawned++;
			if (spawned > 4) break; // a few arrivals per hour at most
		}
	}
	for (int i = 0; i < spawned && bugs.size() < MAX_BUGS && !plants.empty(); i++) {
		std::uniform_int_distribution<size_t> pick(0, plants.size() - 1);
		const Plant& host = plants[pick(rng)];
		Bug b;
		b.x = host.x * Zen::TILE_SIZE + 4.0f;
		b.y = (host.y - 2) * Zen::TILE_SIZE;
		bugs.push_back(b);
	}

	plants.erase(std::remove_if(plants.begin(), plants.end(), [](const Plant& p) { return !p.alive; }), plants.end());
	bugs.erase(std::remove_if(bugs.begin(), bugs.end(), [](const Bug& b) { return !b.alive; }), bugs.end());
}
