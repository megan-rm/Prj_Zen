#include "life_system.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace {
	struct Species_Params {
		int max_growth;
		int grow_cost;          // water units consumed per stage
		int grow_sat;           // soil saturation needed to grow (base genome value)
		int wilt_sat;           // below this the plant stresses (base genome value)
		int min_temp;           // F (base genome value)
		int max_temp;           // (base genome value)
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

	// how far each trait may drift per generation, and the hard bounds it
	// can never mutate past (keeps evolution from running off to nonsense)
	constexpr float TEMP_DRIFT = 1.5f;
	constexpr float SAT_DRIFT = 12.0f;
	constexpr float TEMP_FLOOR = 5.0f,  TEMP_CEIL = 125.0f;
	constexpr int   SAT_FLOOR = 20,     SAT_CEIL = 900;

	constexpr size_t MAX_PLANTS = 2500;
	constexpr size_t MAX_BUGS = 250;
	constexpr size_t MAX_PREDATORS = 40;
	constexpr float BUG_EAT_GAIN = 40.0f;   // energy a bug gains per plant stage eaten
	constexpr float PRED_EAT_GAIN = 30.0f;  // energy a predator gains per bug (was voracious at 55)
	constexpr float PRED_EAT_RADIUS = 7.0f; // px within which a bird snaps up prey
	constexpr float PRED_EAT_COOLDOWN = 2.5f; // seconds between catches — no chain-eating
	constexpr float BIRD_CRUISE_ALT = 120.0f; // px above local ground the bird patrols
	constexpr float BIRD_COMFORT_TEMP = 55.0f; // below this, cold costs the bird extra energy
	constexpr float BIRD_COLD_DRAIN = 0.06f;   // extra energy/sec per degree below comfort

	// fish
	constexpr size_t MAX_FISH = 220;
	constexpr float FISH_ENERGY_PER_ALGAE = 1.0f; // energy per algae unit grazed
	constexpr int FISH_GRAZE = 15;                // max algae eaten per bite
	constexpr float FISH_EAT_COOLDOWN = 0.8f;
	constexpr float FISH_MAX_AGE = 900.0f;        // seconds; senescence sets in past this (~15 min)
	constexpr float FISH_EAT_GAIN_BIRD = 45.0f;   // energy a bird gains from a snatched fish
	constexpr float FISH_CATCH_MAX_SIZE = 4.0f;   // birds can only take fish smaller than this
	constexpr float FISH_SURFACE_REACH = 20.0f;   // px below the surface a bird can still snatch a fish
	constexpr float BIRD_BREED_COST = 70.0f;      // energy a bird spends to raise a chick
	constexpr float BIRD_BREED_RATE = 0.03f;      // chance/sec a satiated bird breeds
	constexpr int ALGAE_GROWTH = 1;               // algae added per pass (slow)
	constexpr int ALGAE_STRIDE = 8;               // 1/8 of columns grown per tick (a full sweep ~1s)

	// base fauna genomes — pioneers start here; evolution drifts from these
	const Bug_Genome BUG_BASE{ 30.0f, 2.0f, 35.0f, 60.0f, 160.0f };      // speed, metab, cold_tol, vision, breed
	// predator breed_energy is now a SATIATION level: birds hunt below it and
	// rest (stop eating) above it, so full birds leave prey alone
	const Predator_Genome PRED_BASE{ 45.0f, 3.0f, 140.0f, 150.0f };      // speed, metab, vision, satiation
	const Fish_Genome FISH_BASE{ 26.0f, 1.0f, 33.0f, 0.5f, 130.0f };     // speed, metab, cold_tol, depth_pref, breed

	inline float fish_size(const Fish& f) { return std::clamp(2.0f + f.age * 0.015f, 2.0f, 7.0f); }
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
			if (!Zen::is_air(t)) { // topmost SOLID tile — the real ground, not a water surface
				surface_row[x] = y;
				break;
			}
		}
	}

	// pixel-accurate ground height per column: the generator writes the true
	// surface pixel-Y per column so plants rest ON uneven ground, not the tile
	// top. Falls back to the tile top if the file is missing (older worlds).
	surface_px.assign(grid_w, 0);
	for (int x = 0; x < grid_w; x++) surface_px[x] = surface_row[x] * Zen::TILE_SIZE;
	std::ifstream sfile(Zen::data_path("world_info/surface.zen"));
	if (sfile.is_open()) {
		std::string line;
		for (int x = 0; x < grid_w && std::getline(sfile, line); x++) {
			try { surface_px[x] = std::stoi(line); } catch (...) {}
		}
	}

	// mark the columns belonging to a real lake/pond basin — algae grows and
	// fish live ONLY here, never in rivers, puddles, or transient standing water
	lake_col.assign(grid_w, 0);
	for (const auto& L : Zen::lakes) {
		const int a = std::clamp(L.start_x / Zen::TILE_SIZE, 0, grid_w - 1);
		const int b = std::clamp(L.end_x / Zen::TILE_SIZE, 0, grid_w - 1);
		for (int x = a; x <= b; x++) lake_col[x] = 1;
	}
}

Genome Life_System::base_genome(Plant_Species species) const {
	const Species_Params& p = params_of(species);
	return Genome{ static_cast<float>(p.min_temp), static_cast<float>(p.max_temp), p.grow_sat, p.wilt_sat };
}

Genome Life_System::mutate(const Genome& parent, Plant_Species species) {
	std::normal_distribution<float> t_drift(0.0f, TEMP_DRIFT);
	std::normal_distribution<float> s_drift(0.0f, SAT_DRIFT);
	Genome g = parent;
	g.min_temp = std::clamp(g.min_temp + t_drift(rng), TEMP_FLOOR, TEMP_CEIL);
	g.max_temp = std::clamp(g.max_temp + t_drift(rng), TEMP_FLOOR, TEMP_CEIL);
	if (g.max_temp < g.min_temp + 10.0f) g.max_temp = g.min_temp + 10.0f; // keep a livable band
	g.grow_sat = std::clamp(g.grow_sat + static_cast<int>(s_drift(rng)), SAT_FLOOR, SAT_CEIL);
	g.wilt_sat = std::clamp(g.wilt_sat + static_cast<int>(s_drift(rng)), SAT_FLOOR, g.grow_sat);
	return g;
}

