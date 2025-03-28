#include <iostream>
#include <SDL.h>
#include "Garden.hpp"

int main(int argc, char* args[])
{
	Garden* zen;
	zen = new Garden("Project Zen", 1280, 960);
	zen->run();
	return 0;
}