#pragma once

enum AssetType
{
	AssetType_Misc,

	AssetType_BitmapFont,
	AssetType_BuildingDefs,
	AssetType_Cursor,
	AssetType_DevKeymap,
	AssetType_Settings,
	AssetType_Shader,
	AssetType_Sprite,
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
	SDL_Cursor *sdlCursor;
};

struct Shader
{
	s32 rendererShaderID;

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

struct Asset
{
	AssetType type;

	// shortName = "foo.png", fullName = "c:/mygame/assets/textures/foo.png"
	String shortName;
	String fullName;
	bool isAFile; // Whether the file at fullName should be loaded into memory when loading this Asset

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
		Shader shader;
		SpriteGroup spriteGroup;
		Texture texture;
	};
};

struct IndexRange
{
	u32 firstIndex;
	u32 lastIndex;
};

struct AssetManager
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
	HashTable<Asset*> assetsByName[AssetTypeCount];

	// TODO: If the theme is an asset, we should remove this direct reference!
	// Will want to make the UITheme itself a densely-packed struct, rather than a set of hashtables,
	// so that we can put the whole thing inside the Asset's data blob.
	// After all, the theme doesn't change after it's loaded. We can use the string identifiers
	// for linking, and then just use direct IDs maybe? IDK, it's worth a thought.
	UITheme theme;

	/*

	TODO
	----

	Our current Asset struct, which keeps a copy of the file contents in memory, is not actually what we want!
	A bunch of assets actually want the file to be loaded, then processed, and the result should be what's
	kept in the Asset, because we never look at the source file's data ever again!
	eg, for Fonts, we probably want the memory* pointer to be to the glyph data so it's easy to locate.

	Asset types:
	- texture assets
	- sprites somehow??? There are thousands of them, because of fonts... maybe font glyphs should use a different, more specialised system?

	Much later...
	- audio! (sfx/music as separate things probably)
	- localised text

	Other changes:
	- lookup by name
	- reload individual files
	- automated reloading
	- automated cataloguing

	*/
};

void loadAsset(AssetManager *assets, Asset *asset);
void ensureAssetIsLoaded(AssetManager *assets, Asset *asset);
bool haveAssetFilesChanged(AssetManager *assets);

Asset *getAssetIfExists(AssetManager *assets, AssetType type, String shortName)
{
	Asset **result = find(&assets->assetsByName[type], shortName);

	return (result == null) ? null : *result;
}

Asset *getAsset(AssetManager *assets, AssetType type, String shortName)
{
	DEBUG_FUNCTION();
	Asset *result = getAssetIfExists(assets, type, shortName);

	if (result == null)
	{
		DEBUG_BREAK();
		logError("Requested asset '{0}' was not found!", {shortName});
	}

	return result;
}

inline SpriteGroup *getSpriteGroup(AssetManager *assets, String name)
{
	return &getAsset(assets, AssetType_Sprite, name)->spriteGroup;
}

inline Sprite *getSprite(SpriteGroup *group, s32 offset)
{
	return group->sprites + (offset % group->count);
}

inline Shader *getShader(AssetManager *assets, String shaderName)
{
	return &getAsset(assets, AssetType_Shader, shaderName)->shader;
}

BitmapFont *getFont(AssetManager *assets, String fontName)
{
	BitmapFont *result = null;

	String *fontFilename = find(&assets->theme.fontNamesToAssetNames, fontName);
	if (fontFilename == null)
	{
		// Fall back to treating it as a filename
		Asset *fontAsset = getAsset(assets, AssetType_BitmapFont, fontName);
		if (fontAsset != null)
		{
			result = &fontAsset->bitmapFont;
		}
		logError("Failed to find font named '{0}' in the UITheme.", {fontName});
	}
	else
	{
		Asset *fontAsset = getAsset(assets, AssetType_BitmapFont, *fontFilename);
		if (fontAsset != null)
		{
			result = &fontAsset->bitmapFont;
		}
	}

	return result;
}

String getAssetPath(AssetManager *assets, AssetType type, String shortName)
{
	String result = shortName;

	switch (type)
	{
	case AssetType_Cursor:
		result = myprintf("{0}/cursors/{1}", {assets->assetsPath, shortName}, true);
		break;
	case AssetType_BitmapFont:
		result = myprintf("{0}/fonts/{1}",    {assets->assetsPath, shortName}, true);
		break;
	case AssetType_Shader:
		result = myprintf("{0}/shaders/{1}",  {assets->assetsPath, shortName}, true);
		break;
	case AssetType_Texture:
		result = myprintf("{0}/textures/{1}", {assets->assetsPath, shortName}, true);
		break;
	default:
		result = myprintf("{0}/{1}", {assets->assetsPath, shortName}, true);
		break;
	}

	return result;
}
inline String getAssetPath(AssetManager *assets, AssetType type, char *shortName)
{
	return getAssetPath(assets, type, makeString(shortName));
}
