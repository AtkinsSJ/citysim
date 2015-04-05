#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "city.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

const int TILE_WIDTH = 16,
			TILE_HEIGHT = 16;

SDL_Texture* loadTexture(SDL_Renderer *renderer, char *path) {
	SDL_Texture *texture = NULL;

	SDL_Surface *loadedSurface = SDL_LoadBMP(path);
	if (loadedSurface == NULL) {
		printf("Failed to load image at '%s': %s\n", path, SDL_GetError());
	} else {
		texture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
		if (texture == NULL) {
			printf("Failed to create texture from image at '%s': %s\n", path, SDL_GetError());
		}

		SDL_FreeSurface(loadedSurface);
	}

	return texture;
}

int main(int argc, char *argv[]) {

// INIT
	SDL_Window *window = NULL;
	SDL_Renderer *renderer = NULL;
	SDL_Texture *texture = NULL;

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

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == NULL) {
		printf("Renderer could not be created! :(\n %s", SDL_GetError());
		return 1;
	}

	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

	texture = loadTexture(renderer, "tiles.bmp");
	if (!texture) return 1;

	srand(0);

	City city = createCity(40,30);
	generateTerrain(&city);

	printf("Created new city, %d by %d.\n", city.width, city.height);
	for (int y=0; y < city.height; y++) {
		for (int x=0; x < city.width; x++) {
			printf("%c", city.terrain[tileIndex(&city,x,y)] == Terrain_Water ? '~' : '#');
		}
		printf("\n");
	}
	printf("Terrain at 5,10 is %d.\n", city.terrain[tileIndex(&city,5,10)]);

// GAME LOOP
	bool quit = false;
	SDL_Event event;

	while (!quit) {
		while (SDL_PollEvent(&event) != 0) {
			switch (event.type) {
				case SDL_QUIT: {
					quit = true;
				} break;
			}
		}

		SDL_RenderClear(renderer);
		SDL_Rect sourceRect, destRect;
		sourceRect.x = 0;
		sourceRect.y = 0;
		sourceRect.w = TILE_WIDTH;
		sourceRect.h = TILE_HEIGHT;
		destRect.x = 0;
		destRect.y = 0;
		destRect.w = TILE_WIDTH;
		destRect.h = TILE_HEIGHT;

		for (int y=0; y < city.height; y++) {
			destRect.y = y * TILE_HEIGHT;
			for (int x=0; x < city.width; x++) {
				destRect.x = x * TILE_WIDTH;
				Terrain t = city.terrain[tileIndex(&city,x,y)];
				switch (t) {
					case Terrain_Ground: {
						sourceRect.x = 0;
					} break;
					case Terrain_Water: {
						sourceRect.x = TILE_WIDTH;
					} break;
				}
				SDL_RenderCopy(renderer, texture, &sourceRect, &destRect);
			}
		}
		// SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
	}

// CLEAN UP
	freeCity(&city);

	SDL_DestroyTexture(texture);
	texture = NULL;

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	renderer = NULL;
	window = NULL;

	SDL_Quit();

	return 0;
}