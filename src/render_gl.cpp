
Renderer *GL_initializeRenderer(SDL_Window *window)
{
	GL_Renderer *gl;
	bootstrapArena(GL_Renderer, gl, renderer.renderArena);
	bool succeeded = (gl != 0);
	Renderer *renderer = &gl->renderer;

	if (!succeeded)
	{
		logCritical("Failed to allocate memory for GL_Renderer!");
	}
	else
	{
		initRenderer(renderer, &renderer->renderArena, window);

		renderer->platformRenderer = gl;
		renderer->windowResized = &GL_windowResized;
		renderer->render        = &GL_render;
		renderer->loadAssets    = &GL_loadAssets;
		renderer->unloadAssets  = &GL_unloadAssets;
		renderer->free          = &GL_freeRenderer;

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
		gl->context = SDL_GL_CreateContext(renderer->window);
		if (gl->context == NULL)
		{
			logCritical("OpenGL context could not be created! :(\n %s", {makeString(SDL_GetError())});
			succeeded = false;
		}

		// GLEW
		glewExperimental = GL_TRUE;
		GLenum glewError = glewInit();
		if (succeeded && glewError != GLEW_OK)
		{
			logCritical("Could not initialise GLEW! :(\n %s", {makeString((char*)glewGetErrorString(glewError))});
			succeeded = false;
		}

#if BUILD_DEBUG
		// Debug callbacks
		if (GLEW_KHR_debug)
		{
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallback(GL_debugCallback, null);
			logInfo("OpenGL debug message callback enabled");
		}
		else
		{
			logInfo("OpenGL debug message callback not available in this context");
		}
#endif

		// VSync
		if (succeeded && SDL_GL_SetSwapInterval(1) < 0)
		{
			logCritical("Could not set vsync! :(\n %s", {makeString(SDL_GetError())});
			succeeded = false;
		}

		// Init OpenGL
		if (succeeded)
		{
			glEnable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

			glGenBuffers(1, &gl->VBO);
			glBindBuffer(GL_ARRAY_BUFFER, gl->VBO);
			GLint vBufferSizeNeeded = RENDER_BATCH_VERTEX_COUNT * sizeof(gl->vertices[0]);
			glBufferData(GL_ARRAY_BUFFER, vBufferSizeNeeded, null, GL_STATIC_DRAW);

			glGenBuffers(1, &gl->IBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl->IBO);
			GLint iBufferSizeNeeded = RENDER_BATCH_INDEX_COUNT * sizeof(gl->indices[0]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, iBufferSizeNeeded, null, GL_STATIC_DRAW);

			gl->vertexCount = 0;
			gl->indexCount = 0;
		}
		else
		{
			logCritical("Could not initialise OpenGL! :(");
		}

		if (!succeeded)
		{
			GL_freeRenderer(renderer);
			gl = 0;
		}
	}

	return renderer;
}

void GL_freeRenderer(Renderer *renderer)
{
	GL_Renderer *gl = (GL_Renderer *)renderer->platformRenderer;
	SDL_GL_DeleteContext(gl->context);
}

void GL_windowResized(s32 newWidth, s32 newHeight)
{
	glViewport(0, 0, newWidth, newHeight);
}

inline void logGLError(GLenum errorCode)
{
	if (errorCode != GL_NO_ERROR)
	{
		logError("{0}", {makeString((char*)gluErrorString(errorCode))});
	}
}

