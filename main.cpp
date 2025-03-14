#include <iostream>
#include <SDL.h>
#include "Garden.hpp"

int main(int argc, char* args[])
{
	Garden zen("Project Zen", 640, 480);
	zen.run();
	return 0;
}