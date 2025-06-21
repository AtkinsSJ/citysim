/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "debug.h"
#include <Assets/AssetManager.h>
#include <Gfx/OpenGL.h>
#include <Gfx/Renderer.h>

static Renderer* s_renderer;

f32 snapZoomLevel(f32 zoom)
{
    return (f32)clamp(round_f32(10 * zoom) * 0.1f, 0.1f, 10.0f);
}

Renderer::Renderer(SDL_Window* window)
    : window(window)
{
    if (!initMemoryArena(&renderArena, "renderArena"_s, 0, MB(1))) {
        logCritical("Failed to create renderer arena!"_s);
        ASSERT(false);
    }

    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    renderBufferChunkSize = KB(64);
    initPool<RenderBufferChunk>(&chunkPool, &renderArena, &allocateRenderBufferChunk, this);

    initPool<RenderBuffer>(&renderBufferPool, &renderArena, [](MemoryArena* arena, void* chunk_pool_pointer) -> RenderBuffer* {
			RenderBuffer *buffer = allocateStruct<RenderBuffer>(arena);
			initRenderBuffer(arena, buffer, nullString, static_cast<Pool<RenderBufferChunk>*>(chunk_pool_pointer));
			return buffer; }, &chunkPool);
    initRenderBuffer(&renderArena, &worldBuffer, "WorldBuffer"_s, &chunkPool);
    initRenderBuffer(&renderArena, &worldOverlayBuffer, "WorldOverlayBuffer"_s, &chunkPool);
    initRenderBuffer(&renderArena, &uiBuffer, "UIBuffer"_s, &chunkPool);
    initRenderBuffer(&renderArena, &windowBuffer, "WindowBuffer"_s, &chunkPool);
    initRenderBuffer(&renderArena, &debugBuffer, "DebugBuffer"_s, &chunkPool);

    renderBuffers = allocateArray<RenderBuffer*>(&renderArena, 5);
    renderBuffers.append(&worldBuffer);
    renderBuffers.append(&worldOverlayBuffer);
    renderBuffers.append(&uiBuffer);
    renderBuffers.append(&windowBuffer);
    renderBuffers.append(&debugBuffer);

    // Init cameras
    V2 windowSize = v2(windowWidth, windowHeight);
    f32 const TILE_SIZE = 16.0f;
    worldCamera = Camera(windowSize, 1.0f / TILE_SIZE, 10000.0f, -10000.0f);
    uiCamera = Camera(windowSize, 1.0f, 10000.0f, -10000.0f, windowSize * 0.5f);

    // Hide cursor until stuff loads
    set_cursor_visible(false);
    markResetPosition(&renderArena);
}

bool Renderer::initialize(SDL_Window* window)
{
    // TODO: Potentially support other renderers.
    auto* gl_renderer = new GL::Renderer(window);

    if (!gl_renderer->set_up_context()) {
        logCritical("Failed to create OpenGL context!"_s);
        delete gl_renderer;
        return false;
    }

    markResetPosition(&gl_renderer->renderArena);

    s_renderer = gl_renderer;
    return true;
}

Renderer* the_renderer()
{
    return s_renderer;
}

void handleWindowEvent(SDL_WindowEvent* event)
{
    switch (event->event) {
    case SDL_WINDOWEVENT_SIZE_CHANGED: {
        s_renderer->windowWidth = event->data1;
        s_renderer->windowHeight = event->data2;

        s_renderer->on_window_resized(s_renderer->windowWidth, s_renderer->windowHeight);

        V2 windowSize = v2(s_renderer->windowWidth, s_renderer->windowHeight);

        s_renderer->worldCamera.set_size(windowSize);

        s_renderer->uiCamera.set_size(windowSize);
        s_renderer->uiCamera.set_position(s_renderer->uiCamera.size() * 0.5f);
    } break;
    }
}

void render()
{
    DEBUG_POOL(&s_renderer->renderBufferPool, "renderBufferPool");
    DEBUG_POOL(&s_renderer->chunkPool, "renderChunkPool");

    s_renderer->render(s_renderer->renderBuffers);

    for (s32 i = 0; i < s_renderer->renderBuffers.count; i++) {
        clearRenderBuffer(s_renderer->renderBuffers[i]);
    }
}

void rendererLoadAssets()
{
    s_renderer->load_assets();

    // Cache the shader IDs so we don't have to do so many hash lookups
    s_renderer->shaderIds.pixelArt = getShader("pixelart.glsl"_s)->rendererShaderID;
    s_renderer->shaderIds.text = getShader("textured.glsl"_s)->rendererShaderID;
    s_renderer->shaderIds.textured = getShader("textured.glsl"_s)->rendererShaderID;
    s_renderer->shaderIds.untextured = getShader("untextured.glsl"_s)->rendererShaderID;

    if (!isEmpty(s_renderer->currentCursorName)) {
        setCursor(s_renderer->currentCursorName);
    }
}

