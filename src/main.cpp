#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef __linux__
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#else
#include <SDL.h>
#include <SDL_image.h>
#endif

enum TextureAtlasItem {
	TextureAtlasItem_GroundTile = 0,
	TextureAtlasItem_WaterTile,
	TextureAtlasItem_Butcher,
	TextureAtlasItem_Hovel,
	TextureAtlasItem_Paddock,
	TextureAtlasItem_Pit,
	TextureAtlasItem_Road,
	TextureAtlasItem_Goblin,
	TextureAtlasItem_Goat,

	TextureAtlasItemCount
};
struct TextureAtlas {
	SDL_Texture *texture;
	SDL_Rect rects[TextureAtlasItemCount];
};

#include "types.h"
#include "building.h"
#include "city.h"
#include "input.h"

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

const int TILE_WIDTH = 16,
			TILE_HEIGHT = 16;
const int CAMERA_MARGIN = 20;

struct Camera {
	int32 windowWidth, windowHeight;
	V2 pos; // Centre of screen
	real32 zoom; // 1 = normal, 2 = things appear twice their size, etc.
};
const real32 CAMERA_PAN_SPEED = 0.5f; // Measured in camera-widths per second
const int CAMERA_EDGE_SCROLL_MARGIN = 8;
/**
 * Takes x and y in screen space, and returns a position in world-tile space.
 */
inline V2 screenPosToWorldPos(int32 x, int32 y, Camera &camera) {
	return {(x - camera.windowWidth/2 + camera.pos.x) / (camera.zoom * TILE_WIDTH),
			(y - camera.windowHeight/2 + camera.pos.y) / (camera.zoom * TILE_HEIGHT)};
}
inline Coord tilePosition(V2 worldPixelPos) {
	return {(int)floor(worldPixelPos.x),
			(int)floor(worldPixelPos.y)};
}

const real32 SECONDS_PER_FRAME = 1.0f / 60.0f;
const int MS_PER_FRAME = (1000 / 60); // 60 frames per second


void drawAtWorldPos(SDL_Renderer *&renderer, Camera &camera, TextureAtlas &textureAtlas, TextureAtlasItem textureAtlasItem, Coord worldTilePosition, Color *color=0) {
	
	const real32 camLeft = camera.pos.x - (camera.windowWidth * 0.5f),
				 camTop = camera.pos.y - (camera.windowHeight * 0.5f);

	const real32 tileWidth = TILE_WIDTH * camera.zoom,
				tileHeight = TILE_HEIGHT * camera.zoom;

	SDL_Rect *sourceRect = &textureAtlas.rects[textureAtlasItem];
	SDL_Rect destRect = {
		(int)((worldTilePosition.x * tileWidth) - camLeft),
		(int)((worldTilePosition.y * tileHeight) - camTop),
		(int)(sourceRect->w * camera.zoom),
		(int)(sourceRect->h * camera.zoom)
	};

	if (color) {
		SDL_SetTextureColorMod(textureAtlas.texture, color->r, color->g, color->b);
		SDL_SetTextureAlphaMod(textureAtlas.texture, color->a);

		SDL_RenderCopy(renderer, textureAtlas.texture, sourceRect, &destRect);

		SDL_SetTextureColorMod(textureAtlas.texture, 255, 255, 255);
		SDL_SetTextureAlphaMod(textureAtlas.texture, 255);
	} else {
		SDL_RenderCopy(renderer, textureAtlas.texture, sourceRect, &destRect);
	}
}

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

