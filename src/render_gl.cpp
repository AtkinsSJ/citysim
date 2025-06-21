/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "render_gl.h"
#include <Assets/Asset.h>
#include <Assets/AssetManager.h>
#include <Util/Deferred.h>
#include <Util/Log.h>

#define OPTIMIZE_IBO 1

bool GL_Renderer::set_up_context()
{
    // Use GL3.1 Core
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

#if BUILD_DEBUG
    s32 contextFlags = 0;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &contextFlags);
    contextFlags |= SDL_GL_CONTEXT_DEBUG_FLAG;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, contextFlags);
#endif

    // Create context
    m_context = SDL_GL_CreateContext(window);
    if (m_context == nullptr) {
        logCritical("OpenGL context could not be created! :(\n {0}"_s, { makeString(SDL_GetError()) });
        return false;
    }

    // GLEW
    glewExperimental = GL_TRUE;
    if (auto const glewError = glewInit(); glewError != GLEW_OK) {
        logCritical("Could not initialise GLEW! :(\n {0}"_s, { makeString((char*)glewGetErrorString(glewError)) });
        return false;
    }

#if BUILD_DEBUG
    // Debug callbacks
    if (GLEW_KHR_debug) {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(GL_debugCallback, nullptr);
        logInfo("OpenGL debug message callback enabled"_s);
    } else {
        logInfo("OpenGL debug message callback not available in this context"_s);
    }
#endif

    // VSync
    if (SDL_GL_SetSwapInterval(1) < 0) {
        logCritical("Could not set vsync! :(\n {0}"_s, { makeString(SDL_GetError()) });
        return false;
    }

    // Init OpenGL
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    GLint vBufferSizeNeeded = RENDER_BATCH_VERTEX_COUNT * sizeof(m_vertices[0]);
    glBufferData(GL_ARRAY_BUFFER, vBufferSizeNeeded, nullptr, GL_DYNAMIC_DRAW);

    //
    // NB: This is a (slightly crazy) optimization, relying on us always passing vertices as quads.
    // If that every changes, we'll have to go back to assigning indices as we add vertices to the
    // VBO, instead of always reusing them like this. But for now, this lets us skip the (slow)
    // call to send the indices to the GPU every draw call.
    // If you want to change back, see the #ifdef'd out code in flushVertices() and pushQuad().
    //
    // - Sam, 29/07/2019
    //
#if OPTIMIZE_IBO
    s32 firstVertex = 0;
    for (s32 i = 0;
        i < RENDER_BATCH_INDEX_COUNT;
        i += 6, firstVertex += 4) {
        GLuint* index = m_indices + i;
        index[0] = firstVertex + 0;
        index[1] = firstVertex + 1;
        index[2] = firstVertex + 2;
        index[3] = firstVertex + 0;
        index[4] = firstVertex + 2;
        index[5] = firstVertex + 3;
    }
#endif

    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    GLint iBufferSizeNeeded = RENDER_BATCH_INDEX_COUNT * sizeof(m_indices[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, iBufferSizeNeeded, m_indices, GL_DYNAMIC_DRAW);

    m_vertex_count = 0;
    m_index_count = 0;

    glGenTextures(1, &m_palette_texture_id);
    glGenTextures(1, &m_raw_texture_id);

    // Other GL_Renderer struct init stuff
    initChunkedArray(&m_shaders, &renderArena, 64);

    initStack(&m_scissor_stack, &renderArena);

    return true;
}

GL_Renderer::GL_Renderer(SDL_Window* window)
    : Renderer(window)
{
}

void GL_Renderer::free()
{
    SDL_GL_DeleteContext(m_context);
    m_context = nullptr;
}

void GL_Renderer::on_window_resized(s32 newWidth, s32 newHeight)
{
    glViewport(0, 0, newWidth, newHeight);
}

