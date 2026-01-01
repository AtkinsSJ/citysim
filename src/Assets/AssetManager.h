/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Asset.h>
#include <Assets/AssetManagerListener.h>
#include <IO/DirectoryWatcher.h>
#include <IO/File.h>
#include <Settings/SettingsChangeListener.h>
#include <Util/ChunkedArray.h>
#include <Util/EnumMap.h>
#include <Util/Set.h>
#include <Util/StringTable.h>

struct AssetManager final : public SettingsChangeListener {
    MemoryArena arena;
    StringTable assetStrings;

    // FIXME: This should be nonnull and initialized on construction.
    OwnPtr<DirectoryWatcher> asset_change_handle;

    u32 asset_generation() const { return m_asset_generation; }
    void fixme_increment_asset_generation() { m_asset_generation++; }

    // TODO: Also include size of the UITheme, somehow.
    smm assetMemoryAllocated;
    smm maxAssetMemoryAllocated;

    String assetsPath;
    HashTable<AssetType> fileExtensionToType;
    HashTable<AssetType> directoryNameToType;

    ChunkedArray<Asset> allAssets;
    EnumMap<AssetType, HashTable<Asset*>> assetsByType;

    // If a requested asset is not found, the one here is used instead.
    // Probably most of these will be empty, but we do need a placeholder sprite at least,
    // so I figure it's better to put this in place for all types while I'm at it.
    // - Sam, 27/03/2020
    EnumMap<AssetType, Asset> placeholderAssets;
    // The missing assets are logged here!
    EnumMap<AssetType, Set<String>> missingAssetNames;

    // TODO: this probably belongs somewhere else? IDK.
    // It feels icky having parts of assets directly in this struct, but when there's only 1, and you
    // have to do a hashtable lookup inside it, it makes more sense to avoid the "find the asset" lookup.
    // FIXME: TextCatalogue?
    HashTable<String> texts;
    HashTable<String> defaultTexts; // "en" locale
    // NB: Sets are stupid right now, they just wrap a ChunkedArray, which means it gets wiped when
    // we reset the assetsArena in reloadAssets()! If we want to remember things across a reload,
    // to eg add a "dump missing texts to a file" command, we'll need to switch to something that
    // survives reloads. Maybe a HashTable of the textID -> whether it found a fallback, that could
    // be useful.
    // - Sam, 02/10/2019
    Set<String> missingTextIDs;

    ChunkedArray<AssetManagerListener*> listeners;
    void register_listener(AssetManagerListener*);
    void unregister_listener(AssetManagerListener*);

private:
    // ^SettingsChangeListener
    virtual void on_settings_changed() override;

    u32 m_asset_generation { 0 };
};

void initAssets();
AssetManager& asset_manager();
Asset* makePlaceholderAsset(AssetType type);

Asset* addAsset(AssetType type, StringView shortName, Flags<AssetFlags> flags = default_asset_flags);
Asset* addNinepatch(StringView name, StringView filename, s32 pu0, s32 pu1, s32 pu2, s32 pu3, s32 pv0, s32 pv1, s32 pv2, s32 pv3);
Asset* addTexture(StringView filename, bool isAlphaPremultiplied);
Asset* addSpriteGroup(StringView name, s32 spriteCount);

void loadAssets();
void loadAsset(Asset* asset);
void ensureAssetIsLoaded(Asset* asset);
void unloadAsset(Asset* asset);
void removeAsset(AssetType type, String name);
void removeAsset(AssetRef const&);

void addAssets();
void addAssetsFromDirectory(String subDirectory, Optional<AssetType> manualAssetType = {});
bool haveAssetFilesChanged();
void reloadAssets();

String getAssetPath(AssetType type, StringView shortName);

Asset* getAsset(AssetType type, String shortName);
Asset* getAssetIfExists(AssetType type, String shortName);

BitmapFont* getFont(String fontName);
BitmapFont* getFont(AssetRef const& fontRef);

Array<Colour>* getPalette(String name);
Shader* getShader(String shaderName);
Sprite* getSprite(String name, s32 offset = 0);
SpriteGroup* getSpriteGroup(String name);

