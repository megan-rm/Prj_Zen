#pragma once
#include <ctime>
#include <string>
#include <vector>

#include "tile.hpp"
#include "utils.hpp"

class Life_System;

/****************************************************************
*	Chronicle: offline time. Records the wall-clock moment the
*	world was saved (world_info/timeline.zen) and, on load,
*	pseudo-simulates the gap so the world looks like it kept
*	living while the app was closed.
*
*	Tiers by gap length:
*	  < 2 min   -> exact: caller replays real sim ticks
*	  < 24 h    -> bulk chronicle: hour-chunks of integrated
*	               evaporation, deck fill, rain-out, percolation
*	  >= 24 h   -> everything rains out & settles, then the
*	               final 24 h are bulk-simulated (after days,
*	               only the last day determines the sky)
*
*	All bulk operations are integer transfers between the same
*	pools the live sim uses, so the water budget stays exact.
*	timeline.zen doubles as a human-readable weather journal.
****************************************************************/
class Chronicle {
public:
	Chronicle() = default;
	~Chronicle() = default;

	// reads timeline.zen; returns seconds since last save (0 = no history)
	long load();

	// pseudo-simulates the gap (weather AND life); returns how many REAL sim
	// ticks the caller should run afterwards (the whole gap for tiny gaps, a
	// settle pass otherwise)
	int catch_up(std::vector<std::vector<Tile>>& world, Life_System* life, long gap_seconds);

	// writes timestamp + journal back to timeline.zen
	void record_save();

private:
	std::time_t last_save = 0;
	std::vector<std::string> journal;

	void add_entry(const std::string& line);

	// mirrors Weather_System's sun model so we can ask "how warm was it
	// at that wall-clock moment" without a Time_System instance
	static float temperature_at(std::time_t when);

	void bulk_hours(std::vector<std::vector<Tile>>& world, Life_System* life, std::time_t from, long seconds);
	void rain_out_everything(std::vector<std::vector<Tile>>& world);
	void percolate(std::vector<std::vector<Tile>>& world, int passes);
	static int find_surface(const std::vector<std::vector<Tile>>& world, int x);
};