void GLAPIENTRY GL_debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	// Prevent warnings
	if (userParam && source) {}

	String typeString = nullString;
	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR:                typeString = makeString("ERROR");                break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:  typeString = makeString("DEPRECATED_BEHAVIOR");  break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:   typeString = makeString("UNDEFINED_BEHAVIOR");   break;
		case GL_DEBUG_TYPE_PORTABILITY:          typeString = makeString("PORTABILITY");          break;
		case GL_DEBUG_TYPE_PERFORMANCE:          typeString = makeString("PERFORMANCE");          break;
		default:                                 typeString = makeString("OTHER");                break;
	}

	String severityString = nullString;
	SDL_LogPriority priority;
	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH:   severityString = makeString("HIGH");    priority = SDL_LOG_PRIORITY_ERROR;  break;
		case GL_DEBUG_SEVERITY_MEDIUM: severityString = makeString("MEDIUM");  priority = SDL_LOG_PRIORITY_WARN;   break;
		case GL_DEBUG_SEVERITY_LOW:    severityString = makeString("LOW");     priority = SDL_LOG_PRIORITY_WARN;   break;
		default:                       severityString = makeString("OTHER");   priority = SDL_LOG_PRIORITY_INFO;   break;
	}

	String messageString = makeString((char*)message, truncate32(length));

	log(priority, "GL DEBUG: {0} (severity {1}, id {2}): {3}", {typeString, severityString, formatInt(id), messageString});

	DEBUG_BREAK();
}

bool compileShader(GL_ShaderProgram *glShader, String shaderName, Shader *shaderProgram, GL_ShaderPart shaderPart)
{
	DEBUG_FUNCTION_T(DCDT_Renderer);

	bool result = false;

	GLuint shaderID = glCreateShader(shaderPart);
	defer { glDeleteShader(shaderID); };

	String source = nullString;
	switch (shaderPart)
	{
		case GL_ShaderPart_Vertex:
		{
			source = shaderProgram->vertexShader;
		} break;

		case GL_ShaderPart_Fragment:
		{
			source = shaderProgram->fragmentShader;
		} break;

		INVALID_DEFAULT_CASE;
	}

	glShaderSource(shaderID, 1, (char **)&source.chars, &source.length);

	glCompileShader(shaderID);

	GLint isCompiled = GL_FALSE;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &isCompiled);
	result = (isCompiled == GL_TRUE);

	if (result)
	{
		glAttachShader(glShader->shaderProgramID, shaderID);
	}
	else
	{
		// Print a big error message about this!
		int logMaxLength = 0;
		
		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logMaxLength);
		String infoLog = pushString(globalFrameTempArena, logMaxLength);
		glGetShaderInfoLog(shaderID, logMaxLength, &infoLog.length, infoLog.chars);

		if (infoLog.length == 0)
		{
			infoLog = makeString("No error log provided by OpenGL. Sad panda.");
		}

		logError("Unable to compile part {3} of shader {0}, \'{1}\'! ({2})", {formatInt(shaderID), shaderName, infoLog, formatInt(shaderPart)});
	}

	return result;
}

void loadShaderAttrib(GL_ShaderProgram *glShader, char *attribName, int *attribLocation)
{
	DEBUG_FUNCTION_T(DCDT_Renderer);
	
	*attribLocation = glGetAttribLocation(glShader->shaderProgramID, attribName);
	if (*attribLocation == -1)
	{
		logWarn("Shader '{0}' does not contain requested variable '{1}'", {glShader->asset->shortName, makeString(attribName)});
	}
}

void loadShaderUniform(GL_ShaderProgram *glShader, char *uniformName, int *uniformLocation)
{
	DEBUG_FUNCTION_T(DCDT_Renderer);
	
	*uniformLocation = glGetUniformLocation(glShader->shaderProgramID, uniformName);
	if (*uniformLocation == -1)
	{
		logWarn("Shader '{0}' does not contain requested uniform '{1}'", {glShader->asset->shortName, makeString(uniformName)});
	}
}