void updateCamera(Camera &camera, MouseState &mouseState, KeyboardState &keyboardState, int32 cityWidth, int32 cityHeight) {
	// Zooming
	if (mouseState.wheelY != 0) {
		// round()ing the zoom so it doesn't gradually drift due to float imprecision
		camera.zoom = clamp(round(10 * camera.zoom + mouseState.wheelY) * 0.1f, 0.1f, 10.0f);
	}

	// Panning
	real32 scrollSpeed = CAMERA_PAN_SPEED * (camera.windowWidth/sqrt(camera.zoom)) * SECONDS_PER_FRAME;
	if (mouseButtonPressed(mouseState, SDL_BUTTON_MIDDLE)) {
		// Click-panning!
		float scale = scrollSpeed * 0.01f;
		Coord clickStartPos = mouseState.clickStartPosition[mouseButtonIndex(SDL_BUTTON_MIDDLE)];
		camera.pos.x += (mouseState.x - clickStartPos.x) * scale;
		camera.pos.y += (mouseState.y - clickStartPos.y) * scale;
	} else {
		// Keyboard/edge-of-screen panning
		if (keyboardState.down[SDL_SCANCODE_LEFT]
			|| (mouseState.x < CAMERA_EDGE_SCROLL_MARGIN)) {
			camera.pos.x -= scrollSpeed;
		} else if (keyboardState.down[SDL_SCANCODE_RIGHT]
			|| (mouseState.x > (camera.windowWidth - CAMERA_EDGE_SCROLL_MARGIN))) {
			camera.pos.x += scrollSpeed;
		}

		if (keyboardState.down[SDL_SCANCODE_UP]
			|| (mouseState.y < CAMERA_EDGE_SCROLL_MARGIN)) {
			camera.pos.y -= scrollSpeed;
		} else if (keyboardState.down[SDL_SCANCODE_DOWN]
			|| (mouseState.y > (camera.windowHeight - CAMERA_EDGE_SCROLL_MARGIN))) {
			camera.pos.y += scrollSpeed;
		}
	}

	// Clamp camera
	int32 cameraWidth = camera.windowWidth,
			cameraHeight = camera.windowHeight;
	real32 scaledCityWidth = cityWidth * camera.zoom,
			scaledCityHeight = cityHeight * camera.zoom;

	if (scaledCityWidth < cameraWidth) {
		// City smaller than camera, so centre on it
		camera.pos.x = scaledCityWidth / 2.0f;
	} else {
		camera.pos.x = clamp(
			camera.pos.x,
			cameraWidth/2.0f - CAMERA_MARGIN,
			scaledCityWidth - (cameraWidth/2.0f - CAMERA_MARGIN)
		);
	}

	if (scaledCityHeight < cameraHeight) {
		// City smaller than camera, so centre on it
		camera.pos.y = scaledCityHeight / 2.0f;
	} else {
		camera.pos.y = clamp(
			camera.pos.y,
			cameraHeight/2.0f - CAMERA_MARGIN,
			scaledCityHeight - (cameraHeight/2.0f - CAMERA_MARGIN)
		);
	}
}

