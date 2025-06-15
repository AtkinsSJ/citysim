/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@ladybird.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/String.h>

enum AssetType {
    AssetType_Misc,

    AssetType_BitmapFont,
    AssetType_BuildingDefs,
    AssetType_Cursor,
    AssetType_CursorDefs,
    AssetType_DevKeymap,
    AssetType_Ninepatch,
    AssetType_Palette,
    AssetType_PaletteDefs,
    AssetType_Shader,
    AssetType_Sprite,
    AssetType_SpriteDefs,
    AssetType_TerrainDefs,
    AssetType_Texts,
    AssetType_Texture,
    AssetType_UITheme,

    AssetType_ButtonStyle,
    AssetType_CheckboxStyle,
    AssetType_ConsoleStyle,
    AssetType_DropDownListStyle,
    AssetType_LabelStyle,
    AssetType_PanelStyle,
    AssetType_RadioButtonStyle,
    AssetType_ScrollbarStyle,
    AssetType_SliderStyle,
    AssetType_TextInputStyle,
    AssetType_WindowStyle,

    AssetTypeCount,
    AssetType_Unknown = -1
};

inline String assetTypeNames[AssetTypeCount] = {
    "Misc"_s,
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
