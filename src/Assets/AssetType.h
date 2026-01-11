/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/EnumMap.h>
#include <Util/String.h>

enum class AssetType : u8 {
    BitmapFont,
    BuildingDefs,
    Cursor,
    CursorDefs,
    DevKeymap,
    Ninepatch,
    Palette,
    PaletteDefs,
    Shader,
    Sprite,
    SpriteDefs,
    TerrainDefs,
    TextDocument,
    Texts,
    Texture,
    UITheme,

    ButtonStyle,
    CheckboxStyle,
    ConsoleStyle,
    DropDownListStyle,
    LabelStyle,
    PanelStyle,
    RadioButtonStyle,
    ScrollbarStyle,
    SliderStyle,
    TextInputStyle,
    WindowStyle,

    COUNT,
};

EnumMap<AssetType, String> const asset_type_names {
    "BitmapFont"_s,
    "BuildingDefs"_s,
    "Cursor"_s,
    "CursorDefs"_s,
    "DevKeymap"_s,
    "Ninepatch"_s,
    "Palette"_s,
    "PaletteDefs"_s,
    "Shader"_s,
    "Sprite"_s,
    "SpriteDefs"_s,
    "TextDocument"_s,
    "Texts"_s,
    "Texture"_s,
    "TerrainDefs"_s,
    "UITheme"_s,

    "ButtonStyle"_s,
    "CheckboxStyle"_s,
    "ConsoleStyle"_s,
    "DropDownListStyle"_s,
    "LabelStyle"_s,
    "PanelStyle"_s,
    "RadioButtonStyle"_s,
    "ScrollbarStyle"_s,
    "SliderStyle"_s,
    "TextInputStyle"_s,
    "WindowStyle"_s,
};
