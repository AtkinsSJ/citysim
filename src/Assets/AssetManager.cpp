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

AssetMetadata* addNinepatch(StringView name, StringView filename, s32 pu0, s32 pu1, s32 pu2, s32 pu3, s32 pv0, s32 pv1, s32 pv2, s32 pv3)
{
    AssetMetadata* texture = addTexture(filename, false);

    AssetMetadata* metadata = asset_manager().add_asset(AssetType::Ninepatch, name, {});
    auto& asset = reinterpret_cast<DeprecatedAsset&>(*metadata->loaded_asset);

    Ninepatch* ninepatch = &asset.ninepatch;
    ninepatch->texture = texture;

    ninepatch->pu0 = pu0;
    ninepatch->pu1 = pu1;
    ninepatch->pu2 = pu2;
    ninepatch->pu3 = pu3;

    ninepatch->pv0 = pv0;
    ninepatch->pv1 = pv1;
    ninepatch->pv2 = pv2;
    ninepatch->pv3 = pv3;

    return metadata;
}

AssetMetadata* addSpriteGroup(StringView name, s32 spriteCount)
{
    ASSERT(spriteCount > 0); // Must have a positive number of sprites in a Sprite Group!

    AssetMetadata* metadata = asset_manager().add_asset(AssetType::Sprite, name, {});
    auto& asset = reinterpret_cast<DeprecatedAsset&>(*metadata->loaded_asset);
    if (asset.data.size() != 0)
        DEBUG_BREAK(); // @Leak! Creating the sprite group multiple times is probably a bad idea for other reasons too.
    asset.data = assetsAllocate(s_assets, spriteCount * sizeof(Sprite));
    asset.spriteGroup.count = spriteCount;
    asset.spriteGroup.sprites = (Sprite*)asset.data.writable_data();

    return metadata;
}