void GL_Renderer::render(Array<RenderBuffer*> buffers)
{
    DEBUG_FUNCTION_T(DCDT_Renderer);

    GL_ShaderProgram* activeShader = nullptr;
    Camera* currentCamera = nullptr;

    for (s32 bufferIndex = 0; bufferIndex < buffers.count; bufferIndex++) {
        RenderBuffer* buffer = buffers[bufferIndex];
        RenderBufferChunk* renderBufferChunk = buffer->firstChunk;

        DEBUG_BEGIN_RENDER_BUFFER(buffer->name, buffer->name);

        smm pos = 0;
        while ((renderBufferChunk != nullptr) && (pos < renderBufferChunk->used)) {
            RenderItemType itemType = *((RenderItemType*)(renderBufferChunk->memory + pos));
            pos += sizeof(RenderItemType);

            switch (itemType) {
            case RenderItemType_NextMemoryChunk: {
                DEBUG_BLOCK_T("render: RenderItemType_NextMemoryChunk", DCDT_Renderer);
                DEBUG_TRACK_RENDER_BUFFER_CHUNK();
                renderBufferChunk = renderBufferChunk->nextChunk;
                pos = 0;
            } break;

            case RenderItemType_SetCamera: {
                DEBUG_BLOCK_T("render: RenderItemType_SetCamera", DCDT_Renderer);
                RenderItem_SetCamera* header = readRenderItem<RenderItem_SetCamera>(renderBufferChunk, &pos);
                currentCamera = header->camera;
            } break;

            case RenderItemType_SetPalette: {
                DEBUG_BLOCK_T("render: RenderItemType_SetPalette", DCDT_Renderer);
                RenderItem_SetPalette* header = readRenderItem<RenderItem_SetPalette>(renderBufferChunk, &pos);

                if (m_vertex_count > 0) {
                    flush_vertices();
                }

                // A palette is a 1D texture
                glActiveTexture(GL_TEXTURE0 + 1);
                glBindTexture(GL_TEXTURE_1D, m_palette_texture_id);

                glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                // Having a transparent border color which we clamp outside m_indices to, means that any parts of the
                // texture that specify a palette color that doesn't exist, will be fully transparent.
                // This is a bit of a hack... but it's a useful one!
                f32 borderColor[4] = { 0, 0, 0, 0 };
                glTexParameterfv(GL_TEXTURE_1D, GL_TEXTURE_BORDER_COLOR, borderColor);
                glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);

                V4* paletteData = readRenderData<V4>(renderBufferChunk, &pos, header->paletteSize);

                glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, header->paletteSize, 0, GL_RGBA, GL_FLOAT, paletteData);

                glUniform1i(activeShader->uPaletteLoc, 1);
            } break;

            case RenderItemType_SetShader: {
                DEBUG_BLOCK_T("render: RenderItemType_SetShader", DCDT_Renderer);
                RenderItem_SetShader* header = readRenderItem<RenderItem_SetShader>(renderBufferChunk, &pos);

                if (m_vertex_count > 0) {
                    flush_vertices();
                }

                activeShader = use_shader(header->shaderID);
                glUniformMatrix4fv(activeShader->uProjectionMatrixLoc, 1, false, currentCamera->projectionMatrix.flat);
                glUniform1f(activeShader->uScaleLoc, currentCamera->zoom);
            } break;

            case RenderItemType_SetTexture: {
                DEBUG_BLOCK_T("render: RenderItemType_SetTexture", DCDT_Renderer);
                RenderItem_SetTexture* header = readRenderItem<RenderItem_SetTexture>(renderBufferChunk, &pos);

                if (m_vertex_count > 0) {
                    flush_vertices();
                }

                if (header->texture == nullptr) {
                    // Raw texture!
                    glActiveTexture(GL_TEXTURE0 + 0);
                    glBindTexture(GL_TEXTURE_2D, m_raw_texture_id);

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                    u8* pixelData = readRenderData<u8>(renderBufferChunk, &pos, header->width * header->height * header->bytesPerPixel);

                    GLenum pixelFormat = 0;
                    switch (header->bytesPerPixel) {
                    case 1:
                        pixelFormat = GL_RED;
                        break;
                    case 2:
                        pixelFormat = GL_RG;
                        break;
                    case 3:
                        pixelFormat = GL_RGB;
                        break;
                    case 4:
                        pixelFormat = GL_RGBA;
                        break;
                    default:
                        ASSERT(false);
                    }

                    upload_texture_2d(pixelFormat, header->width, header->height, pixelData);
                } else {
                    ASSERT(header->texture->state == AssetState_Loaded);

                    Texture* texture = &header->texture->texture;

                    glActiveTexture(GL_TEXTURE0 + 0);
                    glBindTexture(GL_TEXTURE_2D, texture->gl.glTextureID);

                    if (!texture->gl.isLoaded) {
                        // Load texture into GPU
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                        // Upload texture
                        upload_texture_2d(GL_RGBA, texture->surface->w, texture->surface->h, texture->surface->pixels);
                        texture->gl.isLoaded = true;
                    }
                }

                glUniform1i(activeShader->uTextureLoc, 0);
                m_current_texture = header->texture;
            } break;

            case RenderItemType_Clear: {
                DEBUG_BLOCK_T("render: RenderItemType_Clear", DCDT_Renderer);
                RenderItem_Clear* header = readRenderItem<RenderItem_Clear>(renderBufferChunk, &pos);

                // NB: We MUST flush here, otherwise we could render some things after the clear instead of before it!
                if (m_vertex_count > 0) {
                    flush_vertices();
                }

                glClearColor(header->clearColor.r, header->clearColor.g, header->clearColor.b, header->clearColor.a);
                glClear(GL_COLOR_BUFFER_BIT);
            } break;

            case RenderItemType_BeginScissor: {
                DEBUG_BLOCK_T("render: RenderItemType_BeginScissor", DCDT_Renderer);
                RenderItem_BeginScissor* header = readRenderItem<RenderItem_BeginScissor>(renderBufferChunk, &pos);

                // NB: We MUST flush here, otherwise we could render some things after the scissor instead of before it!
                if (m_vertex_count > 0) {
                    flush_vertices();
                }

                if (isEmpty(&m_scissor_stack)) {
                    glEnable(GL_SCISSOR_TEST);
                }

                push(&m_scissor_stack, header->bounds);

                glScissor(header->bounds.x, header->bounds.y, header->bounds.w, header->bounds.h);
            } break;

            case RenderItemType_EndScissor: {
                DEBUG_BLOCK_T("render: RenderItemType_EndScissor", DCDT_Renderer);
                [[maybe_unused]] RenderItem_EndScissor* header = readRenderItem<RenderItem_EndScissor>(renderBufferChunk, &pos);

                // NB: We MUST flush here, otherwise we could render some things after the scissor is removed!
                // (This bug took me nearly half an hour to figure out.)
                if (m_vertex_count > 0) {
                    flush_vertices();
                }

                pop(&m_scissor_stack);

                // Restore previous scissor
                if (!isEmpty(&m_scissor_stack)) {
                    Rect2I* previousScissor = peek(&m_scissor_stack);
                    glScissor(previousScissor->x, previousScissor->y, previousScissor->w, previousScissor->h);
                } else {
                    glDisable(GL_SCISSOR_TEST);
                }
            } break;

            case RenderItemType_DrawRects: {
                DEBUG_BLOCK_T("render: RenderItemType_DrawRects", DCDT_Renderer);
                RenderItem_DrawRects* header = readRenderItem<RenderItem_DrawRects>(renderBufferChunk, &pos);

                for (s32 itemIndex = 0; itemIndex < header->count; itemIndex++) {
                    RenderItem_DrawRects_Item* item = readRenderItem<RenderItem_DrawRects_Item>(renderBufferChunk, &pos);

                    if (m_vertex_count + 4 > RENDER_BATCH_VERTEX_COUNT) {
                        flush_vertices();
                    }
                    push_quad_with_uv(item->bounds, item->color, item->uv);
                }
            } break;

            case RenderItemType_DrawSingleRect: {
                DEBUG_BLOCK_T("render: RenderItemType_DrawSingleRect", DCDT_Renderer);
                RenderItem_DrawSingleRect* item = readRenderItem<RenderItem_DrawSingleRect>(renderBufferChunk, &pos);

                if (m_vertex_count + 4 > RENDER_BATCH_VERTEX_COUNT) {
                    flush_vertices();
                }
                push_quad_with_uv_multicolor(item->bounds, item->color00, item->color01, item->color10, item->color11, item->uv);

            } break;

            case RenderItemType_DrawRings: {
                DEBUG_BLOCK_T("render: RenderItemType_DrawRings", DCDT_Renderer);
                RenderItem_DrawRings* header = readRenderItem<RenderItem_DrawRings>(renderBufferChunk, &pos);

                for (s32 itemIndex = 0; itemIndex < header->count; itemIndex++) {
                    RenderItem_DrawRings_Item* item = readRenderItem<RenderItem_DrawRings_Item>(renderBufferChunk, &pos);

                    s32 ringSegmentsCount = ceil_s32(2.0f * PI32 * item->radius); // TODO: some kind of "detail" parameter?
                    if (ringSegmentsCount * 4 > RENDER_BATCH_VERTEX_COUNT) {
                        s32 maxRingSegmentsCount = RENDER_BATCH_VERTEX_COUNT / 4;
                        logWarn("We want to draw a ring with {0} segments, but the maximum we can fit in a render batch is {1}!"_s, { formatInt(ringSegmentsCount), formatInt(maxRingSegmentsCount) });

                        ringSegmentsCount = maxRingSegmentsCount;
                    }

                    if (m_vertex_count + (4 * ringSegmentsCount) > RENDER_BATCH_VERTEX_COUNT) {
                        flush_vertices();
                    }

                    f32 minRadius = item->radius - (item->thickness * 0.5f);
                    f32 maxRadius = minRadius + item->thickness;
                    f32 radPerSegment = (2.0f * PI32) / (f32)ringSegmentsCount;

                    for (s32 segmentIndex = 0; segmentIndex < ringSegmentsCount; segmentIndex++) {
                        GL_VertexData* vertex = m_vertices + m_vertex_count;
                        [[maybe_unused]] s32 firstVertex = m_vertex_count;

                        f32 startAngle = segmentIndex * radPerSegment;
                        f32 endAngle = (segmentIndex + 1) * radPerSegment;

                        vertex->pos.x = item->centre.x + (minRadius * cos32(startAngle));
                        vertex->pos.y = item->centre.y + (minRadius * sin32(startAngle));
                        vertex->color = item->color;
                        vertex++;

                        vertex->pos.x = item->centre.x + (minRadius * cos32(endAngle));
                        vertex->pos.y = item->centre.y + (minRadius * sin32(endAngle));
                        vertex->color = item->color;
                        vertex++;

                        vertex->pos.x = item->centre.x + (maxRadius * cos32(endAngle));
                        vertex->pos.y = item->centre.y + (maxRadius * sin32(endAngle));
                        vertex->color = item->color;
                        vertex++;

                        vertex->pos.x = item->centre.x + (maxRadius * cos32(startAngle));
                        vertex->pos.y = item->centre.y + (maxRadius * sin32(startAngle));
                        vertex->color = item->color;

                        m_vertex_count += 4;

                        // NB: See comment in GL_initializeRenderer() - we can use the same buffer index buffer data
                        // always, as long as we only render quads.
#if !OPTIMIZE_IBO
                        GLuint* index = m_indices + m_index_count;
                        index[0] = firstVertex + 0;
                        index[1] = firstVertex + 1;
                        index[2] = firstVertex + 2;
                        index[3] = firstVertex + 0;
                        index[4] = firstVertex + 2;
                        index[5] = firstVertex + 3;
#endif
                        m_index_count += 6;
                    }
                }
            } break;

                INVALID_DEFAULT_CASE;
            }
        }

        if (m_vertex_count > 0) {
            flush_vertices();
        }

        DEBUG_END_RENDER_BUFFER();
    }

    ASSERT(isEmpty(&m_scissor_stack));
}

