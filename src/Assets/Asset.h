/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "../font.h"
#include <Assets/AssetType.h>
#include <Assets/Forward.h>
#include <Assets/Sprite.h>
#include <SDL2/SDL_mouse.h>
#include <UI/UITheme.h>
#include <Util/Array.h>
#include <Util/Flags.h>
#include <Util/Memory.h>
#include <Util/Vector.h>

struct AssetID {
    AssetType type;
    String name;
};

inline AssetID makeAssetID(AssetType type, String name)
{
    AssetID result = {};

    result.type = type;
    result.name = name;

    return result;
}

struct Cursor {
    String imageFilePath; // Full path
    V2I hotspot;
    SDL_Cursor* sdlCursor;
};

struct Ninepatch {
    Asset* texture;

    s32 pu0;
    s32 pu1;
    s32 pu2;
    s32 pu3;

    s32 pv0;
    s32 pv1;
    s32 pv2;
    s32 pv3;

    // NB: These UV values do not change once they're set in loadAsset(), so, we
    // could replace them with an array of 9 Rect2s in order to avoid calculating
    // them over and over. BUT, this would greatly increase the size of the Asset
    // struct itself, which we don't want to do. We could move that into the
    // Asset.data blob, but then we're doing an extra indirection every time we
    // read it, and it's not *quite* big enough to warrant that extra complexity.
    // Frustrating! If it was bigger I'd do it, but as it is, I think keeping
    // these 8 floats is the best option. If creating the UV rects every time
    // becomes a bottleneck, we can switch over.
    // - Sam, 16/02/2021
    f32 u0;
    f32 u1;
    f32 u2;
    f32 u3;

    f32 v0;
    f32 v1;
    f32 v2;
    f32 v3;
};

struct Palette {
    enum class Type : u8 {
        Fixed,
        Gradient,
    };
    Type type;
    s32 size;

    union {
        struct
        {
            V4 from;
            V4 to;
        } gradient;
    };

    Array<V4> paletteData;
};

struct Shader {
    s8 rendererShaderID;

    String vertexShader;
    String fragmentShader;
};

struct Texture {
    bool isFileAlphaPremultiplied;
    SDL_Surface* surface;

    // Renderer-specific stuff.
    union {
        struct {
            u32 glTextureID;
            bool isLoaded;
        } gl;
    };
};

enum class AssetFlags : u8 {
    IsAFile,          // The file at Asset.fullName should be loaded into memory when loading this Asset
    IsLocaleSpecific, // File path has a {0} in it which should be filled in with the current locale

    COUNT,
};

constexpr Flags<AssetFlags> default_asset_flags { AssetFlags::IsAFile };

struct Asset {
    AssetType type;

    // shortName = "foo.png", fullName = "c:/mygame/assets/textures/foo.png"
    String shortName;
    String fullName;

    Flags<AssetFlags> flags;

    enum class State : u8 {
        Unloaded,
        Loaded,
    };
    State state;

    Array<AssetID> children;

    // Depending on the AssetType, this could be the file contents, or something else!
    Blob data;

    // Type-specific stuff
    // TODO: This union represents type-specific data size, which affects ALL assets!
    // We might want to move everything into the *memory pointer, so that each Asset is only as big
    // as it needs to be. The Shader struct is 65+ bytes (not including padding) which really inflates
    // the other asset types. Though 65 isn't much, so maybe it's not a big deal. We'll see if we
    // have anything bigger later. (Well, the BitmapFont is definitely bigger!)
    union {
        // NB: Here as a pointer target, so we can cast to the desired type. eg:
        //    (UIBUttonStyle *)(&asset->_localData)
        u8 _localData;

        // Actual local data follows

        BitmapFont bitmapFont;

        struct {
            Array<String> buildingIDs;
        } buildingDefs;

        Cursor cursor;

        Ninepatch ninepatch;

        Palette palette;

        Shader shader;

        SpriteGroup spriteGroup;

        struct {
            Array<String> terrainIDs;
        } terrainDefs;

        struct {
            Array<String> keys;
            bool isFallbackLocale;
        } texts;

        Texture texture;

        UI::ButtonStyle buttonStyle;
        UI::CheckboxStyle checkboxStyle;
        UI::ConsoleStyle consoleStyle;
        UI::DropDownListStyle dropDownListStyle;
        UI::LabelStyle labelStyle;
        UI::PanelStyle panelStyle;
        UI::RadioButtonStyle radioButtonStyle;
        UI::ScrollbarStyle scrollbarStyle;
        UI::SliderStyle sliderStyle;
        UI::TextInputStyle textInputStyle;
        UI::WindowStyle windowStyle;
    };
};
