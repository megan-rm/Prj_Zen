#include <iostream>
#include <SDL.h>
#include "app.hpp"

int main(int argc, char** args)
{
	App zen("Project Zen", 640, 480);
	zen.run();
	return 0;
}