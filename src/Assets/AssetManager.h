/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/AssetManagerListener.h>
#include <Assets/AssetMetadata.h>
#include <Assets/AssetRef.h>
#include <Assets/BuiltinAssetLoader.h>
#include <IO/DirectoryWatcher.h>
#include <IO/File.h>
#include <Settings/SettingsChangeListener.h>
#include <Util/ChunkedArray.h>
#include <Util/Function.h>
#include <Util/Set.h>
#include <Util/StringTable.h>

namespace Assets {

struct AssetManager final : public SettingsChangeListener {
    MemoryArena arena;
    StringTable assetStrings;

    // FIXME: This should be nonnull and initialized on construction.
    OwnPtr<DirectoryWatcher> asset_change_handle;

    u32 asset_generation() const { return m_asset_generation; }
    bool have_asset_files_changed() const;

    smm assetMemoryAllocated;
    smm maxAssetMemoryAllocated;

    String assetsPath;
    HashTable<AssetType> fileExtensionToType;
    HashTable<AssetType> directoryNameToType;

    ChunkedArray<AssetMetadata> allAssets;

    struct AssetTypeData {
        String name;

        AssetLoader& loader;

        HashTable<AssetMetadata*> assets_with_this_type;

        // If a requested asset is not found, the one here is used instead.
        // Probably most of these will be empty, but we do need a placeholder sprite at least,
        // so I figure it's better to put this in place for all types while I'm at it.
        // - Sam, 27/03/2020
        Optional<AssetMetadata> placeholder_asset;

        // The missing assets are logged here!
        Set<String> missing_asset_names;
    };
    ChunkedArray<AssetTypeData> asset_type_data;

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

    String make_asset_path(AssetType, StringView short_name) const;

    void scan_assets();
    AssetMetadata* add_asset(AssetType type, StringView short_name, Flags<AssetFlags> flags = default_asset_flags);
    void load_assets();
    void reload();

    ChunkedArray<NonnullOwnPtr<AssetLoader>> asset_loaders;
    void register_asset_loader(NonnullOwnPtr<AssetLoader>&&);
    AssetLoader& get_asset_loader_for_type(AssetType) const;

    struct AssetConfig {
        Optional<StringView> directory;
        Optional<StringView> file_extension;
    };
    AssetType register_asset_type(String name, AssetLoader&, AssetConfig = {});
    void set_placeholder_asset(AssetType, NonnullOwnPtr<Asset>);
    AssetMetadata& get_placeholder_asset(AssetType);

    template<typename T>
    void for_each_asset_of_type(Function<void(AssetMetadata&, T&)> callback)
    {
        auto& assets_of_type = asset_type_data[T::asset_type()].assets_with_this_type;
        for (auto it = assets_of_type.iterate(); it.hasNext(); it.next()) {
            AssetMetadata* asset = *it.get();
            callback(*asset, dynamic_cast<T&>(*asset->loaded_asset));
        }
    }

private:
    // ^SettingsChangeListener
    virtual void on_settings_changed() override;

    void scan_assets_from_directory(String subdirectory, Optional<AssetType> manual_asset_type = {});

    u32 m_asset_generation { 0 };
    AssetType m_next_asset_type { 0 };
};

void initAssets();
AssetManager& asset_manager();

void loadAsset(AssetMetadata* metadata);
void unloadAsset(AssetMetadata* asset);
void removeAsset(AssetType type, String name);

inline void removeAsset(AssetRef const& ref)
{
    removeAsset(ref.type(), ref.name());
}

AssetMetadata& getAsset(AssetType type, String shortName);
AssetMetadata* getAssetIfExists(AssetType type, String shortName);

String getText(String name);
String getText(String name, std::initializer_list<StringView> args);

//
// Internal
//

// FIXME: Some kind of smart pointer to handle memory allocation tracking.
Blob assets_allocate(smm size);
void assets_deallocate(Blob& data);

}

using Assets::AssetManager;

using Assets::asset_manager;
using Assets::getAsset;
using Assets::getText;
