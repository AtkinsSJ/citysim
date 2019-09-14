#pragma once

enum AssetType
{
	AssetType_Misc,

	AssetType_BitmapFont,
	AssetType_BuildingDefs,
	AssetType_Cursor,
	AssetType_CursorDefs,
	AssetType_DevKeymap,
	AssetType_Palette,
	AssetType_PaletteDefs,
	AssetType_Shader,
	AssetType_Sprite,
	AssetType_Texts,
	AssetType_Texture,
	AssetType_TerrainDefs,
	AssetType_UITheme,

	AssetTypeCount,
	AssetType_Unknown = -1
};

enum AssetState
{
	AssetState_Unloaded,
	AssetState_Loaded,
};

struct Cursor
{
	String imageFilePath; // Full path
	V2I hotspot;
	SDL_Cursor *sdlCursor;
};

enum PaletteType
{
	PaletteType_Gradient,
};
struct Palette
{
	PaletteType type;
	s32 size;
	V4 from;
	V4 to;

	Array<V4> paletteData;
};

struct Shader
{
	s8 rendererShaderID;

	String vertexShader;
	String fragmentShader;
};

struct Sprite
{
	Asset *texture;
	Rect2 uv; // in (0 to 1) space
};

struct SpriteGroup
{
	s32 count;
	Sprite *sprites;
};

struct Texture
{
	bool isFileAlphaPremultiplied;
	SDL_Surface *surface;

	// Renderer-specific stuff.
	union {
		struct {
			u32 glTextureID;
			bool isLoaded;
		} gl;
	};
};

enum AssetFlags
{
	Asset_IsAFile          = 1 << 0, // The file at Asset.fullName should be loaded into memory when loading this Asset
	Asset_IsLocaleSpecific = 1 << 1, // File path has a {0} in it which should be filled in with the current locale

	AssetDefaultFlags = Asset_IsAFile,
};

struct Asset
{
	AssetType type;

	// shortName = "foo.png", fullName = "c:/mygame/assets/textures/foo.png"
	String shortName;
	String fullName;

	u32 flags; // AssetFlags
	AssetState state;

	// Depending on the AssetType, this could be the file contents, or something else!
	Blob data;

	// Type-specific stuff
	// TODO: This union represents type-specific data size, which affects ALL assets!
	// We might want to move everything into the *memory pointer, so that each Asset is only as big
	// as it needs to be. The Shader struct is 65+ bytes (not including padding) which really inflates
	// the other asset types. Though 65 isn't much, so maybe it's not a big deal. We'll see if we
	// have anything bigger later. (Well, the BitmapFont is definitely bigger!)
	union {
		BitmapFont bitmapFont;
		Cursor cursor;
		Palette palette;
		Shader shader;
		SpriteGroup spriteGroup;
		Texture texture;
	};
};

struct Assets
{
	MemoryArena assetArena;
	DirectoryChangeWatchingHandle assetChangeHandle;
	bool assetReloadHasJustHappened;

	// TODO: Also include size of the UITheme, somehow.
	smm assetMemoryAllocated;
	smm maxAssetMemoryAllocated;

	String assetsPath;
	HashTable<AssetType> fileExtensionToType;
	HashTable<AssetType> directoryNameToType;

	ChunkedArray<Asset> allAssets;
	HashTable<Asset*> assetsByType[AssetTypeCount];

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
};

void initAssets();

void reloadLocaleSpecificAssets();

Asset *addAsset(AssetType type, String shortName, u32 flags = AssetDefaultFlags);
Asset *addTexture(String filename, bool isAlphaPremultiplied);
Asset *addSpriteGroup(String name, s32 spriteCount);
void addTiledSprites(String name, String textureFilename, u32 tileWidth, u32 tileHeight, u32 tilesAcross, u32 tilesDown, bool isAlphaPremultiplied=false);
void addTiledSprites(String name, Asset *texture, V2I tileSize, V2I tileBorder, Rect2I selectedTiles);
void addFont(String name, String filename);

void loadAsset(Asset *asset);
void ensureAssetIsLoaded(Asset *asset);
void unloadAsset(Asset *asset);
void reloadAsset(Asset *asset);

void addAssets();
void addAssetsFromDirectory(String subDirectory, AssetType manualAssetType=AssetType_Unknown);
bool haveAssetFilesChanged();
void reloadAssets();

String getAssetPath(AssetType type, String shortName);

Asset *getAsset(AssetType type, String shortName);
Asset *getAssetIfExists(AssetType type, String shortName);

BitmapFont *getFont(String fontName);
Array<V4> *getPalette(String name);
Shader *getShader(String shaderName);
Sprite *getSprite(SpriteGroup *group, s32 offset);
SpriteGroup *getSpriteGroup(String name);

#define LOCAL(str) getText(makeString(str))
String getText(String name);

//
// Internal
//

void loadCursorDefs(Blob data, Asset *asset);
void loadPaletteDefs(Blob data, Asset *asset);

Blob assetsAllocate(Assets *theAssets, smm size);