AssetMetadata* addTexture(StringView filename, bool isAlphaPremultiplied)
{
    AssetMetadata* metadata = asset_manager().add_asset(AssetType::Texture, filename);
    auto& asset = reinterpret_cast<DeprecatedAsset&>(*metadata->loaded_asset);
    asset.texture.isFileAlphaPremultiplied = isAlphaPremultiplied;

    return metadata;
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
    return reinterpret_cast<DeprecatedAsset&>(*fontRef.get().loaded_asset).bitmapFont;
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

void loadCursorDefs(Blob data, AssetMetadata& metadata, DeprecatedAsset&)
{
    DEBUG_FUNCTION();

    LineReader reader { metadata.shortName, data };

    // We store the cursorNames array in the defs asset
    // So, we first need to scan through the file to see how many cursors there are in it!
    s32 cursorCount = 0;
    while (reader.load_next_line()) {
        cursorCount++;
    }

    allocateChildren(&metadata, cursorCount);

    reader.restart();

    while (reader.load_next_line()) {
        auto name_token = reader.next_token();
        auto filename = reader.next_token();
        if (!name_token.has_value() || !filename.has_value())
            continue;

        String name = asset_manager().assetStrings.intern(name_token.release_value());

        auto hot_x = reader.read_int<s32>();
        auto hot_y = reader.read_int<s32>();

        if (hot_x.has_value() && hot_y.has_value()) {
            // Add the cursor
            AssetMetadata* cursor_metadata = asset_manager().add_asset(AssetType::Cursor, name, {});
            auto& cursor_asset = reinterpret_cast<DeprecatedAsset&>(*cursor_metadata->loaded_asset);
            cursor_asset.cursor.imageFilePath = asset_manager().assetStrings.intern(asset_manager().make_asset_path(AssetType::Cursor, filename.value()));
            cursor_asset.cursor.hotspot = v2i(hot_x.release_value(), hot_y.release_value());
            addChildAsset(&metadata, cursor_metadata);
        } else {
            reader.error("Couldn't parse cursor definition. Expected 'name filename.png hot-x hot-y'."_s);
            return;
        }
    }
}

void loadPaletteDefs(Blob data, AssetMetadata& metadata, DeprecatedAsset&)
{
    DEBUG_FUNCTION();

    LineReader reader { metadata.shortName, data };

    // We store the paletteNames array in the defs asset
    // So, we first need to scan through the file to see how many palettes there are in it!
    s32 paletteCount = 0;
    while (reader.load_next_line()) {
        if (auto command = reader.next_token(); command == ":Palette"_s)
            paletteCount++;
    }

    allocateChildren(&metadata, paletteCount);

    reader.restart();

    AssetMetadata* palette_metadata = nullptr;
    while (reader.load_next_line()) {
        auto command_token = reader.next_token();
        if (!command_token.has_value())
            continue;
        auto command = command_token.release_value();

        if (command.starts_with(':')) {
            command = command.substring(1);

            if (command == "Palette"_s) {
                if (auto palette_name = reader.next_token(); palette_name.has_value()) {
                    palette_metadata = asset_manager().add_asset(AssetType::Palette, palette_name.release_value(), {});
                    addChildAsset(&metadata, palette_metadata);
                } else {
                    reader.error("Missing name for Palette"_s);
                    return;
                }
            } else {
                reader.error("Unexpected command ':{0}' in palette-definitions file. Only :Palette is allowed!"_s, { command });
                return;
            }
        } else {
            if (palette_metadata == nullptr) {
                reader.error("Unexpected command '{0}' before the start of a :Palette"_s, { command });
                return;
            }
            auto& palette_asset = reinterpret_cast<DeprecatedAsset&>(*palette_metadata->loaded_asset);

            if (command == "type"_s) {
                auto type = reader.next_token();
                if (!type.has_value()) {
                    reader.error("Missing palette type"_s);
                    return;
                }

                if (type == "fixed"_s) {
                    palette_asset.palette.type = Palette::Type::Fixed;
                } else if (type == "gradient"_s) {
                    palette_asset.palette.type = Palette::Type::Gradient;
                } else {
                    reader.error("Unrecognised palette type '{0}', allowed values are: fixed, gradient"_s, { type.value() });
                    return;
                }
            } else if (command == "size"_s) {
                if (auto size = reader.read_int<s32>(); size.has_value()) {
                    palette_asset.palette.size = size.release_value();
                } else {
                    return;
                }
            } else if (command == "color"_s) {
                if (auto color = Colour::read(reader); color.has_value()) {
                    if (palette_asset.palette.type == Palette::Type::Fixed) {
                        if (!palette_asset.palette.paletteData.isInitialised()) {
                            palette_asset.data = assetsAllocate(s_assets, palette_asset.palette.size * sizeof(Colour));
                            palette_asset.palette.paletteData = makeArray<Colour>(palette_asset.palette.size, reinterpret_cast<Colour*>(palette_asset.data.writable_data()));
                        }

                        s32 colorIndex = palette_asset.palette.paletteData.count;
                        if (colorIndex >= palette_asset.palette.size) {
                            reader.error("Too many 'color' definitions! 'size' must be large enough."_s);
                            return;
                        }
                        palette_asset.palette.paletteData.append(color.release_value());
                    } else {
                        reader.error("'color' is only a valid command for fixed palettes."_s);
                        return;
                    }
                }
            } else if (command == "from"_s) {
                if (auto from = Colour::read(reader); from.has_value()) {
                    if (palette_asset.palette.type == Palette::Type::Gradient) {
                        palette_asset.palette.gradient.from = from.release_value();
                    } else {
                        reader.error("'from' is only a valid command for gradient palettes."_s);
                        return;
                    }
                }
            } else if (command == "to"_s) {
                if (auto to = Colour::read(reader); to.has_value()) {
                    if (palette_asset.palette.type == Palette::Type::Gradient) {
                        palette_asset.palette.gradient.to = to.release_value();
                    } else {
                        reader.error("'to' is only a valid command for gradient palettes."_s);
                        return;
                    }
                }
            } else {
                reader.error("Unrecognised command '{0}'"_s, { command });
                return;
            }
        }
    }
}

void loadSpriteDefs(Blob data, AssetMetadata& metadata, DeprecatedAsset&)
{
    DEBUG_FUNCTION();

    LineReader reader { metadata.shortName, data };

    AssetMetadata* textureAsset = nullptr;
    V2I spriteSize = v2i(0, 0);
    V2I spriteBorder = v2i(0, 0);
    AssetMetadata* spriteGroup = nullptr;
    s32 spriteIndex = 0;

    // Count the number of child s_assets, so we can allocate our spriteNames array
    s32 childAssetCount = 0;
    while (reader.load_next_line()) {
        if (auto command = reader.next_token(); command.has_value() && command.value().starts_with(':'))
            childAssetCount++;
    }
    allocateChildren(&metadata, childAssetCount);

    reader.restart();

    // Now, actually read things
    while (reader.load_next_line()) {
        auto maybe_command = reader.next_token();
        if (!maybe_command.has_value())
            continue;
        auto command = maybe_command.release_value();

        if (command.starts_with(':')) // Definitions
        {
            // Define something
            command = command.substring(1).deprecated_to_string();

            textureAsset = nullptr;
            spriteGroup = nullptr;

            if (command == "Ninepatch"_s) {
                auto name = reader.next_token();
                auto filename = reader.next_token();
                auto pu0 = reader.read_int<s32>();
                auto pu1 = reader.read_int<s32>();
                auto pu2 = reader.read_int<s32>();
                auto pu3 = reader.read_int<s32>();
                auto pv0 = reader.read_int<s32>();
                auto pv1 = reader.read_int<s32>();
                auto pv2 = reader.read_int<s32>();
                auto pv3 = reader.read_int<s32>();

                if (!all_have_values(name, filename, pu0, pu1, pu2, pu3, pv0, pv1, pv2, pv3)) {
                    reader.error("Couldn't parse Ninepatch. Expected: ':Ninepatch identifier filename.png pu0 pu1 pu2 pu3 pv0 pv1 pv2 pv3'"_s);
                    return;
                }

                AssetMetadata* ninepatch = addNinepatch(name.release_value(), filename.release_value(), pu0.release_value(), pu1.release_value(), pu2.release_value(), pu3.release_value(), pv0.release_value(), pv1.release_value(), pv2.release_value(), pv3.release_value());

                addChildAsset(&metadata, ninepatch);
            } else if (command == "Sprite"_s) {
                // @Copypasta from the SpriteGroup branch, and the 'sprite' property
                auto name = reader.next_token();
                auto filename = reader.next_token();
                auto spriteSizeIn = V2I::read(reader);

                if (!all_have_values(name, filename, spriteSizeIn)) {
                    reader.error("Couldn't parse Sprite. Expected: ':Sprite identifier filename.png SWxSH'"_s);
                    return;
                }

                spriteSize = spriteSizeIn.release_value();

                AssetMetadata* group = addSpriteGroup(name.release_value(), 1);
                auto& group_asset = reinterpret_cast<DeprecatedAsset&>(*group->loaded_asset);

                Sprite* sprite = group_asset.spriteGroup.sprites;
                sprite->texture = addTexture(filename.release_value(), false);
                sprite->uv = { 0, 0, spriteSize.x, spriteSize.y };
                sprite->pixelWidth = spriteSize.x;
                sprite->pixelHeight = spriteSize.y;

                addChildAsset(&metadata, group);
            } else if (command == "SpriteGroup"_s) {
                auto name = reader.next_token();
                auto filename = reader.next_token();
                auto spriteSizeIn = V2I::read(reader);

                if (!all_have_values(name, filename, spriteSizeIn)) {
                    reader.error("Couldn't parse SpriteGroup. Expected: ':SpriteGroup identifier filename.png SWxSH'"_s);
                    return;
                }

                textureAsset = addTexture(filename.release_value(), false);
                spriteSize = spriteSizeIn.release_value();

                s32 spriteCount = reader.count_occurrences_of_property_in_current_command("sprite"_s);
                if (spriteCount < 1) {
                    reader.error("SpriteGroup must contain at least 1 sprite!"_s);
                    return;
                }
                spriteGroup = addSpriteGroup(name.release_value(), spriteCount);
                spriteIndex = 0;

                addChildAsset(&metadata, spriteGroup);
            } else {
                reader.error("Unrecognised command: '{0}'"_s, { command });
                return;
            }
        } else // Properties!
        {
            if (spriteGroup == nullptr) {
                reader.error("Found a property outside of a :SpriteGroup!"_s);
                return;
            } else if (command == "border"_s) {
                auto borderW = reader.read_int<s32>();
                auto borderH = reader.read_int<s32>();
                if (borderW.has_value() && borderH.has_value()) {
                    spriteBorder = v2i(borderW.release_value(), borderH.release_value());
                } else {
                    reader.error("Couldn't parse border. Expected 'border width height'."_s);
                    return;
                }
            } else if (command == "sprite"_s) {
                auto mx = reader.read_int<s32>();
                auto my = reader.read_int<s32>();

                if (mx.has_value() && my.has_value()) {
                    s32 x = mx.release_value();
                    s32 y = my.release_value();

                    auto& group_asset = reinterpret_cast<DeprecatedAsset&>(*spriteGroup->loaded_asset);
                    Sprite* sprite = group_asset.spriteGroup.sprites + spriteIndex;
                    sprite->texture = textureAsset;
                    sprite->uv = { spriteBorder.x + x * (spriteSize.x + spriteBorder.x + spriteBorder.x),
                        spriteBorder.y + y * (spriteSize.y + spriteBorder.y + spriteBorder.y),
                        spriteSize.x, spriteSize.y };
                    sprite->pixelWidth = spriteSize.x;
                    sprite->pixelHeight = spriteSize.y;

                    spriteIndex++;
                } else {
                    reader.error("Couldn't parse {0}. Expected '{0} x y'."_s, { command });
                    return;
                }
            } else {
                reader.error("Unrecognised command '{0}'"_s, { command });
                return;
            }
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
