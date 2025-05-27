#pragma once

#ifdef __linux__
#    include <gl/glew.h> // TODO: Check this
#    include <SDL2/SDL_opengl.h>
#else // Windows
#    include <gl/glew.h>
#    include <SDL_opengl.h>
#endif

// render_gl.h

struct GL_VertexData {
    V2 pos;
    V4 color;
    V2 uv;
};

enum GL_ShaderPart {
    GL_ShaderPart_Fragment = GL_FRAGMENT_SHADER,
    GL_ShaderPart_Geometry = GL_GEOMETRY_SHADER,
    GL_ShaderPart_Vertex = GL_VERTEX_SHADER,
};

struct GL_ShaderProgram {
    GLuint shaderProgramID;
    bool isValid;

    Asset* asset;

    // Uniforms
    GLint uPaletteLoc;
    GLint uProjectionMatrixLoc;
    GLint uTextureLoc;
    GLint uScaleLoc;

    // Attributes
    GLint aPositionLoc;
    GLint aColorLoc;
    GLint aUVLoc;
};

// TODO: Figure out what a good number is for this!
int const RENDER_BATCH_SIZE = 1024;
int const RENDER_BATCH_VERTEX_COUNT = RENDER_BATCH_SIZE * 4;
int const RENDER_BATCH_INDEX_COUNT = RENDER_BATCH_SIZE * 6;

struct GL_Renderer {
    Renderer renderer;

    SDL_GLContext context;

    ChunkedArray<GL_ShaderProgram> shaders;
    s32 currentShader;

    GLuint VBO;
    GLuint IBO;

    s32 vertexCount;
    s32 indexCount;
    GL_VertexData vertices[RENDER_BATCH_VERTEX_COUNT];
    GLuint indices[RENDER_BATCH_INDEX_COUNT];

    u32 paletteTextureID;
    u32 rawTextureID; // TODO: @Speed: Using one texture means that it'll get resized a lot. That's probably really bad!

    Stack<Rect2I> scissorStack;

    // For debugging only
    Asset* currentTexture;
};

bool GL_initializeRenderer(SDL_Window* window);
void GL_render(Array<RenderBuffer*> buffers);
void GL_windowResized(s32 newWidth, s32 newHeight);
void GL_loadAssets();
void GL_unloadAssets();
void GL_freeRenderer();

void logGLError(GLenum errorCode);
void GLAPIENTRY GL_debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* userParam);

void loadShaderProgram(Asset* asset, GL_ShaderProgram* glShader);
bool compileShader(GL_ShaderProgram* glShader, String shaderName, Shader* shaderProgram, GL_ShaderPart shaderPart);
// NB: Using char* because we have to pass the names to GL!
void loadShaderAttrib(GL_ShaderProgram* glShader, char const* attribName, int* attribLocation);
void loadShaderUniform(GL_ShaderProgram* glShader, char const* uniformName, int* uniformLocation);
GL_ShaderProgram* useShader(GL_Renderer* gl, s8 shaderID);

// Internal
void uploadTexture2D(GLenum pixelFormat, s32 width, s32 height, void* pixelData);
void pushQuad(GL_Renderer* gl, Rect2 bounds, V4 color);
void pushQuadWithUV(GL_Renderer* gl, Rect2 bounds, V4 color, Rect2 uv);
void pushQuadWithUVMulticolor(GL_Renderer* gl, Rect2 bounds, V4 color00, V4 color01, V4 color10, V4 color11, Rect2 uv);
void flushVertices(GL_Renderer* gl);
