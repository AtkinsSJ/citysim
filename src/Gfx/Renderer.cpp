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
    : m_arena("Renderer"_s)
    , m_render_buffer_pool(m_arena)
    , m_render_buffer_chunk_pool(m_arena)
    , m_sdl_window(window)
{
    SDL_GetWindowSize(window, &m_window_size.x, &m_window_size.y);

    m_world_buffer = &m_render_buffer_pool.obtain(m_arena, "WorldBuffer"_s, &m_render_buffer_chunk_pool);
    m_world_overlay_buffer = &m_render_buffer_pool.obtain(m_arena, "WorldOverlayBuffer"_s, &m_render_buffer_chunk_pool);
    m_ui_buffer = &m_render_buffer_pool.obtain(m_arena, "UIBuffer"_s, &m_render_buffer_chunk_pool);
    m_window_buffer = &m_render_buffer_pool.obtain(m_arena, "WindowBuffer"_s, &m_render_buffer_chunk_pool);
    m_debug_buffer = &m_render_buffer_pool.obtain(m_arena, "DebugBuffer"_s, &m_render_buffer_chunk_pool);

    m_render_buffers = m_arena.allocate_array<RenderBuffer*>(5);
    m_render_buffers.append(m_world_buffer);
    m_render_buffers.append(m_world_overlay_buffer);
    m_render_buffers.append(m_ui_buffer);
    m_render_buffers.append(m_window_buffer);
    m_render_buffers.append(m_debug_buffer);

    // Init cameras
    V2 camera_size = v2(m_window_size);
    f32 const TILE_SIZE = 16.0f;
    m_world_camera = Camera(camera_size, 1.0f / TILE_SIZE, 10000.0f, -10000.0f);
    m_ui_camera = Camera(camera_size, 1.0f, 10000.0f, -10000.0f, camera_size * 0.5f);

    // Hide cursor until stuff loads
    set_cursor_visible(false);
    m_arena.mark_reset_position();
}

Renderer::~Renderer()
{
    if (m_system_wait_cursor != nullptr) {
        SDL_FreeCursor(m_system_wait_cursor);
        m_system_wait_cursor = nullptr;
    }
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

    gl_renderer->m_arena.mark_reset_position();

    s_renderer = gl_renderer;
    return true;
}

Renderer& the_renderer()
{
    ASSERT(s_renderer);
    return *s_renderer;
}

void Renderer::handle_window_event(SDL_WindowEvent const& event)
{
    switch (event.event) {
    case SDL_WINDOWEVENT_SIZE_CHANGED: {
        m_window_size = v2i(event.data1, event.data2);

        on_window_resized(m_window_size.x, m_window_size.y);

        V2 camera_size = v2(m_window_size);

        world_camera().set_size(camera_size);

        ui_camera().set_size(camera_size);
        ui_camera().set_position(ui_camera().size() * 0.5f);
    } break;
    }
}

void Renderer::render()
{
    DEBUG_POOL(&m_render_buffer_pool, "renderBufferPool");
    DEBUG_POOL(&m_render_buffer_chunk_pool, "renderChunkPool");

    render_internal();

    for (s32 i = 0; i < m_render_buffers.count; i++) {
        m_render_buffers[i]->clear_for_pool();
    }
}

void Renderer::load_assets()
{
    if (!isEmpty(m_current_cursor_name)) {
        set_cursor(m_current_cursor_name);
    }
}

void Renderer::unload_assets()
{
    show_system_wait_cursor();
}

void freeRenderer()
{
    s_renderer->free();
    delete s_renderer;
}