void loadShaderProgram(Asset *asset, GL_ShaderProgram *glShader)
{
	DEBUG_FUNCTION_T(DCDT_Renderer);
	
	glShader->shaderProgramID = glCreateProgram();

	if (glShader->shaderProgramID)
	{
		bool isVertexShaderCompiled   = compileShader(glShader, asset->shortName, &asset->shader, GL_ShaderPart_Vertex);
		bool isFragmentShaderCompiled = compileShader(glShader, asset->shortName, &asset->shader, GL_ShaderPart_Fragment);

		// Link shader programs
		if (isVertexShaderCompiled && isFragmentShaderCompiled)
		{
			glLinkProgram(glShader->shaderProgramID);
			GLint programSuccess = GL_FALSE;
			glGetProgramiv(glShader->shaderProgramID, GL_LINK_STATUS, &programSuccess);
			glShader->isValid = (programSuccess == GL_TRUE);

			if (!glShader->isValid)
			{
				// Print a big error message about this!
				int logMaxLength = 0;
				
				glGetProgramiv(glShader->shaderProgramID, GL_INFO_LOG_LENGTH, &logMaxLength);
				String infoLog = pushString(globalFrameTempArena, logMaxLength);
				glGetProgramInfoLog(glShader->shaderProgramID, logMaxLength, &infoLog.length, infoLog.chars);

				if (infoLog.length == 0)
				{
					infoLog = makeString("No error log provided by OpenGL. Sad panda.");
				}

				logError("Unable to link shader program {0}! ({1})", {asset->shortName, infoLog});
			}
			else
			{
				// Vertex attributes
				loadShaderAttrib(glShader, "aPosition", &glShader->aPositionLoc);
				loadShaderAttrib(glShader, "aColor", &glShader->aColorLoc);
				loadShaderAttrib(glShader, "aUV", &glShader->aUVLoc);

				// Uniforms
				loadShaderUniform(glShader, "uProjectionMatrix", &glShader->uProjectionMatrixLoc);
				loadShaderUniform(glShader, "uTexture", &glShader->uTextureLoc);
				loadShaderUniform(glShader, "uScale", &glShader->uScaleLoc);
			}
		}
	}
	else
	{
		logGLError(glGetError());
	}
}

void GL_loadAssets(Renderer *renderer, AssetManager *assets)
{
	DEBUG_FUNCTION_T(DCDT_Renderer);
	
	GL_Renderer *gl = (GL_Renderer *)renderer->platformRenderer;

	// Textures
	for (auto it = iterate(&assets->assetsByType[AssetType_Texture]);
		!it.isDone;
		next(&it))
	{
		Asset *asset = *get(it);
		Texture *texture = &asset->texture;

		glGenTextures(1, &texture->gl.glTextureID);
		texture->gl.isLoaded = false;
	}

	// Shaders
	clear(&gl->shaders); // Just in case
	ASSERT(assets->assetsByType[AssetType_Shader].count <= s8Max);
	for (auto it = iterate(&assets->assetsByType[AssetType_Shader]);
		!it.isDone;
		next(&it))
	{
		Asset *asset = *get(it);

		s8 shaderIndex = (s8) gl->shaders.count;
		GL_ShaderProgram *shader = appendBlank(&gl->shaders);
		shader->asset = asset;
		asset->shader.rendererShaderID = shaderIndex;

		loadShaderProgram(asset, shader);
		if(!shader->isValid)
		{
			logError("Failed to load shader '{0}' into OpenGL.", {asset->shortName});
		}
	}
}

void GL_unloadAssets(Renderer *renderer, AssetManager *assets)
{
	DEBUG_FUNCTION_T(DCDT_Renderer);
	
	GL_Renderer *gl = (GL_Renderer *)renderer->platformRenderer;

	// Textures
	for (auto it = iterate(&assets->assetsByType[AssetType_Texture]);
		!it.isDone;
		next(&it))
	{
		Asset *asset = *get(it);
		Texture *texture = &asset->texture;

		if (texture->gl.isLoaded && texture->gl.glTextureID != 0)
		{
			glDeleteTextures(1, &texture->gl.glTextureID);
			texture->gl.glTextureID = 0;
			texture->gl.isLoaded = false;
		}
	}

	// Shaders
	gl->currentShader = -1;
	for (s8 shaderID=0; shaderID < gl->shaders.count; shaderID++)
	{
		GL_ShaderProgram *shader = pointerTo(&gl->shaders, shaderID);
		glDeleteProgram(shader->shaderProgramID);
		*shader = {};
	}
	clear(&gl->shaders);
}

