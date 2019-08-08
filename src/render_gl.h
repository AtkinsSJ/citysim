#pragma once

#ifdef __linux__
#	include <gl/glew.h> // TODO: Check this
#	include <SDL2/SDL_opengl.h>
#else // Windows
#	include <gl/glew.h>
#	include <SDL_opengl.h>
#endif

// render_gl.h

struct GL_VertexData
{
	V2 pos;
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

	s32 vertexCount;
	s32 indexCount;
	GL_VertexData vertices[RENDER_BATCH_VERTEX_COUNT];
	GLuint        indices [RENDER_BATCH_INDEX_COUNT];

	// For debugging only
	Asset *currentTexture; 
};

bool GL_initializeRenderer(SDL_Window *window);
void GL_render(RenderBufferChunk *firstChunk);
void GL_windowResized(s32 newWidth, s32 newHeight);
void GL_loadAssets();
void GL_unloadAssets();
void GL_freeRenderer();

void logGLError(GLenum errorCode);
void GLAPIENTRY GL_debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

void loadShaderProgram(Asset *asset, GL_ShaderProgram *glShader);
bool compileShader(GL_ShaderProgram *glShader, String shaderName, Shader *shaderProgram, GL_ShaderPart shaderPart);
void loadShaderAttrib(GL_ShaderProgram *glShader, char *attribName, int *attribLocation);
void loadShaderUniform(GL_ShaderProgram *glShader, char *uniformName, int *uniformLocation);
GL_ShaderProgram *useShader(GL_Renderer *gl, s8 shaderID);

void pushQuad(GL_Renderer *gl, Rect2 bounds, V4 color);
void pushQuadWithUV(GL_Renderer *gl, Rect2 bounds, V4 color, Rect2 uv);
void flushVertices(GL_Renderer *gl);
