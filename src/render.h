#pragma once

// General rendering code.

const float TILE_SIZE = 16.0f;
const int TILE_WIDTH = 16,
			TILE_HEIGHT = 16;
const int CAMERA_MARGIN = 1; // How many tiles beyond the map can the camera scroll to show?
const bool canZoom = true;

struct Camera
{
	// TODO: Replace pixel size with a res-independant size which we then convert to pixels.
	// So worldCamera is in world units, uiCamera in pixels.
	union {
		Coord windowSize;
		struct{int32 windowWidth, windowHeight;};
	};
	V2 pos; // Centre of screen, in tiles
	real32 zoom; // 1 = normal, 2 = things appear twice their size, etc.
};
const real32 CAMERA_PAN_SPEED = 10.0f; // Measured in world units per second
const int CAMERA_EDGE_SCROLL_MARGIN = 8;

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

	SDL_Cursor *cursors[Cursor_Count];
};

enum ShaderPrograms
{
	ShaderProgram_Textured,
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
	TextureAssetType frames[16];
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

SDL_Cursor *createCursor(char *path)
{
	SDL_Surface *cursorSurface = IMG_Load(path);
	SDL_Cursor *cursor = SDL_CreateColorCursor(cursorSurface, 0, 0);
	SDL_FreeSurface(cursorSurface);

	return cursor;
}

void setCursor(UiTheme *theme, Cursor cursor)
{
	SDL_SetCursor(theme->cursors[cursor]);
}