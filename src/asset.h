#pragma once

enum AssetType
{
	AssetType_Misc,

	AssetType_BitmapFont,
	AssetType_BuildingDefs,
	AssetType_Cursor,
	AssetType_CursorDefs,
	AssetType_DevKeymap,
	AssetType_Ninepatch,
	AssetType_Palette,
	AssetType_PaletteDefs,
	AssetType_Shader,
	AssetType_Sprite,
	AssetType_SpriteDefs,
	AssetType_TerrainDefs,
	AssetType_Texts,
	AssetType_Texture,
	AssetType_UITheme,

	AssetTypeCount,
	AssetType_Unknown = -1
};

String assetTypeNames[AssetTypeCount] = {
	"Misc"_s,
	"BitmapFont"_s,
	"BuildingDefs"_s,
	"Cursor"_s,
	"CursorDefs"_s,
	"DevKeymap"_s,
	"Ninepatch"_s,
	"Palette"_s,
	"PaletteDefs"_s,
	"Shader"_s,
	"Sprite"_s,
	"SpriteDefs"_s,
	"Texts"_s,
	"Texture"_s,
	"TerrainDefs"_s,
	"UITheme"_s,
};

enum AssetState
{
	AssetState_Unloaded,
	AssetState_Loaded,
};

struct AssetID
{
	AssetType type;
	String name;
};

struct Cursor
{
	String imageFilePath; // Full path
	V2I hotspot;
	SDL_Cursor *sdlCursor;
};

struct Ninepatch
{
	Asset *texture;

	s32 pu0;
	s32 pu1;
	s32 pu2;
	s32 pu3;

	s32 pv0;
	s32 pv1;
	s32 pv2;
	s32 pv3;

	// NB: These UV values do not change once they're set in loadAsset(), so, we
	// could replace them with an array of 9 Rect2s in order to avoid calculating
	// them over and over. BUT, this would greatly increase the size of the Asset
	// struct itself, which we don't want to do. We could move that into the
	// Asset.data blob, but then we're doing an extra indirection every time we
	// read it, and it's not *quite* big enough to warrant that extra complexity.
	// Frustrating! If it was bigger I'd do it, but as it is, I think keeping
	// these 8 floats is the best option. If creating the UV rects every time
	// becomes a bottleneck, we can switch over.
	// - Sam, 16/02/2021
	f32 u0;
	f32 u1;
	f32 u2;
	f32 u3;

	f32 v0;
	f32 v1;
	f32 v2;
	f32 v3;
};

enum PaletteType
{
	PaletteType_Fixed,
	PaletteType_Gradient,
};
struct Palette
{
	PaletteType type;
	s32 size;

	union
	{
		struct
		{
			s32 currentPos;
		} fixed;
		struct
		{
			V4 from;
			V4 to;
		} gradient;
	};

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

struct SpriteRef
{
	String spriteGroupName;
	s32 spriteIndex;

	Sprite *pointer;
	u32 pointerRetrievedTicks;
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

struct AssetRef
{
	AssetType type;
	String name;

	Asset *pointer;
	u32 pointerRetrievedTicks;
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

		struct {
			Array<String> buildingIDs;
		} buildingDefs;

		struct {
			Array<String> cursorNames;
		} cursorDefs;
		Cursor cursor;

		Ninepatch ninepatch;

		struct {
			Array<String> paletteNames;
		} paletteDefs;
		Palette palette;

		Shader shader;

		SpriteGroup spriteGroup;

		struct {
			Array<AssetID> sprites;
		} spriteDefs;

		struct {
			Array<String> terrainIDs;
		} terrainDefs;

		struct {
			Array<String> keys;
			bool isFallbackLocale;
		} texts;

		Texture texture;
	};
};