inline GL_ShaderProgram *useShader(GL_Renderer *renderer, s8 shaderID)
{
	ASSERT(shaderID >= 0 && shaderID < renderer->shaders.count); //Invalid shader!

	// Early-out if nothing is changing!
	if (shaderID == renderer->currentShader)
	{
		return renderer->shaders.items + renderer->currentShader;
	}

	if (renderer->currentShader >= 0 && renderer->currentShader < renderer->shaders.count)
	{
		// Clean up the old shader's stuff
		GL_ShaderProgram *oldShader = renderer->shaders.items + renderer->currentShader;
		glDisableVertexAttribArray(oldShader->aPositionLoc);
		glDisableVertexAttribArray(oldShader->aColorLoc);
		if (oldShader->aUVLoc != -1)
		{
			glDisableVertexAttribArray(oldShader->aUVLoc);
		}
	}

	renderer->currentShader = shaderID;
	GL_ShaderProgram *activeShader = renderer->shaders.items + renderer->currentShader;

	ASSERT(activeShader->isValid); //Attempting to use a shader that isn't loaded!
	glUseProgram(activeShader->shaderProgramID);

	// Initialise the new shader's stuff
	glEnableVertexAttribArray(activeShader->aPositionLoc);
	glVertexAttribPointer(activeShader->aPositionLoc, 3, GL_FLOAT, GL_FALSE, sizeof(GL_VertexData), (GLvoid*)offsetof(GL_VertexData, pos));

	glEnableVertexAttribArray(activeShader->aColorLoc);
	glVertexAttribPointer(activeShader->aColorLoc,    4, GL_FLOAT, GL_FALSE, sizeof(GL_VertexData), (GLvoid*)offsetof(GL_VertexData, color));

	if (activeShader->aUVLoc != -1)
	{
		glEnableVertexAttribArray(activeShader->aUVLoc);
		glVertexAttribPointer(activeShader->aUVLoc,   2, GL_FLOAT, GL_FALSE, sizeof(GL_VertexData), (GLvoid*)offsetof(GL_VertexData, uv));
	}

	return activeShader;
}

