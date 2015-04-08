#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <SDL.h>
#include <SDL_image.h>

#include "types.h"
#include "city.h"

const int MOUSE_BUTTON_COUNT = SDL_BUTTON_X2;
struct MouseState {
	uint32 x,y;
	bool down[MOUSE_BUTTON_COUNT];
	bool wasDown[MOUSE_BUTTON_COUNT];
	int32 wheelX, wheelY;
};
inline bool justPressedMouse(MouseState &mouseState, uint8 mouseButton) {
	uint8 buttonIndex = mouseButton - 1;
	return mouseState.down[buttonIndex] && !mouseState.wasDown[buttonIndex];
}
inline bool justReleasedMouse(MouseState &mouseState, uint8 mouseButton) {
	uint8 buttonIndex = mouseButton - 1;
	return !mouseState.down[buttonIndex] && mouseState.wasDown[buttonIndex];
}

const int KEYBOARD_KEY_COUNT = SDL_NUM_SCANCODES;
struct KeyboardState {
	bool down[KEYBOARD_KEY_COUNT];
};

struct Camera {
	int32 windowWidth, windowHeight;
	V2 pos;
	real32 zoom;
};
const real32 SCROLL_SPEED = 250.0f;

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

const int TILE_WIDTH = 16,
			TILE_HEIGHT = 16;
const int CAMERA_MARGIN = 20;

const real32 SECONDS_PER_FRAME = 1.0f / 60.0f;
const int MS_PER_FRAME = (1000 / 60); // 60 frames per second

SDL_Texture* loadTexture(SDL_Renderer *renderer, char *path) {
	SDL_Texture *texture = NULL;

	SDL_Surface *loadedSurface = IMG_Load(path);
	if (loadedSurface == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to load image at '%s': %s\n", path, SDL_GetError());
	} else {
		texture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
		if (texture == NULL) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create texture from image at '%s': %s\n", path, SDL_GetError());
		}

		SDL_FreeSurface(loadedSurface);
	}

	return texture;
}

// *& is a reference to the pointer. Like a pointer-pointer
bool initialize(SDL_Window *&window, SDL_Renderer *&renderer) {
	// SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL could not be initialised! :(\n %s\n", SDL_GetError());
		return false;
	}

	// SDL_image
	uint8 imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags)) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL_image could not be initialised! :(\n %s\n", IMG_GetError());
		return false;
	}

	// Window
	window = SDL_CreateWindow("Impressionable",
					SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
					SCREEN_WIDTH, SCREEN_HEIGHT,
					SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );
	if (window == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Window could not be created! :(\n %s", SDL_GetError());
		return false;
	}

	// Renderer
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Renderer could not be created! :(\n %s", SDL_GetError());
		return false;
	}
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	return true;
}

