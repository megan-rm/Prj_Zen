#include "chronicle.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

#include "life_system.hpp"

namespace {
	constexpr long FAST_GAP_SECONDS = 120;      // below this: replay real ticks
	constexpr long DAY_SECONDS = 86400;
	constexpr int SETTLE_TICKS = 40;            // ~5 sim-seconds to smooth seams
	constexpr float BULK_DEFICIT = 0.6f;        // average vapor-deficit over an hour
	constexpr float DECK_RAIN_HIGH = 0.55f;     // deck fill fraction that triggers rain-out
	constexpr float DECK_RAIN_KEEP = 0.45f;     // fill fraction left behind after a storm
	constexpr int STORM_MIN_COLUMNS = 15;       // contiguous rained columns = one storm

	std::tm local_tm_of(std::time_t t) {
		std::tm out{};
#ifdef _WIN32
		localtime_s(&out, &t);
#else
		localtime_r(&t, &out);
#endif
		return out;
	}
}

/****************************************************************
*	timeline.zen format:
*	  [LAST_SAVE]
*	  <unix timestamp>
*	  [JOURNAL]
*	  <one entry per line, newest last>
****************************************************************/
long Chronicle::load() {
	std::ifstream file(Zen::data_path("world_info/timeline.zen"));
	if (!file.is_open()) return 0;

	std::string line;
	bool in_journal = false;
	while (std::getline(file, line)) {
		if (line == "[LAST_SAVE]") {
			if (std::getline(file, line)) {
				try { last_save = static_cast<std::time_t>(std::stoll(line)); }
				catch (...) { last_save = 0; }
			}
		}
		else if (line == "[JOURNAL]") {
			in_journal = true;
		}
		else if (in_journal && !line.empty()) {
			journal.push_back(line);
		}
	}
	if (last_save == 0) return 0;

	long gap = static_cast<long>(std::time(nullptr) - last_save);
	return std::max(0L, gap); // clock skew: never go backwards
}

void Chronicle::record_save() {
	std::ofstream file(Zen::data_path("world_info/timeline.zen"));
	if (!file.is_open()) return;
	file << "[LAST_SAVE]" << std::endl;
	file << static_cast<long long>(std::time(nullptr)) << std::endl;
	file << "[JOURNAL]" << std::endl;
	// keep the journal from growing without bound
	const size_t keep = 200;
	const size_t start = journal.size() > keep ? journal.size() - keep : 0;
	for (size_t i = start; i < journal.size(); i++) {
		file << journal[i] << std::endl;
	}
}

void Chronicle::add_entry(const std::string& line) {
	std::tm now = local_tm_of(std::time(nullptr));
	char stamp[32];
	std::snprintf(stamp, sizeof(stamp), "[%04d-%02d-%02d %02d:%02d] ",
		now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min);
	journal.push_back(stamp + line);
	std::cout << "chronicle: " << line << std::endl;
}

int Chronicle::catch_up(std::vector<std::vector<Tile>>& world, Life_System* life, long gap_seconds) {
	if (gap_seconds <= 0) return 0;

	if (gap_seconds <= FAST_GAP_SECONDS) {
		add_entry("resumed after " + std::to_string(gap_seconds) + "s away — replaying exactly");
		return static_cast<int>(gap_seconds * Zen::SIM_HZ);
	}

	const std::time_t now = std::time(nullptr);
	if (gap_seconds >= DAY_SECONDS) {
		// after days away the initial sky is irrelevant: let everything rain
		// out and soak in, then bulk-simulate only the final day
		const long days = gap_seconds / DAY_SECONDS;
		rain_out_everything(world);
		percolate(world, 10);
		add_entry(std::to_string(days) + " day(s) passed — skies cleared, water soaked in");
		bulk_hours(world, life, now - DAY_SECONDS, DAY_SECONDS);
	}
	else {
		bulk_hours(world, life, now - gap_seconds, gap_seconds);
	}
	if (life) {
		add_entry("life endured: " + std::to_string(life->plant_count()) + " plants, "
			+ std::to_string(life->bug_count()) + " bugs");
	}
	return SETTLE_TICKS;
}