void GL_render(Renderer *renderer, RenderBufferChunk *firstChunk)
{
	DEBUG_FUNCTION_T(DCDT_Renderer);

	GL_Renderer *gl = (GL_Renderer *)renderer->platformRenderer;

	GL_ShaderProgram *activeShader = null;
	Camera *currentCamera = null;

	RenderBufferChunk *renderBufferChunk = firstChunk;
	smm pos = 0;
	while ((renderBufferChunk != null) && (pos < renderBufferChunk->used))
	{
		RenderItemType itemType = *((RenderItemType *)(renderBufferChunk->memory + pos));
		pos += sizeof(RenderItemType);

		switch (itemType)
		{
			case RenderItemType_NextMemoryChunk:
			{
				DEBUG_BLOCK_T("render: RenderItemType_NextMemoryChunk", DCDT_Renderer);
				DEBUG_TRACK_RENDER_BUFFER_CHUNK();
				renderBufferChunk = renderBufferChunk->nextChunk;
				pos = 0;
			} break;

			case RenderItemType_SectionMarker:
			{
				DEBUG_BLOCK_T("render: RenderItemType_SectionMarker", DCDT_Renderer);
				RenderItem_SectionMarker *header = readRenderItem<RenderItem_SectionMarker>(renderBufferChunk, &pos);
				DEBUG_BEGIN_RENDER_BUFFER(header->name, header->renderProfileName);
			} break;

			case RenderItemType_SetCamera:
			{
				DEBUG_BLOCK_T("render: RenderItemType_SetCamera", DCDT_Renderer);
				RenderItem_SetCamera *header = readRenderItem<RenderItem_SetCamera>(renderBufferChunk, &pos);
				currentCamera = header->camera;
			} break;

			case RenderItemType_SetShader:
			{
				DEBUG_BLOCK_T("render: RenderItemType_SetShader", DCDT_Renderer);
				RenderItem_SetShader *header = readRenderItem<RenderItem_SetShader>(renderBufferChunk, &pos);

				if (gl->vertexCount > 0)
				{
					flushVertices(gl);
				}

				activeShader = useShader(gl, header->shaderID);
				glUniformMatrix4fv(activeShader->uProjectionMatrixLoc, 1, false, currentCamera->projectionMatrix.flat);
				glUniform1f(activeShader->uScaleLoc, currentCamera->zoom);
			} break;

			case RenderItemType_SetTexture:
			{
				DEBUG_BLOCK_T("render: RenderItemType_SetTexture", DCDT_Renderer);
				RenderItem_SetTexture *header = readRenderItem<RenderItem_SetTexture>(renderBufferChunk, &pos);

				if (gl->vertexCount > 0)
				{
					flushVertices(gl);
				}

				ASSERT(header->texture != null); //Attempted to bind a null texture asset!
				ASSERT(header->texture->state == AssetState_Loaded);

				Texture *texture = &header->texture->texture;

				glActiveTexture(GL_TEXTURE0 + 0);
				glBindTexture(GL_TEXTURE_2D, texture->gl.glTextureID);

				if (!texture->gl.isLoaded)
				{
					// Load texture into GPU
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

					// Upload texture
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->surface->w, texture->surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->surface->pixels);
					texture->gl.isLoaded = true;
				}

				glUniform1i(activeShader->uTextureLoc, 0);
				gl->currentTexture = header->texture;
			} break;

			case RenderItemType_Clear:
			{
				DEBUG_BLOCK_T("render: RenderItemType_Clear", DCDT_Renderer);
				RenderItem_Clear *header = readRenderItem<RenderItem_Clear>(renderBufferChunk, &pos);

				glClearColor(header->clearColor.r, header->clearColor.g, header->clearColor.b, header->clearColor.a);
				glClear(GL_COLOR_BUFFER_BIT);
			} break;

			case RenderItemType_DrawRects:
			{
				DEBUG_BLOCK_T("render: RenderItemType_DrawRects", DCDT_Renderer);
				RenderItem_DrawRects *header = readRenderItem<RenderItem_DrawRects>(renderBufferChunk, &pos);
				
				for (s32 itemIndex = 0; itemIndex < header->count; itemIndex++)
				{
					RenderItem_DrawRects_Item *item = readRenderItem<RenderItem_DrawRects_Item>(renderBufferChunk, &pos);

					if (gl->vertexCount + 4 > RENDER_BATCH_VERTEX_COUNT)
					{
						flushVertices(gl);
					}
					renderQuad(gl, item->bounds, item->uv, item->color);
				}
			} break;

			case RenderItemType_DrawSingleRect:
			{
				DEBUG_BLOCK_T("render: RenderItemType_DrawSingleRect", DCDT_Renderer);
				RenderItem_DrawSingleRect *item = readRenderItem<RenderItem_DrawSingleRect>(renderBufferChunk, &pos);

				if (gl->vertexCount + 4 > RENDER_BATCH_VERTEX_COUNT)
				{
					flushVertices(gl);
				}
				renderQuad(gl, item->bounds, item->uv, item->color);

			} break;

			case RenderItemType_DrawGrid:
			{
				DEBUG_BLOCK_T("render: RenderItemType_DrawGrid", DCDT_Renderer);
				RenderItem_DrawGrid *header = readRenderItem<RenderItem_DrawGrid>(renderBufferChunk, &pos);

				u8 *gridData = (u8 *)(renderBufferChunk->memory + pos);
				pos += (header->gridW * header->gridH * sizeof(u8));

				V4 *paletteData = (V4 *)(renderBufferChunk->memory + pos);
				pos += (header->paletteSize * sizeof(V4));

				// For now, we'll do the easy/lazy thing of just outputting the grid as a series of quads.
				// We could probably send it as a texture or something, IDK.

				Rect2 bounds = rectXYWH(0.0f, 0.0f, 1.0f, 1.0f);
				f32 gridWf = (f32) header->gridW;
				f32 gridHf = (f32) header->gridH;
				if (!equals(header->bounds.w, gridWf, 0.01f))
				{
					bounds.w = header->bounds.w / gridWf;
				}
				if (!equals(header->bounds.h, gridHf, 0.01f))
				{
					bounds.h = header->bounds.h / gridHf;
				}

				Rect2 fakeUV = {};
				for (s32 y = 0; y < header->gridH; y++)
				{
					bounds.y = header->bounds.y + (y * bounds.h);
					for (s32 x = 0; x < header->gridW; x++, gridData++)
					{
						bounds.x = header->bounds.x + (x * bounds.w);

						if (gl->vertexCount + 4 > RENDER_BATCH_VERTEX_COUNT)
						{
							flushVertices(gl);
						}
						renderQuad(gl, bounds, fakeUV, paletteData[*gridData]);
					}
				}
			} break;

			INVALID_DEFAULT_CASE;
		}
	}

	if (gl->vertexCount > 0)
	{
		flushVertices(gl);
	}

	DEBUG_END_RENDER_BUFFER();
}

