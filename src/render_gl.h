#pragma once

// render_gl.h

const int FRAMES_PER_SECOND = 60;
const real32 SECONDS_PER_FRAME = 1.0f / 60.0f;
const int MS_PER_FRAME = (1000 / 60); // 60 frames per second

struct VertexData
{
	V3 pos;
	V4 color;
	V2 uv;
};

struct GL_Texture
{
	bool isValid;
	char *filename;
	GLuint glTextureID;
	uint32 w,h;
};

struct GL_TextureRegion
{
	GLuint textureID; // Redundant for GL_Texture.glTextureID!
	RealRect uv;
};

struct GL_TextureAtlas
{
	GL_TextureRegion textureRegions[TextureAtlasItemCount];
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
	GL_Texture textures[64];
	GL_TextureAtlas textureAtlas;

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
void GL_freeRenderer(GLRenderer *renderer);
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

void GL_render(GLRenderer *renderer);

SDL_Cursor *createCursor(char *path);
void setCursor(GLRenderer *renderer, Cursor cursor);

BitmapFont *readBMFont(MemoryArena *renderArena, TemporaryMemoryArena *tempArena, char *filename, GLRenderer *renderer);

#include "render_gl.cpp"