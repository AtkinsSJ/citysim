/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Gfx/Colour.h>
#include <Gfx/Forward.h>
#include <Util/Basic.h>
#include <Util/Rectangle.h>

enum class RenderItemType : u8 {
    NextMemoryChunk,

    SetCamera,
    SetPalette,
    SetShader,
    SetTexture,

    Clear,

    BeginScissor,
    EndScissor,

    DrawSingleRect,
    DrawRects,
    DrawRings,
};

struct RenderItem_SetCamera {
    Camera* camera;
};

struct RenderItem_SetPalette {
    s32 paletteSize;
    // Palette as a series of Colours follows this struct
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
    Colour clearColor;
};

struct RenderItem_BeginScissor {
    Rect2I bounds;
};
struct RenderItem_EndScissor {
};

struct RenderItem_DrawSingleRect {
    Rect2 bounds;
    Colour color00;
    Colour color01;
    Colour color10;
    Colour color11;
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
    Colour color;
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
    Colour color;
};
