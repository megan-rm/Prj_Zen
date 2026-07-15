#include "cloud_manager.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "wind_manager.hpp"

Cloud_Manager::Cloud_Manager(std::vector<std::vector<Tile>>& world_ref)
	: world(world_ref), rng((std::random_device())()) {
	grid_w = static_cast<int>(world.size());
	grid_h = grid_w > 0 ? static_cast<int>(world.front().size()) : 0;
	labels.assign(static_cast<size_t>(grid_w) * grid_h, -1);
	prev_raining.assign(static_cast<size_t>(grid_w) * grid_h, 0);
}

bool Cloud_Manager::is_cloud_tile(int x, int y) const {
	const Tile& t = world.at(x).at(y);
	return Zen::is_air(t) && t.saturation == 0 && t.humidity >= Zen::CLOUD_TILE_MIN_HUMIDITY;
}

int Cloud_Manager::cluster_id_at(int tile_x, int tile_y) const {
	if (tile_x < 0 || tile_x >= grid_w || tile_y < 0 || tile_y >= grid_h) return -1;
	return labels[static_cast<size_t>(tile_x) * grid_h + tile_y];
}

void Cloud_Manager::sim_update() {
	label_clusters();
	condense();
	spawn_rain();
}

/****************************************************************
*	Flood fill (iterative BFS) over 4-connected cloud tiles.
*	This is what makes a cloud aware of its neighbors: every
*	tile knows which cluster it belongs to, and every cluster
*	knows its size and centroid.
****************************************************************/
void Cloud_Manager::label_clusters() {
	std::fill(labels.begin(), labels.end(), -1);
	clusters.clear();
	cluster_tiles.clear();

	std::vector<int> frontier;
	for (int x = 0; x < grid_w; x++) {
		for (int y = 0; y < grid_h; y++) {
			const size_t idx = static_cast<size_t>(x) * grid_h + y;
			if (labels[idx] != -1 || !is_cloud_tile(x, y)) continue;

			const int id = static_cast<int>(clusters.size());
			clusters.emplace_back();
			cluster_tiles.emplace_back();
			Cloud_Cluster& cluster = clusters.back();

			frontier.clear();
			frontier.push_back(static_cast<int>(idx));
			labels[idx] = id;

			bool was_raining = false;
			double sum_x = 0.0, sum_y = 0.0;
			while (!frontier.empty()) {
				const int flat = frontier.back();
				frontier.pop_back();
				const int cx = flat / grid_h;
				const int cy = flat % grid_h;

				cluster.size++;
				cluster.total_humidity += world.at(cx).at(cy).humidity;
				sum_x += cx;
				sum_y += cy;
				cluster_tiles[id].push_back(flat);
				if (prev_raining[flat]) was_raining = true;

				const int nx[4] = { cx - 1, cx + 1, cx, cx };
				const int ny[4] = { cy, cy, cy - 1, cy + 1 };
				for (int n = 0; n < 4; n++) {
					if (nx[n] < 0 || nx[n] >= grid_w || ny[n] < 0 || ny[n] >= grid_h) continue;
					const size_t nidx = static_cast<size_t>(nx[n]) * grid_h + ny[n];
					if (labels[nidx] != -1 || !is_cloud_tile(nx[n], ny[n])) continue;
					labels[nidx] = id;
					frontier.push_back(static_cast<int>(nidx));
				}
			}
			cluster.cx = static_cast<float>(sum_x / cluster.size);
			cluster.cy = static_cast<float>(sum_y / cluster.size);
			// treat the blob as roughly circular for density falloff
			cluster.radius = std::max(1.5f, std::sqrt(static_cast<float>(cluster.size) / static_cast<float>(M_PI)) * 1.6f);
			// hysteresis: start at RAIN_CLUSTER_TILES, keep raining down to
			// RAIN_STOP_TILES so storms expunge themselves instead of hovering
			// at the threshold forever
			cluster.raining = cluster.size >= Zen::RAIN_CLUSTER_TILES
				|| (was_raining && cluster.size >= Zen::RAIN_STOP_TILES);
		}
	}

	// rebuild per-tile rain memory for next tick's hysteresis check
	std::fill(prev_raining.begin(), prev_raining.end(), 0);
	for (size_t id = 0; id < clusters.size(); id++) {
		if (!clusters[id].raining) continue;
		for (int flat : cluster_tiles[id]) {
			prev_raining[flat] = 1;
		}
	}
}

