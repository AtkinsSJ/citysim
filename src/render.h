#pragma once

// General rendering code.

enum Cursor
{
	Cursor_Main,
	Cursor_Build,
	Cursor_Demolish,
	Cursor_Plant,
	Cursor_Harvest,
	Cursor_Hire,

	Cursor_Count
};

struct UiTheme
{
	struct BitmapFont *font,
					  *buttonFont;

	V4 overlayColor;
			
	V4 labelColor;

	V4 buttonTextColor,
		buttonBackgroundColor,
		buttonHoverColor,
		buttonPressedColor;

	V4 textboxTextColor,
		textboxBackgroundColor;

	V4 tooltipBackgroundColor,
		tooltipColorNormal,
		tooltipColorBad;

	// TODO: Abstract this?
	SDL_Cursor *cursors[Cursor_Count];
};

enum ShaderPrograms
{
	ShaderProgram_Textured,
	ShaderProgram_Untextured,

	ShaderProgram_Count,
	ShaderProgram_Invalid = -1
};

enum TextureAtlasItem
{
	TextureAtlasItem_None,

	TextureAtlasItem_GroundTile,
	TextureAtlasItem_WaterTile,
	TextureAtlasItem_ForestTile,
	TextureAtlasItem_Path,
	TextureAtlasItem_Field,
	TextureAtlasItem_Crop0_0,
	TextureAtlasItem_Crop0_1,
	TextureAtlasItem_Crop0_2,
	TextureAtlasItem_Crop0_3,
	TextureAtlasItem_Potato,
	TextureAtlasItem_Barn,
	TextureAtlasItem_House,

	TextureAtlasItem_Farmer_Stand,
	TextureAtlasItem_Farmer_Walk0,
	TextureAtlasItem_Farmer_Walk1,
	TextureAtlasItem_Farmer_Hold,
	TextureAtlasItem_Farmer_Carry0,
	TextureAtlasItem_Farmer_Carry1,
	TextureAtlasItem_Farmer_Harvest0,
	TextureAtlasItem_Farmer_Harvest1,
	TextureAtlasItem_Farmer_Harvest2,
	TextureAtlasItem_Farmer_Harvest3,
	TextureAtlasItem_Farmer_Plant0,
	TextureAtlasItem_Farmer_Plant1,
	TextureAtlasItem_Farmer_Plant2,
	TextureAtlasItem_Farmer_Plant3,

	TextureAtlasItem_Icon_Planting,
	TextureAtlasItem_Icon_Harvesting,

	TextureAtlasItem_Menu_Logo,

	TextureAtlasItem_Map1,

	TextureAtlasItemCount
};

struct Sprite
{
	RealRect rect;
	real32 depth; // Positive is towards the player

// Turning these off to split from OpenGL. So that's why there are build errors, future me!
	GLint textureID;
	RealRect uv;
	
	// TextureAtlasItem texture;
	V4 color;
};

// Animation code should probably be deleted and redone.
enum AnimationID
{
	Animation_Farmer_Stand,
	Animation_Farmer_Walk,
	Animation_Farmer_Hold,
	Animation_Farmer_Carry,
	Animation_Farmer_Harvest,
	Animation_Farmer_Plant,
	
	Animation_Count,
};

struct Animation
{
	TextureAtlasItem frames[16];
	uint32 frameCount;
};

struct Animator
{
	Animation *animation;
	uint32 currentFrame;
	real32 frameCounter; // Sub-frame ticks
};
const real32 animationFramesPerDay = 10.0f;

inline real32 depthFromY(real32 y)
{
	return (y * 0.1f);
}
inline real32 depthFromY(uint32 y)
{
	return depthFromY((real32)y);
}
inline real32 depthFromY(int32 y)
{
	return depthFromY((real32)y);
}