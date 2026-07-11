/****************************************************************
*	Headless water-conservation test.
*
*	Builds a small synthetic world, seeds soil water and a sky
*	humidity blob big enough to form a >=200-tile cloud cluster,
*	then runs the full sim loop (water, weather, clouds, rain)
*	and asserts the total water budget
*	    sum(saturation) + sum(humidity) + raindrops-in-flight
*	never changes by a single unit.
*
*	Run:  ./conservation_test        (exit 0 = pass)
****************************************************************/
#include <cstdlib>
#include <iostream>
#include <vector>

#include "cloud_manager.hpp"
#include "tile.hpp"
#include "time_system.hpp"
#include "utils.hpp"
#include "water_system.hpp"
#include "weather_system.hpp"

static Uint64 total_water(const std::vector<std::vector<Tile>>& world, const Cloud_Manager& clouds) {
	Uint64 total = 0;
	for (const auto& column : world) {
		for (const auto& tile : column) {
			total += tile.saturation;
			total += tile.humidity;
		}
	}
	total += clouds.water_in_flight();
	return total;
}

int main() {
	const int W = 120;
	const int H = 150;
	const int SURFACE = 120; // rows >= SURFACE are soil

	std::vector<std::vector<Tile>> world(W, std::vector<Tile>(H));
	for (int x = 0; x < W; x++) {
		for (int y = 0; y < H; y++) {
			Tile& t = world[x][y];
			t.img_id = 0;
			t.humidity = 0;
			t.temperature = 70;
			if (y < SURFACE) { // air
				t.permeability = Zen::AIR_PERMEABILITY;
				t.max_saturation = 10000;
				t.saturation = 0;
			}
			else if (y >= H - 3) { // stone floor so water can't "leave"
				t.permeability = 0;
				t.max_saturation = 0;
				t.saturation = 0;
			}
			else { // dirt
				t.permeability = 4176;
				t.max_saturation = 2256;
				t.saturation = 1500;
			}
		}
	}

	// sky humidity blob: 30x20 = 600 tiles at 90 humidity -> a raining cluster
	for (int x = 30; x < 60; x++) {
		for (int y = 25; y < 45; y++) {
			world[x][y].humidity = 90;
		}
	}
	// plus scattered moist air below the deck to exercise buoyancy
	for (int x = 0; x < W; x++) {
		for (int y = 90; y < SURFACE; y++) {
			world[x][y].humidity = 20;
		}
	}

	Water_System water(world, 1);
	Time_System time_system;
	Weather_System weather(world, time_system);
	Cloud_Manager clouds(world);

	const Uint64 budget = total_water(world, clouds);
	std::cout << "initial water budget: " << budget << std::endl;

	bool rained = false;
	bool cluster_seen = false;
	for (int tick = 0; tick < 600; tick++) {
		water.update_saturation(Zen::SIM_DT);
		weather.update_temperatures(Zen::SIM_DT);
		clouds.sim_update();
		// several render-frames worth of raindrop motion per sim tick
		for (int f = 0; f < 8; f++) {
			clouds.update_rain(1.0f / 60.0f);
		}

		for (const auto& c : clouds.get_clusters()) {
			if (c.size >= Zen::RAIN_CLUSTER_TILES) cluster_seen = true;
		}
		if (clouds.water_in_flight() > 0) rained = true;

		const Uint64 now = total_water(world, clouds);
		if (now != budget) {
			std::cout << "FAIL @ tick " << tick << ": budget " << budget
			          << " -> " << now << " (drift " << static_cast<long long>(now - budget) << ")" << std::endl;
			return EXIT_FAILURE;
		}
	}

	// drain remaining drops and re-check
	for (int f = 0; f < 600; f++) clouds.update_rain(1.0f / 60.0f);
	if (total_water(world, clouds) != budget) {
		std::cout << "FAIL after drain: budget drifted" << std::endl;
		return EXIT_FAILURE;
	}

	if (!cluster_seen) {
		std::cout << "FAIL: no >=200-tile cloud cluster ever formed" << std::endl;
		return EXIT_FAILURE;
	}
	if (!rained) {
		std::cout << "FAIL: cluster never rained" << std::endl;
		return EXIT_FAILURE;
	}

	// row-0 stacking check: after 600 ticks the top row should not be saturated wall-to-wall
	int top_row_cloudy = 0;
	for (int x = 0; x < W; x++) {
		if (world[x][0].humidity >= Zen::CLOUD_TILE_MIN_HUMIDITY) top_row_cloudy++;
	}
	std::cout << "top-row cloud tiles: " << top_row_cloudy << "/" << W << std::endl;
	if (top_row_cloudy > W / 2) {
		std::cout << "FAIL: humidity is still stacking on row 0" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << "PASS: water conserved across 600 ticks, clouds clustered, rain fell, no ceiling stacking" << std::endl;
	return EXIT_SUCCESS;
}
