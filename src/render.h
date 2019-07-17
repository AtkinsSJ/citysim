#pragma once

// General rendering code.

const s32 ITILE_SIZE = 16;
const f32 TILE_SIZE = ITILE_SIZE;
const f32 CAMERA_MARGIN = 1; // How many tiles beyond the map can the camera scroll to show?
const bool canZoom = true;

const f32 SECONDS_PER_FRAME = 1.0f / 60.0f;

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

enum RenderItemType
{
	RenderItemType_NextMemoryChunk,
	RenderItemType_DrawThing, // @Cleanup DEPRECATED!!!

	RenderItemType_DrawSingleRect,
	RenderItemType_DrawRects,
};

// @Cleanup DEPRECATED!!!
struct RenderItem_DrawThing
{
	Rect2 rect;
	f32 depth; // Positive is towards the player
	V4 color;
	s32 shaderID;

	Asset *texture;
	Rect2 uv; // in (0 to 1) space
};

struct RenderItem_DrawSingleRect
{
	Rect2 bounds;
	V4 color;
	s32 shaderID;

	Asset *texture;
	Rect2 uv; // in (0 to 1) space
};

const s32 maxRenderItemsPerGroup = 255;
struct RenderItem_DrawRects
{
	s32 count;
	s32 shaderID;
	Asset *texture;
};
struct RenderItem_DrawRects_Item
{
	Rect2 bounds;
	V4 color;
	Rect2 uv;
};

struct DrawRectsSubGroup
{
	RenderItem_DrawRects *header;
	RenderItem_DrawRects_Item *first;
	s32 maxCount;

	DrawRectsSubGroup *prev;
	DrawRectsSubGroup *next;
};

// Think of this like a "handle". It has data inside but you shouldn't touch it from user code!
struct DrawRectsGroup
{
	RenderBuffer *buffer;

	DrawRectsSubGroup firstSubGroup;
	DrawRectsSubGroup *currentSubGroup;

	s32 count;
	s32 maxCount;

	Asset *texture;
	s32 shaderID;
};

struct RenderBufferChunk
{
	smm size;
	smm used;
	u8 *memory;

	RenderBufferChunk *next;
};

struct RenderBuffer
{
	String name;
	Camera camera;
	
	MemoryArena *arena;
	RenderBufferChunk firstChunk;
	RenderBufferChunk *currentChunk;
	RenderBufferChunk *firstFreeChunk;
	smm minimumChunkSize;

	bool hasRangeReserved;
	smm reservedRangeSize;
};

struct Renderer
{
	MemoryArena renderArena;
	
	SDL_Window *window;

	RenderBuffer worldBuffer;
	RenderBuffer worldOverlayBuffer;
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
void freeRenderer(Renderer *renderer);

void initRenderBuffer(MemoryArena *arena, RenderBuffer *buffer, char *name, smm initialSize);

void initCamera(Camera *camera, V2 size, f32 nearClippingPlane, f32 farClippingPlane, V2 position = v2(0,0));
void updateCameraMatrix(Camera *camera);
V2 unproject(Camera *camera, V2 screenPos);

void makeRenderItem(RenderItem_DrawThing *result, Rect2 rect, f32 depth, Asset *texture, Rect2 uv, s32 shaderID, V4 color=makeWhite());
void drawRenderItem(RenderBuffer *buffer, RenderItem_DrawThing *item);

u8* appendRenderItemInternal(RenderBuffer *buffer, RenderItemType type, smm size, smm reservedSize);
inline RenderItem_DrawThing *appendRenderItem(RenderBuffer *buffer)
{
	return (RenderItem_DrawThing *)appendRenderItemInternal(buffer, RenderItemType_DrawThing, sizeof(RenderItem_DrawThing), 0);
}

inline void drawRect(RenderItem_DrawThing *renderItem, Rect2 rect, f32 depth, s32 shaderID, V4 color)
{
	makeRenderItem(renderItem, rect, depth, null, {}, shaderID, color);
}
inline void drawRect(RenderBuffer *buffer, Rect2 rect, f32 depth, s32 shaderID, V4 color)
{
	drawRect(appendRenderItem(buffer), rect, depth, shaderID, color);
}

void drawSingleSprite(RenderBuffer *buffer, s32 shaderID, Sprite *sprite, Rect2 bounds, V4 color);
void drawSingleRect(RenderBuffer *buffer, s32 shaderID, Rect2 bounds, V4 color);

// NB: The Rects drawn must all have the same Texture!
DrawRectsGroup *beginRectsGroup(RenderBuffer *buffer, s32 shaderID, Asset *texture, s32 maxCount);
DrawRectsGroup *beginRectsGroup(RenderBuffer *buffer, s32 shaderID, s32 maxCount);
DrawRectsGroup *beginRectsGroupForText(RenderBuffer *buffer, s32 shaderID, BitmapFont *font, s32 maxCount);
void addRectInternal(DrawRectsGroup *group, Rect2 bounds, V4 color, Rect2 uv);
void addGlyphRect(DrawRectsGroup *state, BitmapFontGlyph *glyph, V2 position, V4 color);
void addSpriteRect(DrawRectsGroup *state, Sprite *sprite, Rect2 bounds, V4 color);
void addUntexturedRect(DrawRectsGroup *group, Rect2 bounds, V4 color);
void offsetRange(DrawRectsGroup *state, s32 startIndex, s32 endIndexInclusive, f32 offsetX, f32 offsetY);
void endRectsGroup(DrawRectsGroup *group);

DrawRectsSubGroup beginRectsSubGroup(DrawRectsGroup *group);
void endCurrentSubGroup(DrawRectsGroup *group);

void resizeWindow(Renderer *renderer, s32 w, s32 h, bool fullscreen);
void onWindowResized(Renderer *renderer, s32 w, s32 h);

// TODO: Some kind of switch to determine which renderer we want to load.
#include "render_gl.h"
#define platform_initializeRenderer GL_initializeRenderer
