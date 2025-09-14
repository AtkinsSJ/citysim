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
#include <Gfx/Colour.h>
#include <SDL2/SDL_mouse.h>
#include <UI/UITheme.h>
#include <Util/Array.h>
#include <Util/Flags.h>
#include <Util/Locale.h>
#include <Util/Memory.h>
#include <Util/Vector.h>

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
    float u0;
    float u1;
    float u2;
    float u3;

    float v0;
    float v1;
    float v2;
    float v3;
};

struct Palette {
    enum class Type : u8 {
        Fixed,
        Gradient,
    };
    Type type;
    s32 size;

    struct
    {
        Colour from;
        Colour to;
    } gradient;

    Array<Colour> paletteData;
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
    struct {
        u32 glTextureID;
        bool isLoaded;
    } gl;
};

enum class AssetFlags : u8 {
    IsAFile, // The file at Asset.fullName should be loaded into memory when loading this Asset

    COUNT,
};

constexpr Flags<AssetFlags> default_asset_flags { AssetFlags::IsAFile };

struct Asset {
    static Asset* make_placeholder(AssetType);

    Asset(AssetType type, String short_name, String full_path, Flags<AssetFlags> flags, Optional<Locale> locale)
        : type(type)
        , shortName(short_name)
        , fullName(full_path)
        , flags(flags)
    {
    }

    virtual ~Asset();

    AssetRef get_ref() const;

    void load();

    AssetType type;

    // shortName = "foo.png", fullName = "c:/mygame/assets/textures/foo.png"
    String shortName;
    String fullName;

    Flags<AssetFlags> flags;

    enum class State : u8 {
        Unloaded,
        Loaded,
    };
    State state { State::Unloaded };
    Optional<Locale> locale;

    Array<AssetRef> children;

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

private:
    virtual void on_load() { }
    virtual void on_unload() { }
};

class Cursor final : public Asset {
public:
    static Cursor* make_placeholder();
    Cursor(String short_name, String full_path, Flags<AssetFlags>, String image_file_path, V2I hotspot, Optional<Locale>);

    SDL_Cursor* sdl_cursor() const { return m_sdl_cursor; }

private:
    virtual void on_load() override;
    virtual void on_unload() override;

    String m_image_file_path;
    V2I m_hotspot;
    SDL_Cursor* m_sdl_cursor { nullptr };
};
