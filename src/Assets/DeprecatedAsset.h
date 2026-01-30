/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "Util/Log.h"

#include <Assets/Asset.h>
#include <Assets/AssetLoader.h>
#include <Gfx/BitmapFont.h>
#include <Gfx/Cursor.h>
#include <Gfx/Ninepatch.h>
#include <Gfx/Palette.h>
#include <Gfx/Shader.h>
#include <Gfx/Sprite.h>
#include <Gfx/TextDocument.h>
#include <Gfx/Texture.h>
#include <UI/UITheme.h>
#include <Util/Array.h>

// Stop-gap which preserves the old Asset behaviour of them all being data stored in a union, until all the asset types
// are converted into subclasses.
class DeprecatedAsset final : public Asset {
public:
    DeprecatedAsset();
    virtual ~DeprecatedAsset() override;

    void unload(AssetMetadata& metadata) override;

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

        TextDocument text_document;

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

class DeprecatedAssetLoader final : public AssetLoader {
public:
    virtual ~DeprecatedAssetLoader() override = default;

    virtual void register_types(AssetManager&) override;
    virtual void create_placeholder_assets(AssetManager&) override;

    virtual ErrorOr<NonnullOwnPtr<Asset>> load_asset(AssetMetadata& metadata, Blob file_data) override;
};