inline void renderQuad(GL_Renderer *gl, Rect2 bounds, Rect2 uv, V4 color)
{
	DEBUG_FUNCTION_T(DCDT_Renderer);
	s32 firstVertex = gl->vertexCount;

	GL_VertexData *vertex = gl->vertices + gl->vertexCount;

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
	vertex->color = color;
	vertex->uv.x = minU;
	vertex->uv.y = minV;
	vertex++;

	vertex->pos.x = maxX;
	vertex->pos.y = minY;
	vertex->color = color;
	vertex->uv.x = maxU;
	vertex->uv.y = minV;
	vertex++;

	vertex->pos.x = maxX;
	vertex->pos.y = maxY;
	vertex->color = color;
	vertex->uv.x = maxU;
	vertex->uv.y = maxV;
	vertex++;

	vertex->pos.x = minX;
	vertex->pos.y = maxY;
	vertex->color = color;
	vertex->uv.x = minU;
	vertex->uv.y = maxV;

	gl->vertexCount += 4;

	GLuint *index = gl->indices + gl->indexCount;
	index[0] = firstVertex + 0;
	index[1] = firstVertex + 1;
	index[2] = firstVertex + 2;
	index[3] = firstVertex + 0;
	index[4] = firstVertex + 2;
	index[5] = firstVertex + 3;
	gl->indexCount += 6;
}

void flushVertices(GL_Renderer *gl)
{
	DEBUG_FUNCTION_T(DCDT_Renderer);

	// Fill VBO
	ASSERT(gl->vertexCount <= RENDER_BATCH_VERTEX_COUNT); //Tried to render too many vertices at once!
	GLint vBufferSizeNeeded = gl->vertexCount * sizeof(gl->vertices[0]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vBufferSizeNeeded, gl->vertices);

	// Fill IBO
	ASSERT(gl->indexCount <= RENDER_BATCH_INDEX_COUNT); //Tried to render too many indices at once!
	GLint iBufferSizeNeeded = gl->indexCount * sizeof(gl->indices[0]);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, iBufferSizeNeeded, gl->indices);

	glDrawElements(GL_TRIANGLES, gl->indexCount, GL_UNSIGNED_INT, NULL);

	GL_ShaderProgram *activeShader = gl->shaders.items + gl->currentShader;
	DEBUG_DRAW_CALL(activeShader->asset->shortName, (gl->currentTexture == null) ? nullString : gl->currentTexture->shortName, (gl->vertexCount >> 2));

	gl->vertexCount = 0;
	gl->indexCount = 0;
}
