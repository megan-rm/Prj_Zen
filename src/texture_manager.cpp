#include "texture_manager.hpp"

SDL_Texture* Texture_Manager::load_texture(std::string texture_name) {
	std::string full_path = path + texture_name + img_ext;
	SDL_Texture* new_texture = IMG_LoadTexture(renderer, full_path.c_str());
	if (!new_texture) {
		std::cout << "Error loading texture: " << texture_name << std::endl;
	}
	textures[texture_name] = new_texture;
	return new_texture;
}

SDL_Texture* Texture_Manager::get_texture(std::string texture_name) {
	if (textures.at(texture_name) != nullptr) {
		return textures.at(texture_name);
	}
}