Bug_Genome Life_System::bug_base_genome() const { return BUG_BASE; }

Bug_Genome Life_System::bug_mutate(const Bug_Genome& p) {
	std::normal_distribution<float> d(0.0f, 1.0f);
	Bug_Genome g = p;
	g.speed        = std::clamp(g.speed + d(rng) * 2.0f, 10.0f, 70.0f);
	g.metabolism   = std::clamp(g.metabolism + d(rng) * 0.2f, 0.8f, 5.0f);
	g.cold_tol     = std::clamp(g.cold_tol + d(rng) * 1.5f, 20.0f, 60.0f);
	g.vision       = std::clamp(g.vision + d(rng) * 6.0f, 10.0f, 180.0f);
	g.breed_energy = std::clamp(g.breed_energy + d(rng) * 8.0f, 80.0f, 320.0f);
	return g;
}

Predator_Genome Life_System::pred_base_genome() const { return PRED_BASE; }

Predator_Genome Life_System::pred_mutate(const Predator_Genome& p) {
	std::normal_distribution<float> d(0.0f, 1.0f);
	Predator_Genome g = p;
	g.speed        = std::clamp(g.speed + d(rng) * 2.5f, 15.0f, 90.0f);
	g.metabolism   = std::clamp(g.metabolism + d(rng) * 0.25f, 1.0f, 6.0f);
	g.vision       = std::clamp(g.vision + d(rng) * 8.0f, 30.0f, 260.0f);
	g.breed_energy = std::clamp(g.breed_energy + d(rng) * 6.0f, 90.0f, 240.0f); // satiation level
	return g;
}

bool Life_System::try_germinate(int x, Plant_Species species, const Genome* parent) {
	if (x < 0 || x >= grid_w) return false;
	if (occupied[x] || plants.size() >= MAX_PLANTS) return false;
	const int sy = surface_row[x];
	Tile& soil = world.at(x).at(sy);
	if (Zen::is_air(soil)) return false;        // no rooting in open water
	if (soil.max_saturation == 0) return false; // bare rock: nothing to root in
	// reject if standing water (lake/river/pond) covers the ground here — a
	// plant belongs on exposed soil, not under or on top of water
	if (sy > 0) {
		const Tile& above = world.at(x).at(sy - 1);
		if (Zen::is_air(above) && above.saturation > 0) return false;
	}
	const Species_Params& p = params_of(species);

	// child inherits parent's genome (mutated); pioneers use the base genome
	Genome genome = parent ? mutate(*parent, species) : base_genome(species);

	if (soil.saturation < genome.grow_sat) return false;
	if (soil.temperature < genome.min_temp || soil.temperature > genome.max_temp) return false;
	if (soil.saturation < p.grow_cost) return false;

	Plant plant;
	plant.x = x;
	plant.y = sy;
	plant.species = species;
	plant.genome = genome;
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

	grow_algae();
	update_plants(delta);
	update_bugs(delta);
	update_fish(delta);
	update_predators(delta);

	// compact the dead
	plants.erase(std::remove_if(plants.begin(), plants.end(), [](const Plant& p) { return !p.alive; }), plants.end());
	bugs.erase(std::remove_if(bugs.begin(), bugs.end(), [](const Bug& b) { return !b.alive; }), bugs.end());
	fish.erase(std::remove_if(fish.begin(), fish.end(), [](const Fish& f) { return !f.alive; }), fish.end());
	predators.erase(std::remove_if(predators.begin(), predators.end(), [](const Predator& p) { return !p.alive; }), predators.end());
}