void Renderer::resize_window(s32 w, s32 h, bool fullscreen)
{
    SDL_RestoreWindow(m_sdl_window);

    // So, I'm not super sure how we want to handle fullscreen. Do we scan the
    // available resolution options and list them to select from? Do we just use
    // the native resolution? Do we use fullscreen or fullscreen-window?
    //
    // For now, I'm thinking we use the native resolution, as a fullscreen window.
    // - Sam, 20/05/2021

    // If the requested setup is what we already have, do nothing!
    // This avoids the window jumping about when you save the settings
    if ((w == s_renderer->window_width())
        && (h == s_renderer->window_height())
        && (fullscreen == s_renderer->window_is_fullscreen())) {
        return;
    }

    s32 newW = w;
    s32 newH = h;

    if (fullscreen) {
        // Fullscreen!
        // Grab the desktop resolution to use that
        s32 displayIndex = SDL_GetWindowDisplayIndex(m_sdl_window);
        SDL_DisplayMode displayMode;
        if (SDL_GetDesktopDisplayMode(displayIndex, &displayMode) == 0) {
            SDL_SetWindowDisplayMode(m_sdl_window, &displayMode);
            SDL_SetWindowFullscreen(m_sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            newW = displayMode.w;
            newH = displayMode.h;
        } else {
            logError("Failed to get desktop display mode: {0}"_s, { makeString(SDL_GetError()) });
        }
    } else {
        // Window!
        SDL_SetWindowSize(m_sdl_window, newW, newH);
        SDL_SetWindowFullscreen(m_sdl_window, 0);

        // Centre it
        s32 displayIndex = SDL_GetWindowDisplayIndex(m_sdl_window);
        SDL_Rect displayBounds;
        if (SDL_GetDisplayBounds(displayIndex, &displayBounds) == 0) {
            s32 leftBorder, rightBorder, topBorder, bottomBorder;
            SDL_GetWindowBordersSize(m_sdl_window, &topBorder, &leftBorder, &bottomBorder, &rightBorder);

            s32 windowW = w + leftBorder + rightBorder;
            s32 windowH = h + topBorder + bottomBorder;
            s32 windowLeft = (displayBounds.w - windowW) / 2;
            s32 windowTop = (displayBounds.h - windowH) / 2;

            SDL_SetWindowPosition(m_sdl_window, displayBounds.x + windowLeft, displayBounds.y + windowTop);
        } else {
            logError("Failed to get display bounds: {0}"_s, { makeString(SDL_GetError()) });

            // As a backup, just centre it on the main display
            SDL_SetWindowPosition(m_sdl_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        }
    }

    s_renderer->m_window_is_fullscreen = fullscreen;
}

void Renderer::show_system_wait_cursor()
{
    if (m_system_wait_cursor == nullptr) {
        m_system_wait_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
    }
    SDL_SetCursor(m_system_wait_cursor);
}

void Renderer::set_cursor(String name)
{
    DEBUG_FUNCTION();

    if (Asset const* new_cursor_asset = getAsset(AssetType::Cursor, name)) {
        m_current_cursor_name = name;
        SDL_SetCursor(new_cursor_asset->cursor.sdlCursor);
    }
}

void Renderer::set_cursor_visible(bool visible)
{
    m_cursor_is_visible = visible;
    SDL_ShowCursor(visible ? 1 : 0);
}

RenderBuffer* Renderer::get_temporary_render_buffer(String name)
{
    return &m_render_buffer_pool.obtain(m_arena, name, &m_render_buffer_chunk_pool);
}

void Renderer::return_temporary_render_buffer(RenderBuffer& buffer)
{
    m_render_buffer_pool.discard(buffer);
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
            appendRenderItemType(buffer, RenderItemType::NextMemoryChunk);
        }

        RenderBufferChunk& new_chunk = buffer->chunkPool->obtain();
        new_chunk.used = 0;
        new_chunk.prevChunk = nullptr;
        new_chunk.nextChunk = nullptr;

        // Add to the renderbuffer
        if (buffer->currentChunk == nullptr) {
            buffer->firstChunk = &new_chunk;
            buffer->currentChunk = &new_chunk;
        } else {
            buffer->currentChunk->nextChunk = &new_chunk;
            new_chunk.prevChunk = buffer->currentChunk;
        }

        buffer->currentChunk = &new_chunk;
    }

    appendRenderItemType(buffer, type);
    u8* result = (buffer->currentChunk->memory + buffer->currentChunk->used);
    buffer->currentChunk->used += size;
    return result;
}

void addSetCamera(RenderBuffer* buffer, Camera* camera)
{
    RenderItem_SetCamera* cameraItem = appendRenderItem<RenderItem_SetCamera>(buffer, RenderItemType::SetCamera);
    cameraItem->camera = camera;
}

