#pragma once

// render_gl.h

const int FRAMES_PER_SECOND = 60;
const real32 SECONDS_PER_FRAME = 1.0f / 60.0f;
const int MS_PER_FRAME = (1000 / 60); // 60 frames per second

#if 1
const float TILE_SIZE = 16.0f;
const int TILE_WIDTH = 16,
			TILE_HEIGHT = 16;
const int CAMERA_MARGIN = 0;
const bool canZoom = true;

struct Camera {
	union {
		Coord windowSize;
		struct{int32 windowWidth, windowHeight;};
	};
	V2 pos; // Centre of screen
	real32 zoom; // 1 = normal, 2 = things appear twice their size, etc.
};
const real32 CAMERA_PAN_SPEED = 10.0f; // Measured in world units per second
const int CAMERA_EDGE_SCROLL_MARGIN = 8;
#endif

struct VertexData {
	V3 pos;
	V4 color;
	V3 uv; // Slightly hacky: we set 'z' to 0 for an untextured rectangle
};

struct UiTheme {
	TTF_Font *font,
			*buttonFont;

	Color overlayColor;
			
	Color labelColor;

	Color buttonTextColor,
		buttonBackgroundColor,
		buttonHoverColor,
		buttonPressedColor;

	Color textboxTextColor,
		textboxBackgroundColor;
};

enum TextureAtlasItem {
	TextureAtlasItem_None = 0,
	TextureAtlasItem_GroundTile = 1,
	TextureAtlasItem_WaterTile,
	TextureAtlasItem_ForestTile,
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

struct Texture {
	bool valid;
	GLuint id;
	uint32 w,h;
};

struct TextureRegion {
	GLuint textureID;
	RealRect bounds;
};

enum AnimationID {
	Animation_Farmer_Stand,
	Animation_Farmer_Walk,
	Animation_Farmer_Hold,
	Animation_Farmer_Carry,
	Animation_Farmer_Harvest,
	Animation_Farmer_Plant,
	
	Animation_Count,
};

struct Animation {
	TextureAtlasItem frames[16];
	uint32 frameCount;
};

struct Animator {
	Animation *animation;
	uint32 currentFrame;
	real32 frameCounter; // Sub-frame ticks
};
const real32 animationFramesPerDay = 10.0f;

struct Sprite {
	TextureAtlasItem textureAtlasItem;
	V2 pos;
	V2 size;
	real32 depth; // Positive is forwards from the camera
	V4 color;
};

const int SPRITE_MAX = 16384;
struct SpriteBuffer {
	Sprite sprites[SPRITE_MAX];
	uint32 count;
};

struct GLRenderer {
	SDL_Window *window;
	SDL_GLContext context;

	GLuint shaderProgramID;
	GLuint VBO,
		   IBO;
	GLint uProjectionMatrixLoc,
		  uTextureLoc;
	GLint aPositionLoc,
		  aColorLoc,
		  aUVLoc;

	Matrix4 projectionMatrix;

	GLuint texture;
	GLenum textureFormat;

	SpriteBuffer spriteBuffer;

	VertexData vertices[SPRITE_MAX * 4];
	uint32 vertexCount;
	GLuint indices[SPRITE_MAX * 6];
	uint32 indexCount;

	Animation animations[Animation_Count];

	TextureRegion textureRegions[TextureAtlasItemCount];
	UiTheme theme;
	Camera camera; // TODO: Remove! Or fix, whatever.
};

bool initializeRenderer(GLRenderer *renderer, const char *gameName);
void freeRenderer(GLRenderer *renderer);
bool initOpenGL(GLRenderer *renderer);
bool loadTextures(GLRenderer *renderer);
void printProgramLog(GLuint program);
void printShaderLog(GLuint shader);

void drawSprite(GLRenderer *renderer, TextureAtlasItem textureAtlasItem,
				V2 position, V2 size, Color *color=0);
void render(GLRenderer *renderer);

SDL_Cursor *createCursor(char *path);

#include "render_gl.cpp"