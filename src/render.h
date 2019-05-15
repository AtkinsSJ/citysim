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
	return {floor_s32(worldPos.x),
			floor_s32(worldPos.y)};
}

struct Camera
{
	V2 pos; // Centre of camera, in camera units
	V2 size; // Size of camera, in camera units
	f32 zoom; // 1 = normal, 2 = things appear twice their size, etc.
	Matrix4 projectionMatrix;

	f32 nearClippingPlane;
	f32 farClippingPlane;

	V2 mousePos;
};
const f32 CAMERA_PAN_SPEED = 10.0f; // Measured in world units per second

struct RenderItem
{
	Rect2 rect;
	f32 depth; // Positive is towards the player
	V4 color;
	Sprite *sprite;
	ShaderType shaderID;
};

struct RenderBuffer
{
	String name;
	Camera camera;
	Array<RenderItem> items;
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