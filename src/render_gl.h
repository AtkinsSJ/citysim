#pragma once

// render_gl.h

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
};

struct Texture
{
	bool isValid;
	char *filename;
	GLuint glTextureID;
	uint32 w,h;
};

struct TextureRegion
{
	GLuint textureID; // Redundant for Texture.glTextureID!
	RealRect uv;
};

struct TextureAtlas
{
	TextureRegion textureRegions[TextureAtlasItemCount];
};

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

struct GLShaderProgram
{
	GLuint shaderProgramID;
	bool isValid;

	char *vertexShaderFilename;
	GLuint vertexShader;
	bool isVertexShaderCompiled;

	char *fragmentShaderFilename;
	GLuint fragmentShader;
	bool isFragmentShaderCompiled;

	// Uniforms
	GLint uProjectionMatrixLoc,
		  uTextureLoc;

	// Attributes
	GLint aPositionLoc,
		  aColorLoc,
		  aUVLoc;
};

struct GLRenderer
{
	MemoryArena renderArena;

	SDL_Window *window;
	SDL_GLContext context;

	GLShaderProgram shaders[ShaderProgram_Count];
	int32 currentShader;

	GLuint VBO,
		   IBO;

	Camera worldCamera;

	RenderBuffer worldBuffer;
	RenderBuffer uiBuffer;

	VertexData vertices[SPRITE_MAX * 4];
	GLuint indices[SPRITE_MAX * 6];

	Animation animations[Animation_Count];

	uint32 textureCount;
	Texture textures[64];
	TextureAtlas textureAtlas;

	// UI Stuff!
	UiTheme theme;
};

const GLint TEXTURE_ID_INVALID = -99999;
const GLint TEXTURE_ID_NONE = -1;

inline void checkForGLError()
{
	GLenum errorCode = glGetError();
	ASSERT(errorCode == 0, "GL Error %d: %s", errorCode, gluErrorString(errorCode));
}

GLRenderer *initializeRenderer(MemoryArena *MemoryArena, const char *GameName);
void freeRenderer(GLRenderer *renderer);
bool initOpenGL(GLRenderer *renderer);
bool loadTextures(GLRenderer *renderer);
void printProgramLog(TemporaryMemoryArena *tempMemory, GLuint program);
void printShaderLog(TemporaryMemoryArena *tempMemory, GLuint shader);

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

BitmapFont *readBMFont(MemoryArena *renderArena, TemporaryMemoryArena *tempArena, char *filename, GLRenderer *renderer);

#include "render_gl.cpp"