SpriteRef getSpriteRef(StringView groupName, s32 spriteIndex);
Sprite* getSprite(SpriteRef* ref);

String getText(String name);
String getText(String name, std::initializer_list<StringView> args);

template<typename T>
bool checkStyleMatchesType(AssetRef const& reference)
{
    switch (reference.type()) {
    case AssetType::ButtonStyle:
        return (typeid(T*) == typeid(UI::ButtonStyle*));
    case AssetType::CheckboxStyle:
        return (typeid(T*) == typeid(UI::CheckboxStyle*));
    case AssetType::ConsoleStyle:
        return (typeid(T*) == typeid(UI::ConsoleStyle*));
    case AssetType::DropDownListStyle:
        return (typeid(T*) == typeid(UI::DropDownListStyle*));
    case AssetType::LabelStyle:
        return (typeid(T*) == typeid(UI::LabelStyle*));
    case AssetType::PanelStyle:
        return (typeid(T*) == typeid(UI::PanelStyle*));
    case AssetType::RadioButtonStyle:
        return (typeid(T*) == typeid(UI::RadioButtonStyle*));
    case AssetType::ScrollbarStyle:
        return (typeid(T*) == typeid(UI::ScrollbarStyle*));
    case AssetType::SliderStyle:
        return (typeid(T*) == typeid(UI::SliderStyle*));
    case AssetType::TextInputStyle:
        return (typeid(T*) == typeid(UI::TextInputStyle*));
    case AssetType::WindowStyle:
        return (typeid(T*) == typeid(UI::WindowStyle*));
        INVALID_DEFAULT_CASE;
    }

    return false;
}

template<typename T>
T* getStyle(AssetRef const& ref)
{
    ASSERT(checkStyleMatchesType<T>(ref));

    Asset* asset = ref.get();

    return (T*)&asset->_localData;
}

template<typename T>
T* getStyle(String styleName, AssetRef const& defaultStyle)
{
    if (styleName.is_empty())
        return getStyle<T>(defaultStyle);
    return getStyle<T>(styleName);
}

template<typename T>
T* getStyle(String styleName)
{
    T* result = nullptr;

    AssetType styleType = [] {
        if constexpr (typeid(T) == typeid(UI::ButtonStyle))
            return AssetType::ButtonStyle;
        else if constexpr (typeid(T) == typeid(UI::CheckboxStyle))
            return AssetType::CheckboxStyle;
        else if constexpr (typeid(T) == typeid(UI::ConsoleStyle))
            return AssetType::ConsoleStyle;
        else if constexpr (typeid(T) == typeid(UI::DropDownListStyle))
            return AssetType::DropDownListStyle;
        else if constexpr (typeid(T) == typeid(UI::LabelStyle))
            return AssetType::LabelStyle;
        else if constexpr (typeid(T) == typeid(UI::PanelStyle))
            return AssetType::PanelStyle;
        else if constexpr (typeid(T) == typeid(UI::RadioButtonStyle))
            return AssetType::RadioButtonStyle;
        else if constexpr (typeid(T) == typeid(UI::ScrollbarStyle))
            return AssetType::ScrollbarStyle;
        else if constexpr (typeid(T) == typeid(UI::SliderStyle))
            return AssetType::SliderStyle;
        else if constexpr (typeid(T) == typeid(UI::TextInputStyle))
            return AssetType::TextInputStyle;
        else if constexpr (typeid(T) == typeid(UI::WindowStyle))
            return AssetType::WindowStyle;
        else
            static_assert(false);
    }();

    Asset* asset = getAsset(styleType, styleName);
    if (asset != nullptr) {
        result = (T*)&asset->_localData;
    }

    return result;
}

//
// Internal
//

Blob assetsAllocate(AssetManager* theAssets, smm size);
void allocateChildren(Asset* asset, s32 childCount);
void addChildAsset(Asset* parent, Asset* child);

void loadCursorDefs(Blob data, Asset* asset);
void loadPaletteDefs(Blob data, Asset* asset);
void loadSpriteDefs(Blob data, Asset* asset);
void loadTexts(HashTable<String>* texts, Asset* asset, Blob file_data);
