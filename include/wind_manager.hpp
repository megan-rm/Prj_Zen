#pragma once
#include <SDL.h>

#include <vector>

#include "tile.hpp"

#include "utils.hpp"

struct Wind_Gust {
	bool dead = false;
	SDL_Rect area = { 0, 0, 0 , 0 };
	int speed = 5;
	float x = 0.0f;
	float get_force(const SDL_Rect& tile) {
		int gust_center_x = area.x + area.w / 2;
		int tile_center_x = tile.x + tile.w / 2;
		int distance_x = std::abs(tile_center_x - gust_center_x);
		int max_distance = area.w / 2;

		if (distance_x >= max_distance) return 0.0f;
		float norm = distance_x / static_cast<float>(max_distance);
		float force = 0.5f * (std::cos(norm * M_PI) + 1.0f);
		return force;
	}
};

class Wind_Manager {
public:
	Wind_Manager(std::vector<std::vector<Tile>>& world_ref) : world_reference(world_ref) {
		gust_time_spawn = 15.0f; // arbitrary bologna
		last_gust = 0.0f;
		create_new_gust();
	}
	~Wind_Manager() = default;
	void update(float delta) {
		if (gusts.size() < 3) { // limit to 3 for now
			float current_time = SDL_GetTicks() / 1000.0f;
			if (current_time >= gust_time_spawn + last_gust) {
				create_new_gust();
				last_gust = SDL_GetTicks() / 1000.0f;
			}
			
		}
		for (auto& gust : gusts) {
			move_gust(gust, delta);
			apply_gust(gust);
		}
	}
	void render(SDL_Renderer* renderer, SDL_Rect& camera) {
		for (auto& gust : gusts) {
			SDL_Rect adjusted_gust = gust.area;
			adjusted_gust.x -= camera.x;
			adjusted_gust.y -= camera.y;

			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderDrawRect(renderer, &adjusted_gust);
		}
	}


private:
	std::vector<std::vector<Tile>>& world_reference;
	std::vector<Wind_Gust> gusts;

	float gust_time_spawn;
	float last_gust;

	void create_new_gust() {
		int speed = 100;
		int width = 250;
		int height = 130 * Zen::TILE_SIZE;

		Wind_Gust gust;
		
		gust.x = 0.0f;
		gust.area = { 0, 0, width, height };
		gust.speed = speed;
		gusts.push_back(gust);
	}

	void move_gust(Wind_Gust& gust, float delta) {
		gust.x += gust.speed * delta;
		gust.area.x = static_cast<int>(gust.x);
		if (gust.area.x + gust.area.w >= world_reference.size() * Zen::TILE_SIZE) {
			gust.x = 0.0f;
			gust.area.x = 0;
		}
	}

	void apply_gust(Wind_Gust& gust) {
		int tile_start_x = gust.area.x / Zen::TILE_SIZE;
		int tile_end_x = (gust.area.x + gust.area.w) / Zen::TILE_SIZE;
		int tile_start_y = gust.area.y / Zen::TILE_SIZE;
		int tile_end_y = (gust.area.y + gust.area.h) / Zen::TILE_SIZE;

		tile_start_x = std::max(0, tile_start_x);
		tile_start_y = std::max(0, tile_start_y);
		tile_end_x = std::min(Zen::TERRAIN_WIDTH, tile_end_x);
		tile_end_y = std::min(Zen::TERRAIN_HEIGHT, tile_end_y);

		for (int y = tile_start_y; y < tile_end_y; y++) {
			for (int x = tile_start_x; x < tile_end_x; x++) {
				Tile& tile = world_reference.at(x).at(y);

				SDL_Rect tile_rect = { x * Zen::TILE_SIZE, y * Zen::TILE_SIZE, Zen::TILE_SIZE, Zen::TILE_SIZE };
				float force = gust.get_force(tile_rect);
				if (force > 0.0f) {
					apply_force(x, y, force);
				}
			}
		}
	}
	
	void apply_force(int x, int y, float force) {
		Tile& tile = world_reference.at(x).at(y);
		int next_x = x + 1;
		if (next_x >= world_reference.size()) {
			next_x = 0;
		}
		Tile& neighbor = world_reference.at(next_x).at(y);
		
		int transfer = static_cast<int>(tile.humidity * force * 0.1f);
		if (transfer <= 0 && tile.humidity > 0 && force > 0.01f) transfer = 1;

		if (transfer <= 0) return;
		tile.humidity -= transfer;
		neighbor.humidity += transfer;

		float temp_diff = (tile.temperature - neighbor.temperature) * 0.05f * force;
		tile.temperature -= temp_diff;
		neighbor.temperature += temp_diff;
	}

};