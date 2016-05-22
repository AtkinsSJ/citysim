#include <stdio.h>
#include <math.h>

#ifdef __linux__
#	include <SDL2/SDL.h>
#	include <SDL2/SDL_image.h>
#	include <gl/glew.h> // TODO: Check this
#	include <SDL2/SDL_opengl.h>
#	include <gl/glu.h> // TODO: Check this
#else // Windows
#	include <SDL.h>
#	include <SDL_image.h>
#	include <gl/glew.h>
#	include <SDL_opengl.h>
#	include <gl/glu.h>
#endif

// Really janky assertion macro, yay
#define ASSERT(expr, msg, ...) if(!(expr)) {SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, msg, ##__VA_ARGS__); *(int *)0 = 0;}

enum GameStatus {
	GameStatus_Setup,
	GameStatus_Playing,
	GameStatus_Won,
	GameStatus_Lost,
	GameStatus_Quit,
};

#include "types.h"
#include "memory.h"
#include "random_mt.h"
#include "platform.h"
#include "localisation.h"
#include "maths.h"
#include "file.h"
#include "render_gl.h"
#include "font.h"
#include "bmfont.h"
#include "input.h"
#include "ui.h"
#include "city.h"

struct GameState
{
	GameStatus status;
	MemoryArena *arena;
	RandomMT rng;
	City city;
	UIState uiState;
};

#include "ui.cpp"
#include "pathing.cpp"
#include "city.cpp"

const int gameStartFunds = 10000;
const int gameWinFunds = 30000;
#include "game_ui.cpp"
#include "game.cpp"

void updateInput(InputState *inputState)
{
	// Clear mouse state
	inputState->wheelX = 0;
	inputState->wheelY = 0;

	for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
		inputState->mouseWasDown[i] = inputState->mouseDown[i];
	}

	for (int i=0; i < KEYBOARD_KEY_COUNT; i++) {
		inputState->keyWasDown[i] = inputState->keyDown[i];
	}

	inputState->textEntered[0] = 0;

	inputState->receivedQuitSignal = false;
	inputState->wasWindowResized = false;

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			// WINDOW EVENTS
			case SDL_QUIT: {
				inputState->receivedQuitSignal = true;
			} break;
			case SDL_WINDOWEVENT: {
				switch (event.window.event) {
					case SDL_WINDOWEVENT_RESIZED: {
						inputState->wasWindowResized = true;
						inputState->newWindowWidth = event.window.data1;
						inputState->newWindowHeight = event.window.data2;
					} break;
				}
			} break;

			// MOUSE EVENTS
			// NB: If we later handle TOUCH events, then we need to discard mouse events where event.X.which = SDL_TOUCH_MOUSEID
			case SDL_MOUSEMOTION: {
				inputState->mousePos.x = event.motion.x;
				inputState->mousePos.y = event.motion.y;
			} break;
			case SDL_MOUSEBUTTONDOWN: {
				uint8 buttonIndex = event.button.button - 1;
				inputState->mouseDown[buttonIndex] = true;
			} break;
			case SDL_MOUSEBUTTONUP: {
				uint8 buttonIndex = event.button.button - 1;
				inputState->mouseDown[buttonIndex] = false;
			} break;
			case SDL_MOUSEWHEEL: {
				if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
					inputState->wheelX = -event.wheel.x;
					inputState->wheelY = -event.wheel.y;
				} else {
					inputState->wheelX = event.wheel.x;
					inputState->wheelY = event.wheel.y;
				}
			} break;

			// KEYBOARD EVENTS
			case SDL_KEYDOWN: {
				inputState->keyDown[event.key.keysym.scancode] = true;
			} break;
			case SDL_KEYUP: {
				inputState->keyDown[event.key.keysym.scancode] = false;
			} break;
			case SDL_TEXTINPUT: {
				strncpy(inputState->textEntered, event.text.text, SDL_TEXTINPUTEVENT_TEXT_SIZE);
			} break;
		}
	}

	for (uint8 i = 1; i <= MOUSE_BUTTON_COUNT; ++i) {
		if (mouseButtonJustPressed(inputState, i)) {
			// Store the initial click position
			inputState->clickStartPosition[mouseButtonIndex(i)] = inputState->mousePos;
		}
	}
}

int main(int argc, char *argv[]) {
	// SDL requires these params, and the compiler keeps complaining they're unused, so a hack! Yay!
	if (argc && argv) {}

// INIT
	MemoryArena memoryArena;
	if (!initMemoryArena(&memoryArena, MB(128)))
	{
		printf("Failed to allocate memory!");
		return 1;
	}

	GLRenderer *renderer = initializeRenderer(&memoryArena, "Under London");
	if (!renderer) {
		return 1;
	}

// Game setup
	MemoryArena gameArena = allocateSubArena(&memoryArena, MB(32));
	
	GameState *gameState = startGame(&gameArena);

// GAME LOOP
	gameState->status = GameStatus_Setup;

	InputState inputState = {};

	V2 mouseDragStartPos = {};
	Rect dragRect = irectXYWH(-1,-1,0,0);

	renderer->worldCamera.zoom = 1.0f;
	SDL_GetWindowSize(renderer->window, &renderer->worldCamera.windowWidth, &renderer->worldCamera.windowHeight);
	renderer->worldCamera.pos = v2(gameState->city.width/2, gameState->city.height/2);
	renderer->uiBuffer.projectionMatrix = orthographicMatrix4(
		0, (GLfloat) renderer->worldCamera.windowWidth,
		0, (GLfloat) renderer->worldCamera.windowHeight,
		-1000.0f, 1000.0f
	);

	initUiState(&gameState->uiState);	

	uint32 lastFrame = 0,
			currentFrame = 0;
	real32 framesPerSecond = 0;
	
	// GAME LOOP
	while (gameState->status != GameStatus_Quit) {

		updateInput(&inputState);

		if (inputState.receivedQuitSignal)
		{
			gameState->status = GameStatus_Quit;
			break;
		}

		if (inputState.wasWindowResized)
		{
			resizeWindow(renderer, inputState.newWindowWidth, inputState.newWindowHeight);
		}

		gameUpdateAndRender(gameState, renderer, &inputState);

	// Actually draw things!
		render(renderer);

	// FRAMERATE MONITORING AND CAPPING
		currentFrame = SDL_GetTicks(); // Milliseconds
		uint32 msForFrame = currentFrame - lastFrame;

		// Cap the framerate!
		if (msForFrame < MS_PER_FRAME) {
			SDL_Delay(MS_PER_FRAME - msForFrame);
		}

		framesPerSecond = 1000.0f / fmax((real32)(currentFrame - lastFrame), 1.0f);
		SDL_Log("FPS: %f, took %d ticks\n", framesPerSecond, currentFrame-lastFrame);
		lastFrame = SDL_GetTicks();
	}

// CLEAN UP

	freeRenderer(renderer);

	return 0;
}