int main(int argc, char *argv[]) {

// INIT
	SDL_Window *window = NULL;
	SDL_Renderer *renderer = NULL;
	if (!initialize(window, renderer)) {
		return 1;
	}

// Load texture data
	TextureAtlas textureAtlas = {};
	textureAtlas.texture = loadTexture(renderer, "combined.png");
	if (!textureAtlas.texture) return 1;
	const int tw = TILE_WIDTH;
	textureAtlas.rects[TextureAtlasItem_GroundTile] = {0,0,tw,tw};
	textureAtlas.rects[TextureAtlasItem_WaterTile] = {tw,0,tw,tw};
	textureAtlas.rects[TextureAtlasItem_Butcher] = {tw*3,tw*5,tw*2,tw*2};
	textureAtlas.rects[TextureAtlasItem_Hovel] = {tw,tw,tw,tw};
	textureAtlas.rects[TextureAtlasItem_Paddock] = {0,tw*2,tw*3,tw*3};
	textureAtlas.rects[TextureAtlasItem_Pit] = {tw*3,0,tw*5,tw*5};
	textureAtlas.rects[TextureAtlasItem_Road] = {0,tw,tw,tw};
	textureAtlas.rects[TextureAtlasItem_Goblin] = {tw,tw*5,tw*2,tw*2};
	textureAtlas.rects[TextureAtlasItem_Goat] = {0,tw*5,tw,tw*2};

// Game setup
	srand(0); // TODO: Seed the random number generator!
	City city = createCity(40,30);
	generateTerrain(city);

// GAME LOOP
	bool quit = false;
	SDL_Event event;
	MouseState mouseState = {};
	KeyboardState keyboardState = {};
	Camera camera = {};
	camera.zoom = 1.0f;
	SDL_GetWindowSize(window, &camera.windowWidth, &camera.windowHeight);

	BuildingArchetype selectedBuildingArchetype = BA_Pit;

	uint32 lastFrame = 0,
			currentFrame = 0;
	real32 framesPerSecond = 0;

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
				} break;
				case SDL_KEYUP: {
					keyboardState.down[event.key.keysym.scancode] = false;
				} break;				
			}
		}

		for (uint8 i = 1; i <= MOUSE_BUTTON_COUNT; ++i) {
			if (mouseButtonJustPressed(mouseState, i)) {
				// Store the initial click position
				mouseState.clickStartPosition[mouseButtonIndex(i)] = {mouseState.x, mouseState.y};
			}
		}

		// Camera controls
		updateCamera(camera, mouseState, keyboardState, city.width*TILE_WIDTH, city.height*TILE_HEIGHT);

		V2 mouseWorldPos = screenPosToWorldPos(mouseState.x, mouseState.y, camera);
		Coord mouseTilePos = tilePosition(mouseWorldPos);

	// UI CODE
		if (keyboardState.down[SDL_SCANCODE_1]) {
			selectedBuildingArchetype = BA_Hovel;
		} else if (keyboardState.down[SDL_SCANCODE_2]) {
			selectedBuildingArchetype = BA_Pit;
		} else if (keyboardState.down[SDL_SCANCODE_3]) {
			selectedBuildingArchetype = BA_Paddock;
		} else if (keyboardState.down[SDL_SCANCODE_4]) {
			selectedBuildingArchetype = BA_Butcher;
		} else if (keyboardState.down[SDL_SCANCODE_5]) {
			selectedBuildingArchetype = BA_Road;
		} else if (keyboardState.down[SDL_SCANCODE_ESCAPE]) {
			selectedBuildingArchetype = BA_None;
		}

		if (selectedBuildingArchetype != BA_None
			&& mouseButtonJustPressed(mouseState, SDL_BUTTON_LEFT)) {
			// Try and build a thing
			Building building = createBuilding(selectedBuildingArchetype, mouseTilePos);
			bool succeeded = placeBuilding(city, building);
			SDL_Log("Attempted to add building '%s', and %s", 
					buildingDefinitions[selectedBuildingArchetype].name.c_str(),
					succeeded ? "succeeded" : "failed"
			);
		}

	// RENDERING
		SDL_RenderClear(renderer);

		TextureAtlasItem textureAtlasItem = TextureAtlasItem_GroundTile;

		// Draw terrain
		for (uint16 y=0; y < city.height; y++) {
			for (uint16 x=0; x < city.width; x++) {
				Terrain t = terrainAt(city,x,y);
				switch (t) {
					case Terrain_Ground: {
						textureAtlasItem = TextureAtlasItem_GroundTile;
					} break;
					case Terrain_Water: {
						textureAtlasItem = TextureAtlasItem_WaterTile;
					} break;
				}

				drawAtWorldPos(renderer, camera, textureAtlas, textureAtlasItem, {x,y});
			}
		}

		// Draw buildings
		for (uint16 i=0; i<city.buildingCount; i++) {
			Building building = city.buildings[i];
			BuildingDefinition *def = buildingDefinitions + building.archetype;
			drawAtWorldPos(renderer, camera, textureAtlas, def->textureAtlasItem, building.footprint.pos);
		}

		if (mouseButtonJustPressed(mouseState, SDL_BUTTON_LEFT)) {
			SDL_Log("Clicked at world position: %f, %f; tile position %d, %d",
					mouseWorldPos.x, mouseWorldPos.y, mouseTilePos.x, mouseTilePos.y);
		}

		// Building preview
		if (selectedBuildingArchetype != BA_None) {
			Color ghostColor = {255,255,255,128};
			if (!canPlaceBuilding(city, selectedBuildingArchetype, mouseTilePos)) {
				ghostColor = {255,0,0,128};
			}
			drawAtWorldPos(renderer, camera, textureAtlas, buildingDefinitions[selectedBuildingArchetype].textureAtlasItem, mouseTilePos, &ghostColor);
		}

		SDL_RenderPresent(renderer);

	// FRAMERATE MONITORING AND CAPPING

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
	freeCity(city);

	SDL_DestroyTexture(textureAtlas.texture);

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	IMG_Quit();
	SDL_Quit();

	return 0;
}