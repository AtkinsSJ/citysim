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
	s32 shaderID;

	Asset *texture;
	Rect2 uv; // in (0 to 1) space
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
	void (*render)(Renderer *);
	void (*loadAssets)(Renderer *, AssetManager *);
	void (*unloadAssets)(Renderer *, AssetManager *);
	void (*free)(Renderer *);
};

inline f32 depthFromY(f32 y)
{
	return (y * 0.1f);
}
inline f32 depthFromY(u32 y)
{
	return depthFromY((f32)y);
}
inline f32 depthFromY(s32 y)
{
	return depthFromY((f32)y);
}

void initRenderer(Renderer *renderer, MemoryArena *renderArena, SDL_Window *window);
void initCamera(Camera *camera, V2 size, f32 nearClippingPlane, f32 farClippingPlane, V2 position = v2(0,0));
void sortRenderBuffer(RenderBuffer *buffer);

void makeRenderItem(RenderItem *result, Rect2 rect, f32 depth, Asset *texture, Rect2 uv, s32 shaderID, V4 color=makeWhite());
void drawRenderItem(RenderBuffer *buffer, RenderItem *item);

inline void drawRect(RenderBuffer *buffer, Rect2 rect, f32 depth, s32 shaderID, V4 color)
{
	makeRenderItem(appendUninitialised(&buffer->items), rect, depth, null, {}, shaderID, color);
}

inline void drawSprite(RenderBuffer *buffer, Sprite *sprite, Rect2 rect, f32 depth, s32 shaderID, V4 color=makeWhite())
{
	makeRenderItem(appendUninitialised(&buffer->items), rect, depth, sprite->texture, sprite->uv, shaderID, color);
}

inline void drawRenderItem(RenderBuffer *buffer, RenderItem *item, V2 offsetP, f32 depthOffset, V4 color, s32 shaderID)
{
	makeRenderItem(appendUninitialised(&buffer->items), offset(item->rect, offsetP), item->depth + depthOffset, item->texture, item->uv, shaderID, color);
}

// TODO: Some kind of switch to determine which renderer we want to load.
#include "render_gl.h"
#define platform_initializeRenderer GL_initializeRenderer