/****************************************************************
*	Mirrors Weather_System::get_day_temperature + the diurnal
*	scalar in sun_temperature_update, for an arbitrary moment.
****************************************************************/
float Chronicle::temperature_at(std::time_t when) {
	std::tm lt = local_tm_of(when);
	const int day_of_year = lt.tm_yday + 1;

	const float min_temp = -19.0f;
	const float max_temp = 30.0f;
	float seasonal = (std::sin(2 * M_PI * (day_of_year - 81) / 365.0f) + 1.0f) / 2.0f;
	float day_temp = min_temp + (max_temp - min_temp) * seasonal;
	day_temp = day_temp * (9.0f / 5.0f) + 32.0f;
	const float night_temp = day_temp * 0.6f;

	const float day_pct = (lt.tm_hour * 3600 + lt.tm_min * 60 + lt.tm_sec) / 86400.0f;
	float scalar = std::cos(2 * M_PI * day_pct);
	scalar = 0.5f * (1.0f - scalar);
	scalar = -0.5f * scalar + 0.5f;
	return night_temp + (day_temp - night_temp) * scalar;
}

int Chronicle::find_surface(const std::vector<std::vector<Tile>>& world, int x) {
	for (int y = 0; y < static_cast<int>(world.at(x).size()); y++) {
		const Tile& t = world.at(x).at(y);
		if (!Zen::is_air(t) || t.saturation > 0) return y;
	}
	return static_cast<int>(world.at(x).size()) - 1;
}

/****************************************************************
*	One hour-chunk at a time: integrated evaporation into the
*	deck, rain-out where the deck overfills, percolation. Every
*	move is a bounded integer transfer — the budget cannot drift.
****************************************************************/
void Chronicle::bulk_hours(std::vector<std::vector<Tile>>& world, Life_System* life, std::time_t from, long seconds) {
	const int grid_w = static_cast<int>(world.size());
	Uint64 total_evaporated = 0;
	Uint64 total_rained = 0;
	int storm_count = 0;

	long remaining = seconds;
	std::time_t clock = from;
	while (remaining > 0) {
		const long chunk = std::min(remaining, 3600L);
		const float temp = temperature_at(clock + chunk / 2);

		// --- evaporation into the deck, column by column -------------------
		if (temp > 40.0f) {
			const float temp_factor = (temp - 40.0f) / 30.0f;
			for (int x = 0; x < grid_w; x++) {
				const int sy = find_surface(world, x);
				Tile& surface = world.at(x).at(sy);
				if (surface.saturation == 0) continue;

				long want = static_cast<long>(surface.saturation * Zen::EVAPORATION_RATE * temp_factor * BULK_DEFICIT * chunk);
				want = std::min(want, static_cast<long>(surface.saturation));
				if (want <= 0) continue;

				// fill this column's deck from the base upward
				long moved = 0;
				for (int y = Zen::CLOUD_BASE_ROW; y >= Zen::CLOUD_TOP_ROW && moved < want; y--) {
					if (y >= sy) continue;
					Tile& t = world.at(x).at(y);
					if (!Zen::is_air(t) || t.saturation > 0) continue;
					const long space = Zen::HUMIDITY_MAX - t.humidity;
					const long put = std::min(space, want - moved);
					if (put > 0) {
						t.humidity += static_cast<Uint16>(put);
						moved += put;
					}
				}
				surface.saturation -= static_cast<Uint16>(moved);
				total_evaporated += moved;
			}
		}

		// --- rain-out where the deck is overfull ---------------------------
		bool prev_rained = false;
		int run_length = 0;
		for (int x = 0; x < grid_w; x++) {
			long deck_total = 0;
			long deck_capacity = 0;
			for (int y = Zen::CLOUD_TOP_ROW; y <= Zen::CLOUD_BASE_ROW; y++) {
				const Tile& t = world.at(x).at(y);
				if (!Zen::is_air(t) || t.saturation > 0) continue;
				deck_total += t.humidity;
				deck_capacity += Zen::HUMIDITY_MAX;
			}
			bool rained_here = false;
			if (deck_capacity > 0 && deck_total > static_cast<long>(deck_capacity * DECK_RAIN_HIGH)) {
				long excess = deck_total - static_cast<long>(deck_capacity * DECK_RAIN_KEEP);
				// pull excess from the deck, top rows first
				long pulled = 0;
				for (int y = Zen::CLOUD_TOP_ROW; y <= Zen::CLOUD_BASE_ROW && pulled < excess; y++) {
					Tile& t = world.at(x).at(y);
					if (!Zen::is_air(t) || t.saturation > 0) continue;
					const long take = std::min<long>(t.humidity, excess - pulled);
					t.humidity -= static_cast<Uint16>(take);
					pulled += take;
				}
				// deposit downward into saturation, walking deeper as layers fill
				long deposited = 0;
				const int sy = find_surface(world, x);
				for (int y = sy; y < static_cast<int>(world.at(x).size()) && deposited < pulled; y++) {
					Tile& t = world.at(x).at(y);
					if (t.saturation < t.max_saturation) {
						const long put = std::min<long>(t.max_saturation - t.saturation, pulled - deposited);
						t.saturation += static_cast<Uint16>(put);
						deposited += put;
					}
				}
				// ground completely full: the leftover stays airborne (conserved)
				long leftover = pulled - deposited;
				for (int y = Zen::CLOUD_BASE_ROW; y >= Zen::CLOUD_TOP_ROW && leftover > 0; y--) {
					Tile& t = world.at(x).at(y);
					if (!Zen::is_air(t) || t.saturation > 0) continue;
					const long put = std::min<long>(Zen::HUMIDITY_MAX - t.humidity, leftover);
					t.humidity += static_cast<Uint16>(put);
					leftover -= put;
				}
				total_rained += deposited;
				rained_here = deposited > 0;
			}
			// storm counting: contiguous rained columns form one storm
			if (rained_here) run_length++;
			else {
				if (prev_rained && run_length >= STORM_MIN_COLUMNS) storm_count++;
				run_length = 0;
			}
			prev_rained = rained_here;
		}
		if (run_length >= STORM_MIN_COLUMNS) storm_count++;

		percolate(world, 2);

		// life lives through the same hour, at the same temperature
		if (life) life->bulk_hour(temp);

		clock += chunk;
		remaining -= chunk;
	}

	const long hours = (seconds + 1800) / 3600;
	std::ostringstream entry;
	entry << hours << "h away: evaporated " << total_evaporated
	      << ", " << storm_count << " storm(s) rained " << total_rained << " back down";
	add_entry(entry.str());
}

