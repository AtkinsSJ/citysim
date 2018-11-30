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

struct GL_TextureInfo
{
	GLuint glTextureID;
	bool isLoaded;
};

enum GL_ShaderType
{
	GL_ShaderType_Fragment = GL_FRAGMENT_SHADER,
	GL_ShaderType_Geometry = GL_GEOMETRY_SHADER,
	GL_ShaderType_Vertex = GL_VERTEX_SHADER,
};

struct GL_ShaderProgram
{
	ShaderProgramType assetID;
	GLuint shaderProgramID;
	bool isValid;

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
	Renderer renderer;

	SDL_GLContext context;

	GL_ShaderProgram shaders[ShaderProgramCount];
	s32 currentShader;

	GLuint VBO;
	GLuint IBO;

	GL_VertexData vertices[SPRITE_MAX * 4];
	GLuint indices[SPRITE_MAX * 6];

	GLuint textureCount;
	GL_TextureInfo textureInfo[64]; // TODO: Make this the right length
};

#include "render_gl.cpp"