/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "bmfont.h"
#include "font.h"
#include "types.h"
#include <Assets/Forward.h>
#include <SDL2/SDL_events.h>
#include <Util/Basic.h>
#include <Util/Matrix4.h>
#include <Util/MemoryArena.h>
#include <Util/Pool.h>
#include <Util/Rectangle.h>
#include <Util/String.h>
#include <Util/Vector.h>

struct RenderBuffer;

// General rendering code.

f32 const SECONDS_PER_FRAME = 1.0f / 60.0f;

struct Camera {
    V2 pos;        // Centre of camera, in camera units
    V2 size;       // Size of camera, in camera units
    f32 sizeRatio; // Size of window is multiplied by this to produce the camera's size
    f32 zoom;      // 1 = normal, 2 = things appear twice their size, etc.
    Matrix4 projectionMatrix;

    // NB: We don't use depth anywhere any more, but these do get used in generating the
    // projection matrix. - Sam, 26/07/2019
    f32 nearClippingPlane;
    f32 farClippingPlane;

    V2 mousePos;
};

enum RenderItemType {
    RenderItemType_NextMemoryChunk,

    RenderItemType_SetCamera,
    RenderItemType_SetPalette,
    RenderItemType_SetShader,
    RenderItemType_SetTexture,

    RenderItemType_Clear,

    RenderItemType_BeginScissor,
    RenderItemType_EndScissor,

    RenderItemType_DrawSingleRect,
    RenderItemType_DrawRects,
    RenderItemType_DrawRings,
};

struct RenderItem_SetCamera {
    Camera* camera;
};

struct RenderItem_SetPalette {
    s32 paletteSize;
    // Palette as a series of V4s follows this struct
};

struct RenderItem_SetShader {
    s8 shaderID;
};

struct RenderItem_SetTexture {
    Asset* texture;

    // These are only set for "raw" textures, AKA ones that have the data inline.
    // The pixel data follows directly after this struct
    s32 width;
    s32 height;
    u8 bytesPerPixel;
};

struct RenderItem_Clear {
    V4 clearColor;
};

struct RenderItem_BeginScissor {
    Rect2I bounds;
};
struct RenderItem_EndScissor {
};

struct RenderItem_DrawSingleRect {
    Rect2 bounds;
    V4 color00;
    V4 color01;
    V4 color10;
    V4 color11;
    Rect2 uv; // in (0 to 1) space
};

struct DrawRectPlaceholder {
    RenderItem_SetTexture* setTexture;
    RenderItem_DrawSingleRect* drawRect;
};

u8 const maxRenderItemsPerGroup = u8Max;
struct RenderItem_DrawRects {
    u8 count;
};
struct RenderItem_DrawRects_Item {
    Rect2 bounds;
    V4 color;
    Rect2 uv;
};

struct DrawRectsSubGroup {
    RenderItem_DrawRects* header;
    RenderItem_DrawRects_Item* first;
    s32 maxCount;

    DrawRectsSubGroup* prev;
    DrawRectsSubGroup* next;
};

// Think of this like a "handle". It has data inside but you shouldn't touch it from user code!
struct DrawRectsGroup {
    RenderBuffer* buffer;

    DrawRectsSubGroup firstSubGroup;
    DrawRectsSubGroup* currentSubGroup;

    s32 count;
    s32 maxCount;

    Asset* texture;
};

struct RenderItem_DrawRings {
    u8 count;
};
struct RenderItem_DrawRings_Item {
    V2 centre;
    f32 radius;
    f32 thickness;
    V4 color;
};

struct DrawRingsSubGroup {
    RenderItem_DrawRings* header;
    RenderItem_DrawRings_Item* first;
    s32 maxCount;

    DrawRingsSubGroup* prev;
    DrawRingsSubGroup* next;
};

// Think of this like a "handle". It has data inside but you shouldn't touch it from user code!
struct DrawRingsGroup {
    RenderBuffer* buffer;

    DrawRingsSubGroup firstSubGroup;
    DrawRingsSubGroup* currentSubGroup;

    s32 count;
    s32 maxCount;
};

struct RenderBufferChunk : PoolItem {
    smm size;
    smm used;
    u8* memory;

    // TODO: @Size: Could potentially re-use the pointers from the PoolItem, if we wanted.
    RenderBufferChunk* prevChunk;
    RenderBufferChunk* nextChunk;
};

struct RenderBuffer : PoolItem {
    String name;

    RenderBufferChunk* firstChunk;
    RenderBufferChunk* currentChunk;
    Pool<RenderBufferChunk>* chunkPool;

    // Transient stuff
    Camera* currentCamera;
    bool hasRangeReserved;
    smm reservedRangeSize;
    s32 scissorCount;

    s8 currentShader;
    Asset* currentTexture;
};

struct Renderer {
    virtual ~Renderer() = default;

