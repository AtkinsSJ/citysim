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

#include <Gfx/Renderer.h>
#include <Util/ChunkedArray.h>
#include <Util/Stack.h>

namespace GL {

struct VertexData {
    V2 pos;
    V4 color;
    V2 uv;
};

enum ShaderPart {
    GL_ShaderPart_Fragment = GL_FRAGMENT_SHADER,
    GL_ShaderPart_Geometry = GL_GEOMETRY_SHADER,
    GL_ShaderPart_Vertex = GL_VERTEX_SHADER,
};

struct ShaderProgram {
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

class Renderer final : public ::Renderer {
public:
    explicit Renderer(SDL_Window*);
    virtual ~Renderer() override = default;

    bool set_up_context();

    // FIXME: Move free() into the destructor, once I sort out the MemoryArena mess.
    virtual void free() override;

    virtual void on_window_resized(s32 width, s32 height) override;
    virtual void render_internal() override;
    virtual void load_assets() override;
    virtual void unload_assets() override;

    ShaderProgram* use_shader(s8 shaderID);

private:
    void upload_texture_2d(GLenum pixelFormat, s32 width, s32 height, void* pixelData);
    void push_quad(Rect2 bounds, V4 color);
    void push_quad_with_uv(Rect2 bounds, V4 color, Rect2 uv);
    void push_quad_with_uv_multicolor(Rect2 bounds, V4 color00, V4 color01, V4 color10, V4 color11, Rect2 uv);
    void flush_vertices();

    SDL_GLContext m_context {};

    ChunkedArray<ShaderProgram> m_shaders {};
    s32 m_current_shader { -1 };

    GLuint m_vbo { 0 };
    GLuint m_ibo { 0 };

    s32 m_vertex_count { 0 };
    s32 m_index_count { 0 };
    VertexData m_vertices[RENDER_BATCH_VERTEX_COUNT] {};
    GLuint m_indices[RENDER_BATCH_INDEX_COUNT] {};

    u32 m_palette_texture_id { 0 };
    u32 m_raw_texture_id { 0 }; // TODO: @Speed: Using one texture means that it'll get resized a lot. That's probably really bad!

    Stack<Rect2I> m_scissor_stack {};

    // For debugging only
    Asset* m_current_texture { nullptr };
};

void logGLError(GLenum errorCode);
void GLAPIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* userParam);

void loadShaderProgram(Asset* asset, ShaderProgram* glShader);
bool compileShader(ShaderProgram* glShader, String shaderName, Shader* shaderProgram, ShaderPart shaderPart);
// NB: Using char* because we have to pass the names to GL!
void loadShaderAttrib(ShaderProgram* glShader, char const* attribName, int* attribLocation);
void loadShaderUniform(ShaderProgram* glShader, char const* uniformName, int* uniformLocation);

}