/****************************************************************
*	Densification toward the center — but HORIZONTAL only, and
*	only down-gradient. Each tile nudges a little humidity to its
*	inward (center-ward) neighbor, and stops once that neighbor
*	is as full as itself. That builds a gentle dome — dense
*	middle columns, wispy edges — WITHOUT collapsing the whole
*	cloud into one saturated column (which is what funneled all
*	the rain into a single tile). Vertical spread is preserved.
*	Conserves exactly.
****************************************************************/
void Cloud_Manager::condense() {
	for (size_t id = 0; id < clusters.size(); id++) {
		const Cloud_Cluster& cluster = clusters[id];
		// small clouds must NOT condense: concentrating into fewer, fuller
		// tiles shrinks their connected count below the rain threshold and
		// strands them forever. let them stay wide, drift, and merge.
		if (cluster.size < Zen::CLOUD_CONDENSE_MIN_TILES) continue;

		for (int flat : cluster_tiles[id]) {
			const int x = flat / grid_h;
			const int y = flat % grid_h;
			Tile& self = world.at(x).at(y);
			int spare = static_cast<int>(self.humidity) - Zen::CLOUD_TILE_MIN_HUMIDITY;
			if (spare <= 0) continue;
			if (std::abs(x - cluster.cx) < 0.5f) continue; // already a center column

			const int dir = (x < cluster.cx) ? 1 : -1;      // one step toward the core
			if (cluster_id_at(x + dir, y) != static_cast<int>(id)) continue;
			Tile& target = world.at(x + dir).at(y);
			// down-gradient guard: only pile inward while the inner tile is
			// less full — this reaches a stable dome instead of a spike
			if (target.humidity >= self.humidity || target.humidity >= Zen::HUMIDITY_MAX) continue;

			int transfer = std::min({ Zen::CLOUD_CONDENSE_RATE, spare,
				Zen::HUMIDITY_MAX - static_cast<int>(target.humidity),
				(static_cast<int>(self.humidity) - static_cast<int>(target.humidity)) / 2 });
			if (transfer <= 0) continue;
			self.humidity -= transfer;
			target.humidity += transfer;
		}
	}
}