    static bool initialize(SDL_Window*);

    MemoryArena renderArena {};

    SDL_Window* window { nullptr };
    s32 windowWidth { 0 };
    s32 windowHeight { 0 };
    bool isFullscreen { false };

    Camera worldCamera {};
    Camera uiCamera {};

    // Cursor stuff
    String currentCursorName {};
    bool cursorIsVisible { true };
    SDL_Cursor* systemWaitCursor { nullptr };

    Pool<RenderBuffer> renderBufferPool {};
    Array<RenderBuffer*> renderBuffers {};
    RenderBuffer worldBuffer {};
    RenderBuffer worldOverlayBuffer {};
    RenderBuffer uiBuffer {};
    RenderBuffer windowBuffer {};
    RenderBuffer debugBuffer {};

    smm renderBufferChunkSize { 0 };
    Pool<RenderBufferChunk> chunkPool {};

    // Not convinced this is the best way of doing it, but it's better than what we had before!
    // Really, we do want to have this stuff in code, because it's accessed a LOT and we don't
    // want to be doing a million hashtable lookups all the time. It does feel like a hack, but
    // practicality is more important than perfection!
    // - Sam, 23/07/2019
    struct
    {
        s8 paletted { 0 };
        s8 pixelArt { 0 };
        s8 text { 0 };
        s8 textured { 0 };
        s8 untextured { 0 };
    } shaderIds;

    virtual void on_window_resized(s32 width, s32 height) = 0;
    virtual void render(Array<RenderBuffer*>) = 0;
    virtual void load_assets() = 0;
    virtual void unload_assets() = 0;
    virtual void free() = 0;

    void set_cursor_visible(bool);

protected:
    explicit Renderer(SDL_Window*);
};

Renderer* the_renderer();
void handleWindowEvent(SDL_WindowEvent* event);
void render();
void rendererLoadAssets();
void rendererUnloadAssets();
void freeRenderer();

void initRenderBuffer(MemoryArena* arena, RenderBuffer* buffer, String name, Pool<RenderBufferChunk>* chunkPool);
RenderBufferChunk* allocateRenderBufferChunk(MemoryArena* arena, void* userData);
void clearRenderBuffer(RenderBuffer* buffer);

RenderBuffer* getTemporaryRenderBuffer(String name);
void transferRenderBufferData(RenderBuffer* buffer, RenderBuffer* targetBuffer);
void returnTemporaryRenderBuffer(RenderBuffer* buffer);

void initCamera(Camera* camera, V2 size, f32 sizeRatio, f32 nearClippingPlane, f32 farClippingPlane, V2 position = v2(0, 0));
void updateCameraMatrix(Camera* camera);
V2 unproject(Camera* camera, V2 screenPos);
void setCameraPos(Camera* camera, V2 position, f32 zoom);
// rounding the zoom so it doesn't gradually drift due to float imprecision
f32 snapZoomLevel(f32 zoom);

void setCursor(String cursorName);

void appendRenderItemType(RenderBuffer* buffer, RenderItemType type);

u8* appendRenderItemInternal(RenderBuffer* buffer, RenderItemType type, smm size, smm reservedSize);

template<typename T>
T* appendRenderItem(RenderBuffer* buffer, RenderItemType type)
{
    u8* data = appendRenderItemInternal(buffer, type, sizeof(T), 0);
    return (T*)data;
}

template<typename T>
struct RenderItemAndData {
    T* item;
    u8* data;
};
template<typename T>
RenderItemAndData<T> appendRenderItem(RenderBuffer* buffer, RenderItemType type, smm dataSize)
{
    RenderItemAndData<T> result = {};

    u8* bufferData = appendRenderItemInternal(buffer, type, sizeof(T) + dataSize, 0);
    result.item = (T*)bufferData;
    result.data = bufferData + sizeof(T);

    return result;
}

template<typename T>
T* readRenderItem(RenderBufferChunk* renderBufferChunk, smm* pos)
{
    T* item = (T*)(renderBufferChunk->memory + *pos);
    *pos += sizeof(T);
    return item;
}

template<typename T>
T* readRenderData(RenderBufferChunk* renderBufferChunk, smm* pos, s32 count)
{
    T* item = (T*)(renderBufferChunk->memory + *pos);
    *pos += sizeof(T) * count;
    return item;
}

void addSetCamera(RenderBuffer* buffer, Camera* camera);
void addSetShader(RenderBuffer* buffer, s8 shaderID);
void addSetTexture(RenderBuffer* buffer, Asset* texture);
void addSetTextureRaw(RenderBuffer* buffer, s32 width, s32 height, u8 bytesPerPixel, u8* pixels);

void addClear(RenderBuffer* buffer, V4 clearColor = {});

