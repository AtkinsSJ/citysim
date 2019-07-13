#pragma once

#ifdef __linux__
#	include <gl/glew.h> // TODO: Check this
#	include <SDL2/SDL_opengl.h>
#else // Windows
#	include <gl/glew.h>
#	include <SDL_opengl.h>
#endif

// render_gl.h

#define CHECK_BUFFERS_SORTED 0

struct GL_VertexData
{
	V3 pos;
	V4 color;
	V2 uv;
};

enum GL_ShaderPart
{
	GL_ShaderPart_Fragment = GL_FRAGMENT_SHADER,
	GL_ShaderPart_Geometry = GL_GEOMETRY_SHADER,
	GL_ShaderPart_Vertex   = GL_VERTEX_SHADER,
};

struct GL_ShaderProgram
{
	GLuint shaderProgramID;
	bool isValid;

	Asset *asset;

	// Uniforms
	GLint uProjectionMatrixLoc;
	GLint uTextureLoc;
	GLint uScaleLoc;

	// Attributes
	GLint aPositionLoc;
	GLint aColorLoc;
	GLint aUVLoc;
};

// TODO: Figure out what a good number is for this!
const int RENDER_BATCH_SIZE = 1024;
const int RENDER_BATCH_VERTEX_COUNT = RENDER_BATCH_SIZE * 4;
const int RENDER_BATCH_INDEX_COUNT  = RENDER_BATCH_SIZE * 6;

struct GL_Renderer
{
	Renderer renderer;

	SDL_GLContext context;

	Array<GL_ShaderProgram> shaders;
	s32 currentShader;

	GLuint VBO;
	GLuint IBO;

	GL_VertexData vertices[RENDER_BATCH_VERTEX_COUNT];
	GLuint indices[RENDER_BATCH_INDEX_COUNT];
};

#include "render_gl.cpp"