void GL_Renderer::load_assets()
{
    DEBUG_FUNCTION_T(DCDT_Renderer);

    // Textures
    for (auto it = asset_manager().assetsByType[AssetType_Texture].iterate();
        it.hasNext();
        it.next()) {
        Asset* asset = *it.get();
        Texture* texture = &asset->texture;

        glGenTextures(1, &texture->gl.glTextureID);
        texture->gl.isLoaded = false;
    }

    // Shaders
    m_shaders.clear(); // Just in case
    ASSERT(asset_manager().assetsByType[AssetType_Shader].count <= s8Max);
    for (auto it = asset_manager().assetsByType[AssetType_Shader].iterate();
        it.hasNext();
        it.next()) {
        Asset* asset = *it.get();

        s8 shaderIndex = (s8)m_shaders.count;
        GL_ShaderProgram* shader = m_shaders.appendBlank();
        shader->asset = asset;
        asset->shader.rendererShaderID = shaderIndex;

        loadShaderProgram(asset, shader);
        if (!shader->isValid) {
            logError("Failed to load shader '{0}' into OpenGL."_s, { asset->shortName });
        }
    }
}

void GL_Renderer::unload_assets()
{
    DEBUG_FUNCTION_T(DCDT_Renderer);

    // Textures
    for (auto it = asset_manager().assetsByType[AssetType_Texture].iterate();
        it.hasNext();
        it.next()) {
        Asset* asset = *it.get();
        Texture* texture = &asset->texture;

        if (texture->gl.isLoaded && texture->gl.glTextureID != 0) {
            glDeleteTextures(1, &texture->gl.glTextureID);
            texture->gl.glTextureID = 0;
            texture->gl.isLoaded = false;
        }
    }

    // Shaders
    m_current_shader = -1;
    for (s8 shaderID = 0; shaderID < m_shaders.count; shaderID++) {
        GL_ShaderProgram* shader = m_shaders.get(shaderID);
        glDeleteProgram(shader->shaderProgramID);
        *shader = {};
    }
    m_shaders.clear();
}

