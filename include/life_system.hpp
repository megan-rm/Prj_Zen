#pragma once
#include <SDL.h>

#include <random>
#include <vector>

#include "tile.hpp"
#include "utils.hpp"

/****************************************************************
*	Life skeleton: plants + bugs, rigged to the same emergent
*	rules as everything else.
*
*	Plants are a FOURTH water pool (soil, air, raindrops,
*	biomass): growing a stage moves water units from the soil
*	tile into stored_water; transpiration trickles it back to
*	the air as humidity (meadows feed clouds, like real
*	forests); death and being eaten return it to the soil.
*	Conservation stays exact to the unit.
*
*	Bugs hold no water — eating a growth stage releases that
*	stage's water into the soil (respiration/frass). They eat,
*	wander, breed at high energy, starve or freeze.
*
*	Not yet persisted: return_water_to_soil() is called before
*	saving so world.zen alone still carries the whole budget.
****************************************************************/

enum class Plant_Species { GRASS, SHRUB };

/****************************************************************
*	Genome: the evolvable traits. On seeding, a child copies its
*	parent's genome with small random drift; plants whose traits
*	suit their locale survive the stress-death rule and pass those
*	traits on. No fitness function is written anywhere — selection
*	IS the existing drought/cold death. Over simulated years the
*	lakeshore and the dry mountainside diverge on their own.
****************************************************************/
struct Genome {
	float min_temp;   // cold tolerance (F)
	float max_temp;   // heat tolerance (F)
	int grow_sat;     // soil saturation needed to grow a stage
	int wilt_sat;     // stress accrues below this saturation
};

struct Plant {
	int x = 0;               // column
	int y = 0;               // surface (soil) tile row it's rooted in
	Plant_Species species = Plant_Species::GRASS;
	Genome genome{};         // evolvable traits (fixed traits still come from species)
	int growth = 1;          // stages, 1..max_growth
	int stored_water = 0;    // water units held in biomass
	float stress = 0.0f;     // accumulated drought/cold; lethal past a limit
	float grow_cooldown = 0.0f;
	bool alive = true;
};

/****************************************************************
*	Fauna genomes — same evolutionary contract as plants: children
*	inherit a parent's traits with small drift, and selection is
*	just who survives to breed. Bugs and their predators both carry
*	one, so prey and hunter co-evolve into an arms race.
****************************************************************/
struct Bug_Genome {
	float speed;        // px/s
	float metabolism;   // energy burned per second
	float cold_tol;     // freezes below this (F)
	float vision;       // px radius to spot & flee predators
	float breed_energy; // energy needed to reproduce
};

struct Bug {
	float x = 0.0f;          // px
	float y = 0.0f;
	float vx = 0.0f;
	float vy = 0.0f;
	float energy = 100.0f;
	float eat_cooldown = 0.0f;
	Bug_Genome genome{};
	bool alive = true;
};

// one enum today (BIRD), room for a ground CRAWLER later without reshaping code
enum class Predator_Type { BIRD };

struct Predator_Genome {
	float speed;        // px/s
	float metabolism;   // energy burned per second
	float vision;       // px radius to spot prey
	float breed_energy; // energy needed to reproduce
};

struct Predator {
	Predator_Type type = Predator_Type::BIRD;
	float x = 0.0f;          // px
	float y = 0.0f;
	float vx = 0.0f;
	float vy = 0.0f;
	float energy = 120.0f;
	float eat_cooldown = 0.0f;
	Predator_Genome genome{};
	bool hunting = false;    // transient: locked onto prey this tick
	bool sated = false;      // full: won't hunt again until hunger returns (hysteresis)
	float tx = 0.0f, ty = 0.0f; // current target position
	bool alive = true;
};

// Fish — aquatic life, confined to standing water, grazing algae. Young (small)
// fish near the surface are catchable by hungry birds; old fish grow too big.
struct Fish_Genome {
	float speed;        // px/s
	float metabolism;   // energy/s
	float cold_tol;     // dies if the local water is colder than this (F)
	float depth_pref;   // 0 = hugs the surface, 1 = hugs the bottom
	float breed_energy; // energy needed to spawn
};

struct Fish {
	float x = 0.0f, y = 0.0f, vx = 0.0f, vy = 0.0f;
	float energy = 60.0f;
	float age = 0.0f;        // seconds alive; size grows with age
	float eat_cooldown = 0.0f;
	Fish_Genome genome{};
	bool alive = true;
};

class Life_System {
public:
	Life_System(std::vector<std::vector<Tile>>& world);
	~Life_System() = default;

	void scatter_seeds(int count);      // initial population attempt
	void scatter_predators(int count);  // initial birds
	void scatter_fish(int count);       // initial fish, seeded into water bodies
	void update(float delta);           // per sim tick: growth, stress, eating, breeding
	void update_motion(float delta);    // per frame: smooth bug + bird movement
	void render(SDL_Renderer* renderer, const SDL_Rect& camera);

	Uint64 water_in_biomass() const;    // for budget debugging
	void return_water_to_soil();        // call before saving the world

	// persistence: flora.zen stores structure only; water always lives in
	// world.zen (plants re-drink stored water from their soil tile on load)
	void save_life();
	bool load_life();

	// offline catch-up: one coarse hour of growth/death/seeding/grazing,
	// driven by the Chronicle with that hour's estimated temperature
	void bulk_hour(float temp_f);
	size_t plant_count() const { return plants.size(); }
	size_t bug_count() const { return bugs.size(); }
	size_t predator_count() const { return predators.size(); }
	size_t fish_count() const { return fish.size(); }

private:
	std::vector<std::vector<Tile>>& world;
	int grid_w;
	int grid_h;
	std::vector<Plant> plants;
	std::vector<Bug> bugs;
	std::vector<Predator> predators;
	std::vector<Fish> fish;
	int algae_scan = 0; // strided cursor for spreading algae growth across ticks
	std::vector<Uint8> occupied;   // one plant per column (skeleton simplification)
	std::vector<int> surface_row;  // cached soil surface tile row per column
	std::vector<int> surface_px;   // pixel-accurate ground height per column (for resting on uneven terrain)
	std::vector<Uint8> lake_col;   // columns inside a real lake basin (algae + fish only live here)
	std::mt19937 rng;

	// parent != nullptr -> inherit its genome with mutation; else base species genome
	bool try_germinate(int x, Plant_Species species, const Genome* parent = nullptr);
	Genome base_genome(Plant_Species species) const;
	Genome mutate(const Genome& parent, Plant_Species species); // uses rng (non-const)
	void kill_plant(Plant& plant);              // returns stored water to its soil tile
	void deposit_water(int x, int y, int units); // into soil, overflow to air humidity
	void update_plants(float delta);
	void update_bugs(float delta);
	void update_predators(float delta);
	void update_predator_motion(float delta);
	void update_fish(float delta);
	void update_fish_motion(float delta);
	void grow_algae();                     // strided per-tick algae growth in sunlit water
	bool watery(int x) const;              // standing water at this column's surface?
	bool submerged(int tx, int ty) const;  // is this tile swimmable water?
	int water_surface_row(int x) const;    // topmost standing-water tile row (-1 if none)

	Bug_Genome bug_base_genome() const;
	Bug_Genome bug_mutate(const Bug_Genome& parent);
	Predator_Genome pred_base_genome() const;
	Predator_Genome pred_mutate(const Predator_Genome& parent);
	Fish_Genome fish_base_genome() const;
	Fish_Genome fish_mutate(const Fish_Genome& parent);
};