/****************************************************************
*	A cluster rains only when it's a proper cloud — at least
*	Zen::RAIN_CLUSTER_TILES connected members. Drops fall from
*	the bottom half of the cluster, biased toward the dense
*	center. Each drop removes exactly 1 humidity here and
*	returns exactly 1 water on impact in deposit().
****************************************************************/
void Cloud_Manager::spawn_rain() {
	for (size_t id = 0; id < clusters.size(); id++) {
		Cloud_Cluster& cluster = clusters[id];
		if (!cluster.raining) continue;

		int drops_wanted = std::min(Zen::RAIN_MAX_DROPS_PER_TICK, cluster.size / 16);
		if (drops_wanted <= 0) drops_wanted = 1;

		/****************************************************************
		*	Rain as a CURTAIN, column by column. For every column the
		*	cluster spans, find its cloud base (lowest wet tile) and its
		*	spare humidity, then spread the drops across all wet columns
		*	proportionally. This structurally cannot funnel to one tile:
		*	rain falls under the whole cloud, heaviest below the dense
		*	middle, tapering at the edges.
		****************************************************************/
		std::unordered_map<int, int> base_flat; // column x -> flat index of its lowest wet tile
		std::unordered_map<int, long> weight;   // column x -> spare humidity
		long total_weight = 0;
		for (int flat : cluster_tiles[id]) {
			const int x = flat / grid_h;
			const int y = flat % grid_h;
			const int spare = static_cast<int>(world.at(x).at(y).humidity) - Zen::CLOUD_TILE_MIN_HUMIDITY;
			if (spare <= 0) continue;
			weight[x] += spare;
			total_weight += spare;
			auto it = base_flat.find(x);
			if (it == base_flat.end() || (it->second % grid_h) < y) base_flat[x] = flat; // keep the lowest
		}
		if (total_weight <= 0) continue;

		std::uniform_real_distribution<float> unit(0.0f, 1.0f);
		std::uniform_real_distribution<float> jitter(0.0f, static_cast<float>(Zen::TILE_SIZE));
		for (const auto& col : weight) {
			const int x = col.first;
			// this column's fair share of the drop budget, stochastically rounded
			float expected = drops_wanted * static_cast<float>(col.second) / static_cast<float>(total_weight);
			int n = static_cast<int>(expected);
			if (unit(rng) < (expected - n)) n += 1;
			if (n <= 0) continue;

			const int base = base_flat[x];
			const int by = base % grid_h;
			for (int d = 0; d < n; d++) {
				Tile& tile = world.at(x).at(by);
				if (tile.humidity <= Zen::CLOUD_TILE_MIN_HUMIDITY) break; // column tapped out this tick
				tile.humidity -= 1; // one raindrop = 1 humidity
				Raindrop drop;
				drop.x = x * Zen::TILE_SIZE + jitter(rng);
				drop.y = (by + 1) * Zen::TILE_SIZE;
				drop.vy = 30.0f;
				drops.push_back(drop);
			}
		}
	}
}

void Cloud_Manager::update_rain(float delta) {
	const float world_px = static_cast<float>(grid_w) * Zen::TILE_SIZE;
	for (size_t i = 0; i < drops.size();) {
		Raindrop& drop = drops[i];
		drop.vy = std::min(drop.vy + Zen::RAIN_GRAVITY * delta, Zen::RAIN_MAX_FALL);
		drop.y += drop.vy * delta;
		if (wind) {
			// drops ride a FRACTION of the wind (inertia) — slant without funneling
			float drift = wind->wind_at(static_cast<int>(drop.x) / Zen::TILE_SIZE,
			                            static_cast<int>(drop.y) / Zen::TILE_SIZE) * Zen::RAIN_WIND_COUPLING;
			drift = std::clamp(drift, -Zen::RAIN_DRIFT_MAX, Zen::RAIN_DRIFT_MAX);
			drop.x += drift * delta;
			if (drop.x < 0.0f) drop.x += world_px;      // cylinder world:
			if (drop.x >= world_px) drop.x -= world_px; // drops wrap too
		}

		const int tx = static_cast<int>(drop.x) / Zen::TILE_SIZE;
		int ty = static_cast<int>(drop.y) / Zen::TILE_SIZE;
		bool landed = false;

		if (tx < 0 || tx >= grid_w) {
			landed = true; // shouldn't happen; guard anyway — recycle below
			ty = grid_h - 1;
		}
		else if (ty >= grid_h) {
			ty = grid_h - 1;
			landed = true;
		}
		else if (ty >= 0) {
			const Tile& t = world.at(tx).at(ty);
			// impact on soil/stone, or on standing water
			if (!Zen::is_air(t) || t.saturation > 0) landed = true;
		}

		if (landed) {
			deposit(std::clamp(tx, 0, grid_w - 1), ty);
			drops[i] = drops.back();
			drops.pop_back();
		}
		else {
			i++;
		}
	}
}