void logGLError(GLenum errorCode)
{
    if (errorCode != GL_NO_ERROR) {
        logError(makeString((char*)gluErrorString(errorCode)));
    }
}

void GLAPIENTRY GL_debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* userParam)
{
    // Prevent warnings
    if (userParam && source) { }

    String typeString = nullString;
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        typeString = "ERROR"_s;
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        typeString = "DEPRECATED_BEHAVIOR"_s;
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        typeString = "UNDEFINED_BEHAVIOR"_s;
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        typeString = "PORTABILITY"_s;
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        typeString = "PERFORMANCE"_s;
        break;
    default:
        typeString = "OTHER"_s;
        break;
    }

    String severityString = nullString;
    SDL_LogPriority priority;
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        severityString = "HIGH"_s;
        priority = SDL_LOG_PRIORITY_ERROR;
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        severityString = "MEDIUM"_s;
        priority = SDL_LOG_PRIORITY_WARN;
        break;
    case GL_DEBUG_SEVERITY_LOW:
        severityString = "LOW"_s;
        priority = SDL_LOG_PRIORITY_WARN;
        break;
    default:
        severityString = "OTHER"_s;
        priority = SDL_LOG_PRIORITY_INFO;
        break;
    }

    String messageString = makeString((char*)message, truncate32(length));

    log(priority, "GL DEBUG: {0} (severity {1}, id {2}): {3}"_s, { typeString, severityString, formatInt(id), messageString });

    DEBUG_BREAK();
}

