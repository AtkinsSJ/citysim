/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "file.h"
#include <Assets/Asset.h>
#include <Util/ChunkedArray.h>
#include <Util/Set.h>
#include <Util/StringTable.h>

struct AssetManager {
    MemoryArena assetArena;
    StringTable assetStrings;

    DirectoryChangeWatchingHandle assetChangeHandle;
    bool assetReloadHasJustHappened;
    u32 lastAssetReloadTicks;

    // TODO: Also include size of the UITheme, somehow.
    smm assetMemoryAllocated;
    smm maxAssetMemoryAllocated;

    String assetsPath;
    HashTable<AssetType> fileExtensionToType;
    HashTable<AssetType> directoryNameToType;

    ChunkedArray<Asset> allAssets;
    HashTable<Asset*> assetsByType[AssetTypeCount];

    // If a requested asset is not found, the one here is used instead.
    // Probably most of these will be empty, but we do need a placeholder sprite at least,
    // so I figure it's better to put this in place for all types while I'm at it.
    // - Sam, 27/03/2020
    Asset placeholderAssets[AssetTypeCount];
    // The missing assets are logged here!
    Set<String> missingAssetNames[AssetTypeCount];

    // TODO: this probably belongs somewhere else? IDK.
    // It feels icky having parts of assets directly in this struct, but when there's only 1, and you
    // have to do a hashtable lookup inside it, it makes more sense to avoid the "find the asset" lookup.
    HashTable<String> texts;
    HashTable<String> defaultTexts; // "en" locale
    // NB: Sets are stupid right now, they just wrap a ChunkedArray, which means it gets wiped when
    // we reset the assetsArena in reloadAssets()! If we want to remember things across a reload,
    // to eg add a "dump missing texts to a file" command, we'll need to switch to something that
    // survives reloads. Maybe a HashTable of the textID -> whether it found a fallback, that could
    // be useful.
    // - Sam, 02/10/2019
    Set<String> missingTextIDs;
};

void initAssets();
AssetManager& asset_manager();
Asset* makePlaceholderAsset(AssetType type);

void reloadLocaleSpecificAssets();

Asset* addAsset(AssetType type, String shortName, u32 flags = AssetDefaultFlags);
Asset* addNinepatch(String name, String filename, s32 pu0, s32 pu1, s32 pu2, s32 pu3, s32 pv0, s32 pv1, s32 pv2, s32 pv3);
Asset* addTexture(String filename, bool isAlphaPremultiplied);
Asset* addSpriteGroup(String name, s32 spriteCount);

void loadAssets();
void loadAsset(Asset* asset);
void ensureAssetIsLoaded(Asset* asset);
void unloadAsset(Asset* asset);
void removeAsset(AssetType type, String name);
void removeAssets(Array<AssetID> assetsToRemove);

void addAssets();
void addAssetsFromDirectory(String subDirectory, AssetType manualAssetType = AssetType_Unknown);
bool haveAssetFilesChanged();
void reloadAssets();

String getAssetPath(AssetType type, String shortName);

Asset* getAsset(AssetType type, String shortName);
Asset* getAssetIfExists(AssetType type, String shortName);
AssetRef getAssetRef(AssetType type, String shortName);
Asset* getAsset(AssetRef* ref);

BitmapFont* getFont(String fontName);
BitmapFont* getFont(AssetRef* fontRef);

Array<V4>* getPalette(String name);
Shader* getShader(String shaderName);
Sprite* getSprite(String name, s32 offset = 0);
SpriteGroup* getSpriteGroup(String name);

SpriteRef getSpriteRef(String groupName, s32 spriteIndex);
Sprite* getSprite(SpriteRef* ref);

String getText(String name);
String getText(String name, std::initializer_list<String> args);

template<typename T>
bool checkStyleMatchesType(AssetRef* reference)
{
    switch (reference->type) {
    case AssetType_ButtonStyle:
        return (typeid(T*) == typeid(UI::ButtonStyle*));
    case AssetType_CheckboxStyle:
        return (typeid(T*) == typeid(UI::CheckboxStyle*));
    case AssetType_ConsoleStyle:
        return (typeid(T*) == typeid(UI::ConsoleStyle*));
    case AssetType_DropDownListStyle:
        return (typeid(T*) == typeid(UI::DropDownListStyle*));
    case AssetType_LabelStyle:
        return (typeid(T*) == typeid(UI::LabelStyle*));
    case AssetType_PanelStyle:
        return (typeid(T*) == typeid(UI::PanelStyle*));
    case AssetType_RadioButtonStyle:
        return (typeid(T*) == typeid(UI::RadioButtonStyle*));
    case AssetType_ScrollbarStyle:
        return (typeid(T*) == typeid(UI::ScrollbarStyle*));
    case AssetType_SliderStyle:
        return (typeid(T*) == typeid(UI::SliderStyle*));
    case AssetType_TextInputStyle:
        return (typeid(T*) == typeid(UI::TextInputStyle*));
    case AssetType_WindowStyle:
        return (typeid(T*) == typeid(UI::WindowStyle*));
        INVALID_DEFAULT_CASE;
    }

    return false;
}

template<typename T>
T* getStyle(AssetRef* ref)
{
    ASSERT(checkStyleMatchesType<T>(ref));

    Asset* asset = getAsset(ref);

    return (T*)&asset->_localData;
}

template<typename T>
T* getStyle(String styleName, AssetRef* defaultStyle)
{
    T* result = nullptr;
    if (!isEmpty(styleName))
        result = getStyle<T>(styleName);
    if (result == nullptr)
        result = getStyle<T>(defaultStyle);

    return result;
}

template<typename T>
T* getStyle(String styleName)
{
    T* result = nullptr;

    AssetType styleType = AssetType_Unknown;
    if (typeid(T) == typeid(UI::ButtonStyle))
        styleType = AssetType_ButtonStyle;
    else if (typeid(T) == typeid(UI::CheckboxStyle))
        styleType = AssetType_CheckboxStyle;
    else if (typeid(T) == typeid(UI::ConsoleStyle))
        styleType = AssetType_ConsoleStyle;
    else if (typeid(T) == typeid(UI::DropDownListStyle))
        styleType = AssetType_DropDownListStyle;
    else if (typeid(T) == typeid(UI::LabelStyle))
        styleType = AssetType_LabelStyle;
    else if (typeid(T) == typeid(UI::PanelStyle))
        styleType = AssetType_PanelStyle;
    else if (typeid(T) == typeid(UI::RadioButtonStyle))
        styleType = AssetType_RadioButtonStyle;
    else if (typeid(T) == typeid(UI::ScrollbarStyle))
        styleType = AssetType_ScrollbarStyle;
    else if (typeid(T) == typeid(UI::SliderStyle))
        styleType = AssetType_SliderStyle;
    else if (typeid(T) == typeid(UI::TextInputStyle))
        styleType = AssetType_TextInputStyle;
    else if (typeid(T) == typeid(UI::WindowStyle))
        styleType = AssetType_WindowStyle;
    else
        ASSERT(false);

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
void loadTexts(HashTable<String>* texts, Asset* asset, Blob fileData);
