#include "app.hpp"

App::App() {
	window_title = "App";
	screen_width = 640;
	screen_height = 480;

	window = SDL_CreateWindow(window_title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_SHOWN);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	running = true;
}

App::App(std::string st, int sw, int sh) {
	window_title = st;
	screen_width = sw;
	screen_height = sh;

	window = SDL_CreateWindow(window_title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_SHOWN);
	
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	running = true;

	// THIS NEED BE SET DIFFERENT
	int w = sw;
	int h = sh;
	grid.resize(w / 8);
	for (int i = 0; i < w / 8; i++) {
		grid.at(i).resize(h / 8);
	}
	for (int i = 0; i < w / 8; i++) {
		for (int j = 0; j < h / 8; j++) {
			grid.at(i).at(j).w = 8;
			grid.at(i).at(j).h = 8;
			grid.at(i).at(j).x = i * 8;
			grid.at(i).at(j).y = j * 8;
		}
	}


}

App::~App() {
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}
void App::update() {

}

void App::render() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 136, 140, 141, 0);
	for (int i = 0; i < screen_width / 8; i++) {
		SDL_RenderFillRect(renderer, &grid.at(i).at((screen_height / 8) - 1));
	}
	for (int i = 0; i < screen_width / 8; i++) {
		SDL_RenderDrawRect(renderer, &grid.at(i).at((screen_height / 8)-1));
	}
	SDL_RenderPresent(renderer);
}

void App::input() {
	while (SDL_PollEvent(&events)) {
		if (events.type == SDL_QUIT) {
			running = false;
			continue;
		}
		else if (events.type = SDL_KEYDOWN)
		{
			switch (events.key.keysym.sym) {
			case SDLK_ESCAPE:
				running = false;
				break;
			case SDLK_SPACE:
				break;
			case SDLK_RIGHT:
				break;
			case SDLK_LEFT:
				break;
			}
		}
	}
}

void App::run()
{
	while (running) {
		input();
		update();
		render();

	}
}