void Life_System::update_plants(float delta) {
	std::uniform_real_distribution<float> roll(0.0f, 1.0f);
	std::uniform_int_distribution<int> spread(-25, 25);

	// defer new plants/bugs so we never push into `plants`/`bugs` while
	// iterating them (that would invalidate the references we hold)
	struct Seed { int x; Plant_Species species; Genome genome; };
	std::vector<Seed> pending_seeds;
	std::vector<Bug> pending_bugs;

	const size_t count = plants.size();
	for (size_t i = 0; i < count; i++) {
		Plant& plant = plants[i];
		if (!plant.alive) continue;
		Tile& soil = world.at(plant.x).at(plant.y);
		const Species_Params& p = params_of(plant.species);
		const Genome& g = plant.genome;

		// stress from cold, heat, or drought; recovery when comfortable.
		// the thresholds are the plant's OWN evolved genome — this is the
		// selection pressure that shapes local populations.
		if (soil.temperature < g.min_temp || soil.temperature > g.max_temp) plant.stress += delta;
		else if (soil.saturation < g.wilt_sat) plant.stress += delta * 0.5f;
		else plant.stress = std::max(0.0f, plant.stress - delta);
		if (plant.stress > p.stress_limit) {
			kill_plant(plant);
			continue;
		}

		// growth: move water from soil into biomass
		plant.grow_cooldown -= delta;
		if (plant.growth < p.max_growth && plant.grow_cooldown <= 0.0f &&
			soil.saturation >= g.grow_sat && soil.saturation >= p.grow_cost) {
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

		// mature plants scatter seeds nearby, passing on their genome
		if (plant.growth == p.max_growth && roll(rng) < p.seed_rate * delta) {
			pending_seeds.push_back({ plant.x + spread(rng), plant.species, plant.genome });
		}

		// mature plants occasionally attract a bug into existence
		if (plant.growth == p.max_growth && (bugs.size() + pending_bugs.size()) < MAX_BUGS
			&& roll(rng) < 0.002f * delta * 60.0f) {
			Bug bug;
			bug.x = plant.x * Zen::TILE_SIZE + 4.0f;
			bug.y = surface_px[plant.x] - 2 * Zen::TILE_SIZE;
			bug.genome = bug_base_genome();
			pending_bugs.push_back(bug);
		}
	}

	// apply deferred spawns now that iteration is done
	for (const auto& s : pending_seeds) try_germinate(s.x, s.species, &s.genome);
	for (const auto& b : pending_bugs) if (bugs.size() < MAX_BUGS) bugs.push_back(b);
}

void Life_System::update_bugs(float delta) {
	std::uniform_real_distribution<float> roll(-1.0f, 1.0f);
	std::vector<Bug> pending; // deferred births so we never realloc mid-iteration

	const size_t count = bugs.size();
	for (size_t i = 0; i < count; i++) {
		Bug& bug = bugs[i];
		if (!bug.alive) continue;
		const Bug_Genome g = bug.genome;
		bug.energy -= g.metabolism * delta;
		bug.eat_cooldown -= delta;

		const int tx = std::clamp(static_cast<int>(bug.x) / Zen::TILE_SIZE, 0, grid_w - 1);
		if (watery(tx)) { bug.alive = false; continue; } // drowned
		const int sy = surface_row[tx];
		if (world.at(tx).at(sy).temperature < g.cold_tol || bug.energy <= 0.0f) { bug.alive = false; continue; }

		// FLEE: a predator within this bug's vision overrides everything — the
		// selection pressure driving the prey/hunter arms race
		const Predator* threat = nullptr;
		float threat_d2 = g.vision * g.vision;
		for (const auto& pr : predators) {
			if (!pr.alive) continue;
			const float dx = pr.x - bug.x, dy = pr.y - bug.y;
			const float d2 = dx * dx + dy * dy;
			if (d2 < threat_d2) { threat_d2 = d2; threat = &pr; }
		}

		if (threat) {
			bug.vx += (bug.x < threat->x ? -1.0f : 1.0f) * g.speed * 3.0f * delta; // bolt away
		}
		else {
			// hungry: steer toward the nearest edible plant; otherwise wander
			Plant* meal = nullptr;
			if (bug.energy < 80.0f) {
				float best = 100.0f * 100.0f;
				for (auto& plant : plants) {
					if (!plant.alive || plant.growth <= 1) continue;
					const float dx = plant.x * Zen::TILE_SIZE - bug.x;
					const float d2 = dx * dx;
					if (d2 < best) { best = d2; meal = &plant; }
				}
			}
			if (meal) {
				const float dx = meal->x * Zen::TILE_SIZE - bug.x;
				bug.vx += (dx > 0 ? 1.0f : -1.0f) * g.speed * delta;
				if (std::abs(dx) < 10.0f && bug.eat_cooldown <= 0.0f) {
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
				bug.vx += roll(rng) * g.speed * 2.0f * delta;
			}
		}

		// well-fed bugs split, passing on a mutated genome
		if (bug.energy >= g.breed_energy && (bugs.size() + pending.size()) < MAX_BUGS) {
			bug.energy *= 0.5f;
			Bug child = bug;
			child.genome = bug_mutate(bug.genome);
			child.energy = bug.energy;
			child.vx = -bug.vx;
			pending.push_back(child);
		}
	}
	for (auto& c : pending) if (bugs.size() < MAX_BUGS) bugs.push_back(c);
}

// standing water (lake/river/pond) sitting on top of the ground in this column
bool Life_System::watery(int x) const {
	if (x < 0 || x >= grid_w) return false;
	for (int y = 0; y < grid_h; y++) {
		const Tile& t = world.at(x).at(y);
		if (Zen::is_air(t)) {
			if (t.saturation > 0) return true; // hit standing water before ground
		} else {
			return false; // hit dry solid ground first
		}
	}
	return false;
}

void Life_System::update_motion(float delta) {
	const float world_px = static_cast<float>(grid_w) * Zen::TILE_SIZE;
	for (auto& bug : bugs) {
		const float sp = bug.genome.speed;
		bug.vx = std::clamp(bug.vx * (1.0f - 1.5f * delta), -sp, sp);
		float nx = bug.x + bug.vx * delta;
		if (nx < 0.0f) nx += world_px;
		if (nx >= world_px) nx -= world_px;
		const int ntx = std::clamp(static_cast<int>(nx) / Zen::TILE_SIZE, 0, grid_w - 1);
		if (watery(ntx)) {
			bug.vx = -bug.vx; // water ahead — turn back at the shore, bugs keep to dry land
		} else {
			bug.x = nx;
		}
		// hover a couple tiles above the local dry surface
		const int tx = std::clamp(static_cast<int>(bug.x) / Zen::TILE_SIZE, 0, grid_w - 1);
		const float target_y = surface_px[tx] - 2 * Zen::TILE_SIZE;
		bug.y += (target_y - bug.y) * std::min(1.0f, 2.0f * delta);
	}

	update_predator_motion(delta);
	update_fish_motion(delta);
}

void Life_System::scatter_predators(int count) {
	std::uniform_int_distribution<int> col(0, grid_w - 1);
	std::uniform_real_distribution<float> dirp(0.0f, 1.0f);
	for (int i = 0; i < count && static_cast<int>(predators.size()) < MAX_PREDATORS; i++) {
		Predator pr;
		pr.type = Predator_Type::BIRD;
		pr.genome = pred_base_genome();
		const int cx = col(rng);
		pr.x = static_cast<float>(cx * Zen::TILE_SIZE);
		pr.y = std::max(16.0f, surface_px[cx] - BIRD_CRUISE_ALT);
		pr.vx = (dirp(rng) < 0.5f ? -1.0f : 1.0f) * pr.genome.speed * 0.6f;
		pr.energy = 120.0f;
		predators.push_back(pr);
	}
}

// per sim tick: birds burn energy, lock onto the nearest bug, snatch it if
// close, breed when well-fed, starve otherwise
void Life_System::update_predators(float delta) {
	std::vector<Predator> newborns;
	const size_t count = predators.size();
	for (size_t i = 0; i < count; i++) {
		Predator& pr = predators[i];
		if (!pr.alive) continue;
		const Predator_Genome g = pr.genome;
		// energy cost = base metabolism + a cold penalty from the local weather,
		// so a bird's life depends on food AND climate (lean, cold winters cull them)
		const int btx = std::clamp(static_cast<int>(pr.x) / Zen::TILE_SIZE, 0, grid_w - 1);
		const int bty = std::clamp(static_cast<int>(pr.y) / Zen::TILE_SIZE, 0, grid_h - 1);
		const float local_temp = static_cast<float>(world.at(btx).at(bty).temperature);
		const float cold_drain = std::max(0.0f, BIRD_COMFORT_TEMP - local_temp) * BIRD_COLD_DRAIN;
		pr.energy -= (g.metabolism + cold_drain) * delta;
		pr.eat_cooldown -= delta;
		if (pr.energy <= 0.0f) { pr.alive = false; continue; }

		// hunger hysteresis: a bird gorges to satiation, then WON'T hunt again
		// until it has burned down to ~1/3 — so it rests most of the time and
		// only feeds in bursts, instead of topping off after every bite
		if (pr.energy >= g.breed_energy) pr.sated = true;
		else if (pr.energy <= g.breed_energy * 0.34f) pr.sated = false;

		pr.hunting = false;
		if (!pr.sated) {
			float best = g.vision * g.vision;
			int tb = -1, tf = -1;
			for (size_t b = 0; b < bugs.size(); b++) {
				if (!bugs[b].alive) continue;
				const float dx = bugs[b].x - pr.x, dy = bugs[b].y - pr.y;
				const float d2 = dx * dx + dy * dy;
				if (d2 < best) { best = d2; tb = static_cast<int>(b); tf = -1; }
			}
			// only when quite hungry does it bother diving for a small surface fish
			if (pr.energy < g.breed_energy * 0.5f) {
				for (size_t k = 0; k < fish.size(); k++) {
					if (!fish[k].alive || fish_size(fish[k]) >= FISH_CATCH_MAX_SIZE) continue;
					const int fx = std::clamp(static_cast<int>(fish[k].x) / Zen::TILE_SIZE, 0, grid_w - 1);
					const int surf = water_surface_row(fx);
					if (surf < 0 || fish[k].y - surf * Zen::TILE_SIZE > FISH_SURFACE_REACH) continue;
					const float dx = fish[k].x - pr.x, dy = fish[k].y - pr.y;
					const float d2 = dx * dx + dy * dy;
					if (d2 < best) { best = d2; tf = static_cast<int>(k); tb = -1; }
				}
			}
			if (tb >= 0 || tf >= 0) {
				pr.hunting = true;
				const bool close = best < PRED_EAT_RADIUS * PRED_EAT_RADIUS && pr.eat_cooldown <= 0.0f;
				if (tf >= 0) {
					pr.tx = fish[tf].x; pr.ty = fish[tf].y;
					if (close) { fish[tf].alive = false; pr.energy += FISH_EAT_GAIN_BIRD; pr.eat_cooldown = PRED_EAT_COOLDOWN; }
				} else {
					pr.tx = bugs[tb].x; pr.ty = bugs[tb].y;
					if (close) { bugs[tb].alive = false; pr.energy += PRED_EAT_GAIN; pr.eat_cooldown = PRED_EAT_COOLDOWN; }
				}
			}
		}

		// a satiated bird occasionally raises a chick, paying energy for it (so
		// breeding draws down the surplus instead of doubling on every catch)
		if (pr.energy >= g.breed_energy && (predators.size() + newborns.size()) < MAX_PREDATORS) {
			std::uniform_real_distribution<float> br(0.0f, 1.0f);
			if (br(rng) < BIRD_BREED_RATE * delta) {
				pr.energy -= BIRD_BREED_COST;
				Predator child = pr;
				child.genome = pred_mutate(pr.genome);
				child.energy = BIRD_BREED_COST;
				child.vx = -pr.vx;
				child.hunting = false;
				newborns.push_back(child);
			}
		}
	}
	for (auto& c : newborns) predators.push_back(c);

	// MIGRATION: a wandering bird arrives when prey is plentiful but birds are
	// scarce — so extinction is never permanent (mirrors plant reseeding and
	// bugs hatching from mature plants). This is what lets the sky repopulate.
	if (predators.size() < 6 && bugs.size() > 40) {
		std::uniform_real_distribution<float> roll(0.0f, 1.0f);
		if (roll(rng) < 0.02f * delta) {
			std::uniform_int_distribution<int> col(0, grid_w - 1);
			const int cx = col(rng);
			Predator pr;
			pr.genome = pred_base_genome();
			pr.x = static_cast<float>(cx * Zen::TILE_SIZE);
			pr.y = std::max(16.0f, surface_px[cx] - BIRD_CRUISE_ALT);
			pr.vx = (roll(rng) < 0.5f ? -1.0f : 1.0f) * pr.genome.speed * 0.6f;
			pr.energy = 120.0f;
			predators.push_back(pr);
		}
	}
}

// per frame: dive toward locked prey, else cruise at patrol altitude
void Life_System::update_predator_motion(float delta) {
	const float world_px = static_cast<float>(grid_w) * Zen::TILE_SIZE;
	for (auto& pr : predators) {
		if (!pr.alive) continue;
		const float sp = pr.genome.speed;
		if (pr.hunting) {
			const float dx = pr.tx - pr.x, dy = pr.ty - pr.y;
			const float len = std::sqrt(dx * dx + dy * dy) + 0.001f;
			pr.vx += (dx / len) * sp * 3.0f * delta;
			pr.vy += (dy / len) * sp * 3.0f * delta;
		} else {
			const int cx = std::clamp(static_cast<int>(pr.x) / Zen::TILE_SIZE, 0, grid_w - 1);
			const float alt = surface_px[cx] - BIRD_CRUISE_ALT;
			const float dir = (pr.vx >= 0.0f) ? 1.0f : -1.0f;
			pr.vx += dir * sp * 1.2f * delta;       // keep cruising
			pr.vy += (alt - pr.y) * 2.5f * delta;   // seek altitude
			pr.vy *= (1.0f - std::min(1.0f, 2.0f * delta));
		}
		const float v = std::sqrt(pr.vx * pr.vx + pr.vy * pr.vy);
		if (v > sp) { pr.vx = pr.vx / v * sp; pr.vy = pr.vy / v * sp; }
		pr.x += pr.vx * delta;
		pr.y += pr.vy * delta;
		if (pr.x < 0.0f) pr.x += world_px;
		if (pr.x >= world_px) pr.x -= world_px;
		const int cx = std::clamp(static_cast<int>(pr.x) / Zen::TILE_SIZE, 0, grid_w - 1);
		float floor_y = surface_px[cx] - 6.0f; // never drop into the ground...
		const int wsurf = water_surface_row(cx);
		if (wsurf >= 0) floor_y = std::min(floor_y, static_cast<float>(wsurf * Zen::TILE_SIZE) - 2.0f); // ...or below the water surface
		if (pr.y > floor_y) { pr.y = floor_y; pr.vy = -std::abs(pr.vy) * 0.5f; }
		if (pr.y < 16.0f) { pr.y = 16.0f; if (pr.vy < 0.0f) pr.vy = 0.0f; }
	}
}

// ---- algae + fish --------------------------------------------------------

bool Life_System::submerged(int tx, int ty) const {
	if (tx < 0 || tx >= grid_w || ty < 0 || ty >= grid_h) return false;
	const Tile& t = world.at(tx).at(ty);
	return Zen::is_air(t) && t.saturation > 0 && t.temperature > Zen::FREEZE_TEMP; // liquid, not ice
}

int Life_System::water_surface_row(int x) const {
	if (x < 0 || x >= grid_w) return -1;
	for (int y = 0; y < grid_h; y++) {
		const Tile& t = world.at(x).at(y);
		if (Zen::is_air(t) && t.saturation > 0) return y;
		if (!Zen::is_air(t)) return -1; // solid ground before any water
	}
	return -1;
}

// algae accrues in sunlit (shallow), warm, unfrozen water — a slice of columns
// each tick so the cost is spread out. This is the fish's food supply.
void Life_System::grow_algae() {
	for (int x = algae_scan; x < grid_w; x += ALGAE_STRIDE) {
		if (!lake_col[x]) continue; // algae only in real lake basins
		int water_top = -1;
		for (int y = 0; y < grid_h; y++) {
			Tile& t = world.at(x).at(y);
			const bool water = Zen::is_air(t) && t.saturation > 0;
			if (water) {
				if (water_top < 0) water_top = y;
				if (t.temperature > Zen::FREEZE_TEMP) {
					const int depth = y - water_top;
					const float light = std::clamp(1.0f - depth / static_cast<float>(Zen::ALGAE_LIGHT_DEPTH), 0.08f, 1.0f);
					const float warmth = std::clamp((t.temperature - Zen::FREEZE_TEMP) / 30.0f, 0.0f, 1.0f);
					const int grow = static_cast<int>(ALGAE_GROWTH * light * warmth);
					t.algae = static_cast<Uint8>(std::min<int>(Zen::ALGAE_MAX, t.algae + grow));
				}
			} else if (!Zen::is_air(t) && water_top >= 0) {
				break; // reached the lakebed
			}
		}
	}
	algae_scan = (algae_scan + 1) % ALGAE_STRIDE;
}

Fish_Genome Life_System::fish_base_genome() const { return FISH_BASE; }

Fish_Genome Life_System::fish_mutate(const Fish_Genome& p) {
	std::normal_distribution<float> d(0.0f, 1.0f);
	Fish_Genome g = p;
	g.speed        = std::clamp(g.speed + d(rng) * 1.5f, 8.0f, 55.0f);
	g.metabolism   = std::clamp(g.metabolism + d(rng) * 0.08f, 0.25f, 2.0f);
	g.cold_tol     = std::clamp(g.cold_tol + d(rng) * 1.2f, 20.0f, 55.0f);
	g.depth_pref   = std::clamp(g.depth_pref + d(rng) * 0.08f, 0.0f, 1.0f);
	g.breed_energy = std::clamp(g.breed_energy + d(rng) * 7.0f, 70.0f, 260.0f);
	return g;
}

void Life_System::scatter_fish(int count) {
	std::vector<int> water_cols;
	for (int x = 0; x < grid_w; x++) if (lake_col[x] && water_surface_row(x) >= 0) water_cols.push_back(x);
	if (water_cols.empty()) return;

	// prime the lakes with a starting algae bloom so the first fish have food
	// right away instead of starving during the slow build-up
	for (int x : water_cols) {
		const int top = water_surface_row(x);
		if (top < 0) continue;
		for (int y = top; y < grid_h; y++) {
			Tile& t = world.at(x).at(y);
			if (Zen::is_air(t) && t.saturation > 0) t.algae = 90;
			else if (!Zen::is_air(t)) break;
		}
	}

	std::uniform_int_distribution<size_t> pickcol(0, water_cols.size() - 1);
	std::uniform_real_distribution<float> agedist(0.0f, 250.0f); // mix of young (catchable) and old
	for (int i = 0; i < count && fish.size() < MAX_FISH; i++) {
		const int x = water_cols[pickcol(rng)];
		const int top = water_surface_row(x);
		int bot = top;
		for (int y = top; y < grid_h; y++) {
			const Tile& t = world.at(x).at(y);
			if (Zen::is_air(t) && t.saturation > 0) bot = y; else if (!Zen::is_air(t)) break;
		}
		std::uniform_int_distribution<int> yd(top, std::max(top, bot));
		Fish f;
		f.genome = fish_base_genome();
		f.x = static_cast<float>(x * Zen::TILE_SIZE + 4);
		f.y = static_cast<float>(yd(rng) * Zen::TILE_SIZE + 4);
		f.energy = 90.0f; // reserve to outlast the algae bootstrap
		f.age = agedist(rng);
		fish.push_back(f);
	}
}

// per sim tick: fish age, burn energy, graze the algae in their tile, breed,
// and die if their water dries up, freezes, or they starve
void Life_System::update_fish(float delta) {
	std::uniform_real_distribution<float> roll(0.0f, 1.0f);
	std::vector<Fish> newborns;
	const size_t count = fish.size();
	for (size_t i = 0; i < count; i++) {
		Fish& f = fish[i];
		if (!f.alive) continue;
		const Fish_Genome g = f.genome;
		f.age += delta;
		// old age: past its lifespan a fish's death chance climbs, so the
		// population turns over instead of freezing at the cap
		if (f.age > FISH_MAX_AGE && roll(rng) < (f.age - FISH_MAX_AGE) / FISH_MAX_AGE * 0.5f * delta) {
			f.alive = false; continue;
		}
		f.energy -= g.metabolism * delta;
		f.eat_cooldown -= delta;

		const int tx = std::clamp(static_cast<int>(f.x) / Zen::TILE_SIZE, 0, grid_w - 1);
		const int ty = std::clamp(static_cast<int>(f.y) / Zen::TILE_SIZE, 0, grid_h - 1);
		Tile& here = world.at(tx).at(ty);
		const bool in_water = Zen::is_air(here) && here.saturation > 0;
		if (!in_water || here.temperature <= g.cold_tol || f.energy <= 0.0f) { f.alive = false; continue; }

		if (f.eat_cooldown <= 0.0f && here.algae > 0) {
			const int bite = std::min<int>(here.algae, FISH_GRAZE);
			here.algae = static_cast<Uint8>(here.algae - bite);
			f.energy += bite * FISH_ENERGY_PER_ALGAE;
			f.eat_cooldown = FISH_EAT_COOLDOWN;
		}

		if (f.energy >= g.breed_energy && (fish.size() + newborns.size()) < MAX_FISH) {
			f.energy *= 0.5f;
			Fish child = f;
			child.genome = fish_mutate(f.genome);
			child.energy = f.energy;
			child.age = 0.0f;
			child.vx = -f.vx;
			newborns.push_back(child);
		}
	}
	for (auto& c : newborns) fish.push_back(c);
}

// per frame: fish swim toward algae-rich water at their preferred depth, and
// stay strictly inside liquid water (bounce off the surface, bottom, banks, ice)
void Life_System::update_fish_motion(float delta) {
	std::uniform_real_distribution<float> wander(-1.0f, 1.0f);
	for (auto& f : fish) {
		if (!f.alive) continue;
		const float sp = f.genome.speed;
		const int tx = std::clamp(static_cast<int>(f.x) / Zen::TILE_SIZE, 0, grid_w - 1);
		const int top = water_surface_row(tx);
		if (top < 0) { f.alive = false; continue; }
		int bot = top;
		for (int y = top; y < grid_h; y++) {
			const Tile& t = world.at(tx).at(y);
			if (Zen::is_air(t) && t.saturation > 0) bot = y; else if (!Zen::is_air(t)) break;
		}
		// vertical: seek preferred depth within the water column
		const float pref_y = (top + (bot - top) * f.genome.depth_pref) * Zen::TILE_SIZE + 4.0f;
		f.vy += (pref_y - f.y) * 1.5f * delta;
		// horizontal: drift toward the algae-richer neighbor + a little wander
		const int cy = std::clamp(static_cast<int>(f.y) / Zen::TILE_SIZE, 0, grid_h - 1);
		const int lx = std::clamp(tx - 1, 0, grid_w - 1), rx = std::clamp(tx + 1, 0, grid_w - 1);
		const int la = submerged(lx, cy) ? world.at(lx).at(cy).algae : -1;
		const int ra = submerged(rx, cy) ? world.at(rx).at(cy).algae : -1;
		float bias = (la > ra) ? -1.0f : (ra > la) ? 1.0f : 0.0f;
		f.vx += bias * sp * 1.2f * delta + wander(rng) * sp * 0.8f * delta;

		const float v = std::sqrt(f.vx * f.vx + f.vy * f.vy);
		if (v > sp) { f.vx = f.vx / v * sp; f.vy = f.vy / v * sp; }

		const float nx = f.x + f.vx * delta, ny = f.y + f.vy * delta;
		const int ntx = std::clamp(static_cast<int>(nx) / Zen::TILE_SIZE, 0, grid_w - 1);
		const int nty = std::clamp(static_cast<int>(ny) / Zen::TILE_SIZE, 0, grid_h - 1);
		if (submerged(ntx, cy)) f.x = nx; else f.vx = -f.vx * 0.6f; // bank / ice wall
		if (submerged(tx, nty)) f.y = ny; else f.vy = -f.vy * 0.6f; // surface / bottom
	}
}

void Life_System::render(SDL_Renderer* renderer, const SDL_Rect& camera) {
	for (const auto& plant : plants) {
		const int sx = plant.x * Zen::TILE_SIZE - camera.x;
		if (sx < -Zen::TILE_SIZE || sx >= camera.w) continue;
		const int height = plant.growth * 2;
		const int base_y = surface_px[plant.x] - camera.y; // rest on the true ground surface
		// tint by evolved cold-tolerance: hardy (low min_temp) reads cool
		// blue-green, tender reads warm yellow-green — divergent populations
		// become visibly different at a glance
		const float base_min = params_of(plant.species).min_temp;
		const float hardiness = std::clamp((base_min - plant.genome.min_temp) / 25.0f, -1.0f, 1.0f);
		const Uint8 red = static_cast<Uint8>(std::clamp(52 - hardiness * 40.0f, 0.0f, 255.0f));
		const Uint8 blue = static_cast<Uint8>(std::clamp(60 + hardiness * 60.0f, 0.0f, 255.0f));
		if (plant.species == Plant_Species::GRASS) {
			SDL_SetRenderDrawColor(renderer, red, 160, blue, 255);
			SDL_Rect blade{ sx + 3, base_y - height, 2, height };
			SDL_RenderFillRect(renderer, &blade);
		}
		else {
			SDL_SetRenderDrawColor(renderer, static_cast<Uint8>(red * 0.6f), 120, static_cast<Uint8>(blue * 0.8f), 255);
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
	// fish: a small orange body + tail, growing with age (distinct from the
	// dark bugs on land and the green algae in the water)
	for (const auto& f : fish) {
		const int sx = static_cast<int>(f.x) - camera.x;
		const int sy = static_cast<int>(f.y) - camera.y;
		if (sx < 2 || sx >= camera.w || sy < 0 || sy >= camera.h) continue;
		const int s = static_cast<int>(fish_size(f));
		const int h = std::max(1, s / 2);
		SDL_SetRenderDrawColor(renderer, 232, 140, 58, 255);
		SDL_Rect body{ sx, sy, s, h };
		SDL_Rect tail{ sx - 2, sy, 2, h };
		SDL_RenderFillRect(renderer, &body);
		SDL_RenderFillRect(renderer, &tail);
	}

	// birds: a little dark "V" (two angled wing pixels), diving ones flash reddish
	for (const auto& pr : predators) {
		const int sx = static_cast<int>(pr.x) - camera.x;
		const int sy = static_cast<int>(pr.y) - camera.y;
		if (sx < 1 || sx >= camera.w - 1 || sy < 0 || sy >= camera.h) continue;
		if (pr.hunting) SDL_SetRenderDrawColor(renderer, 90, 30, 30, 255);
		else            SDL_SetRenderDrawColor(renderer, 25, 22, 30, 255);
		SDL_Rect body{ sx, sy, 2, 2 };
		SDL_Rect lwing{ sx - 2, sy - 1, 2, 2 };
		SDL_Rect rwing{ sx + 2, sy - 1, 2, 2 };
		SDL_RenderFillRect(renderer, &body);
		SDL_RenderFillRect(renderer, &lwing);
		SDL_RenderFillRect(renderer, &rwing);
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
		file << p.x << "," << static_cast<int>(p.species) << "," << p.growth << "," << p.stress
		     << "," << p.genome.min_temp << "," << p.genome.max_temp
		     << "," << p.genome.grow_sat << "," << p.genome.wilt_sat << std::endl;
	}
	file << "[BUGS]" << std::endl;
	for (const auto& b : bugs) {
		file << static_cast<int>(b.x) << "," << b.energy
		     << "," << b.genome.speed << "," << b.genome.metabolism << "," << b.genome.cold_tol
		     << "," << b.genome.vision << "," << b.genome.breed_energy << std::endl;
	}
	file << "[PREDATORS]" << std::endl;
	for (const auto& pr : predators) {
		file << static_cast<int>(pr.x) << "," << static_cast<int>(pr.y) << "," << pr.energy
		     << "," << static_cast<int>(pr.type)
		     << "," << pr.genome.speed << "," << pr.genome.metabolism
		     << "," << pr.genome.vision << "," << pr.genome.breed_energy << std::endl;
	}
	file << "[FISH]" << std::endl;
	for (const auto& f : fish) {
		file << static_cast<int>(f.x) << "," << static_cast<int>(f.y) << "," << f.energy << "," << f.age
		     << "," << f.genome.speed << "," << f.genome.metabolism << "," << f.genome.cold_tol
		     << "," << f.genome.depth_pref << "," << f.genome.breed_energy << std::endl;
	}
}

bool Life_System::load_life() {
	std::ifstream file(Zen::data_path("world_info/flora.zen"));
	if (!file.is_open()) return false;

	std::string line;
	bool in_plants = false, in_bugs = false, in_preds = false, in_fish = false;
	while (std::getline(file, line)) {
		if (line == "[PLANTS]") { in_plants = true; in_bugs = false; in_preds = false; in_fish = false; continue; }
		if (line == "[BUGS]") { in_plants = false; in_bugs = true; in_preds = false; in_fish = false; continue; }
		if (line == "[PREDATORS]") { in_plants = false; in_bugs = false; in_preds = true; in_fish = false; continue; }
		if (line == "[FISH]") { in_plants = false; in_bugs = false; in_preds = false; in_fish = true; continue; }
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
			// genome (optional: worlds saved before evolution fall back to base)
			p.genome = base_genome(p.species);
			try {
				if (std::getline(ss, field, ',')) p.genome.min_temp = std::stof(field);
				if (std::getline(ss, field, ',')) p.genome.max_temp = std::stof(field);
				if (std::getline(ss, field, ',')) p.genome.grow_sat = std::stoi(field);
				if (std::getline(ss, field, ',')) p.genome.wilt_sat = std::stoi(field);
			}
			catch (...) { p.genome = base_genome(p.species); }
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
			// genome (optional: pre-evolution saves fall back to base)
			b.genome = bug_base_genome();
			try {
				if (std::getline(ss, field, ',')) b.genome.speed = std::stof(field);
				if (std::getline(ss, field, ',')) b.genome.metabolism = std::stof(field);
				if (std::getline(ss, field, ',')) b.genome.cold_tol = std::stof(field);
				if (std::getline(ss, field, ',')) b.genome.vision = std::stof(field);
				if (std::getline(ss, field, ',')) b.genome.breed_energy = std::stof(field);
			}
			catch (...) { b.genome = bug_base_genome(); }
			const int tx = std::clamp(static_cast<int>(b.x) / Zen::TILE_SIZE, 0, grid_w - 1);
			b.y = surface_px[tx] - 2 * Zen::TILE_SIZE;
			bugs.push_back(b);
		}
		else if (in_preds && predators.size() < MAX_PREDATORS) {
			Predator pr;
			try {
				std::getline(ss, field, ','); pr.x = std::stof(field);
				std::getline(ss, field, ','); pr.y = std::stof(field);
				std::getline(ss, field, ','); pr.energy = std::stof(field);
				std::getline(ss, field, ','); pr.type = static_cast<Predator_Type>(std::stoi(field));
			}
			catch (...) { continue; }
			pr.genome = pred_base_genome();
			try {
				if (std::getline(ss, field, ',')) pr.genome.speed = std::stof(field);
				if (std::getline(ss, field, ',')) pr.genome.metabolism = std::stof(field);
				if (std::getline(ss, field, ',')) pr.genome.vision = std::stof(field);
				if (std::getline(ss, field, ',')) pr.genome.breed_energy = std::stof(field);
			}
			catch (...) { pr.genome = pred_base_genome(); }
			predators.push_back(pr);
		}
		else if (in_fish && fish.size() < MAX_FISH) {
			Fish f;
			try {
				std::getline(ss, field, ','); f.x = std::stof(field);
				std::getline(ss, field, ','); f.y = std::stof(field);
				std::getline(ss, field, ','); f.energy = std::stof(field);
				std::getline(ss, field, ','); f.age = std::stof(field);
			}
			catch (...) { continue; }
			f.genome = fish_base_genome();
			try {
				if (std::getline(ss, field, ',')) f.genome.speed = std::stof(field);
				if (std::getline(ss, field, ',')) f.genome.metabolism = std::stof(field);
				if (std::getline(ss, field, ',')) f.genome.cold_tol = std::stof(field);
				if (std::getline(ss, field, ',')) f.genome.depth_pref = std::stof(field);
				if (std::getline(ss, field, ',')) f.genome.breed_energy = std::stof(field);
			}
			catch (...) { f.genome = fish_base_genome(); }
			fish.push_back(f);
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

	struct Seed { int x; Plant_Species species; Genome genome; };
	std::vector<Seed> pending_seeds;

	const size_t count = plants.size();
	for (size_t i = 0; i < count; i++) {
		Plant& plant = plants[i];
		if (!plant.alive) continue;
		Tile& soil = world.at(plant.x).at(plant.y);
		const Species_Params& p = params_of(plant.species);
		const Genome& g = plant.genome;
		if (temp_f < g.min_temp || temp_f > g.max_temp || soil.saturation < g.wilt_sat) {
			kill_plant(plant);
			continue;
		}
		int stages = static_cast<int>(3600.0f / p.grow_seconds);
		while (stages-- > 0 && plant.growth < p.max_growth &&
			soil.saturation >= g.grow_sat && soil.saturation >= p.grow_cost) {
			soil.saturation -= static_cast<Uint16>(p.grow_cost);
			plant.stored_water += p.grow_cost;
			plant.growth++;
		}
		if (plant.growth == p.max_growth) {
			for (int s = 0; s < 2; s++) {
				pending_seeds.push_back({ plant.x + spread(rng), plant.species, plant.genome });
			}
		}
	}
	for (const auto& s : pending_seeds) try_germinate(s.x, s.species, &s.genome);

	int spawned = 0;
	for (auto& bug : bugs) {
		if (!bug.alive) continue;
		if (temp_f < bug.genome.cold_tol) { bug.alive = false; continue; } // froze this hour
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
	if (temp_f >= BUG_BASE.cold_tol) {
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
		b.y = surface_px[host.x] - 2 * Zen::TILE_SIZE;
		b.genome = bug_base_genome();
		bugs.push_back(b);
	}

	// birds live through the hour too: each grazes a few bugs, breeds when fed,
	// starves without prey (coarse offline mirror of the live hunt)
	{
		int hatched = 0;
		for (auto& pr : predators) {
			if (!pr.alive) continue;
			int caught = 0;
			for (int attempt = 0; attempt < 8 && caught < 3 && !bugs.empty(); attempt++) {
				std::uniform_int_distribution<size_t> pick(0, bugs.size() - 1);
				Bug& prey = bugs[pick(rng)];
				if (!prey.alive) continue;
				prey.alive = false;
				pr.energy += PRED_EAT_GAIN;
				caught++;
			}
			// an hour of flying, plus a cold penalty in winter (mirrors the live drain)
			pr.energy -= 30.0f + std::max(0.0f, BIRD_COMFORT_TEMP - temp_f) * 0.8f;
			if (pr.energy <= 0.0f) { pr.alive = false; continue; }
			// occasional, cost-paid breeding when satiated (mirrors the live rule)
			if (pr.energy >= pr.genome.breed_energy && roll(rng) < 0.3f) { pr.energy -= BIRD_BREED_COST; hatched++; }
		}
		for (int i = 0; i < hatched && predators.size() < MAX_PREDATORS; i++) {
			Predator child;
			child.genome = pred_mutate(pred_base_genome());
			std::uniform_int_distribution<int> col(0, grid_w - 1);
			const int cx = col(rng);
			child.x = static_cast<float>(cx * Zen::TILE_SIZE);
			child.y = std::max(16.0f, surface_px[cx] - BIRD_CRUISE_ALT);
			predators.push_back(child);
		}
		// offline migration: birds recolonize a prey-rich world during the gap
		if (predators.size() < 6 && bugs.size() > 40 && roll(rng) < 0.4f) {
			std::uniform_int_distribution<int> col(0, grid_w - 1);
			const int cx = col(rng);
			Predator pr;
			pr.genome = pred_base_genome();
			pr.x = static_cast<float>(cx * Zen::TILE_SIZE);
			pr.y = std::max(16.0f, surface_px[cx] - BIRD_CRUISE_ALT);
			pr.energy = 120.0f;
			predators.push_back(pr);
		}
	}

	// fish through the hour: warm water grows algae that feeds them, cold water
	// starves or kills them; the well-fed spawn (coarse offline mirror)
	{
		std::vector<Fish> hatch;
		for (auto& f : fish) {
			if (!f.alive) continue;
			if (temp_f <= f.genome.cold_tol) { f.alive = false; continue; } // water too cold
			f.age += 3600.0f;
			if (f.age > FISH_MAX_AGE && roll(rng) < 0.5f) { f.alive = false; continue; } // old age
			const float feed = std::clamp((temp_f - Zen::FREEZE_TEMP) / 30.0f, 0.0f, 1.0f) * 45.0f;
			f.energy += feed - 25.0f; // hourly graze minus upkeep
			if (f.energy <= 0.0f) { f.alive = false; continue; }
			if (f.energy >= f.genome.breed_energy && (fish.size() + hatch.size()) < MAX_FISH) {
				f.energy *= 0.5f;
				Fish c = f; c.genome = fish_mutate(f.genome); c.energy = f.energy; c.age = 0.0f;
				hatch.push_back(c);
			}
		}
		for (auto& c : hatch) fish.push_back(c);
	}

	plants.erase(std::remove_if(plants.begin(), plants.end(), [](const Plant& p) { return !p.alive; }), plants.end());
	bugs.erase(std::remove_if(bugs.begin(), bugs.end(), [](const Bug& b) { return !b.alive; }), bugs.end());
	fish.erase(std::remove_if(fish.begin(), fish.end(), [](const Fish& f) { return !f.alive; }), fish.end());
	predators.erase(std::remove_if(predators.begin(), predators.end(), [](const Predator& p) { return !p.alive; }), predators.end());
}
