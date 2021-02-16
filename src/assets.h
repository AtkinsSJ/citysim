#pragma once

struct Assets
{
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

	// TODO: If the theme is an asset, we should remove this direct reference!
	// Will want to make the UITheme itself a densely-packed struct, rather than a set of hashtables,
	// so that we can put the whole thing inside the Asset's data blob.
	// After all, the theme doesn't change after it's loaded. We can use the string identifiers
	// for linking, and then just use direct IDs maybe? IDK, it's worth a thought.
	UITheme theme;

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

void reloadLocaleSpecificAssets();

Asset *addAsset(AssetType type, String shortName, u32 flags = AssetDefaultFlags);
Asset *addFont(String name, String filename);
Asset *addNinepatch(String name, String filename, s32 pu0, s32 pu1, s32 pu2, s32 pu3, s32 pv0, s32 pv1, s32 pv2, s32 pv3);
Asset *addTexture(String filename, bool isAlphaPremultiplied);
Asset *addSpriteGroup(String name, s32 spriteCount);

void loadAsset(Asset *asset);
void ensureAssetIsLoaded(Asset *asset);
void unloadAsset(Asset *asset);
void removeAsset(AssetType type, String name);

void addAssets();
void addAssetsFromDirectory(String subDirectory, AssetType manualAssetType=AssetType_Unknown);
bool haveAssetFilesChanged();
void reloadAssets();

String getAssetPath(AssetType type, String shortName);

Asset *getAsset(AssetType type, String shortName);
Asset *getAssetIfExists(AssetType type, String shortName);
AssetRef getAssetRef(AssetType type, String shortName);
Asset *getAsset(AssetRef *ref);

BitmapFont *getFont(String fontName);
BitmapFont *getFont(FontReference *fontRef);

Array<V4> *getPalette(String name);
Shader *getShader(String shaderName);
Sprite *getSprite(String name, s32 offset = 0);
// @Deprecated
Sprite *getSprite(SpriteGroup *group, s32 offset);
SpriteGroup *getSpriteGroup(String name);

SpriteRef getSpriteRef(String groupName, s32 spriteIndex);
Sprite *getSprite(SpriteRef *ref);

String getText(String name);

//
// Internal
//

Blob assetsAllocate(Assets *theAssets, smm size);

void loadCursorDefs(Blob data, Asset *asset);
void loadPaletteDefs(Blob data, Asset *asset);
void loadSpriteDefs(Blob data, Asset *asset);
void loadTexts(HashTable<String> *texts, Asset *asset, Blob fileData);

void removeSpriteDefs(Array<String> namesToRemove);
