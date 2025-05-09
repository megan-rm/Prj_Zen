#pragma once
#include <vector>

#include <SDL.h>

struct Cloud {
	SDL_Rect dst = { 0,0,0,0 };
	float spawn_time = 0.0f;
	float life_span = 1.0f;
	int max_alpha = 220;

	float get_alpha(float current_time) {
		float age = current_time - spawn_time;
		if (age >= life_span) {
			return 0.0f;
		}
		float pct = 1.0f - (age / life_span);
		return max_alpha * pct;
	}
};

class Cloud_Manager {
public:
	Cloud_Manager(SDL_Renderer* ren, SDL_Texture* texture) : renderer(ren), cloud_texture(texture) {// we're using celestial bodies for now... this probably should change, and the name from celestial bodies to ambient or some such
		src = { 0, 0, 16, 16 };

	}
	~Cloud_Manager() = default;

	void add_cloud(int x, int y, float current_time, float life_span, int max_alpha) {
		Cloud cloud;
		cloud.dst = { x, y, 12, 12 };
		cloud.spawn_time = current_time;
		cloud.life_span = life_span;
		cloud.max_alpha = max_alpha;
		clouds.push_back(cloud);
	}
	
	void update(float current_time) {
		for (int i = 0; i < clouds.size(); i++) {
			if (current_time - clouds[i].spawn_time >= clouds[i].life_span) {
				std::swap(clouds[i], clouds.back());
				clouds.pop_back();
				i--;
			}
		}
	}

	void draw(float current_time) {
		for (auto i : clouds) {
			Uint8 alpha = static_cast<Uint8>(i.get_alpha(current_time));
			SDL_SetTextureAlphaMod(cloud_texture, alpha);
			SDL_RenderCopy(renderer, cloud_texture, &src, &i.dst);
		}
		SDL_SetTextureAlphaMod(cloud_texture, 255);
	}

private:
	std::vector<Cloud> clouds;
	SDL_Renderer* renderer;
	SDL_Texture* cloud_texture;
	SDL_Rect src;
};