bool compileShader(GL_ShaderProgram* glShader, String shaderName, Shader* shaderProgram, GL_ShaderPart shaderPart)
{
    DEBUG_FUNCTION_T(DCDT_Renderer);

    bool result = false;

    GLuint shaderID = glCreateShader(shaderPart);
    Deferred defer_delete_shader = [shaderID] { glDeleteShader(shaderID); };

    String source = nullString;
    switch (shaderPart) {
    case GL_ShaderPart_Vertex: {
        source = shaderProgram->vertexShader;
    } break;

    case GL_ShaderPart_Fragment: {
        source = shaderProgram->fragmentShader;
    } break;

        INVALID_DEFAULT_CASE;
    }

    glShaderSource(shaderID, 1, (char**)&source.chars, &source.length);

    glCompileShader(shaderID);

    GLint isCompiled = GL_FALSE;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &isCompiled);
    result = (isCompiled == GL_TRUE);

    if (result) {
        glAttachShader(glShader->shaderProgramID, shaderID);
    } else {
        // Print a big error message about this!
        int logMaxLength = 0;

        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logMaxLength);
        String infoLog = pushString(&temp_arena(), logMaxLength);
        glGetShaderInfoLog(shaderID, logMaxLength, &infoLog.length, infoLog.chars);

        if (isEmpty(infoLog)) {
            infoLog = "No error log provided by OpenGL. Sad panda."_s;
        }

        logError("Unable to compile part {3} of shader {0}, \'{1}\'! ({2})"_s, { formatInt(shaderID), shaderName, infoLog, formatInt(shaderPart) });
    }

    return result;
}

