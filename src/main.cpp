#include <SDL.h>
#include <iostream>

#include "garden.hpp"

int main(int argc, char* argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cout << "Danger, will robinson: " << SDL_GetError() << std::endl;
		return 1;
	}
	{
		Garden zen("Project Zen", 1280, 960);
		zen.run();
	}
	SDL_Quit();
	return 0;
}
