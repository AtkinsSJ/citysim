/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#ifdef __linux__
#    include <GL/glew.h>
#    include <SDL2/SDL_opengl.h>
#else // Windows
#    include <SDL_opengl.h>
#    include <gl/glew.h>
#endif

#include "render.h"

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

struct GL_Renderer final : public Renderer {
    virtual ~GL_Renderer() override = default;
    // FIXME: Move free() into the destructor, once I sort out the MemoryArena mess.
    virtual void free() override;

    virtual void on_window_resized(s32 width, s32 height) override;
    virtual void render(Array<RenderBuffer*>) override;
    virtual void load_assets() override;
    virtual void unload_assets() override;

    GL_ShaderProgram* use_shader(s8 shaderID);

private:
    void upload_texture_2d(GLenum pixelFormat, s32 width, s32 height, void* pixelData);
    void push_quad(Rect2 bounds, V4 color);
    void push_quad_with_uv(Rect2 bounds, V4 color, Rect2 uv);
    void push_quad_with_uv_multicolor(Rect2 bounds, V4 color00, V4 color01, V4 color10, V4 color11, Rect2 uv);
    void flush_vertices();

public:
    // FIXME: Un-publicize
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

void logGLError(GLenum errorCode);
void GLAPIENTRY GL_debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* userParam);

void loadShaderProgram(Asset* asset, GL_ShaderProgram* glShader);
bool compileShader(GL_ShaderProgram* glShader, String shaderName, Shader* shaderProgram, GL_ShaderPart shaderPart);
// NB: Using char* because we have to pass the names to GL!
void loadShaderAttrib(GL_ShaderProgram* glShader, char const* attribName, int* attribLocation);
void loadShaderUniform(GL_ShaderProgram* glShader, char const* uniformName, int* uniformLocation);