void loadShaderAttrib(GL_ShaderProgram* glShader, char const* attribName, int* attribLocation)
{
    DEBUG_FUNCTION_T(DCDT_Renderer);

    *attribLocation = glGetAttribLocation(glShader->shaderProgramID, attribName);
    if (*attribLocation == -1) {
        logWarn("Shader '{0}' does not contain requested variable '{1}'"_s, { glShader->asset->shortName, makeString(attribName) });
    }
}

void loadShaderUniform(GL_ShaderProgram* glShader, char const* uniformName, int* uniformLocation)
{
    DEBUG_FUNCTION_T(DCDT_Renderer);

    *uniformLocation = glGetUniformLocation(glShader->shaderProgramID, uniformName);
    if (*uniformLocation == -1) {
        logWarn("Shader '{0}' does not contain requested uniform '{1}'"_s, { glShader->asset->shortName, makeString(uniformName) });
    }
}

void loadShaderProgram(Asset* asset, GL_ShaderProgram* glShader)
{
    DEBUG_FUNCTION_T(DCDT_Renderer);

    glShader->shaderProgramID = glCreateProgram();

    if (glShader->shaderProgramID) {
        bool isVertexShaderCompiled = compileShader(glShader, asset->shortName, &asset->shader, GL_ShaderPart_Vertex);
        bool isFragmentShaderCompiled = compileShader(glShader, asset->shortName, &asset->shader, GL_ShaderPart_Fragment);

        // Link shader programs
        if (isVertexShaderCompiled && isFragmentShaderCompiled) {
            glLinkProgram(glShader->shaderProgramID);
            GLint programSuccess = GL_FALSE;
            glGetProgramiv(glShader->shaderProgramID, GL_LINK_STATUS, &programSuccess);
            glShader->isValid = (programSuccess == GL_TRUE);

            if (!glShader->isValid) {
                // Print a big error message about this!
                int logMaxLength = 0;

                glGetProgramiv(glShader->shaderProgramID, GL_INFO_LOG_LENGTH, &logMaxLength);
                String infoLog = pushString(&temp_arena(), logMaxLength);
                glGetProgramInfoLog(glShader->shaderProgramID, logMaxLength, &infoLog.length, infoLog.chars);

                if (isEmpty(infoLog)) {
                    infoLog = "No error log provided by OpenGL. Sad panda."_s;
                }

                logError("Unable to link shader program {0}! ({1})"_s, { asset->shortName, infoLog });
            } else {
                // Vertex attributes
                loadShaderAttrib(glShader, "aPosition", &glShader->aPositionLoc);
                loadShaderAttrib(glShader, "aColor", &glShader->aColorLoc);
                loadShaderAttrib(glShader, "aUV", &glShader->aUVLoc);

                // Uniforms
                loadShaderUniform(glShader, "uPalette", &glShader->uPaletteLoc);
                loadShaderUniform(glShader, "uProjectionMatrix", &glShader->uProjectionMatrixLoc);
                loadShaderUniform(glShader, "uTexture", &glShader->uTextureLoc);
                loadShaderUniform(glShader, "uScale", &glShader->uScaleLoc);
            }
        }
    } else {
        logGLError(glGetError());
    }
}

