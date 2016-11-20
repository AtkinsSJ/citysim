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

struct GL_TextureInfo
{
	GLuint glTextureID;
	bool isLoaded;
};

struct GL_ShaderProgram
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

struct GL_Renderer
{
	MemoryArena renderArena;

	SDL_Window *window;
	SDL_GLContext context;

	GL_ShaderProgram shaders[ShaderProgram_Count];
	int32 currentShader;

	GLuint VBO,
		   IBO;

	RenderBuffer worldBuffer;
	RenderBuffer uiBuffer;

	VertexData vertices[SPRITE_MAX * 4];
	GLuint indices[SPRITE_MAX * 6];

	// Animation animations[Animation_Count];

	GL_TextureInfo textureInfo[64]; // TODO: Make this the right length

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

BitmapFont *readBMFont(MemoryArena *renderArena, TemporaryMemoryArena *tempArena, char *filename, GL_Renderer *renderer);

#include "render_gl.cpp"