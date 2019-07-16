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

	RenderItemType_DrawRectangles,
	RenderItemType_DrawText,
	RenderItemType_DrawSprites,
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

struct RenderItem_DrawRectangles
{
	s32 shaderID;
	s32 count;
};
struct RenderItem_DrawRectangles_Item
{
	Rect2 bounds;
	V4 color;
};

struct RenderItem_DrawText
{
	s32 shaderID;
	Asset *texture;
	s32 count;
};
struct RenderItem_DrawText_Item
{
	Rect2 bounds;
	V4 color;
	Rect2 uv;
};

struct RenderItem_DrawSprites
{
	s32 shaderID;
	Asset *texture;
	s32 count;
};
struct RenderItem_DrawSprites_Item
{
	Rect2 bounds;
	V4 color;
	Rect2 uv;
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

inline void drawSprite(RenderBuffer *buffer, Sprite *sprite, Rect2 rect, f32 depth, s32 shaderID, V4 color=makeWhite())
{
	makeRenderItem(appendRenderItem(buffer), rect, depth, sprite->texture, sprite->uv, shaderID, color);
}

struct DrawRectanglesState
{
	RenderBuffer *buffer;

	RenderItem_DrawRectangles *header;
	RenderItem_DrawRectangles_Item *first;
	s32 maxCount;
};
DrawRectanglesState startDrawingRectangles(RenderBuffer *buffer, s32 shaderID, s32 maxCount);
void drawRectangle(DrawRectanglesState *state, Rect2 bounds, V4 color);
void finishRectangles(DrawRectanglesState *state);

struct DrawTextState
{
	RenderBuffer *buffer;

	RenderItem_DrawText *header;
	RenderItem_DrawText_Item *first;
	s32 maxCount;
};
DrawTextState startDrawingText(RenderBuffer *buffer, s32 shaderID, BitmapFont *font, s32 maxCount);
void drawTextItem(DrawTextState *state, BitmapFontGlyph *glyph, V2 position, V4 color);
RenderItem_DrawText_Item *getTextItem(DrawTextState *state, s32 index);
void offsetRange(DrawTextState *state, s32 startIndex, s32 endIndexInclusive, f32 offsetX, f32 offsetY);
void finishText(DrawTextState *state);

struct DrawSpritesState
{
	RenderBuffer *buffer;

	RenderItem_DrawSprites *header;
	RenderItem_DrawSprites_Item *first;
	s32 maxCount;
};
// NB: The Sprites drawn must all have the same Texture! This is asserted in drawSpritesItem().
DrawSpritesState startDrawingSprites(RenderBuffer *buffer, s32 shaderID, s32 maxCount);
void drawSpritesItem(DrawSpritesState *state, Sprite *sprite, Rect2 bounds, V4 color);
void finishSprites(DrawSpritesState *state);

//
// NB: Some operations are massively sped up if we can ensure there is space up front,
// and then just write them with a pointer offset. The downside is we then can't add
// any other RenderItems until the reserved range is finished with.
//
// Usage:
// 		RenderItem_DrawThing *firstItem = reserveRenderItemRange(buffer, itemsToReserve);
// 		... write to firstItem, firstItem + 1, ... firstItem + (itemsToReserve - 1)
// 		finishReservedRenderItemRange(buffer, numberOfItemsWeActuallyAdded);
//
// `numberOfItemsWeActuallyAdded` needs to be <= `itemsToReserve`.
// There are a bunch of asserts and checks to (hopefully) prevent any mistakes when
// using this, but hey, I'm great at inventing exciting new ways to mess up!
//
// - Sam, 24/06/2019
//
RenderItem_DrawThing *reserveRenderItemRange(RenderBuffer *buffer, s32 count);
RenderItem_DrawThing *getItemInRange(RenderItem_DrawThing *first, s32 index)
{
	u8 *start = (u8*)first;
	smm stride = sizeof(RenderItem_DrawThing) + sizeof(RenderItemType);
	return (RenderItem_DrawThing *)(start + (stride * index));
}
void finishReservedRenderItemRange(RenderBuffer *buffer, s32 itemsAdded);

void applyOffsetToRenderItems(RenderItem_DrawThing *firstItem, RenderItem_DrawThing *lastItem, f32 offsetX, f32 offsetY);

void resizeWindow(Renderer *renderer, s32 w, s32 h, bool fullscreen);
void onWindowResized(Renderer *renderer, s32 w, s32 h);

// TODO: Some kind of switch to determine which renderer we want to load.
#include "render_gl.h"
#define platform_initializeRenderer GL_initializeRenderer
