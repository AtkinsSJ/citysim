/*
 * Copyright (c) 2016-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AssetManager.h"
#include <Assets/Asset.h>
#include <Debug/Console.h>
#include <Gfx/Renderer.h>
#include <IO/DirectoryIterator.h>
#include <IO/DirectoryWatcher.h>
#include <IO/File.h>
#include <IO/LineReader.h>
#include <SDL2/SDL_filesystem.h>
#include <SDL2/SDL_image.h>
#include <Settings/Settings.h>
#include <Sim/Building.h>
#include <Sim/BuildingCatalogue.h>
#include <Util/StringBuilder.h>

AssetManager* s_assets;

void initAssets()
{
    s_assets = MemoryArena::bootstrap<AssetManager>("Assets"_s);

    String basePath = String::from_null_terminated(SDL_GetBasePath());
    s_assets->assetsPath = s_assets->assetStrings.intern(constructPath({ basePath, "assets"_s }));

    // NB: The arena block size is 1MB currently, so make sure that this number * sizeof(Asset) is less than that!
    // (Otherwise, we waste a LOT of memory with almost-empty memory blocks.)
    initChunkedArray(&s_assets->allAssets, &s_assets->arena, 1024);
    s_assets->assetMemoryAllocated = 0;
    s_assets->maxAssetMemoryAllocated = 0;

    auto compareStrings = [](String* a, String* b) { return *a == *b; };

    for (auto asset_type : enum_values<AssetType>()) {
        initSet<String>(&s_assets->missingAssetNames[asset_type], &s_assets->arena, compareStrings);
    }

    UI::initStyleConstants();

    initSet<String>(&s_assets->missingTextIDs, &s_assets->arena, compareStrings);

    initChunkedArray(&s_assets->listeners, &s_assets->arena, 32);
    initChunkedArray(&s_assets->asset_loaders, &s_assets->arena, 32);

    // NB: This might fail, or we might be on a platform where it isn't implemented.
    // That's OK though!
    auto asset_watcher = DirectoryWatcher::watch(s_assets->assetsPath);
    if (asset_watcher.is_error()) {
        logError("Failed to watch assets directory: {}"_s, { asset_watcher.error() });
    } else {
        s_assets->asset_change_handle = asset_watcher.release_value();
    }

    Settings::the().register_listener(*s_assets);
}

AssetManager& asset_manager()
{
    return *s_assets;
}

Blob assetsAllocate(AssetManager* theAssets, smm size)
{
    Blob result { size, allocateRaw(size) };

    theAssets->assetMemoryAllocated += size;
    theAssets->maxAssetMemoryAllocated = max(theAssets->assetMemoryAllocated, theAssets->maxAssetMemoryAllocated);

    return result;
}

void allocateChildren(AssetMetadata* asset, s32 childCount)
{
    asset->children_data = assetsAllocate(s_assets, childCount * sizeof(AssetRef));
    asset->children = makeArray(childCount, reinterpret_cast<AssetRef*>(asset->children_data.writable_data()));
}

void addChildAsset(AssetMetadata* parent, AssetMetadata* child)
{
    parent->children.append(AssetRef { child->type, child->shortName });
    if (child->loaded_asset)
        child->state = AssetMetadata::State::Loaded;
}

AssetMetadata* AssetManager::add_asset(AssetType type, StringView short_name, Flags<AssetFlags> flags)
{
    String internedShortName = assetStrings.intern(short_name);

    if (AssetMetadata* existing = getAssetIfExists(type, internedShortName))
        return existing;

    AssetMetadata* asset = allAssets.appendBlank();
    asset->type = type;
    asset->shortName = internedShortName;
    if (flags.has(AssetFlags::IsAFile)) {
        asset->fullName = assetStrings.intern(make_asset_path(asset->type, internedShortName));
        if (auto locale_string = get_file_locale_segment(asset->fullName); locale_string.has_value()) {
            asset->locale = locale_from_string(locale_string.value().deprecated_to_string());
            if (!asset->locale.has_value())
                logWarn("Unrecognized locale for asset '{0}'."_s, { asset->fullName });
        }
    }
    asset->state = AssetMetadata::State::Unloaded;
    asset->children_data = {};
    asset->flags = flags;

    assetsByType[type].put(internedShortName, asset);

    return asset;
}

void loadAsset(AssetMetadata* metadata)
{
    DEBUG_FUNCTION();
    if (metadata->state != AssetMetadata::State::Unloaded)
        return;

    if (metadata->locale.has_value()) {
        // Only load assets that match our locale
        if (metadata->locale != get_locale()) {
            if (metadata->locale == Locale::En) {
                logInfo("Loading asset {0} as a default-locale fallback. (Locale {1}, current is {2})"_s, { metadata->fullName, to_string(metadata->locale.value()), to_string(get_locale()) });
            } else {
                logInfo("Skipping asset {0} because it's the wrong locale. ({1}, current is {2})"_s, { metadata->fullName, to_string(metadata->locale.value()), to_string(get_locale()) });
                return;
            }
        }
    }

    Blob file_data = {};
    // Some assets (meta-assets?) have no file associated with them, because they are composed of other assets.
    // eg, ShaderPrograms are made of several ShaderParts.
    if (metadata->flags.has(AssetFlags::IsAFile)) {
        file_data = readTempFile(metadata->fullName);
    }

    auto& loader = asset_manager().get_asset_loader_for_type(metadata->type);
    auto asset = loader.load_asset(*metadata, file_data);
    if (asset.is_error()) {
        logError("Failed to load asset {}: {}"_s, { metadata->fullName, asset.error() });
        metadata->state = AssetMetadata::State::Error;
    } else {
        metadata->loaded_asset = asset.release_value();
        metadata->state = AssetMetadata::State::Loaded;
    }
}

void unloadAsset(AssetMetadata* asset)
{
    DEBUG_FUNCTION();

    if (asset->state == AssetMetadata::State::Unloaded)
        return;

    if (!asset->children.isEmpty()) {
        for (auto const& child : asset->children)
            removeAsset(child);
        asset->children = makeEmptyArray<AssetRef>();
    }

    asset->loaded_asset->unload(*asset);
    if (asset->children_data.data() != nullptr) {
        s_assets->assetMemoryAllocated -= asset->children_data.size();
        deallocateRaw(asset->children_data.writable_data());
        asset->children_data = {};
    }
    asset->state = AssetMetadata::State::Unloaded;
}

void removeAsset(AssetType type, String name)
{
    AssetMetadata* asset = getAssetIfExists(type, name);
    if (asset == nullptr) {
        logError("Attempted to remove an asset (name `{0}`, type {1}) which doesn't exist!"_s, { name, formatInt(type) });
    } else {
        unloadAsset(asset);
        s_assets->assetsByType[type].removeKey(name);
    }
}

void removeAsset(AssetRef const& ref)
{
    removeAsset(ref.type(), ref.name());
}

void AssetManager::load_assets()
{
    DEBUG_FUNCTION();

    for (auto it = allAssets.iterate(); it.hasNext(); it.next()) {
        auto& asset = it.get();
        loadAsset(&asset);
    }

    m_asset_generation++;

    for (auto it = listeners.iterate(); it.hasNext(); it.next()) {
        it.getValue()->after_assets_loaded();
    }
}

void AssetManager::scan_assets_from_directory(String subdirectory, Optional<AssetType> manual_asset_type)
{
    String pathToScan;
    if (subdirectory.is_empty()) {
        pathToScan = constructPath({ s_assets->assetsPath });
    } else {
        pathToScan = constructPath({ s_assets->assetsPath, subdirectory });
    }

    auto assetFlags = default_asset_flags;

    auto iterate = iterate_directory(pathToScan);
    if (iterate.is_error()) {
        logError("Failed to iterate asset directory: {}"_s, { iterate.error() });
        return;
    }
    for (auto it : iterate.release_value()) {
        if (it.is_error()) {
            logError("Failed to iterate asset directory: {}"_s, { it.error() });
            break;
        }
        auto& file_info = it.value();

        if (file_info.flags.has(FileFlags::Directory) || file_info.flags.has(FileFlags::Hidden)
            || (file_info.filename[0] == '.')) {
            continue;
        }

        String filename = assetStrings.intern(file_info.filename);
        auto assetType = [this, &manual_asset_type, &filename]() -> Optional<AssetType> {
            // Attempt to categorise the asset based on file extension
            if (manual_asset_type.has_value())
                return manual_asset_type.value();
            auto file_extension = get_file_extension(filename);
            return fileExtensionToType.find_value(file_extension.deprecated_to_string());
        }();

        if (assetType.has_value()) {
            add_asset(assetType.value(), filename, assetFlags);
        } else {
            logInfo("Skipping unrecognized asset file `{}`"_s, { filename });
        }
    }
}

void AssetManager::scan_assets()
{
    DEBUG_FUNCTION();

    scan_assets_from_directory({});

    for (auto it = s_assets->directoryNameToType.iterate();
        it.hasNext();
        it.next()) {
        auto entry = it.getEntry();
        scan_assets_from_directory(entry->key, entry->value);
    }
}

bool AssetManager::have_asset_files_changed() const
{
    auto result = asset_change_handle->has_changed();
    if (result.is_error()) {
        logError("Failed to check for asset changes: {}"_s, { result.error() });
        return false;
    }
    return result.value();
}

void AssetManager::reload()
{
    DEBUG_FUNCTION();

    // Preparation
    logInfo("Reloading assets..."_s);
    for (auto it = listeners.iterate(); it.hasNext(); it.next()) {
        it.getValue()->before_assets_unloaded();
    }

    // Clear managed s_assets
    for (auto it = allAssets.iterate(); it.hasNext(); it.next()) {
        auto& asset = it.get();
        unloadAsset(&asset);
    }

    // Clear the hash tables
    for (auto asset_type : enum_values<AssetType>()) {
        assetsByType[asset_type].clear();

        // Reset missing text warnings
        missingAssetNames[asset_type].clear();
    }

    missingTextIDs.clear();

    // Regenerate asset catalogue
    allAssets.clear();
    scan_assets();
    load_assets();
    logInfo("AssetManager reloaded successfully!"_s);
}

AssetMetadata& getAsset(AssetType type, String shortName)
{
    DEBUG_FUNCTION();
    AssetMetadata* asset = getAssetIfExists(type, shortName);
    if (asset && asset->state == AssetMetadata::State::Loaded)
        return *asset;

    if (s_assets->missingAssetNames[type].add(shortName)) {
        logWarn("Requested {0} asset '{1}' was missing or unusable! Using placeholder."_s, { asset_type_names[type], shortName });
    }

    return s_assets->placeholderAssets[type];
}

AssetMetadata* getAssetIfExists(AssetType type, String shortName)
{
    auto asset = s_assets->assetsByType[type].find_value(shortName);
    return asset.value_or(nullptr);
}

BitmapFont& getFont(AssetRef const& fontRef)
{
    ASSERT(fontRef.type() == AssetType::BitmapFont);
    return dynamic_cast<DeprecatedAsset&>(*fontRef.get().loaded_asset).bitmapFont;
}

String getText(String name)
{
    DEBUG_FUNCTION();

    name.hash();

    String result = name;

    auto found_text = s_assets->texts.find_value(name);
    if (found_text.has_value()) {
        result = found_text.release_value();
    } else {
        // Try to fall back to english if possible
        auto default_text = s_assets->defaultTexts.find_value(name);
        if (default_text.has_value()) {
            result = default_text.release_value();
        }

        // Dilemma: We probably want to report missing texts, somehow... because we'd want to know
        // that a text is missing! But, we only want to report each missing text once, otherwise
        // we'll spam the console with the same warning about the same missing text over and over!
        // We used to avoid that by logging the warning, then inserting the text's name into the
        // table as its value, so that future getText() calls would find it.
        // However, now there are two places we look: the texts table, and defaultTexts. Falling
        // back to a default text should also provide some kind of lesser warning, as it could be
        // intentional. In any case, we can't only warn once unless we copy the default into texts,
        // but then when the defaults file is unloaded, it won't unload the copy (though it will
        // erase the memory that the copy points to) so we can't do that.
        //
        // Had two different ideas for solutions. One is we don't store Strings, but a wrapper that
        // also has a "thing was missing" flag somehow. That doesn't really work when I think about
        // it though... Idea 2 is to keep a Set of missing strings, add stuff to it here, and write
        // it to a file at some point. (eg, any time the set gets more members.)

        // What we're doing for now is to only report a missing text if it's not in the missingTextIDs
        // set. (And then add it.)

        if (s_assets->missingTextIDs.add(name)) {
            if (default_text.has_value()) {
                logWarn("Locale {0} is missing text for '{1}'. (Fell back to using the default locale.)"_s, { to_string(get_locale()), name });
            } else {
                logWarn("Locale {0} is missing text for '{1}'. (No default found!)"_s, { to_string(get_locale()), name });
            }
        }
    }

    return result;
}

String getText(String name, std::initializer_list<StringView> args)
{
    String format = getText(name);

    return myprintf(format, args);
}

String AssetManager::make_asset_path(AssetType type, StringView short_name) const
{
    switch (type) {
    case AssetType::Cursor:
        return myprintf("{0}/cursors/{1}"_s, { s_assets->assetsPath, short_name }, true);
    case AssetType::BitmapFont:
        return myprintf("{0}/fonts/{1}"_s, { s_assets->assetsPath, short_name }, true);
    case AssetType::Shader:
        return myprintf("{0}/shaders/{1}"_s, { s_assets->assetsPath, short_name }, true);
    case AssetType::Texts:
        return myprintf("{0}/locale/{1}"_s, { s_assets->assetsPath, short_name }, true);
    case AssetType::Texture:
        return myprintf("{0}/textures/{1}"_s, { s_assets->assetsPath, short_name }, true);
    default:
        return myprintf("{0}/{1}"_s, { s_assets->assetsPath, short_name }, true);
    }
}

void AssetManager::register_asset_loader(NonnullOwnPtr<AssetLoader>&& asset_loader_ptr)
{
    auto& asset_loader = *asset_loader_ptr;
    asset_loaders.append(move(asset_loader_ptr));

    asset_loader.register_types(*this);
    asset_loader.create_placeholder_assets(*this);
}

AssetLoader& AssetManager::get_asset_loader_for_type(AssetType type) const
{
    auto* result = asset_loaders_by_type[type];
    ASSERT(result);
    return *result;
}

void AssetManager::on_settings_changed()
{
    // Reload locale-specific assets

    // Clear the list of missing texts because they might not be missing in the new locale!
    s_assets->missingTextIDs.clear();

    for (auto it = s_assets->allAssets.iterate(); it.hasNext(); it.next()) {
        auto& asset = it.get();
        if (asset.locale.has_value()) {
            unloadAsset(&asset);
        }
    }

    for (auto it = s_assets->allAssets.iterate(); it.hasNext(); it.next()) {
        auto& asset = it.get();
        // FIXME: Only try to load if the locale matches.
        if (asset.locale.has_value()) {
            loadAsset(&asset);
        }
    }
}

void AssetManager::register_listener(AssetManagerListener* listener)
{
    listeners.append(listener);
}

void AssetManager::unregister_listener(AssetManagerListener* listener)
{
    listeners.findAndRemove(listener);
}