void rendererUnloadAssets()
{
    if (s_renderer->systemWaitCursor == nullptr) {
        s_renderer->systemWaitCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
    }
    SDL_SetCursor(s_renderer->systemWaitCursor);
    s_renderer->unload_assets();
}

void freeRenderer()
{
    // FIXME: Put this in destructor
    if (s_renderer->systemWaitCursor != nullptr) {
        SDL_FreeCursor(s_renderer->systemWaitCursor);
        s_renderer->systemWaitCursor = nullptr;
    }
    s_renderer->free();
}

void resizeWindow(s32 w, s32 h, bool fullscreen)
{
    SDL_RestoreWindow(s_renderer->window);

    // So, I'm not super sure how we want to handle fullscreen. Do we scan the
    // available resolution options and list them to select from? Do we just use
    // the native resolution? Do we use fullscreen or fullscreen-window?
    //
    // For now, I'm thinking we use the native resolution, as a fullscreen window.
    // - Sam, 20/05/2021

    // If the requested setup is what we already have, do nothing!
    // This avoids the window jumping about when you save the settings
    if ((w == s_renderer->windowWidth)
        && (h == s_renderer->windowHeight)
        && (fullscreen == s_renderer->isFullscreen)) {
        return;
    }

    s32 newW = w;
    s32 newH = h;

    if (fullscreen) {
        // Fullscreen!
        // Grab the desktop resolution to use that
        s32 displayIndex = SDL_GetWindowDisplayIndex(s_renderer->window);
        SDL_DisplayMode displayMode;
        if (SDL_GetDesktopDisplayMode(displayIndex, &displayMode) == 0) {
            SDL_SetWindowDisplayMode(s_renderer->window, &displayMode);
            SDL_SetWindowFullscreen(s_renderer->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            newW = displayMode.w;
            newH = displayMode.h;
        } else {
            logError("Failed to get desktop display mode: {0}"_s, { makeString(SDL_GetError()) });
        }
    } else {
        // Window!
        SDL_SetWindowSize(s_renderer->window, newW, newH);
        SDL_SetWindowFullscreen(s_renderer->window, 0);

        // Centre it
        s32 displayIndex = SDL_GetWindowDisplayIndex(s_renderer->window);
        SDL_Rect displayBounds;
        if (SDL_GetDisplayBounds(displayIndex, &displayBounds) == 0) {
            s32 leftBorder, rightBorder, topBorder, bottomBorder;
            SDL_GetWindowBordersSize(s_renderer->window, &topBorder, &leftBorder, &bottomBorder, &rightBorder);

            s32 windowW = w + leftBorder + rightBorder;
            s32 windowH = h + topBorder + bottomBorder;
            s32 windowLeft = (displayBounds.w - windowW) / 2;
            s32 windowTop = (displayBounds.h - windowH) / 2;

            SDL_SetWindowPosition(s_renderer->window, displayBounds.x + windowLeft, displayBounds.y + windowTop);
        } else {
            logError("Failed to get display bounds: {0}"_s, { makeString(SDL_GetError()) });

            // As a backup, just centre it on the main display
            SDL_SetWindowPosition(s_renderer->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        }
    }

    s_renderer->isFullscreen = fullscreen;
}

void setCursor(String cursorName)
{
    DEBUG_FUNCTION();

    Asset* newCursorAsset = getAsset(AssetType_Cursor, cursorName);
    if (newCursorAsset != nullptr) {
        s_renderer->currentCursorName = cursorName;
        SDL_SetCursor(newCursorAsset->cursor.sdlCursor);
    }
}

void Renderer::set_cursor_visible(bool visible)
{
    cursorIsVisible = visible;
    SDL_ShowCursor(visible ? 1 : 0);
}

void initRenderBuffer(MemoryArena* arena, RenderBuffer* buffer, String name, Pool<RenderBufferChunk>* chunkPool)
{
    *buffer = {};

    buffer->name = pushString(arena, name);
    hashString(&buffer->name);

    buffer->hasRangeReserved = false;
    buffer->scissorCount = 0;

    buffer->chunkPool = chunkPool;

    buffer->firstChunk = nullptr;
    buffer->currentChunk = nullptr;

    buffer->currentShader = -1;
    buffer->currentTexture = nullptr;
}

RenderBufferChunk* allocateRenderBufferChunk(MemoryArena* arena, void* userData)
{
    smm newChunkSize = ((Renderer*)userData)->renderBufferChunkSize;
    RenderBufferChunk* result = (RenderBufferChunk*)allocate(arena, newChunkSize + sizeof(RenderBufferChunk));
    result->size = newChunkSize;
    result->used = 0;
    result->memory = (u8*)(result + 1);

    return result;
}

void clearRenderBuffer(RenderBuffer* buffer)
{
    buffer->currentShader = -1;
    buffer->currentTexture = nullptr;

    for (RenderBufferChunk* chunk = buffer->firstChunk;
        chunk != nullptr;
        chunk = chunk->nextChunk) {
        chunk->used = 0;
        addItemToPool(&s_renderer->chunkPool, chunk);
    }

    buffer->firstChunk = nullptr;
    buffer->currentChunk = nullptr;
}

RenderBuffer* getTemporaryRenderBuffer(String name)
{
    RenderBuffer* result = getItemFromPool(&s_renderer->renderBufferPool);

    clearRenderBuffer(result);

    // We only use this for debugging, and it's a leak, so limit it to debug builds
    if (globalDebugState != nullptr) {
        result->name = pushString(&globalDebugState->debugArena, name);
    }

    return result;
}

void returnTemporaryRenderBuffer(RenderBuffer* buffer)
{
    buffer->name = nullString;
    clearRenderBuffer(buffer);
    addItemToPool(&s_renderer->renderBufferPool, buffer);
}

void transferRenderBufferData(RenderBuffer* buffer, RenderBuffer* targetBuffer)
{
    if (buffer->currentChunk != nullptr) {
        // If the target is empty, we can take a shortcut and just copy the pointers
        if (targetBuffer->currentChunk == nullptr) {
            targetBuffer->firstChunk = buffer->firstChunk;
            targetBuffer->currentChunk = buffer->currentChunk;
        } else {
            ASSERT((targetBuffer->currentChunk->size - targetBuffer->currentChunk->used) > sizeof(RenderItemType)); // Need space for the next-chunk message
            appendRenderItemType(targetBuffer, RenderItemType_NextMemoryChunk);
            targetBuffer->currentChunk->nextChunk = buffer->firstChunk;
            buffer->firstChunk->prevChunk = targetBuffer->currentChunk;
            targetBuffer->currentChunk = buffer->currentChunk;
        }
    }

    buffer->firstChunk = nullptr;
    buffer->currentChunk = nullptr;
    clearRenderBuffer(buffer); // Make sure things are reset
}

void appendRenderItemType(RenderBuffer* buffer, RenderItemType type)
{
    *(RenderItemType*)(buffer->currentChunk->memory + buffer->currentChunk->used) = type;
    buffer->currentChunk->used += sizeof(RenderItemType);
}

// NB: reservedSize is for extra data that you want to make sure there is room for,
// but you're not allocating it right away. Make sure not to do any other allocations
// in between, and make sure to allocate the space you used when you're done!
u8* appendRenderItemInternal(RenderBuffer* buffer, RenderItemType type, smm size, smm reservedSize)
{
    ASSERT(!buffer->hasRangeReserved); // Can't append renderitems while a range is reserved!

    smm totalSizeRequired = (smm)(sizeof(RenderItemType) + size + reservedSize + sizeof(RenderItemType));

    if (buffer->currentChunk == nullptr || ((buffer->currentChunk->size - buffer->currentChunk->used) <= totalSizeRequired)) {
        // Out of room! Push a "go to next chunk" item and append some more memory
        if (buffer->currentChunk != nullptr) {
            ASSERT((buffer->currentChunk->size - buffer->currentChunk->used) > sizeof(RenderItemType)); // Need space for the next-chunk message
            appendRenderItemType(buffer, RenderItemType_NextMemoryChunk);
        }

        RenderBufferChunk* newChunk = getItemFromPool(buffer->chunkPool);
        newChunk->used = 0;
        newChunk->prevChunk = nullptr;
        newChunk->nextChunk = nullptr;

        // Add to the renderbuffer
        if (buffer->currentChunk == nullptr) {
            buffer->firstChunk = newChunk;
            buffer->currentChunk = newChunk;
        } else {
            buffer->currentChunk->nextChunk = newChunk;
            newChunk->prevChunk = buffer->currentChunk;
        }

        buffer->currentChunk = newChunk;
    }

    appendRenderItemType(buffer, type);
    u8* result = (buffer->currentChunk->memory + buffer->currentChunk->used);
    buffer->currentChunk->used += size;
    return result;
}

void addSetCamera(RenderBuffer* buffer, Camera* camera)
{
    RenderItem_SetCamera* cameraItem = appendRenderItem<RenderItem_SetCamera>(buffer, RenderItemType_SetCamera);
    cameraItem->camera = camera;
}

void addSetShader(RenderBuffer* buffer, s8 shaderID)
{
    if (buffer->currentShader != shaderID) {
        RenderItem_SetShader* shaderItem = appendRenderItem<RenderItem_SetShader>(buffer, RenderItemType_SetShader);
        *shaderItem = {};
        shaderItem->shaderID = shaderID;

        buffer->currentShader = shaderID;
        // NB: Setting the opengl shader loses the texture, so we replicate that here.
        // I suppose in the future we could make sure to bind the texture when the shader changes, but
        // most of the time we want to set a new texture anyway, so that would be a waste of cpu time.
        buffer->currentTexture = nullptr;
    }
}

void addSetTexture(RenderBuffer* buffer, Asset* texture)
{
    ASSERT(texture->state == AssetState_Loaded);

    if (buffer->currentTexture != texture) {
        RenderItem_SetTexture* textureItem = appendRenderItem<RenderItem_SetTexture>(buffer, RenderItemType_SetTexture);
        *textureItem = {};
        textureItem->texture = texture;

        buffer->currentTexture = texture;
    }
}

void addSetTextureRaw(RenderBuffer* buffer, s32 width, s32 height, u8 bytesPerPixel, u8* pixels)
{
    buffer->currentTexture = nullptr;

    smm pixelDataSize = (width * height * bytesPerPixel);
    auto itemAndData = appendRenderItem<RenderItem_SetTexture>(buffer, RenderItemType_SetTexture, pixelDataSize);

    RenderItem_SetTexture* textureItem = itemAndData.item;
    *textureItem = {};
    textureItem->texture = nullptr;
    textureItem->width = width;
    textureItem->height = height;
    textureItem->bytesPerPixel = bytesPerPixel;

    copyMemory(pixels, itemAndData.data, pixelDataSize);
}

void addSetPalette(RenderBuffer* buffer, s32 paletteSize, V4* palette)
{
    auto itemAndData = appendRenderItem<RenderItem_SetPalette>(buffer, RenderItemType_SetPalette, sizeof(V4) * paletteSize);

    itemAndData.item->paletteSize = paletteSize;

    copyMemory(palette, (V4*)itemAndData.data, paletteSize);
}

void addClear(RenderBuffer* buffer, V4 clearColor)
{
    RenderItem_Clear* clear = appendRenderItem<RenderItem_Clear>(buffer, RenderItemType_Clear);
    clear->clearColor = clearColor;
}

void addBeginScissor(RenderBuffer* buffer, Rect2I bounds)
{
    ASSERT(bounds.w >= 0);
    ASSERT(bounds.h >= 0);

    RenderItem_BeginScissor* scissor = appendRenderItem<RenderItem_BeginScissor>(buffer, RenderItemType_BeginScissor);

    // We have to flip the bounds rectangle vertically because OpenGL has the origin in the bottom-left,
    // whereas our system uses the top-left!

    bounds.y = s_renderer->windowHeight - bounds.y - bounds.h;

    // Crop to window bounds
    scissor->bounds = intersect(bounds, irectXYWH(0, 0, s_renderer->windowWidth, s_renderer->windowHeight));

    ASSERT(scissor->bounds.w >= 0);
    ASSERT(scissor->bounds.h >= 0);

    buffer->scissorCount++;
}

void addEndScissor(RenderBuffer* buffer)
{
    ASSERT(buffer->scissorCount > 0);
    [[maybe_unused]] RenderItem_EndScissor* scissor = appendRenderItem<RenderItem_EndScissor>(buffer, RenderItemType_EndScissor);
    buffer->scissorCount--;
}

void drawSingleSprite(RenderBuffer* buffer, Sprite* sprite, Rect2 bounds, s8 shaderID, V4 color)
{
    addSetShader(buffer, shaderID);
    addSetTexture(buffer, sprite->texture);

    RenderItem_DrawSingleRect* rect = appendRenderItem<RenderItem_DrawSingleRect>(buffer, RenderItemType_DrawSingleRect);

    rect->bounds = bounds;
    rect->color00 = color;
    rect->color01 = color;
    rect->color10 = color;
    rect->color11 = color;
    rect->uv = sprite->uv;
}

void drawSingleRect(RenderBuffer* buffer, Rect2 bounds, s8 shaderID, V4 color)
{
    drawSingleRect(buffer, bounds, shaderID, color, color, color, color);
}

void drawSingleRect(RenderBuffer* buffer, Rect2I bounds, s8 shaderID, V4 color)
{
    drawSingleRect(buffer, rect2(bounds), shaderID, color, color, color, color);
}

void drawSingleRect(RenderBuffer* buffer, Rect2 bounds, s8 shaderID, V4 color00, V4 color01, V4 color10, V4 color11)
{
    addSetShader(buffer, shaderID);

    RenderItem_DrawSingleRect* rect = appendRenderItem<RenderItem_DrawSingleRect>(buffer, RenderItemType_DrawSingleRect);

    rect->bounds = bounds;
    rect->color00 = color00;
    rect->color01 = color01;
    rect->color10 = color10;
    rect->color11 = color11;
    rect->uv = rectXYWH(0.0f, 0.0f, 1.0f, 1.0f);
}

void drawSingleRect(RenderBuffer* buffer, Rect2I bounds, s8 shaderID, V4 color00, V4 color01, V4 color10, V4 color11)
{
    drawSingleRect(buffer, rect2(bounds), shaderID, color00, color01, color10, color11);
}

DrawRectPlaceholder appendDrawRectPlaceholder(RenderBuffer* buffer, s8 shaderID, bool hasTexture)
{
    addSetShader(buffer, shaderID);

    DrawRectPlaceholder result = {};

    if (hasTexture) {
        RenderItem_SetTexture* textureItem = appendRenderItem<RenderItem_SetTexture>(buffer, RenderItemType_SetTexture);
        *textureItem = {};

        // We need to clear this, because we don't know what this texture will be, so any following draw calls
        // have to assume that they need to set their texture
        buffer->currentTexture = nullptr;

        result.setTexture = textureItem;
    }

    result.drawRect = appendRenderItem<RenderItem_DrawSingleRect>(buffer, RenderItemType_DrawSingleRect);

    return result;
}

void fillDrawRectPlaceholder(DrawRectPlaceholder* placeholder, Rect2 bounds, V4 color)
{
    fillDrawRectPlaceholder(placeholder, bounds, color, color, color, color);
}

void fillDrawRectPlaceholder(DrawRectPlaceholder* placeholder, Rect2I bounds, V4 color)
{
    fillDrawRectPlaceholder(placeholder, rect2(bounds), color, color, color, color);
}

void fillDrawRectPlaceholder(DrawRectPlaceholder* placeholder, Rect2 bounds, V4 color00, V4 color01, V4 color10, V4 color11)
{
    RenderItem_DrawSingleRect* rect = placeholder->drawRect;

    rect->bounds = bounds;
    rect->color00 = color00;
    rect->color01 = color01;
    rect->color10 = color10;
    rect->color11 = color11;
    rect->uv = rectXYWH(0.0f, 0.0f, 1.0f, 1.0f);
}

void fillDrawRectPlaceholder(DrawRectPlaceholder* placeholder, Rect2I bounds, V4 color00, V4 color01, V4 color10, V4 color11)
{
    fillDrawRectPlaceholder(placeholder, rect2(bounds), color00, color01, color10, color11);
}

void fillDrawRectPlaceholder(DrawRectPlaceholder* placeholder, Rect2 bounds, Sprite* sprite, V4 color)
{
    ASSERT(placeholder->setTexture != nullptr);

    placeholder->setTexture->texture = sprite->texture;

    RenderItem_DrawSingleRect* rect = placeholder->drawRect;
    rect->bounds = bounds;
    rect->color00 = color;
    rect->color01 = color;
    rect->color10 = color;
    rect->color11 = color;
    rect->uv = sprite->uv;
}

void drawNinepatch(RenderBuffer* buffer, Rect2I bounds, s8 shaderID, Ninepatch* ninepatch, V4 color)
{
    DrawRectsGroup* group = beginRectsGroupInternal(buffer, ninepatch->texture, shaderID, 9);

    // NB: See comments in the Ninepatch struct for how we could avoid doing the  UV calculations repeatedly,
    // and why we haven't done so.

    f32 x0 = (f32)(bounds.x);
    f32 x1 = (f32)(bounds.x + ninepatch->pu1 - ninepatch->pu0);
    f32 x2 = (f32)(bounds.x + bounds.w - (ninepatch->pu3 - ninepatch->pu2));
    f32 x3 = (f32)(bounds.x + bounds.w);

    f32 y0 = (f32)(bounds.y);
    f32 y1 = (f32)(bounds.y + ninepatch->pv1 - ninepatch->pv0);
    f32 y2 = (f32)(bounds.y + bounds.h - (ninepatch->pv3 - ninepatch->pv2));
    f32 y3 = (f32)(bounds.y + bounds.h);

    // top left
    addRectInternal(group, rectMinMax(x0, y0, x1, y1), color, rectMinMax(ninepatch->u0, ninepatch->v0, ninepatch->u1, ninepatch->v1));

    // top
    addRectInternal(group, rectMinMax(x1, y0, x2, y1), color, rectMinMax(ninepatch->u1, ninepatch->v0, ninepatch->u2, ninepatch->v1));

    // top-right
    addRectInternal(group, rectMinMax(x2, y0, x3, y1), color, rectMinMax(ninepatch->u2, ninepatch->v0, ninepatch->u3, ninepatch->v1));

    // middle left
    addRectInternal(group, rectMinMax(x0, y1, x1, y2), color, rectMinMax(ninepatch->u0, ninepatch->v1, ninepatch->u1, ninepatch->v2));

    // middle
    addRectInternal(group, rectMinMax(x1, y1, x2, y2), color, rectMinMax(ninepatch->u1, ninepatch->v1, ninepatch->u2, ninepatch->v2));

    // middle-right
    addRectInternal(group, rectMinMax(x2, y1, x3, y2), color, rectMinMax(ninepatch->u2, ninepatch->v1, ninepatch->u3, ninepatch->v2));

    // bottom left
    addRectInternal(group, rectMinMax(x0, y2, x1, y3), color, rectMinMax(ninepatch->u0, ninepatch->v2, ninepatch->u1, ninepatch->v3));

    // bottom
    addRectInternal(group, rectMinMax(x1, y2, x2, y3), color, rectMinMax(ninepatch->u1, ninepatch->v2, ninepatch->u2, ninepatch->v3));

    // bottom-right
    addRectInternal(group, rectMinMax(x2, y2, x3, y3), color, rectMinMax(ninepatch->u2, ninepatch->v2, ninepatch->u3, ninepatch->v3));

    endRectsGroup(group);
}

DrawNinepatchPlaceholder appendDrawNinepatchPlaceholder(RenderBuffer* buffer, Asset* texture, s8 shaderID)
{
    DrawNinepatchPlaceholder placeholder = {};

    DrawRectsGroup* rectsGroup = beginRectsGroupTextured(buffer, texture, shaderID, 9);
    placeholder.firstRect = reserve(rectsGroup, 9);
    endRectsGroup(rectsGroup);

    return placeholder;
}

void fillDrawNinepatchPlaceholder(DrawNinepatchPlaceholder* placeholder, Rect2I bounds, Ninepatch* ninepatch, V4 color)
{
    // @Copypasta drawNinepatch()
    // NB: See comments in the Ninepatch struct for how we could avoid doing the  UV calculations repeatedly,
    // and why we haven't done so.

    f32 x0 = (f32)(bounds.x);
    f32 x1 = (f32)(bounds.x + ninepatch->pu1 - ninepatch->pu0);
    f32 x2 = (f32)(bounds.x + bounds.w - (ninepatch->pu3 - ninepatch->pu2));
    f32 x3 = (f32)(bounds.x + bounds.w);

    f32 y0 = (f32)(bounds.y);
    f32 y1 = (f32)(bounds.y + ninepatch->pv1 - ninepatch->pv0);
    f32 y2 = (f32)(bounds.y + bounds.h - (ninepatch->pv3 - ninepatch->pv2));
    f32 y3 = (f32)(bounds.y + bounds.h);

    RenderItem_DrawRects_Item* rect = placeholder->firstRect;

    // top left
    rect->bounds = rectMinMax(x0, y0, x1, y1);
    rect->color = color;
    rect->uv = rectMinMax(ninepatch->u0, ninepatch->v0, ninepatch->u1, ninepatch->v1);
    rect++;

    // top
    rect->bounds = rectMinMax(x1, y0, x2, y1);
    rect->color = color;
    rect->uv = rectMinMax(ninepatch->u1, ninepatch->v0, ninepatch->u2, ninepatch->v1);
    rect++;

    // top-right
    rect->bounds = rectMinMax(x2, y0, x3, y1);
    rect->color = color;
    rect->uv = rectMinMax(ninepatch->u2, ninepatch->v0, ninepatch->u3, ninepatch->v1);
    rect++;

    // middle left
    rect->bounds = rectMinMax(x0, y1, x1, y2);
    rect->color = color;
    rect->uv = rectMinMax(ninepatch->u0, ninepatch->v1, ninepatch->u1, ninepatch->v2);
    rect++;

    // middle
    rect->bounds = rectMinMax(x1, y1, x2, y2);
    rect->color = color;
    rect->uv = rectMinMax(ninepatch->u1, ninepatch->v1, ninepatch->u2, ninepatch->v2);
    rect++;

    // middle-right
    rect->bounds = rectMinMax(x2, y1, x3, y2);
    rect->color = color;
    rect->uv = rectMinMax(ninepatch->u2, ninepatch->v1, ninepatch->u3, ninepatch->v2);
    rect++;

    // bottom left
    rect->bounds = rectMinMax(x0, y2, x1, y3);
    rect->color = color;
    rect->uv = rectMinMax(ninepatch->u0, ninepatch->v2, ninepatch->u1, ninepatch->v3);
    rect++;

    // bottom
    rect->bounds = rectMinMax(x1, y2, x2, y3);
    rect->color = color;
    rect->uv = rectMinMax(ninepatch->u1, ninepatch->v2, ninepatch->u2, ninepatch->v3);
    rect++;

    // bottom-right
    rect->bounds = rectMinMax(x2, y2, x3, y3);
    rect->color = color;
    rect->uv = rectMinMax(ninepatch->u2, ninepatch->v2, ninepatch->u3, ninepatch->v3);
}

DrawRectsGroup* beginRectsGroupInternal(RenderBuffer* buffer, Asset* texture, s8 shaderID, s32 maxCount)
{
    addSetShader(buffer, shaderID);
    if (texture != nullptr)
        addSetTexture(buffer, texture);

    DrawRectsGroup* result = allocateStruct<DrawRectsGroup>(&temp_arena());
    *result = {};

    result->buffer = buffer;
    result->count = 0;
    result->maxCount = maxCount;
    result->texture = texture;

    result->firstSubGroup = beginRectsSubGroup(result);
    result->currentSubGroup = &result->firstSubGroup;

    return result;
}

DrawRectsGroup* beginRectsGroupTextured(RenderBuffer* buffer, Asset* texture, s8 shaderID, s32 maxCount)
{
    return beginRectsGroupInternal(buffer, texture, shaderID, maxCount);
}

DrawRectsGroup* beginRectsGroupUntextured(RenderBuffer* buffer, s8 shaderID, s32 maxCount)
{
    return beginRectsGroupInternal(buffer, nullptr, shaderID, maxCount);
}

DrawRectsGroup* beginRectsGroupForText(RenderBuffer* buffer, BitmapFont* font, s8 shaderID, s32 maxCount)
{
    return beginRectsGroupInternal(buffer, font->texture, shaderID, maxCount);
}

DrawRectsSubGroup beginRectsSubGroup(DrawRectsGroup* group)
{
    DrawRectsSubGroup result = {};

    s32 subGroupItemCount = min(group->maxCount - group->count, (s32)maxRenderItemsPerGroup);
    ASSERT(subGroupItemCount > 0 && subGroupItemCount <= maxRenderItemsPerGroup);

    smm reservedSize = sizeof(RenderItem_DrawRects_Item) * subGroupItemCount;
    u8* data = appendRenderItemInternal(group->buffer, RenderItemType_DrawRects, sizeof(RenderItem_DrawRects), reservedSize);
    group->buffer->hasRangeReserved = true;

    result.header = (RenderItem_DrawRects*)data;
    result.first = (RenderItem_DrawRects_Item*)(data + sizeof(RenderItem_DrawRects));
    result.maxCount = subGroupItemCount;

    *result.header = {};
    result.header->count = 0;

    return result;
}

void endCurrentSubGroup(DrawRectsGroup* group)
{
    ASSERT(group->buffer->hasRangeReserved); // Attempted to finish a range while a range is not reserved!
    group->buffer->hasRangeReserved = false;

    group->buffer->currentChunk->used += group->currentSubGroup->header->count * sizeof(RenderItem_DrawRects_Item);
}

RenderItem_DrawRects_Item* reserve(DrawRectsGroup* group, s32 count)
{
    ASSERT((group->currentSubGroup->header->count + count) <= group->currentSubGroup->maxCount);

    RenderItem_DrawRects_Item* result = group->currentSubGroup->first + group->currentSubGroup->header->count;

    group->currentSubGroup->header->count = (u8)(group->currentSubGroup->header->count + count);
    group->count += count;

    return result;
}

void addRectInternal(DrawRectsGroup* group, Rect2 bounds, V4 color, Rect2 uv)
{
    ASSERT(group->count < group->maxCount);

    if (group->currentSubGroup->header->count == group->currentSubGroup->maxCount) {
        endCurrentSubGroup(group);
        DrawRectsSubGroup* prevSubGroup = group->currentSubGroup;
        group->currentSubGroup = allocateStruct<DrawRectsSubGroup>(&temp_arena());
        *group->currentSubGroup = beginRectsSubGroup(group);
        prevSubGroup->next = group->currentSubGroup;
        group->currentSubGroup->prev = prevSubGroup;
    }
    ASSERT(group->currentSubGroup->header->count < group->currentSubGroup->maxCount);

    RenderItem_DrawRects_Item* item = group->currentSubGroup->first + group->currentSubGroup->header->count++;
    item->bounds = bounds;
    item->color = color;
    item->uv = uv;

    group->count++;
}

void addUntexturedRect(DrawRectsGroup* group, Rect2 bounds, V4 color)
{
    addRectInternal(group, bounds, color, rectXYWH(0, 0, 0, 0));
}

void addGlyphRect(DrawRectsGroup* group, BitmapFontGlyph* glyph, V2 position, V4 color)
{
    Rect2 bounds = rectXYWH(position.x + glyph->xOffset, position.y + glyph->yOffset, glyph->width, glyph->height);
    addRectInternal(group, bounds, color, glyph->uv);
}

void addSpriteRect(DrawRectsGroup* group, Sprite* sprite, Rect2 bounds, V4 color)
{
    ASSERT(group->texture == sprite->texture);

    addRectInternal(group, bounds, color, sprite->uv);
}

void offsetRange(DrawRectsGroup* group, s32 startIndex, s32 endIndexInclusive, f32 offsetX, f32 offsetY)
{
    DEBUG_FUNCTION_T(DCDT_Renderer);

    ASSERT(startIndex >= 0 && startIndex < group->count);
    ASSERT(endIndexInclusive >= 0 && endIndexInclusive < group->count);
    ASSERT(startIndex <= endIndexInclusive);

    s32 debugItemsUpdated = 0;

    DrawRectsSubGroup* subGroup = &group->firstSubGroup;
    s32 firstIndexInSubGroup = 0;
    // Find the first subgroup that's involved
    while (firstIndexInSubGroup + subGroup->header->count < startIndex) {
        firstIndexInSubGroup += subGroup->header->count;
        subGroup = subGroup->next;
    }

    // Update the items in the range
    while (firstIndexInSubGroup <= endIndexInclusive) {
        s32 index = startIndex - firstIndexInSubGroup;
        if (index < 0)
            index = 0;
        for (;
            (index <= endIndexInclusive - firstIndexInSubGroup) && (index < subGroup->header->count);
            index++) {
            RenderItem_DrawRects_Item* item = subGroup->first + index;
            item->bounds.x += offsetX;
            item->bounds.y += offsetY;
            debugItemsUpdated++;
        }

        firstIndexInSubGroup += subGroup->header->count;
        subGroup = subGroup->next;
    }

    ASSERT(debugItemsUpdated == (endIndexInclusive - startIndex) + 1);
}

void endRectsGroup(DrawRectsGroup* group)
{
    endCurrentSubGroup(group);
}

void drawGrid(RenderBuffer* buffer, Rect2 bounds, s32 gridW, s32 gridH, u8* grid, u16 paletteSize, V4* palette)
{
    DEBUG_FUNCTION_T(DCDT_Renderer);

    addSetShader(buffer, s_renderer->shaderIds.paletted);

    addSetTextureRaw(buffer, gridW, gridH, 1, grid);
    addSetPalette(buffer, paletteSize, palette);

    RenderItem_DrawSingleRect* rect = appendRenderItem<RenderItem_DrawSingleRect>(buffer, RenderItemType_DrawSingleRect);
    rect->bounds = bounds;
    rect->color00 = makeWhite();
    rect->color01 = makeWhite();
    rect->color10 = makeWhite();
    rect->color11 = makeWhite();
    rect->uv = rectXYWH(0, 0, 1, 1);
}

DrawRingsGroup* beginRingsGroup(RenderBuffer* buffer, s32 maxCount)
{
    // @Copypasta beginRectsGroupInternal() - maybe factor out common "grouped render items" code?
    addSetShader(buffer, s_renderer->shaderIds.untextured);

    DrawRingsGroup* result = allocateStruct<DrawRingsGroup>(&temp_arena());
    *result = {};

    result->buffer = buffer;
    result->count = 0;
    result->maxCount = maxCount;

    result->firstSubGroup = beginRingsSubGroup(result);
    result->currentSubGroup = &result->firstSubGroup;

    return result;
}

DrawRingsSubGroup beginRingsSubGroup(DrawRingsGroup* group)
{
    // @Copypasta beginRectsSubGroup() - maybe factor out common "grouped render items" code?
    DrawRingsSubGroup result = {};

    s32 subGroupItemCount = min(group->maxCount - group->count, (s32)maxRenderItemsPerGroup);
    ASSERT(subGroupItemCount > 0 && subGroupItemCount <= maxRenderItemsPerGroup);

    smm reservedSize = sizeof(RenderItem_DrawRings_Item) * subGroupItemCount;
    u8* data = appendRenderItemInternal(group->buffer, RenderItemType_DrawRings, sizeof(RenderItem_DrawRings), reservedSize);
    group->buffer->hasRangeReserved = true;

    result.header = (RenderItem_DrawRings*)data;
    result.first = (RenderItem_DrawRings_Item*)(data + sizeof(RenderItem_DrawRings));
    result.maxCount = subGroupItemCount;

    *result.header = {};
    result.header->count = 0;

    return result;
}

void endCurrentSubGroup(DrawRingsGroup* group)
{
    // @Copypasta endCurrentSubGroup(DrawRectsGroup*) - maybe factor out common "grouped render items" code?
    ASSERT(group->buffer->hasRangeReserved); // Attempted to finish a range while a range is not reserved!
    group->buffer->hasRangeReserved = false;

    group->buffer->currentChunk->used += group->currentSubGroup->header->count * sizeof(RenderItem_DrawRings_Item);
}

void addRing(DrawRingsGroup* group, V2 centre, f32 radius, f32 thickness, V4 color)
{
    // @Copypasta addRectInternal() - maybe factor out common "grouped render items" code?
    ASSERT(group->count < group->maxCount);

    if (group->currentSubGroup->header->count == group->currentSubGroup->maxCount) {
        endCurrentSubGroup(group);
        DrawRingsSubGroup* prevSubGroup = group->currentSubGroup;
        group->currentSubGroup = allocateStruct<DrawRingsSubGroup>(&temp_arena());
        *group->currentSubGroup = beginRingsSubGroup(group);
        prevSubGroup->next = group->currentSubGroup;
        group->currentSubGroup->prev = prevSubGroup;
    }
    ASSERT(group->currentSubGroup->header->count < group->currentSubGroup->maxCount);

    RenderItem_DrawRings_Item* item = group->currentSubGroup->first + group->currentSubGroup->header->count++;
    item->centre = centre;
    item->radius = radius;
    item->thickness = thickness;
    item->color = color;

    group->count++;
}

void endRingsGroup(DrawRingsGroup* group)
{
    endCurrentSubGroup(group);
}
