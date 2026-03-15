#include <iostream>
#include <SDL.h>
#include "Garden.hpp"
#include <unistd.h>
#include <limits.h>

char cwd[PATH_MAX];
if (getcwd(cwd, sizeof(cwd)) != nullptr) {
    std::cout << "Current working directory: " << cwd << std::endl;
} else {
    perror("getcwd() error");
}

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