int main(int argc, char *argv[]) {

// INIT
	SDL_Window *window = NULL;
	SDL_Renderer *renderer = NULL;
	SDL_Texture *texture = NULL;
	uint32 lastFrame = 0,
			currentFrame = 0;
	real32 framesPerSecond = 0;

	if (!initialize(window, renderer)) {
		return 1;
	}

	texture = loadTexture(renderer, "tiles.png");
	if (!texture) return 1;

	srand(0); // TODO: Seed the random number generator!

	City city = createCity(40,30);
	generateTerrain(&city);

	SDL_Log("Created new city, %d by %d.\n", city.width, city.height);
	char *line = new char[city.width+1];
	line[city.width] = 0;
	for (int y=0; y < city.height; y++) {
		for (int x=0; x < city.width; x++) {
			line[x] = city.terrain[tileIndex(&city,x,y)] == Terrain_Water ? '~' : '#';
		}
		SDL_Log(line);
	}
	delete[] line;
	SDL_Log("Terrain at 5,10 is %d.\n", city.terrain[tileIndex(&city,5,10)]);

// GAME LOOP
	bool quit = false;
	SDL_Event event;
	MouseState mouseState = {};
	KeyboardState keyboardState = {};
	Camera camera = {};
	camera.zoom = 1;
	SDL_GetWindowSize(window, &camera.windowWidth, &camera.windowHeight);

	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
	while (!quit) {

		// Clear mouse state
		mouseState.wheelX = 0;
		mouseState.wheelY = 0;

		for (int i = 0; i < MOUSE_BUTTON_COUNT; ++i) {
			mouseState.wasDown[i] = mouseState.down[i];
		}

		while (SDL_PollEvent(&event) != 0) {
			switch (event.type) {
				// WINDOW EVENTS
				case SDL_QUIT: {
					quit = true;
				} break;
				case SDL_WINDOWEVENT: {
					switch (event.window.event) {
						case SDL_WINDOWEVENT_RESIZED: {
							camera.windowWidth = event.window.data1;
							camera.windowHeight = event.window.data2;
						} break;
					}
				} break;

				// MOUSE EVENTS
				// NB: If we later handle TOUCH events, then we need to discard mouse events where event.X.which = SDL_TOUCH_MOUSEID
				case SDL_MOUSEMOTION: {
					mouseState.x = event.motion.x;
					mouseState.y = event.motion.y;
				} break;
				case SDL_MOUSEBUTTONDOWN: {
					uint8 buttonIndex = event.button.button - 1;
					mouseState.down[buttonIndex] = true;
				} break;
				case SDL_MOUSEBUTTONUP: {
					uint8 buttonIndex = event.button.button - 1;
					mouseState.down[buttonIndex] = false;
				} break;
				case SDL_MOUSEWHEEL: {
					// TODO: Uncomment if we upgrade to SDL 2.0.4+, to handle inverted scroll wheel values.
					// if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
					// 	mouseState.wheelX = -event.wheel.x;
					// 	mouseState.wheelY = -event.wheel.y;
					// } else {
						mouseState.wheelX = event.wheel.x;
						mouseState.wheelY = event.wheel.y;
					// }
				} break;

				// KEYBOARD EVENTS
				case SDL_KEYDOWN: {
					keyboardState.down[event.key.keysym.scancode] = true;
					// SDL_Log("Pressed key: %d", event.key.keysym.scancode);
				} break;
				case SDL_KEYUP: {
					keyboardState.down[event.key.keysym.scancode] = false;
					// SDL_Log("Released key: %d", event.key.keysym.scancode);
				} break;				
			}
		}

		for (int i = 1; i <= MOUSE_BUTTON_COUNT; ++i) {
			if (justPressedMouse(mouseState, i)) {
				SDL_Log("Just pressed mouse button: %d at %d,%d\n", i, mouseState.x, mouseState.y);
			} else if (justReleasedMouse(mouseState, i)) {
				SDL_Log("Just released mouse button: %d\n", i);
			}
		}
		if (mouseState.wheelX != 0) {
			SDL_Log("Scrolled mouse in X: %d\n", mouseState.wheelX);
		} else if (mouseState.wheelY != 0) {
			SDL_Log("Scrolled mouse in Y: %d\n", mouseState.wheelY);
		}

		// Camera controls
		{
			real32 scrollSpeed = SCROLL_SPEED * camera.zoom * SECONDS_PER_FRAME;

			if (keyboardState.down[SDL_SCANCODE_LEFT]) {
				camera.pos.x -= scrollSpeed;
			} else if (keyboardState.down[SDL_SCANCODE_RIGHT]) {
				camera.pos.x += scrollSpeed;
			}
			if (keyboardState.down[SDL_SCANCODE_UP]) {
				camera.pos.y -= scrollSpeed;
			} else if (keyboardState.down[SDL_SCANCODE_DOWN]) {
				camera.pos.y += scrollSpeed;
			}

			// Clamp camera
			real32 cameraWidth = camera.windowWidth * camera.zoom,
					cameraHeight = camera.windowHeight * camera.zoom;
			real32 cityWidth = city.width * TILE_WIDTH,
					cityHeight = city.height * TILE_HEIGHT;

			if (cityWidth < cameraWidth) {
				// City smaller than camera, so centre on it
				camera.pos.x = cityWidth / 2.0f;
			} else {
				camera.pos.x = clamp(
					camera.pos.x,
					cameraWidth/2.0f - CAMERA_MARGIN,
					cityWidth - (cameraWidth/2.0f - CAMERA_MARGIN)
				);
			}

			if (cityHeight < cameraHeight) {
				// City smaller than camera, so centre on it
				camera.pos.y = cityHeight / 2.0f;
			} else {
				camera.pos.y = clamp(
					camera.pos.y,
					cameraHeight/2.0f - CAMERA_MARGIN,
					cityHeight - (cameraHeight/2.0f - CAMERA_MARGIN)
				);
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

		// Convert camera position to ints, to stop gaps in the rendering caused by float imprecision.
		int camX = (int)camera.pos.x;
		int camY = (int)camera.pos.y;

		for (int y=0; y < city.height; y++) {
			destRect.y = (y * TILE_HEIGHT) - camY;
			for (int x=0; x < city.width; x++) {
				destRect.x = (x * TILE_WIDTH) - camX;
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
		SDL_RenderPresent(renderer);

		currentFrame = SDL_GetTicks(); // Milliseconds
		uint32 msForFrame = currentFrame - lastFrame;

		// Cap the framerate!
		if (msForFrame < MS_PER_FRAME) {
			SDL_Delay(MS_PER_FRAME - msForFrame);
		}

		// framesPerSecond = 1000.0f / fmax((real32)(currentFrame - lastFrame), 1.0f);
		// SDL_Log("FPS: %f, took %d ticks\n", framesPerSecond, currentFrame-lastFrame);
		lastFrame = SDL_GetTicks();
	}

// CLEAN UP
	freeCity(&city);

	SDL_DestroyTexture(texture);
	texture = NULL;

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	renderer = NULL;
	window = NULL;

	IMG_Quit();
	SDL_Quit();

	return 0;
}