GL_ShaderProgram* GL_Renderer::use_shader(s8 shaderID)
{
    ASSERT(shaderID >= 0 && shaderID < m_shaders.count); // Invalid shader!

    // Early-out if nothing is changing!
    if (shaderID == m_current_shader) {
        return m_shaders.get(m_current_shader);
    }

    if (m_current_shader >= 0 && m_current_shader < m_shaders.count) {
        // Clean up the old shader's stuff
        GL_ShaderProgram* oldShader = m_shaders.get(m_current_shader);
        glDisableVertexAttribArray(oldShader->aPositionLoc);
        glDisableVertexAttribArray(oldShader->aColorLoc);
        if (oldShader->aUVLoc != -1) {
            glDisableVertexAttribArray(oldShader->aUVLoc);
        }
    }

    m_current_shader = shaderID;
    GL_ShaderProgram* activeShader = m_shaders.get(m_current_shader);

    ASSERT(activeShader->isValid); // Attempting to use a shader that isn't loaded!
    glUseProgram(activeShader->shaderProgramID);

    // Initialise the new shader's stuff
    glEnableVertexAttribArray(activeShader->aPositionLoc);
    glVertexAttribPointer(activeShader->aPositionLoc, 3, GL_FLOAT, GL_FALSE, sizeof(GL_VertexData), (GLvoid*)offsetof(GL_VertexData, pos));

    glEnableVertexAttribArray(activeShader->aColorLoc);
    glVertexAttribPointer(activeShader->aColorLoc, 4, GL_FLOAT, GL_FALSE, sizeof(GL_VertexData), (GLvoid*)offsetof(GL_VertexData, color));

    if (activeShader->aUVLoc != -1) {
        glEnableVertexAttribArray(activeShader->aUVLoc);
        glVertexAttribPointer(activeShader->aUVLoc, 2, GL_FLOAT, GL_FALSE, sizeof(GL_VertexData), (GLvoid*)offsetof(GL_VertexData, uv));
    }

    return activeShader;
}

void GL_Renderer::upload_texture_2d(GLenum pixelFormat, s32 width, s32 height, void* pixelData)
{
    // OpenGL appears to pad images to the nearest multiple of 4 bytes wide, which messes things up.
    // So, we only send the whole image as once if that isn't going to happen.
    if ((width % 4) == 0) {
        glTexImage2D(GL_TEXTURE_2D, 0, pixelFormat, width, height, 0, pixelFormat, GL_UNSIGNED_BYTE, pixelData);
    } else {
        // Send the image as individual rows
        glTexImage2D(GL_TEXTURE_2D, 0, pixelFormat, width, height, 0, pixelFormat, GL_UNSIGNED_BYTE, nullptr);

        s32 stride = width;
        switch (pixelFormat) {
        case GL_RED:
            stride = width * 1;
            break;
        case GL_RG:
            stride = width * 2;
            break;
        case GL_RGB:
            stride = width * 3;
            break;
        case GL_RGBA:
            stride = width * 4;
            break;
        default:
            ASSERT(false);
        }

        u8* pos = (u8*)pixelData;
        for (s32 y = 0; y < height; y++) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, width, 1, pixelFormat, GL_UNSIGNED_BYTE, pos);
            pos += stride;
        }
    }
}

void GL_Renderer::push_quad(Rect2 bounds, V4 color)
{
    DEBUG_FUNCTION_T(DCDT_Renderer);
    [[maybe_unused]] s32 firstVertex = m_vertex_count;

    GL_VertexData* vertex = m_vertices + m_vertex_count;

    f32 minX = bounds.x;
    f32 maxX = bounds.x + bounds.w;
    f32 minY = bounds.y;
    f32 maxY = bounds.y + bounds.h;

    vertex->pos.x = minX;
    vertex->pos.y = minY;
    vertex->color = color;
    vertex++;

    vertex->pos.x = maxX;
    vertex->pos.y = minY;
    vertex->color = color;
    vertex++;

    vertex->pos.x = maxX;
    vertex->pos.y = maxY;
    vertex->color = color;
    vertex++;

    vertex->pos.x = minX;
    vertex->pos.y = maxY;
    vertex->color = color;

    m_vertex_count += 4;

// NB: See comment in GL_initializeRenderer() - we can use the same buffer index buffer data
// always, as long as we only render quads.
#if !OPTIMIZE_IBO
    GLuint* index = m_indices + m_index_count;
    index[0] = firstVertex + 0;
    index[1] = firstVertex + 1;
    index[2] = firstVertex + 2;
    index[3] = firstVertex + 0;
    index[4] = firstVertex + 2;
    index[5] = firstVertex + 3;
#endif
    m_index_count += 6;
}

