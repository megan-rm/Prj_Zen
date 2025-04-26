#pragma once
#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <string>
#include <unordered_map>
class Texture_Manager {
public:
	Texture_Manager(SDL_Renderer* ren) : renderer(ren) {
	};
	~Texture_Manager() {
		for (auto i : textures) SDL_DestroyTexture(i.second);
	}
	SDL_Texture* get_texture(std::string tex_name);
	SDL_Texture* load_texture(std::string tex_name);

private:
	SDL_Renderer* renderer;
	std::unordered_map<std::string, SDL_Texture*> textures;
	static inline const std::string path = "/assets/images/";
	static inline const std::string img_ext = ".png";
};