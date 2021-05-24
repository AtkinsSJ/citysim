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

	AssetType_ButtonStyle,
	AssetType_CheckboxStyle,
	AssetType_ConsoleStyle,
	AssetType_DropDownListStyle,
	AssetType_LabelStyle,
	AssetType_PanelStyle,
	AssetType_ScrollbarStyle,
	AssetType_TextInputStyle,
	AssetType_WindowStyle,

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

	"ButtonStyle"_s,
	"CheckboxStyle"_s,
	"ConsoleStyle"_s,
	"DropDownListStyle"_s,
	"LabelStyle"_s,
	"PanelStyle"_s,
	"ScrollbarStyle"_s,
	"TextInputStyle"_s,
	"WindowStyle"_s,
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

inline AssetID makeAssetID(AssetType type, String name)
{
	AssetID result = {};

	result.type = type;
	result.name = name;

	return result;
}

struct Asset;

struct AssetRef
{
	AssetType type;
	String name;

	Asset *pointer;
	u32 pointerRetrievedTicks;

	bool isValid() {
		return !isEmpty(name);
	}
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
	s32 pixelWidth;
	s32 pixelHeight;
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

#include "uitheme.h"

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

	Array<AssetID> children;

	// Depending on the AssetType, this could be the file contents, or something else!
	Blob data;

	// Type-specific stuff
	// TODO: This union represents type-specific data size, which affects ALL assets!
	// We might want to move everything into the *memory pointer, so that each Asset is only as big
	// as it needs to be. The Shader struct is 65+ bytes (not including padding) which really inflates
	// the other asset types. Though 65 isn't much, so maybe it's not a big deal. We'll see if we
	// have anything bigger later. (Well, the BitmapFont is definitely bigger!)
	union {
		// NB: Here as a pointer target, so we can cast to the desired type. eg:
		//    (UIBUttonStyle *)(&asset->_localData) 
		u8 _localData;

		// Actual local data follows

		BitmapFont bitmapFont;

		struct {
			Array<String> buildingIDs;
		} buildingDefs;

		Cursor cursor;

		Ninepatch ninepatch;

		Palette palette;

		Shader shader;

		SpriteGroup spriteGroup;

		struct {
			Array<String> terrainIDs;
		} terrainDefs;

		struct {
			Array<String> keys;
			bool isFallbackLocale;
		} texts;

		Texture texture;

		UIButtonStyle    	buttonStyle;
		UICheckboxStyle  	checkboxStyle;
		UIConsoleStyle   	consoleStyle;
		UIDropDownListStyle	dropDownListStyle;
		UILabelStyle     	labelStyle;
		UIPanelStyle     	panelStyle;
		UIScrollbarStyle 	scrollbarStyle;
		UITextInputStyle 	textInputStyle;
		UIWindowStyle    	windowStyle;
	};
};
