/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/AssetType.h>
#include <Assets/Forward.h>
#include <Gfx/BitmapFont.h>
#include <Gfx/Colour.h>
#include <Gfx/Cursor.h>
#include <Gfx/Ninepatch.h>
#include <Gfx/Palette.h>
#include <Gfx/Shader.h>
#include <Gfx/Sprite.h>
#include <Gfx/Texture.h>
#include <UI/UITheme.h>
#include <Util/Array.h>
#include <Util/Flags.h>
#include <Util/Locale.h>
#include <Util/Memory.h>

enum class AssetFlags : u8 {
    IsAFile, // The file at Asset.fullName should be loaded into memory when loading this Asset

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
        Error,
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