// all airborne humidity returns to the ground (used for multi-day gaps)
void Chronicle::rain_out_everything(std::vector<std::vector<Tile>>& world) {
	const int grid_w = static_cast<int>(world.size());
	for (int x = 0; x < grid_w; x++) {
		long airborne = 0;
		for (auto& t : world.at(x)) {
			if (Zen::is_air(t) && t.saturation == 0 && t.humidity > 0) {
				airborne += t.humidity;
				t.humidity = 0;
			}
		}
		if (airborne == 0) continue;
		long deposited = 0;
		const int sy = find_surface(world, x);
		for (int y = sy; y < static_cast<int>(world.at(x).size()) && deposited < airborne; y++) {
			Tile& t = world.at(x).at(y);
			if (t.saturation < t.max_saturation) {
				const long put = std::min<long>(t.max_saturation - t.saturation, airborne - deposited);
				t.saturation += static_cast<Uint16>(put);
				deposited += put;
			}
		}
		// column full to the brim: keep the remainder as humidity just above it
		long leftover = airborne - deposited;
		for (int y = sy - 1; y >= 0 && leftover > 0; y--) {
			Tile& t = world.at(x).at(y);
			if (!Zen::is_air(t) || t.saturation > 0) continue;
			const long put = std::min<long>(Zen::HUMIDITY_MAX - t.humidity, leftover);
			t.humidity += static_cast<Uint16>(put);
			leftover -= put;
		}
	}
}

// gravity settling for soil water: each pass lets saturation sink into
// whatever room exists below it
void Chronicle::percolate(std::vector<std::vector<Tile>>& world, int passes) {
	const int grid_w = static_cast<int>(world.size());
	const int grid_h = grid_w > 0 ? static_cast<int>(world.front().size()) : 0;
	for (int pass = 0; pass < passes; pass++) {
		for (int x = 0; x < grid_w; x++) {
			for (int y = grid_h - 2; y >= 0; y--) {
				Tile& self = world.at(x).at(y);
				if (self.saturation == 0 || Zen::is_air(self)) continue;
				Tile& below = world.at(x).at(y + 1);
				if (Zen::is_air(below)) continue;
				const int room = below.max_saturation - below.saturation;
				if (room <= 0) continue;
				// half-speed settling scaled by permeability, like the live sim's flow
				int amount = std::min(static_cast<int>(self.saturation), room);
				amount = static_cast<int>(amount * 0.5f * (below.permeability / 10000.0f + 0.1f));
				if (amount <= 0) continue;
				self.saturation -= static_cast<Uint16>(amount);
				below.saturation += static_cast<Uint16>(amount);
			}
		}
	}
}