// Scissors are a stack, so you can nest them. Note that nested scissors don't crop to each other!
// NB: bounds rectangle is in window pixels, with the origin at the top-left
void addBeginScissor(RenderBuffer* buffer, Rect2I bounds);
void addEndScissor(RenderBuffer* buffer);

void drawSingleSprite(RenderBuffer* buffer, Sprite* sprite, Rect2 bounds, s8 shaderID, V4 color);
void drawSingleRect(RenderBuffer* buffer, Rect2 bounds, s8 shaderID, V4 color);
void drawSingleRect(RenderBuffer* buffer, Rect2I bounds, s8 shaderID, V4 color);
void drawSingleRect(RenderBuffer* buffer, Rect2 bounds, s8 shaderID, V4 color00, V4 color01, V4 color10, V4 color11);
void drawSingleRect(RenderBuffer* buffer, Rect2I bounds, s8 shaderID, V4 color00, V4 color01, V4 color10, V4 color11);

// For when you want something to appear NOW in the render-order, but you don't know its details until later
DrawRectPlaceholder appendDrawRectPlaceholder(RenderBuffer* buffer, s8 shaderID, bool hasTexture);
void fillDrawRectPlaceholder(DrawRectPlaceholder* placeholder, Rect2 bounds, V4 color);
void fillDrawRectPlaceholder(DrawRectPlaceholder* placeholder, Rect2I bounds, V4 color);
void fillDrawRectPlaceholder(DrawRectPlaceholder* placeholder, Rect2 bounds, V4 color00, V4 color01, V4 color10, V4 color11);
void fillDrawRectPlaceholder(DrawRectPlaceholder* placeholder, Rect2I bounds, V4 color00, V4 color01, V4 color10, V4 color11);
void fillDrawRectPlaceholder(DrawRectPlaceholder* placeholder, Rect2 bounds, Sprite* sprite, V4 color);

struct DrawNinepatchPlaceholder {
    RenderItem_DrawRects_Item* firstRect;
};
void drawNinepatch(RenderBuffer* buffer, Rect2I bounds, s8 shaderID, Ninepatch* ninepatch, V4 color = makeWhite());
DrawNinepatchPlaceholder appendDrawNinepatchPlaceholder(RenderBuffer* buffer, Asset* texture, s8 shaderID);
void fillDrawNinepatchPlaceholder(DrawNinepatchPlaceholder* placeholder, Rect2I bounds, Ninepatch* ninepatch, V4 color = makeWhite());

// NB: The Rects drawn must all have the same Texture!
DrawRectsGroup* beginRectsGroupInternal(RenderBuffer* buffer, Asset* texture, s8 shaderID, s32 maxCount);
// TODO: Have the shaderID default to a sensible value for these like beginRectsGroupForText
DrawRectsGroup* beginRectsGroupTextured(RenderBuffer* buffer, Asset* texture, s8 shaderID, s32 maxCount);
DrawRectsGroup* beginRectsGroupUntextured(RenderBuffer* buffer, s8 shaderID, s32 maxCount);
// TODO: Have the shaderID be last and default to the standard text shader, so I don't have to always pass it
DrawRectsGroup* beginRectsGroupForText(RenderBuffer* buffer, BitmapFont* font, s8 shaderID, s32 maxCount);

// Makes space for `count` rects, and returns a pointer to the first one, so you can modify them later.
// Guarantees that they're contiguous, but that means that there must be room for `count` items in the
// group's current sub-group.
// Basically, call this before adding anything, with count <= 255, and you'll be fine.
RenderItem_DrawRects_Item* reserve(DrawRectsGroup* group, s32 count);
void addRectInternal(DrawRectsGroup* group, Rect2 bounds, V4 color, Rect2 uv);
void addGlyphRect(DrawRectsGroup* state, BitmapFontGlyph* glyph, V2 position, V4 color);
void addSpriteRect(DrawRectsGroup* state, Sprite* sprite, Rect2 bounds, V4 color);
void addUntexturedRect(DrawRectsGroup* group, Rect2 bounds, V4 color);
void offsetRange(DrawRectsGroup* state, s32 startIndex, s32 endIndexInclusive, f32 offsetX, f32 offsetY);
void endRectsGroup(DrawRectsGroup* group);

DrawRectsSubGroup beginRectsSubGroup(DrawRectsGroup* group);
void endCurrentSubGroup(DrawRectsGroup* group);

void drawGrid(RenderBuffer* buffer, Rect2 bounds, s32 gridW, s32 gridH, u8* grid, u16 paletteSize, V4* palette);

DrawRingsGroup* beginRingsGroup(RenderBuffer* buffer, s32 maxCount);
DrawRingsSubGroup beginRingsSubGroup(DrawRingsGroup* group);
void addRing(DrawRingsGroup* group, V2 centre, f32 radius, f32 thickness, V4 color);
void endRingsGroup(DrawRingsGroup* group);

void resizeWindow(s32 w, s32 h, bool fullscreen);
