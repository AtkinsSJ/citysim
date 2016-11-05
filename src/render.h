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
	ShaderProgram_GL_Textured,
	ShaderProgram_Untextured,

	ShaderProgram_Count,
	ShaderProgram_Invalid = -1
};


struct Sprite
{
	RealRect rect;
	real32 depth; // Positive is towards the player

// Turning these off to split from OpenGL. So that's why there are build errors, future me!
	GLint textureID;
	RealRect uv;
	
	// GL_TextureAtlasItem texture;
	V4 color;
};

// Animation code should probably be deleted and redone.
enum AnimationID
{
	Animation_TempToStopComplaint,
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