void GL_Renderer::push_quad_with_uv(Rect2 bounds, V4 color, Rect2 uv)
{
    return push_quad_with_uv_multicolor(bounds, color, color, color, color, uv);
}

void GL_Renderer::push_quad_with_uv_multicolor(Rect2 bounds, V4 color00, V4 color01, V4 color10, V4 color11, Rect2 uv)
{
    DEBUG_FUNCTION_T(DCDT_Renderer);
    [[maybe_unused]] s32 firstVertex = m_vertex_count;

    GL_VertexData* vertex = m_vertices + m_vertex_count;

    f32 minX = bounds.x;
    f32 maxX = bounds.x + bounds.w;
    f32 minY = bounds.y;
    f32 maxY = bounds.y + bounds.h;

    f32 minU = uv.x;
    f32 maxU = uv.x + uv.w;
    f32 minV = uv.y;
    f32 maxV = uv.y + uv.h;

    vertex->pos.x = minX;
    vertex->pos.y = minY;
    vertex->color = color00;
    vertex->uv.x = minU;
    vertex->uv.y = minV;
    vertex++;

    vertex->pos.x = maxX;
    vertex->pos.y = minY;
    vertex->color = color01;
    vertex->uv.x = maxU;
    vertex->uv.y = minV;
    vertex++;

    vertex->pos.x = maxX;
    vertex->pos.y = maxY;
    vertex->color = color11;
    vertex->uv.x = maxU;
    vertex->uv.y = maxV;
    vertex++;

    vertex->pos.x = minX;
    vertex->pos.y = maxY;
    vertex->color = color10;
    vertex->uv.x = minU;
    vertex->uv.y = maxV;

    m_vertex_count += 4;

// NB: See comment in GL_initializeRenderer() - we can use the same buffer index buffer data
// always, as long as we only render quads.
#if !OPTIMIZE_IBO
    GLuint* index = m_indices + m_index_count;
    index[0] = firstVertex + 0;
    index[1] = firstVertex + 1;
    index[2] = firstVertex + 2;
    index[3] = firstVertex + 0;
    index[4] = firstVertex + 2;
    index[5] = firstVertex + 3;
#endif
    m_index_count += 6;
}

void GL_Renderer::flush_vertices()
{
    DEBUG_FUNCTION_T(DCDT_Renderer);

    // Fill VBO
    {
        // DEBUG_BLOCK_T("flushVertices - Fill VBO", DCDT_Renderer);
        ASSERT(m_vertex_count <= RENDER_BATCH_VERTEX_COUNT); // Tried to render too many vertices at once!
        GLint vBufferSizeNeeded = m_vertex_count * sizeof(m_vertices[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vBufferSizeNeeded, m_vertices);
    }

// Fill IBO
// NB: See comment in GL_initializeRenderer() - we can use the same buffer index buffer data
// always, as long as we only render quads.
#if !OPTIMIZE_IBO
    {
        DEBUG_BLOCK_T("flushVertices - Fill IBO", DCDT_Renderer);
        ASSERT(m_index_count <= RENDER_BATCH_INDEX_COUNT); // Tried to render too many indices at once!
        GLint iBufferSizeNeeded = m_index_count * sizeof(m_indices[0]);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, iBufferSizeNeeded, m_indices);
    }
#endif

    {
        // DEBUG_BLOCK_T("flushVertices - glDrawElements", DCDT_Renderer);
        glDrawElements(GL_TRIANGLES, m_index_count, GL_UNSIGNED_INT, nullptr);
    }

    DEBUG_DRAW_CALL(m_shaders.get(m_current_shader)->asset->shortName, (m_current_texture == nullptr) ? nullString : m_current_texture->shortName, (m_vertex_count >> 2));

    m_vertex_count = 0;
    m_index_count = 0;
}
