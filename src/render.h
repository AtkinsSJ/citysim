#pragma once

// General rendering code.

const s32 ITILE_SIZE = 16;
const f32 TILE_SIZE = ITILE_SIZE;
const f32 CAMERA_MARGIN = 1; // How many tiles beyond the map can the camera scroll to show?
const bool canZoom = true;

const int FRAMES_PER_SECOND = 60;
const f32 SECONDS_PER_FRAME = 1.0f / 60.0f;
const int MS_PER_FRAME = (1000 / 60); // 60 frames per second

inline V2I tilePosition(V2 worldPos) {
	return {(int)floor(worldPos.x),
			(int)floor(worldPos.y)};
}

struct Camera
{
	V2 pos; // Centre of camera, in camera units
	V2 size; // Size of camera, in camera units
	f32 zoom; // 1 = normal, 2 = things appear twice their size, etc.
	Matrix4 projectionMatrix;

	V2 mousePos;
};
const f32 CAMERA_PAN_SPEED = 10.0f; // Measured in world units per second
const f32 CAMERA_EDGE_SCROLL_MARGIN = 0.1f; // In normalised screen coordinates, so 0.1 is 5% from the edge

struct RenderItem
{
	Rect2 rect;
	f32 depth; // Positive is towards the player
	V4 color;
	u32 textureRegionID;
};

const int WORLD_SPRITE_MAX = 16384;
const int UI_SPRITE_MAX    = 16384;
const int SPRITE_MAX = MAX(WORLD_SPRITE_MAX, UI_SPRITE_MAX);

struct RenderBuffer
{
	String name;
	Camera camera;
	RenderItem *items;
	u32 itemCount;
	u32 maxItems;
};

struct Renderer
{
	MemoryArena renderArena;
	
	SDL_Window *window;
	RenderBuffer worldBuffer;
	RenderBuffer uiBuffer;

	void *platformRenderer;

	void (*windowResized)(s32, s32);
	void (*render)(Renderer *, AssetManager *);
	void (*loadAssets)(Renderer *, AssetManager *);
	void (*unloadAssets)(Renderer *);
	void (*free)(Renderer *);
};

#include "render.cpp"

// TODO: Some kind of switch to determine which renderer we want to load.
#include "render_gl.h"
#define platform_initializeRenderer GL_initializeRenderer