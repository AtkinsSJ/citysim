#pragma once

// render_gl.h

enum Alignment {
	ALIGN_LEFT = 1,
	ALIGN_H_CENTRE = 2,
	ALIGN_RIGHT = 4,

	ALIGN_H = ALIGN_LEFT | ALIGN_H_CENTRE | ALIGN_RIGHT,

	ALIGN_TOP = 8,
	ALIGN_V_CENTRE = 16,
	ALIGN_BOTTOM = 32,
	
	ALIGN_V = ALIGN_TOP | ALIGN_V_CENTRE | ALIGN_BOTTOM,

	ALIGN_CENTRE = ALIGN_H_CENTRE | ALIGN_V_CENTRE,
};

const int FRAMES_PER_SECOND = 60;
const real32 SECONDS_PER_FRAME = 1.0f / 60.0f;
const int MS_PER_FRAME = (1000 / 60); // 60 frames per second

const float TILE_SIZE = 16.0f;
const int TILE_WIDTH = 16,
			TILE_HEIGHT = 16;
const int CAMERA_MARGIN = 1; // How many tiles beyond the map can the camera scroll to show?
const bool canZoom = true;

struct Camera
{
	union {
		Coord windowSize;
		struct{int32 windowWidth, windowHeight;};
	};
	V2 pos; // Centre of screen, in tiles
	real32 zoom; // 1 = normal, 2 = things appear twice their size, etc.
};
const real32 CAMERA_PAN_SPEED = 10.0f; // Measured in world units per second
const int CAMERA_EDGE_SCROLL_MARGIN = 8;

struct VertexData
{
	V3 pos;
	V4 color;
	V2 uv;
	GLint textureID;
};

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

enum TextureAtlasItem
{
	TextureAtlasItem_None = 0,
	TextureAtlasItem_GroundTile = 1,
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

	TextureAtlasItemCount
};

struct Texture
{
	bool valid;
	GLuint id;
	uint32 w,h;
};

struct TextureRegion
{
	GLuint textureID;
	RealRect uv;
};

struct TextureAtlas
{
	TextureRegion textureRegions[TextureAtlasItemCount];
};

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

struct Sprite
{
	RealRect rect;
	real32 depth; // Positive is towards the player

	GLint textureID;
	RealRect uv;
	
	V4 color;
};

inline Sprite makeSprite(RealRect rect, real32 depth, GLint textureID, RealRect uv, V4 color)
{
	Sprite sprite = {};
	sprite.rect = rect;
	sprite.depth = depth;
	sprite.textureID = textureID;
	sprite.uv = uv;
	sprite.color = color;
	
	return sprite;
}

const int WORLD_SPRITE_MAX = 16384;
const int UI_SPRITE_MAX = 1024;
const int SPRITE_MAX = WORLD_SPRITE_MAX;

struct RenderBuffer
{
	Matrix4 projectionMatrix;
	Sprite *sprites;
	uint32 spriteCount;
	uint32 maxSprites;
};

struct GLRenderer
{
	MemoryArena renderArena;

	SDL_Window *window;
	SDL_GLContext context;

	GLuint shaderProgramID;
	GLuint VBO,
		   IBO;
	GLint uProjectionMatrixLoc,
		  uTexturesLoc;
	GLint aPositionLoc,
		  aColorLoc,
		  aUVLoc,
		  aTextureIDLoc;

	GLuint textureArrayID;
	GLenum textureFormat;

	Camera worldCamera;

	RenderBuffer worldBuffer;
	RenderBuffer uiBuffer;

	VertexData vertices[SPRITE_MAX * 4];
	GLuint indices[SPRITE_MAX * 6];

	Animation animations[Animation_Count];

	TextureAtlas textureAtlas;

	// UI Stuff!
	UiTheme theme;
};

const uint32 TEXTURE_WIDTH = 1024,
			 TEXTURE_HEIGHT = 1024;
 const GLint TEXTURE_ID_NONE = -1;

struct TexturesToLoad
{
	char filenamesBuffer[4096];
	int32 bufferPos;

	int32 count;
	char *filenames[64];
	bool isAlphaPremultiplied[64];
};
GLint pushTextureToLoad(TexturesToLoad *textures, char *filename, bool isAlphaPremultiplied=false)
{
	ASSERT(textures->count < ArrayCount(textures->filenames), "TexturesToLoad filenames limit reached!");

	uint32 filenameLength = strlen(filename) + 1;
	ASSERT(textures->bufferPos + filenameLength < ArrayCount(textures->filenamesBuffer), "No more space for texture filenames in TexturesToLoad!");

	uint32 textureID = textures->count++;

	textures->filenames[textureID] = textures->filenamesBuffer + textures->bufferPos;
	textures->isAlphaPremultiplied[textureID] = isAlphaPremultiplied;
	
	// NB: Don't strncpy; we already check there's room above. strncpy causes Bad Things™
	strcpy(textures->filenames[textureID], filename);
	textures->bufferPos += filenameLength;

	return (GLint)textureID;
}

inline void checkForGLError()
{
	GLenum errorCode = glGetError();
	ASSERT(errorCode == 0, "GL Error: %d", errorCode);
}

GLRenderer *initializeRenderer(MemoryArena *MemoryArena, const char *GameName);
void freeRenderer(GLRenderer *renderer);
bool initOpenGL(GLRenderer *renderer);
bool loadTextures(TemporaryMemoryArena *TempArena, GLRenderer *renderer, TexturesToLoad *texturesToLoad);
void printProgramLog(GLuint program);
void printShaderLog(GLuint shader);

void drawSprite(GLRenderer *renderer, bool isUI, Sprite *sprite, V3 offset);

void drawQuad(GLRenderer *renderer, bool isUI, RealRect rect, real32 depth,
				GLint textureID, RealRect uv, V4 color=makeWhite());

void drawTextureAtlasItem(GLRenderer *renderer, bool isUI, TextureAtlasItem textureAtlasItem,
				V2 position, V2 size, real32 depth, V4 color=makeWhite());

void drawRect(GLRenderer *renderer, bool isUI, RealRect rect, real32 depth, V4 color);

void drawAnimator(GLRenderer *renderer, bool isUI, Animator *animator,
				real32 daysPerFrame, V2 worldTilePosition, V2 size, real32 depth, V4 color=makeWhite());

void setAnimation(Animator *animator, GLRenderer *renderer, AnimationID animationID,
				bool restart = false);

void render(GLRenderer *renderer);

SDL_Cursor *createCursor(char *path);
void setCursor(GLRenderer *renderer, Cursor cursor);

BitmapFont *readBMFont(MemoryArena *renderArena, TemporaryMemoryArena *tempArena, char *filename, TexturesToLoad *texturesToLoad);

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

#include "render_gl.cpp"