#include <iostream>
#include <SDL.h>
#include "Garden.hpp"

int main(int argc, char* args[])
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cout << "Danger, will robinson.";
		return 1;
	}
	Garden* zen;
	zen = new Garden("Project Zen", 1280, 960);
	zen->run();
	return 0;
}