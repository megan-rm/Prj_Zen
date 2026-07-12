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

struct Plant {
	int x = 0;               // column
	int y = 0;               // surface (soil) tile row it's rooted in
	Plant_Species species = Plant_Species::GRASS;
	int growth = 1;          // stages, 1..max_growth
	int stored_water = 0;    // water units held in biomass
	float stress = 0.0f;     // accumulated drought/cold; lethal past a limit
	float grow_cooldown = 0.0f;
	bool alive = true;
};

struct Bug {
	float x = 0.0f;          // px
	float y = 0.0f;
	float vx = 0.0f;
	float vy = 0.0f;
	float energy = 100.0f;
	float eat_cooldown = 0.0f;
	bool alive = true;
};

class Life_System {
public:
	Life_System(std::vector<std::vector<Tile>>& world);
	~Life_System() = default;

	void scatter_seeds(int count);      // initial population attempt
	void update(float delta);           // per sim tick: growth, stress, eating, breeding
	void update_motion(float delta);    // per frame: smooth bug movement
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

private:
	std::vector<std::vector<Tile>>& world;
	int grid_w;
	int grid_h;
	std::vector<Plant> plants;
	std::vector<Bug> bugs;
	std::vector<Uint8> occupied;   // one plant per column (skeleton simplification)
	std::vector<int> surface_row;  // cached soil surface per column
	std::mt19937 rng;

	bool try_germinate(int x, Plant_Species species);
	void kill_plant(Plant& plant);              // returns stored water to its soil tile
	void deposit_water(int x, int y, int units); // into soil, overflow to air humidity
	void update_plants(float delta);
	void update_bugs(float delta);
};