/****************************************************************
*	Return the drop's single unit of water to the world:
*	saturate the impact tile if it has room, otherwise walk
*	upward until something (soil saturation or air humidity)
*	can take it. Never destroys water.
****************************************************************/
bool Cloud_Manager::deposit(int tile_x, int tile_y) {
	for (int y = tile_y; y >= 0; y--) {
		Tile& t = world.at(tile_x).at(y);
		if (!Zen::is_air(t) || t.saturation > 0) {
			// landed on a surface (soil or standing water): freezing -> snow,
			// otherwise soak in. snow rests on top as its own water pool.
			if (t.temperature <= Zen::FREEZE_TEMP) {
				t.snow += 1;
				return true;
			}
			if (t.saturation < t.max_saturation) {
				t.saturation += 1;
				return true;
			}
		}
		else if (t.humidity < Zen::HUMIDITY_MAX) {
			t.humidity += 1;
			return true;
		}
	}
	// column completely full — extraordinarily unlikely, but never lose water
	world.at(tile_x).at(0).humidity += 1;
	return false;
}

/****************************************************************
*	Rendering: one puff per cloud tile, but alpha and size are
*	cluster-aware — denser toward the centroid, wispy at the
*	edges. Raindrops are little streaks.
****************************************************************/
void Cloud_Manager::render(SDL_Renderer* renderer, SDL_Texture* puff_texture, const SDL_Rect& camera,
                           const std::vector<std::vector<Tile>>& snapshot) {
	if (!puff_texture) return;

	int start_x = std::max(0, camera.x / Zen::TILE_SIZE);
	int start_y = std::max(0, camera.y / Zen::TILE_SIZE);
	int end_x = std::min(grid_w, (camera.x + camera.w) / Zen::TILE_SIZE + 1);
	int end_y = std::min(grid_h, (camera.y + camera.h) / Zen::TILE_SIZE + 1);

	SDL_Rect src = { 0, 0, 16, 16 };
	SDL_SetTextureColorMod(puff_texture, 255, 255, 255);

	for (int x = start_x; x < end_x; x++) {
		for (int y = start_y; y < end_y; y++) {
			const int id = cluster_id_at(x, y);
			if (id < 0) continue;
			const Cloud_Cluster& cluster = clusters[id];
			const Tile& tile = snapshot.at(x).at(y);

			float humidity_pct = static_cast<float>(tile.humidity - Zen::CLOUD_TILE_MIN_HUMIDITY)
				/ static_cast<float>(Zen::HUMIDITY_MAX - Zen::CLOUD_TILE_MIN_HUMIDITY);
			humidity_pct = std::clamp(humidity_pct, 0.0f, 1.0f);

			const float dx = x - cluster.cx;
			const float dy = y - cluster.cy;
			const float dist = std::sqrt(dx * dx + dy * dy);
			const float center_boost = std::clamp(1.25f - (dist / cluster.radius) * 0.6f, 0.5f, 1.25f);

			const int alpha = std::clamp(static_cast<int>(220.0f * (0.25f + 0.75f * humidity_pct) * center_boost), 0, 235);

			// cheap deterministic wobble so the deck reads as puffs, not a grid
			const unsigned int hash = (static_cast<unsigned int>(x) * 73856093u) ^ (static_cast<unsigned int>(y) * 19349663u);
			const int wobble = static_cast<int>(hash % 3) - 1;
			const int inflate = 2 + static_cast<int>(humidity_pct * 3.0f);

			SDL_Rect dst{
				x * Zen::TILE_SIZE - camera.x - inflate + wobble,
				y * Zen::TILE_SIZE - camera.y - inflate,
				Zen::TILE_SIZE + inflate * 2,
				Zen::TILE_SIZE + inflate * 2
			};
			SDL_SetTextureAlphaMod(puff_texture, static_cast<Uint8>(alpha));
			SDL_RenderCopy(renderer, puff_texture, &src, &dst);
		}
	}
	SDL_SetTextureAlphaMod(puff_texture, 255);

	// raindrops
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, 90, 140, 255, 200);
	for (const Raindrop& drop : drops) {
		const int sx = static_cast<int>(drop.x) - camera.x;
		const int sy = static_cast<int>(drop.y) - camera.y;
		if (sx < 0 || sx >= camera.w || sy < -3 || sy >= camera.h) continue;
		SDL_Rect streak{ sx, sy, 1, 3 };
		SDL_RenderFillRect(renderer, &streak);
	}
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
