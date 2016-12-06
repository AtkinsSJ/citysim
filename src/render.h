#pragma once

// General rendering code.

const float TILE_SIZE = 16.0f;
const int TILE_WIDTH = 16,
			TILE_HEIGHT = 16;
const int CAMERA_MARGIN = 1; // How many tiles beyond the map can the camera scroll to show?
const bool canZoom = true;

inline Coord tilePosition(V2 worldPos) {
	return {(int)floor(worldPos.x),
			(int)floor(worldPos.y)};
}

struct Camera
{
	// TODO: Replace pixel size with a res-independant size which we then convert to pixels.
	// So worldCamera is in world units, uiCamera in pixels.
	union {
		Coord windowSize;
		struct{int32 windowWidth, windowHeight;};
	};
	V2 pos; // Centre of screen, in tiles
	real32 zoom; // 1 = normal, 2 = things appear twice their size, etc.

	Matrix4 projectionMatrix;
};
const real32 CAMERA_PAN_SPEED = 10.0f; // Measured in world units per second
const real32 CAMERA_EDGE_SCROLL_MARGIN = 0.1f; // In normalised screen coordinates, so 0.1 is 5% from the edge

struct RenderItem
{
	RealRect rect;
	real32 depth; // Positive is towards the player
	V4 color;
	uint32 textureRegionID;
};

const int WORLD_SPRITE_MAX = 16384;
const int UI_SPRITE_MAX = 1024;
const int SPRITE_MAX = WORLD_SPRITE_MAX;

struct RenderBuffer
{
	char *name;
	Camera camera;
	RenderItem *items;
	uint32 itemCount;
	uint32 maxItems;
};

struct Renderer
{
	RenderBuffer worldBuffer;
	RenderBuffer uiBuffer;
};

// Animation code should probably be deleted and redone.
#if 0
enum AnimationID
{
	Animation_TempToStopComplaint,
	Animation_Count,
};

struct Animation
{
	TextureAssetType frames[16];
	uint32 frameCount;
};

struct Animator
{
	Animation *animation;
	uint32 currentFrame;
	real32 frameCounter; // Sub-frame ticks
};
const real32 animationFramesPerDay = 10.0f;
#endif

#include "render.cpp"