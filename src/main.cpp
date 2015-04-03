#include <SDL.h>
#include <stdio.h>

#include "types.h"
#include "city.h"
#include "city.cpp"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

int main(int argc, char *argv[]) {
	SDL_Window *window = NULL;
	SDL_Surface *screenSurface = NULL;

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL could not be initialised! :(\n %s", SDL_GetError());
		return 1;
	}

	window = SDL_CreateWindow("Impressionable",
					SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
					SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if (window == NULL) {
		printf("Window could not be created! :(\n %s", SDL_GetError());
		return 1;
	}

	screenSurface = SDL_GetWindowSurface(window);
	SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));
	SDL_UpdateWindowSurface(window);

	City city = createCity(40,30);
	printf("Created new city, %d by %d.\n", city.width, city.height);
	printf("Terrain at 5,10 is %d.\n", city.terrain[tileIndex(&city,5,10)]);
	generateTerrain(&city);
	printf("Terrain at 5,10 is %d.\n", city.terrain[tileIndex(&city,5,10)]);
	freeCity(&city);

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}