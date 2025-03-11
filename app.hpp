#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <SDL.h>

class App
{
public:
	App();
	App(std::string st, int sw, int sh);
	~App();
	void run();
	void input();
	void render();
	void update();
private:
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Event events;
	std::vector<std::vector<SDL_Rect>> grid; // 2d array of 8x8px 'blocks' in the garden
	bool running;
	int screen_width;
	int screen_height;
	std::string window_title;
};