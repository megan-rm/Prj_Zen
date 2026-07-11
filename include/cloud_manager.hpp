#pragma once
#include <SDL.h>

#include <random>
#include <vector>

#include "tile.hpp"
#include "utils.hpp"

/****************************************************************
*	Clouds are emergent: any air tile at/above
*	Zen::CLOUD_TILE_MIN_HUMIDITY is a "cloud tile", and touching
*	cloud tiles (4-connected) form a cluster. Clusters know their
*	size and centroid, so:
*	  - a cluster only rains once it has Zen::RAIN_CLUSTER_TILES
*	    (~200) connected members
*	  - each tick a little humidity is pulled toward the cluster's
*	    core, so clouds are denser in the middle and billow
*	  - one raindrop = exactly 1 humidity, deposited back into
*	    tile saturation on impact. Water is conserved.
****************************************************************/

class Wind_Manager;

struct Raindrop {
	float x = 0.0f;  // pixel coords
	float y = 0.0f;
	float vy = 0.0f;
};

struct Cloud_Cluster {
	int size = 0;             // connected cloud tiles
	long total_humidity = 0;
	float cx = 0.0f;          // centroid, tile coords
	float cy = 0.0f;
	float radius = 1.0f;      // approximate blob radius, tile units
	bool raining = false;
};

class Cloud_Manager {
public:
	Cloud_Manager(std::vector<std::vector<Tile>>& world);
	~Cloud_Manager() = default;

	void sim_update();              // run once per sim tick: label clusters, condense cores, spawn rain
	void update_rain(float delta);  // run every frame: advance + deposit raindrops
	void render(SDL_Renderer* renderer, SDL_Texture* puff_texture, const SDL_Rect& camera,
	            const std::vector<std::vector<Tile>>& snapshot);

	int cluster_id_at(int tile_x, int tile_y) const;
	const std::vector<Cloud_Cluster>& get_clusters() const { return clusters; }
	Uint64 water_in_flight() const { return static_cast<Uint64>(drops.size()); } // 1 humidity per drop
	void register_wind(Wind_Manager* wind_mgr) { wind = wind_mgr; } // raindrops slant with the wind

private:
	std::vector<std::vector<Tile>>& world;
	int grid_w;
	int grid_h;
	std::vector<int> labels;                     // grid_w * grid_h, -1 = not a cloud tile
	std::vector<Uint8> prev_raining;             // per-tile memory for rain hysteresis across relabeling
	std::vector<std::vector<int>> cluster_tiles; // flat tile indices per cluster
	std::vector<Cloud_Cluster> clusters;
	std::vector<Raindrop> drops;
	std::mt19937 rng;
	Wind_Manager* wind = nullptr;

	bool is_cloud_tile(int x, int y) const;
	void label_clusters();
	void condense();
	void spawn_rain();
	bool deposit(int tile_x, int tile_y); // returns the drop's 1 humidity to the world
};
