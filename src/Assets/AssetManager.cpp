/*
 * Copyright (c) 2016-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Assets/AssetManager.h>
#include <Debug/Console.h>
#include <Gfx/BMFont.h>
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
#include <Sim/TerrainCatalogue.h>

AssetManager* s_assets;

void initAssets()
{
    s_assets = MemoryArena::bootstrap<AssetManager>("Assets"_s);

    String basePath = String::from_null_terminated(SDL_GetBasePath());
    s_assets->assetsPath = intern(&s_assets->assetStrings, constructPath({ basePath, "assets"_s }));

    // NB: We only need to define these for s_assets in the root s_assets/ directory
    // Well, for now at least.
    // - Sam, 19/05/2019
    s_assets->fileExtensionToType.put(intern(&s_assets->assetStrings, "buildings"_s), AssetType::BuildingDefs);
    s_assets->fileExtensionToType.put(intern(&s_assets->assetStrings, "cursors"_s), AssetType::CursorDefs);
    s_assets->fileExtensionToType.put(intern(&s_assets->assetStrings, "keymap"_s), AssetType::DevKeymap);
    s_assets->fileExtensionToType.put(intern(&s_assets->assetStrings, "palettes"_s), AssetType::PaletteDefs);
    s_assets->fileExtensionToType.put(intern(&s_assets->assetStrings, "sprites"_s), AssetType::SpriteDefs);
    s_assets->fileExtensionToType.put(intern(&s_assets->assetStrings, "terrain"_s), AssetType::TerrainDefs);
    s_assets->fileExtensionToType.put(intern(&s_assets->assetStrings, "theme"_s), AssetType::UITheme);

    s_assets->directoryNameToType.put(intern(&s_assets->assetStrings, "fonts"_s), AssetType::BitmapFont);
    s_assets->directoryNameToType.put(intern(&s_assets->assetStrings, "shaders"_s), AssetType::Shader);
    s_assets->directoryNameToType.put(intern(&s_assets->assetStrings, "textures"_s), AssetType::Texture);
    s_assets->directoryNameToType.put(intern(&s_assets->assetStrings, "locale"_s), AssetType::Texts);

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

    // Placeholder s_assets!
    {
        // BitmapFont
        makePlaceholderAsset(AssetType::BitmapFont);

        // BuildingDefs
        makePlaceholderAsset(AssetType::BuildingDefs);

        // Cursor
        Asset* placeholderCursor = makePlaceholderAsset(AssetType::Cursor);
        placeholderCursor->cursor.sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);

        // CursorDefs
        makePlaceholderAsset(AssetType::CursorDefs);

        // DevKeymap
        makePlaceholderAsset(AssetType::DevKeymap);

        // Ninepatch
        Asset* placeholderNinepatch = makePlaceholderAsset(AssetType::Ninepatch);
        placeholderNinepatch->ninepatch.texture = &s_assets->placeholderAssets[AssetType::Texture];

        // Palette
        Asset* placeholderPalette = makePlaceholderAsset(AssetType::Palette);
        placeholderPalette->palette.type = Palette::Type::Fixed;
        placeholderPalette->palette.size = 0;
        placeholderPalette->palette.paletteData = makeEmptyArray<Colour>();

        // PaletteDefs
        makePlaceholderAsset(AssetType::PaletteDefs);

        // Shader
        makePlaceholderAsset(AssetType::Shader);

        // Sprite!
        Asset* placeholderSprite = makePlaceholderAsset(AssetType::Sprite);
        placeholderSprite->data = assetsAllocate(s_assets, 1 * sizeof(Sprite));
        placeholderSprite->spriteGroup.count = 1;
        placeholderSprite->spriteGroup.sprites = (Sprite*)placeholderSprite->data.writable_data();
        placeholderSprite->spriteGroup.sprites[0].texture = &s_assets->placeholderAssets[AssetType::Texture];
        placeholderSprite->spriteGroup.sprites[0].uv = { 0.0f, 0.0f, 1.0f, 1.0f };

        // SpriteDefs
        makePlaceholderAsset(AssetType::SpriteDefs);

        // TerrainDefs
        makePlaceholderAsset(AssetType::TerrainDefs);

        // Texts
        makePlaceholderAsset(AssetType::Texts);

        // Texture
        Asset* placeholderTexture = makePlaceholderAsset(AssetType::Texture);
        placeholderTexture->data = assetsAllocate(s_assets, 2 * 2 * sizeof(u32));
        u32* pixels = (u32*)placeholderTexture->data.writable_data();
        pixels[0] = pixels[3] = 0xffff00ff;
        pixels[1] = pixels[2] = 0xff000000;
        placeholderTexture->texture.surface = SDL_CreateRGBSurfaceFrom(pixels, 2, 2, 32, 2 * sizeof(u32),
            0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
        placeholderTexture->texture.isFileAlphaPremultiplied = true;

        // UITheme
        makePlaceholderAsset(AssetType::UITheme);
    }

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

Asset* makePlaceholderAsset(AssetType type)
{
    Asset* result = &s_assets->placeholderAssets[type];
    result->type = type;
    result->shortName = nullString;
    result->fullName = nullString;
    result->flags = {};
    result->state = Asset::State::Loaded;
    result->children = makeEmptyArray<AssetRef>();
    result->data = {};

    return result;
}

Blob assetsAllocate(AssetManager* theAssets, smm size)
{
    Blob result { size, allocateRaw(size) };

    theAssets->assetMemoryAllocated += size;
    theAssets->maxAssetMemoryAllocated = max(theAssets->assetMemoryAllocated, theAssets->maxAssetMemoryAllocated);

    return result;
}

void allocateChildren(Asset* asset, s32 childCount)
{
    asset->data = assetsAllocate(s_assets, childCount * sizeof(AssetRef));
    asset->children = makeArray(childCount, reinterpret_cast<AssetRef*>(asset->data.writable_data()));
}

void addChildAsset(Asset* parent, Asset* child)
{
    parent->children.append(AssetRef { child->type, child->shortName });
}

Asset* addAsset(AssetType type, String shortName, Flags<AssetFlags> flags)
{
    String internedShortName = intern(&s_assets->assetStrings, shortName);

    Asset* existing = getAssetIfExists(type, internedShortName);
    if (existing)
        return existing;

    Asset* asset = s_assets->allAssets.appendBlank();
    asset->type = type;
    asset->shortName = internedShortName;
    if (flags.has(AssetFlags::IsAFile)) {
        asset->fullName = intern(&s_assets->assetStrings, getAssetPath(asset->type, internedShortName));
        if (auto locale_string = get_file_locale_segment(asset->fullName); locale_string.has_value()) {
            asset->locale = locale_from_string(locale_string.value());
            if (!asset->locale.has_value())
                logWarn("Unrecognized locale for asset '{0}'."_s, { asset->fullName });
        }
    }
    asset->state = Asset::State::Unloaded;
    asset->data = {};
    asset->flags = flags;

    s_assets->assetsByType[type].put(internedShortName, asset);

    return asset;
}

void copyFileIntoAsset(Blob* fileData, Asset* asset)
{
    asset->data = assetsAllocate(s_assets, fileData->size());
    memcpy(asset->data.writable_data(), fileData->data(), fileData->size());

    // NB: We set the fileData to point at the new copy, so that code after calling copyFileIntoAsset()
    // can still use fileData without having to mess with it. Already had one bug caused by not doing this!
    // FIXME: Stop doing this?
    *fileData = asset->data;
}

SDL_Surface* createSurfaceFromFileData(Blob fileData, String name)
{
    SDL_Surface* result = nullptr;

    ASSERT(fileData.size() > 0);      //, "Attempted to create a surface from an unloaded asset! ({0})", {name});
    ASSERT(fileData.size() < s32Max); //, "File '{0}' is too big for SDL's RWOps!", {name});

    SDL_RWops* rw = SDL_RWFromConstMem(fileData.data(), truncate32(fileData.size()));
    if (rw) {
        result = IMG_Load_RW(rw, 0);

        if (result == nullptr) {
            logError("Failed to create SDL_Surface from asset '{0}'!\n{1}"_s, { name, String::from_null_terminated(IMG_GetError()) });
        }

        SDL_RWclose(rw);
    } else {
        logError("Failed to create SDL_RWops from asset '{0}'!\n{1}"_s, { name, String::from_null_terminated(SDL_GetError()) });
    }

    return result;
}

void ensureAssetIsLoaded(Asset* asset)
{
    if (asset->state == Asset::State::Loaded)
        return;

    loadAsset(asset);

    if (asset->state != Asset::State::Loaded) {
        logError("Failed to load asset '{0}'"_s, { asset->shortName });
    }
}

void loadAsset(Asset* asset)
{
    DEBUG_FUNCTION();
    if (asset->state != Asset::State::Unloaded)
        return;

    if (asset->locale.has_value()) {
        // Only load assets that match our locale
        if (asset->locale == get_locale()) {
            asset->texts.isFallbackLocale = false;
        } else {
            if (asset->locale == Locale::En) {
                logInfo("Loading asset {0} as a default-locale fallback. (Locale {1}, current is {2})"_s, { asset->fullName, to_string(asset->locale.value()), to_string(get_locale()) });
                asset->texts.isFallbackLocale = true;
            } else {
                logInfo("Skipping asset {0} because it's the wrong locale. ({1}, current is {2})"_s, { asset->fullName, to_string(asset->locale.value()), to_string(get_locale()) });
                return;
            }
        }
    }

    Blob fileData = {};
    // Some s_assets (meta-s_assets?) have no file associated with them, because they are composed of other s_assets.
    // eg, ShaderPrograms are made of several ShaderParts.
    if (asset->flags.has(AssetFlags::IsAFile)) {
        fileData = readTempFile(asset->fullName);
    }

    // Type-specific loading
    switch (asset->type) {
    case AssetType::BitmapFont: {
        loadBMFont(fileData, asset);
        asset->state = Asset::State::Loaded;
    } break;

    case AssetType::BuildingDefs: {
        loadBuildingDefs(fileData, asset);
        asset->state = Asset::State::Loaded;
    } break;

    case AssetType::Cursor: {
        fileData = readTempFile(asset->cursor.imageFilePath);
        SDL_Surface* cursorSurface = createSurfaceFromFileData(fileData, asset->shortName);
        asset->cursor.sdlCursor = SDL_CreateColorCursor(cursorSurface, asset->cursor.hotspot.x, asset->cursor.hotspot.y);
        SDL_FreeSurface(cursorSurface);
        asset->state = Asset::State::Loaded;
    } break;

    case AssetType::CursorDefs: {
        loadCursorDefs(fileData, asset);
        asset->state = Asset::State::Loaded;
    } break;

    case AssetType::DevKeymap: {
        if (globalConsole != nullptr) {
            // NB: We keep the keymap file in the asset memory, so that the CommandShortcut.command can
            // directly refer to the string data from the file, instead of having to assetsAllocate a copy
            // and not be able to free it ever. This is more memory efficient.
            copyFileIntoAsset(&fileData, asset);
            loadConsoleKeyboardShortcuts(globalConsole, fileData, asset->shortName);
        }
        asset->state = Asset::State::Loaded;
    } break;

    case AssetType::Ninepatch: {
        // Convert UVs from pixel space to 0-1 space
        Asset* t = asset->ninepatch.texture;
        ensureAssetIsLoaded(t);
        float textureWidth = (float)t->texture.surface->w;
        float textureHeight = (float)t->texture.surface->h;

        asset->ninepatch.u0 = asset->ninepatch.pu0 / textureWidth;
        asset->ninepatch.u1 = asset->ninepatch.pu1 / textureWidth;
        asset->ninepatch.u2 = asset->ninepatch.pu2 / textureWidth;
        asset->ninepatch.u3 = asset->ninepatch.pu3 / textureWidth;

        asset->ninepatch.v0 = asset->ninepatch.pv0 / textureHeight;
        asset->ninepatch.v1 = asset->ninepatch.pv1 / textureHeight;
        asset->ninepatch.v2 = asset->ninepatch.pv2 / textureHeight;
        asset->ninepatch.v3 = asset->ninepatch.pv3 / textureHeight;

        asset->state = Asset::State::Loaded;
    } break;

    case AssetType::Palette: {
        Palette* palette = &asset->palette;
        switch (palette->type) {
        case Palette::Type::Gradient: {
            asset->data = assetsAllocate(s_assets, palette->size * sizeof(Colour));
            palette->paletteData = makeArray<Colour>(palette->size, reinterpret_cast<Colour*>(asset->data.writable_data()), palette->size);

            float ratio = 1.0f / (float)(palette->size);
            for (s32 i = 0; i < palette->size; i++) {
                palette->paletteData[i] = lerp(palette->gradient.from, palette->gradient.to, i * ratio);
            }
        } break;

        case Palette::Type::Fixed: {
        } break;

            INVALID_DEFAULT_CASE;
        }
        asset->state = Asset::State::Loaded;
    } break;

    case AssetType::PaletteDefs: {
        loadPaletteDefs(fileData, asset);
        asset->state = Asset::State::Loaded;
    } break;

    case AssetType::Shader: {
        copyFileIntoAsset(&fileData, asset);
        splitInTwo(String::from_blob(fileData).value(), '$', &asset->shader.vertexShader, &asset->shader.fragmentShader);
        asset->state = Asset::State::Loaded;
    } break;

    case AssetType::Sprite: {
        // Convert UVs from pixel space to 0-1 space
        for (s32 i = 0; i < asset->spriteGroup.count; i++) {
            Sprite* sprite = asset->spriteGroup.sprites + i;
            Asset* t = sprite->texture;
            ensureAssetIsLoaded(t);
            float textureWidth = (float)t->texture.surface->w;
            float textureHeight = (float)t->texture.surface->h;

            sprite->uv = {
                sprite->uv.x() / textureWidth,
                sprite->uv.y() / textureHeight,
                sprite->uv.width() / textureWidth,
                sprite->uv.height() / textureHeight
            };
        }

        asset->state = Asset::State::Loaded;
    } break;

    case AssetType::SpriteDefs: {
        loadSpriteDefs(fileData, asset);
        asset->state = Asset::State::Loaded;
    } break;

    case AssetType::TerrainDefs: {
        loadTerrainDefs(fileData, asset);
        asset->state = Asset::State::Loaded;
    } break;

    case AssetType::Texts: {
        HashTable<String>* textsTable = (asset->texts.isFallbackLocale ? &s_assets->defaultTexts : &s_assets->texts);
        loadTexts(textsTable, asset, fileData);
        asset->state = Asset::State::Loaded;
    } break;

    case AssetType::Texture: {
        // TODO: Emergency debug texture that's used if loading a file fails.
        // Right now, we just crash! (Not shippable)
        SDL_Surface* surface = createSurfaceFromFileData(fileData, asset->fullName);
        if (surface->format->BytesPerPixel != 4) {
            logError("Texture asset '{0}' is not 32bit, which is all we support right now. (BytesPerPixel = {1})"_s, { asset->shortName, formatInt(surface->format->BytesPerPixel) });
            return;
        }

        if (!asset->texture.isFileAlphaPremultiplied) {
            // Premultiply alpha
            u32 Rmask = surface->format->Rmask,
                Gmask = surface->format->Gmask,
                Bmask = surface->format->Bmask,
                Amask = surface->format->Amask;
            float rRmask = (float)Rmask,
                  rGmask = (float)Gmask,
                  rBmask = (float)Bmask,
                  rAmask = (float)Amask;

            u32 pixelCount = surface->w * surface->h;
            for (u32 pixelIndex = 0;
                pixelIndex < pixelCount;
                pixelIndex++) {
                u32 pixel = ((u32*)surface->pixels)[pixelIndex];
                float rr = (float)(pixel & Rmask) / rRmask;
                float rg = (float)(pixel & Gmask) / rGmask;
                float rb = (float)(pixel & Bmask) / rBmask;
                float ra = (float)(pixel & Amask) / rAmask;

                u32 r = (u32)(rr * ra * rRmask) & Rmask;
                u32 g = (u32)(rg * ra * rGmask) & Gmask;
                u32 b = (u32)(rb * ra * rBmask) & Bmask;
                u32 a = (u32)(ra * rAmask) & Amask;

                ((u32*)surface->pixels)[pixelIndex] = (u32)r | (u32)g | (u32)b | (u32)a;
            }
        }

        asset->texture.surface = surface;
        asset->state = Asset::State::Loaded;
    } break;

    case AssetType::UITheme: {
        loadUITheme(fileData, asset);
        asset->state = Asset::State::Loaded;
    } break;

    default: {
        if (asset->flags.has(AssetFlags::IsAFile)) {
            copyFileIntoAsset(&fileData, asset);
        }

        asset->state = Asset::State::Loaded;
    } break;
    }
}

void unloadAsset(Asset* asset)
{
    DEBUG_FUNCTION();

    if (asset->state == Asset::State::Unloaded)
        return;

    switch (asset->type) {
    case AssetType::BuildingDefs: {
        // Remove all of our terrain defs
        removeBuildingDefs(asset->buildingDefs.buildingIDs);
        asset->buildingDefs.buildingIDs = makeEmptyArray<String>();
    } break;

    case AssetType::Cursor: {
        if (asset->cursor.sdlCursor != nullptr) {
            SDL_FreeCursor(asset->cursor.sdlCursor);
            asset->cursor.sdlCursor = nullptr;
        }
    } break;

    case AssetType::TerrainDefs: {
        // Remove all of our terrain defs
        removeTerrainDefs(asset->terrainDefs.terrainIDs);
        asset->terrainDefs.terrainIDs = makeEmptyArray<String>();
    } break;

    case AssetType::Texts: {
        // Remove all of our texts from the table
        HashTable<String>* textsTable = (asset->texts.isFallbackLocale ? &s_assets->defaultTexts : &s_assets->texts);
        for (auto const& key : asset->texts.keys)
            textsTable->removeKey(key);
        asset->texts.keys = makeEmptyArray<String>();
    } break;

    case AssetType::Texture: {
        if (asset->texture.surface != nullptr) {
            SDL_FreeSurface(asset->texture.surface);
            asset->texture.surface = nullptr;
        }
    } break;

    default:
        break;
    }

    if (!asset->children.isEmpty()) {
        for (auto const& child : asset->children)
            removeAsset(child);
        asset->children = makeEmptyArray<AssetRef>();
    }

    if (asset->data.data() != nullptr) {
        s_assets->assetMemoryAllocated -= asset->data.size();
        deallocateRaw(asset->data.writable_data());
        asset->data = {};
    }

    asset->state = Asset::State::Unloaded;
}

void removeAsset(AssetType type, String name)
{
    Asset* asset = getAssetIfExists(type, name);
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

Asset* addNinepatch(String name, String filename, s32 pu0, s32 pu1, s32 pu2, s32 pu3, s32 pv0, s32 pv1, s32 pv2, s32 pv3)
{
    Asset* texture = addTexture(filename, false);

    Asset* asset = addAsset(AssetType::Ninepatch, name, {});

    Ninepatch* ninepatch = &asset->ninepatch;
    ninepatch->texture = texture;

    ninepatch->pu0 = pu0;
    ninepatch->pu1 = pu1;
    ninepatch->pu2 = pu2;
    ninepatch->pu3 = pu3;

    ninepatch->pv0 = pv0;
    ninepatch->pv1 = pv1;
    ninepatch->pv2 = pv2;
    ninepatch->pv3 = pv3;

    return asset;
}

Asset* addSpriteGroup(String name, s32 spriteCount)
{
    ASSERT(spriteCount > 0); // Must have a positive number of sprites in a Sprite Group!

    Asset* spriteGroup = addAsset(AssetType::Sprite, name, {});
    if (spriteGroup->data.size() != 0)
        DEBUG_BREAK(); // @Leak! Creating the sprite group multiple times is probably a bad idea for other reasons too.
    spriteGroup->data = assetsAllocate(s_assets, spriteCount * sizeof(Sprite));
    spriteGroup->spriteGroup.count = spriteCount;
    spriteGroup->spriteGroup.sprites = (Sprite*)spriteGroup->data.writable_data();

    return spriteGroup;
}

Asset* addTexture(String filename, bool isAlphaPremultiplied)
{
    Asset* asset = addAsset(AssetType::Texture, filename);
    asset->texture.isFileAlphaPremultiplied = isAlphaPremultiplied;

    return asset;
}

void loadAssets()
{
    DEBUG_FUNCTION();

    for (auto it = s_assets->allAssets.iterate(); it.hasNext(); it.next()) {
        Asset* asset = it.get();
        loadAsset(asset);
    }

    s_assets->fixme_increment_asset_generation();

    for (auto it = s_assets->listeners.iterate(); it.hasNext(); it.next()) {
        it.getValue()->after_assets_loaded();
    }
}

void addAssetsFromDirectory(String subDirectory, Optional<AssetType> manualAssetType)
{
    String pathToScan;
    if (subDirectory.is_empty()) {
        pathToScan = constructPath({ s_assets->assetsPath });
    } else {
        pathToScan = constructPath({ s_assets->assetsPath, subDirectory });
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

        String filename = intern(&s_assets->assetStrings, file_info.filename);
        AssetType assetType = [&manualAssetType, &filename]() {
            // Attempt to categorise the asset based on file extension
            if (manualAssetType.has_value())
                return manualAssetType.value();
            String fileExtension = getFileExtension(filename);
            auto foundAssetType = s_assets->fileExtensionToType.find_value(fileExtension);
            return foundAssetType.value_or(AssetType::Misc);
        }();

        addAsset(assetType, filename, assetFlags);
    }
}

void addAssets()
{
    DEBUG_FUNCTION();

    addAssetsFromDirectory(nullString);

    for (auto it = s_assets->directoryNameToType.iterate();
        it.hasNext();
        it.next()) {
        auto entry = it.getEntry();
        addAssetsFromDirectory(entry->key, entry->value);
    }
}

bool haveAssetFilesChanged()
{
    auto result = s_assets->asset_change_handle->has_changed();
    if (result.is_error()) {
        logError("Failed to check for asset changes: {}"_s, { result.error() });
        return false;
    }
    return result.value();
}

void reloadAssets()
{
    DEBUG_FUNCTION();

    // Preparation
    logInfo("Reloading assets..."_s);
    for (auto it = s_assets->listeners.iterate(); it.hasNext(); it.next()) {
        it.getValue()->before_assets_unloaded();
    }

    // Clear managed s_assets
    for (auto it = s_assets->allAssets.iterate(); it.hasNext(); it.next()) {
        Asset* asset = it.get();
        unloadAsset(asset);
    }

    // Clear the hash tables
    for (auto asset_type : enum_values<AssetType>()) {
        s_assets->assetsByType[asset_type].clear();

        // Reset missing text warnings
        s_assets->missingAssetNames[asset_type].clear();
    }

    s_assets->missingTextIDs.clear();

    // Regenerate asset catalogue
    s_assets->allAssets.clear();
    addAssets();
    loadAssets();
    logInfo("AssetManager reloaded successfully!"_s);
}

Asset* getAsset(AssetType type, String shortName)
{
    DEBUG_FUNCTION();
    Asset* result = getAssetIfExists(type, shortName);

    if (result == nullptr) {
        if (s_assets->missingAssetNames[type].add(shortName)) {
            logWarn("Requested {0} asset '{1}' was not found! Using placeholder."_s, { asset_type_names[type], shortName });
        }
        result = &s_assets->placeholderAssets[type];
    }

    return result;
}

Asset* getAssetIfExists(AssetType type, String shortName)
{
    auto asset = s_assets->assetsByType[type].find_value(shortName);
    return asset.value_or(nullptr);
}

Array<Colour>* getPalette(String name)
{
    return &getAsset(AssetType::Palette, name)->palette.paletteData;
}

SpriteGroup* getSpriteGroup(String name)
{
    return &getAsset(AssetType::Sprite, name)->spriteGroup;
}

Sprite* getSprite(String name, s32 offset)
{
    Sprite* result = nullptr;

    SpriteGroup* group = getSpriteGroup(name);
    if (group != nullptr) {
        result = group->sprites + (offset % group->count);
    }

    return result;
}

SpriteRef getSpriteRef(String groupName, s32 spriteIndex)
{
    SpriteRef result = {};

    result.spriteGroupName = intern(&s_assets->assetStrings, groupName);
    result.spriteIndex = spriteIndex;

    // NB: We don't retrieve the sprite now, we just leave the pointerRetrievedTicks value at 0
    // so that it WILL be retrieved the first time we call getSprite().

    return result;
}

Sprite* getSprite(SpriteRef* ref)
{
    if (s_assets->asset_generation() > ref->asset_generation) {
        SpriteGroup* group = getSpriteGroup(ref->spriteGroupName);
        if (group != nullptr) {
            ref->pointer = group->sprites + (ref->spriteIndex % group->count);
        } else {
            // TODO: Dummy sprite!
            ASSERT(!"Sprite group missing");
        }

        ref->asset_generation = s_assets->asset_generation();
    }

    return ref->pointer;
}

Shader* getShader(String shaderName)
{
    return &getAsset(AssetType::Shader, shaderName)->shader;
}

BitmapFont* getFont(String fontName)
{
    BitmapFont* result = nullptr;

    Asset* fontAsset = getAsset(AssetType::BitmapFont, fontName);
    if (fontAsset != nullptr) {
        result = &fontAsset->bitmapFont;
    } else {
        logError("Failed to find font named '{0}'."_s, { fontName });
    }

    return result;
}

BitmapFont* getFont(AssetRef const& fontRef)
{
    ASSERT(fontRef.type() == AssetType::BitmapFont);

    BitmapFont* result = nullptr;
    Asset* asset = fontRef.get();

    if (asset != nullptr) {
        result = &asset->bitmapFont;
    }

    return result;
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

String getText(String name, std::initializer_list<String> args)
{
    String format = getText(name);

    return myprintf(format, args);
}

String getAssetPath(AssetType type, String shortName)
{
    String result = shortName;

    switch (type) {
    case AssetType::Cursor:
        result = myprintf("{0}/cursors/{1}"_s, { s_assets->assetsPath, shortName }, true);
        break;
    case AssetType::BitmapFont:
        result = myprintf("{0}/fonts/{1}"_s, { s_assets->assetsPath, shortName }, true);
        break;
    case AssetType::Shader:
        result = myprintf("{0}/shaders/{1}"_s, { s_assets->assetsPath, shortName }, true);
        break;
    case AssetType::Texts:
        result = myprintf("{0}/locale/{1}"_s, { s_assets->assetsPath, shortName }, true);
        break;
    case AssetType::Texture:
        result = myprintf("{0}/textures/{1}"_s, { s_assets->assetsPath, shortName }, true);
        break;
    default:
        result = myprintf("{0}/{1}"_s, { s_assets->assetsPath, shortName }, true);
        break;
    }

    return result;
}

void AssetManager::on_settings_changed()
{
    // Reload locale-specific assets

    // Clear the list of missing texts because they might not be missing in the new locale!
    s_assets->missingTextIDs.clear();

    for (auto it = s_assets->allAssets.iterate(); it.hasNext(); it.next()) {
        Asset* asset = it.get();
        if (asset->locale.has_value()) {
            unloadAsset(asset);
        }
    }

    for (auto it = s_assets->allAssets.iterate(); it.hasNext(); it.next()) {
        Asset* asset = it.get();
        // FIXME: Only try to load if the locale matches.
        if (asset->locale.has_value()) {
            loadAsset(asset);
        }
    }
}

void loadCursorDefs(Blob data, Asset* asset)
{
    DEBUG_FUNCTION();

    LineReader reader { asset->shortName, data };

    // We store the cursorNames array in the defs asset
    // So, we first need to scan through the file to see how many cursors there are in it!
    s32 cursorCount = 0;
    while (reader.load_next_line()) {
        cursorCount++;
    }

    allocateChildren(asset, cursorCount);

    reader.restart();

    while (reader.load_next_line()) {
        String name = intern(&s_assets->assetStrings, reader.next_token());
        String filename = reader.next_token();

        auto hot_x = reader.read_int<s32>();
        auto hot_y = reader.read_int<s32>();

        if (hot_x.has_value() && hot_y.has_value()) {
            // Add the cursor
            Asset* cursorAsset = addAsset(AssetType::Cursor, name, {});
            cursorAsset->cursor.imageFilePath = intern(&s_assets->assetStrings, getAssetPath(AssetType::Cursor, filename));
            cursorAsset->cursor.hotspot = v2i(hot_x.release_value(), hot_y.release_value());
            addChildAsset(asset, cursorAsset);
        } else {
            reader.error("Couldn't parse cursor definition. Expected 'name filename.png hot-x hot-y'."_s);
            return;
        }
    }
}

void loadPaletteDefs(Blob data, Asset* asset)
{
    DEBUG_FUNCTION();

    LineReader reader { asset->shortName, data };

    // We store the paletteNames array in the defs asset
    // So, we first need to scan through the file to see how many palettes there are in it!
    s32 paletteCount = 0;
    while (reader.load_next_line()) {
        String command = reader.next_token();
        if (command == ":Palette"_s) {
            paletteCount++;
        }
    }

    allocateChildren(asset, paletteCount);

    reader.restart();

    Asset* paletteAsset = nullptr;
    while (reader.load_next_line()) {
        String command = reader.next_token();
        if (command[0] == ':') {
            command.length--;
            command.chars++;

            if (command == "Palette"_s) {
                paletteAsset = addAsset(AssetType::Palette, reader.next_token(), {});
                addChildAsset(asset, paletteAsset);
            } else {
                reader.error("Unexpected command ':{0}' in palette-definitions file. Only :Palette is allowed!"_s, { command });
                return;
            }
        } else {
            if (paletteAsset == nullptr) {
                reader.error("Unexpected command '{0}' before the start of a :Palette"_s, { command });
                return;
            }

            if (command == "type"_s) {
                String type = reader.next_token();

                if (type == "fixed"_s) {
                    paletteAsset->palette.type = Palette::Type::Fixed;
                } else if (type == "gradient"_s) {
                    paletteAsset->palette.type = Palette::Type::Gradient;
                } else {
                    reader.error("Unrecognised palette type '{0}', allowed values are: fixed, gradient"_s, { type });
                }
            } else if (command == "size"_s) {
                if (auto size = reader.read_int<s32>(); size.has_value()) {
                    paletteAsset->palette.size = size.release_value();
                }
            } else if (command == "color"_s) {
                if (auto color = Colour::read(reader); color.has_value()) {
                    if (paletteAsset->palette.type == Palette::Type::Fixed) {
                        if (!paletteAsset->palette.paletteData.isInitialised()) {
                            paletteAsset->data = assetsAllocate(s_assets, paletteAsset->palette.size * sizeof(Colour));
                            paletteAsset->palette.paletteData = makeArray<Colour>(paletteAsset->palette.size, reinterpret_cast<Colour*>(paletteAsset->data.writable_data()));
                        }

                        s32 colorIndex = paletteAsset->palette.paletteData.count;
                        if (colorIndex >= paletteAsset->palette.size) {
                            reader.error("Too many 'color' definitions! 'size' must be large enough."_s);
                        } else {
                            paletteAsset->palette.paletteData.append(color.release_value());
                        }
                    } else {
                        reader.error("'color' is only a valid command for fixed palettes."_s);
                    }
                }
            } else if (command == "from"_s) {
                if (auto from = Colour::read(reader); from.has_value()) {
                    if (paletteAsset->palette.type == Palette::Type::Gradient) {
                        paletteAsset->palette.gradient.from = from.release_value();
                    } else {
                        reader.error("'from' is only a valid command for gradient palettes."_s);
                    }
                }
            } else if (command == "to"_s) {
                if (auto to = Colour::read(reader); to.has_value()) {
                    if (paletteAsset->palette.type == Palette::Type::Gradient) {
                        paletteAsset->palette.gradient.to = to.release_value();
                    } else {
                        reader.error("'to' is only a valid command for gradient palettes."_s);
                    }
                }
            } else {
                reader.error("Unrecognised command '{0}'"_s, { command });
            }
        }
    }
}

void loadSpriteDefs(Blob data, Asset* asset)
{
    DEBUG_FUNCTION();

    LineReader reader { asset->shortName, data };

    Asset* textureAsset = nullptr;
    V2I spriteSize = v2i(0, 0);
    V2I spriteBorder = v2i(0, 0);
    Asset* spriteGroup = nullptr;
    s32 spriteIndex = 0;

    // Count the number of child s_assets, so we can allocate our spriteNames array
    s32 childAssetCount = 0;
    while (reader.load_next_line()) {
        String command = reader.next_token();

        if (command.chars[0] == ':') {
            childAssetCount++;
        }
    }
    allocateChildren(asset, childAssetCount);

    reader.restart();

    // Now, actually read things
    while (reader.load_next_line()) {
        String command = reader.next_token();

        if (command.chars[0] == ':') // Definitions
        {
            // Define something
            command.chars++;
            command.length--;

            textureAsset = nullptr;
            spriteGroup = nullptr;

            if (command == "Ninepatch"_s) {
                String name = reader.next_token();
                String filename = reader.next_token();
                auto pu0 = reader.read_int<s32>();
                auto pu1 = reader.read_int<s32>();
                auto pu2 = reader.read_int<s32>();
                auto pu3 = reader.read_int<s32>();
                auto pv0 = reader.read_int<s32>();
                auto pv1 = reader.read_int<s32>();
                auto pv2 = reader.read_int<s32>();
                auto pv3 = reader.read_int<s32>();

                if (name.is_empty() || filename.is_empty() || !all_have_values(pu0, pu1, pu2, pu3, pv0, pv1, pv2, pv3)) {
                    reader.error("Couldn't parse Ninepatch. Expected: ':Ninepatch identifier filename.png pu0 pu1 pu2 pu3 pv0 pv1 pv2 pv3'"_s);
                    return;
                }

                Asset* ninepatch = addNinepatch(name, filename, pu0.release_value(), pu1.release_value(), pu2.release_value(), pu3.release_value(), pv0.release_value(), pv1.release_value(), pv2.release_value(), pv3.release_value());

                addChildAsset(asset, ninepatch);
            } else if (command == "Sprite"_s) {
                // @Copypasta from the SpriteGroup branch, and the 'sprite' property
                String name = reader.next_token();
                String filename = reader.next_token();
                auto spriteSizeIn = V2I::read(reader);

                if (name.is_empty() || filename.is_empty() || !spriteSizeIn.has_value()) {
                    reader.error("Couldn't parse Sprite. Expected: ':Sprite identifier filename.png SWxSH'"_s);
                    return;
                }

                spriteSize = spriteSizeIn.release_value();

                Asset* group = addSpriteGroup(name, 1);

                Sprite* sprite = group->spriteGroup.sprites;
                sprite->texture = addTexture(filename, false);
                sprite->uv = { 0, 0, spriteSize.x, spriteSize.y };
                sprite->pixelWidth = spriteSize.x;
                sprite->pixelHeight = spriteSize.y;

                addChildAsset(asset, group);
            } else if (command == "SpriteGroup"_s) {
                String name = reader.next_token();
                String filename = reader.next_token();
                auto spriteSizeIn = V2I::read(reader);

                if (name.is_empty() || filename.is_empty() || !spriteSizeIn.has_value()) {
                    reader.error("Couldn't parse SpriteGroup. Expected: ':SpriteGroup identifier filename.png SWxSH'"_s);
                    return;
                }

                textureAsset = addTexture(filename, false);
                spriteSize = spriteSizeIn.release_value();

                s32 spriteCount = reader.count_occurrences_of_property_in_current_command("sprite"_s);
                if (spriteCount < 1) {
                    reader.error("SpriteGroup must contain at least 1 sprite!"_s);
                    return;
                }
                spriteGroup = addSpriteGroup(name, spriteCount);
                spriteIndex = 0;

                addChildAsset(asset, spriteGroup);
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

                    Sprite* sprite = spriteGroup->spriteGroup.sprites + spriteIndex;
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

void loadTexts(HashTable<String>* texts, Asset* asset, Blob fileData)
{
    LineReader reader { asset->shortName, fileData };

    // NB: We store the strings inside the asset data, so it's one block of memory instead of many small ones.
    // However, since we allocate before we parse the file, we need to make sure that the output texts are
    // never longer than the input texts, or we could run out of space!
    // Right now, the only processing we do is replacing \n with a newline character, and similar, so the
    // output can only ever be smaller or the same size as the input.
    // - Sam, 01/10/2019

    // We also put an array of keys into the same allocation.
    // We use the number of lines in the file as a heuristic - we know it'll be slightly more than
    // the number of texts in the file, because they're 1 per line, and we don't have many blanks.

    s32 lineCount = LineReader::count_lines(fileData);
    smm keyArraySize = sizeof(String) * lineCount;
    asset->data = assetsAllocate(s_assets, fileData.size() + keyArraySize);

    asset->texts.keys = makeArray(lineCount, (String*)asset->data.writable_data());

    smm currentSize = keyArraySize;
    char* currentPos = (char*)(asset->data.writable_data() + keyArraySize);

    while (reader.load_next_line()) {
        String inputKey = reader.next_token();
        String inputText = reader.remainder_of_current_line();

        // Store the key
        ASSERT(currentSize + inputKey.length <= asset->data.size());
        String key { currentPos, (size_t)inputKey.length };
        copyString(inputKey, &key);
        currentSize += key.length;
        currentPos += key.length;

        // Store the text
        ASSERT(currentSize + inputText.length <= asset->data.size());
        String text { currentPos, 0 };

        for (s32 charIndex = 0; charIndex < inputText.length; charIndex++) {
            char c = inputText[charIndex];
            if (c == '\\') {
                if (((charIndex + 1) < inputText.length)
                    && (inputText[charIndex + 1] == 'n')) {
                    text.chars[text.length] = '\n';
                    text.length++;
                    charIndex++;
                    continue;
                }
            }

            text.chars[text.length] = c;
            text.length++;
        }

        currentSize += text.length;
        currentPos += text.length;

        // Check that we don't already have a text with that name.
        // If we do, one will overwrite the other, and that could be unpredictable if they're
        // in different files. Things will still work, but it would be confusing! And unintended.
        auto existing_text = texts->find_value(key);
        if (existing_text.has_value() && existing_text != key) {
            reader.warn("Text asset with ID '{0}' already exists in the texts table! Existing value: \"{1}\""_s, { key, existing_text.value() });
        }

        asset->texts.keys.append(key);

        texts->put(key, text);
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