void addSetShader(RenderBuffer* buffer, s8 shaderID)
{
    if (buffer->currentShader != shaderID) {
        RenderItem_SetShader* shaderItem = appendRenderItem<RenderItem_SetShader>(buffer, RenderItemType::SetShader);
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
    ASSERT(texture->state == Asset::State::Loaded);

    if (buffer->currentTexture != texture) {
        RenderItem_SetTexture* textureItem = appendRenderItem<RenderItem_SetTexture>(buffer, RenderItemType::SetTexture);
        *textureItem = {};
        textureItem->texture = texture;

        buffer->currentTexture = texture;
    }
}

void addSetTextureRaw(RenderBuffer* buffer, s32 width, s32 height, u8 bytesPerPixel, u8* pixels)
{
    buffer->currentTexture = nullptr;

    smm pixelDataSize = (width * height * bytesPerPixel);
    auto itemAndData = appendRenderItem<RenderItem_SetTexture>(buffer, RenderItemType::SetTexture, pixelDataSize);

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
    auto itemAndData = appendRenderItem<RenderItem_SetPalette>(buffer, RenderItemType::SetPalette, sizeof(V4) * paletteSize);

    itemAndData.item->paletteSize = paletteSize;

    copyMemory(palette, (V4*)itemAndData.data, paletteSize);
}

void addClear(RenderBuffer* buffer, V4 clearColor)
{
    RenderItem_Clear* clear = appendRenderItem<RenderItem_Clear>(buffer, RenderItemType::Clear);
    clear->clearColor = clearColor;
}

void addBeginScissor(RenderBuffer* buffer, Rect2I bounds)
{
    ASSERT(bounds.w >= 0);
    ASSERT(bounds.h >= 0);

    RenderItem_BeginScissor* scissor = appendRenderItem<RenderItem_BeginScissor>(buffer, RenderItemType::BeginScissor);

    // We have to flip the bounds rectangle vertically because OpenGL has the origin in the bottom-left,
    // whereas our system uses the top-left!

    bounds.y = s_renderer->window_height() - bounds.y - bounds.h;

    // Crop to window bounds
    scissor->bounds = intersect(bounds, irectXYWH(0, 0, s_renderer->window_width(), s_renderer->window_height()));

    ASSERT(scissor->bounds.w >= 0);
    ASSERT(scissor->bounds.h >= 0);

    buffer->scissorCount++;
}

void addEndScissor(RenderBuffer* buffer)
{
    ASSERT(buffer->scissorCount > 0);
    [[maybe_unused]] RenderItem_EndScissor* scissor = appendRenderItem<RenderItem_EndScissor>(buffer, RenderItemType::EndScissor);
    buffer->scissorCount--;
}

void drawSingleSprite(RenderBuffer* buffer, Sprite* sprite, Rect2 bounds, s8 shaderID, V4 color)
{
    addSetShader(buffer, shaderID);
    addSetTexture(buffer, sprite->texture);

    RenderItem_DrawSingleRect* rect = appendRenderItem<RenderItem_DrawSingleRect>(buffer, RenderItemType::DrawSingleRect);

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

    RenderItem_DrawSingleRect* rect = appendRenderItem<RenderItem_DrawSingleRect>(buffer, RenderItemType::DrawSingleRect);

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
        RenderItem_SetTexture* textureItem = appendRenderItem<RenderItem_SetTexture>(buffer, RenderItemType::SetTexture);
        *textureItem = {};

        // We need to clear this, because we don't know what this texture will be, so any following draw calls
        // have to assume that they need to set their texture
        buffer->currentTexture = nullptr;

        result.setTexture = textureItem;
    }

    result.drawRect = appendRenderItem<RenderItem_DrawSingleRect>(buffer, RenderItemType::DrawSingleRect);

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

    DrawRectsGroup* result = temp_arena().allocate<DrawRectsGroup>();
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
    u8* data = appendRenderItemInternal(group->buffer, RenderItemType::DrawRects, sizeof(RenderItem_DrawRects), reservedSize);
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
        group->currentSubGroup = temp_arena().allocate<DrawRectsSubGroup>();
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

    RenderItem_DrawSingleRect* rect = appendRenderItem<RenderItem_DrawSingleRect>(buffer, RenderItemType::DrawSingleRect);
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

    DrawRingsGroup* result = temp_arena().allocate<DrawRingsGroup>();
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
    u8* data = appendRenderItemInternal(group->buffer, RenderItemType::DrawRings, sizeof(RenderItem_DrawRings), reservedSize);
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
        group->currentSubGroup = temp_arena().allocate<DrawRingsSubGroup>();
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
