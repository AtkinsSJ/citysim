// render_gl.cpp

void GL_freeRenderer(Renderer *renderer)
{
	GL_Renderer *gl = (GL_Renderer *)renderer->platformRenderer;
	SDL_GL_DeleteContext(gl->context);
}

void GL_windowResized(s32 newWidth, s32 newHeight)
{
	glViewport(0, 0, newWidth, newHeight);
}

static inline void logGLError(GLenum errorCode)
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


static bool compileShader(GL_ShaderProgram *glShader, String shaderName, Shader *shaderProgram, GL_ShaderPart shaderPart)
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

static void loadShaderAttrib(GL_ShaderProgram *glShader, char *attribName, int *attribLocation)
{
	DEBUG_FUNCTION_T(DCDT_Renderer);
	
	*attribLocation = glGetAttribLocation(glShader->shaderProgramID, attribName);
	if (*attribLocation == -1)
	{
		logWarn("Shader '{0}' does not contain requested variable '{1}'", {glShader->asset->shortName, makeString(attribName)});
	}
}

static void loadShaderUniform(GL_ShaderProgram *glShader, char *uniformName, int *uniformLocation)
{
	DEBUG_FUNCTION_T(DCDT_Renderer);
	
	*uniformLocation = glGetUniformLocation(glShader->shaderProgramID, uniformName);
	if (*uniformLocation == -1)
	{
		logWarn("Shader '{0}' does not contain requested uniform '{1}'", {glShader->asset->shortName, makeString(uniformName)});
	}
}

static void loadShaderProgram(Asset *asset, GL_ShaderProgram *glShader)
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

static void GL_loadAssets(Renderer *renderer, AssetManager *assets)
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
	for (auto it = iterate(&assets->assetsByType[AssetType_Shader]);
		!it.isDone;
		next(&it))
	{
		Asset *asset = *get(it);

		s32 shaderIndex = gl->shaders.count;
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

static void GL_unloadAssets(Renderer *renderer, AssetManager *assets)
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
	for (s32 shaderID=0; shaderID < gl->shaders.count; shaderID++)
	{
		GL_ShaderProgram *shader = pointerTo(&gl->shaders, shaderID);
		glDeleteProgram(shader->shaderProgramID);
		*shader = {};
	}
	clear(&gl->shaders);
}

inline GL_ShaderProgram *useShader(GL_Renderer *renderer, s32 shaderID)
{
	DEBUG_FUNCTION_T(DCDT_Renderer);
	ASSERT(shaderID >= 0 && shaderID < renderer->shaders.count); //Invalid shader!


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

void bindTexture(Asset *asset, s32 uniformID, u32 textureSlot=0)
{
	DEBUG_FUNCTION_T(DCDT_Renderer);
	ASSERT(asset != null); //Attempted to bind a null texture asset!
	ASSERT(asset->state == AssetState_Loaded);

	Texture *texture = &asset->texture;

	glActiveTexture(GL_TEXTURE0 + textureSlot);
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

	glUniform1i(uniformID, textureSlot);
}

void renderPartOfBuffer(GL_Renderer *renderer, u32 vertexCount, u32 indexCount)
{
	DEBUG_FUNCTION_T(DCDT_Renderer);

	// Fill VBO
	ASSERT(vertexCount <= RENDER_BATCH_VERTEX_COUNT); //Tried to render too many vertices at once!
	GLint vBufferSizeNeeded = vertexCount * sizeof(renderer->vertices[0]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vBufferSizeNeeded, renderer->vertices);

	// Fill IBO
	ASSERT(indexCount <= RENDER_BATCH_INDEX_COUNT); //Tried to render too many indices at once!
	GLint iBufferSizeNeeded = indexCount * sizeof(renderer->indices[0]);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, iBufferSizeNeeded, renderer->indices);

	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, NULL);
}

static void renderBuffer(GL_Renderer *renderer, RenderBuffer *buffer)
{
	DEBUG_FUNCTION_T(DCDT_Renderer);

	DEBUG_BEGIN_RENDER_BUFFER(buffer);

	u32 drawCallCount = 0;

	smm pos = 0;
	if (buffer->data.used > 0)
	{
		while (pos < buffer->data.used)
		{
			RenderItemType itemType = *((RenderItemType *)(buffer->data.memory + pos));
			pos += sizeof(RenderItemType);

			switch (itemType)
			{
				case RenderItemType_NextMemoryChunk:
				{
					// TODO: Unimplemented
				} break;

				case RenderItemType_DrawThing:
				{
					RenderItem_DrawThing *item = (RenderItem_DrawThing *)(buffer->data.memory + pos);
					pos += sizeof(RenderItem_DrawThing);

					GL_ShaderProgram *activeShader = useShader(renderer, item->shaderID);

					glUniformMatrix4fv(activeShader->uProjectionMatrixLoc, 1, false, buffer->camera.projectionMatrix.flat);
					glUniform1f(activeShader->uScaleLoc, buffer->camera.zoom);

					if ((activeShader->uTextureLoc != -1) && (item->texture != null))
					{
						bindTexture(item->texture, activeShader->uTextureLoc, 0);
					}

					u32 vertexCount = 0;
					u32 indexCount = 0;

					u32 firstVertex = vertexCount;
					renderer->vertices[vertexCount++] = {
						v3(item->rect.x, item->rect.y, item->depth),
						item->color,
						v2(item->uv.x, item->uv.y)
					};
					renderer->vertices[vertexCount++] = {
						v3(item->rect.x + item->rect.size.x, item->rect.y, item->depth),
						item->color,
						v2(item->uv.x + item->uv.w, item->uv.y)
					};
					renderer->vertices[vertexCount++] = {
						v3(item->rect.x + item->rect.size.x, item->rect.y + item->rect.size.y, item->depth),
						item->color,
						v2(item->uv.x + item->uv.w, item->uv.y + item->uv.h)
					};
					renderer->vertices[vertexCount++] = {
						v3(item->rect.x, item->rect.y + item->rect.size.y, item->depth),
						item->color,
						v2(item->uv.x, item->uv.y + item->uv.h)
					};

					renderer->indices[indexCount++] = firstVertex + 0;
					renderer->indices[indexCount++] = firstVertex + 1;
					renderer->indices[indexCount++] = firstVertex + 2;
					renderer->indices[indexCount++] = firstVertex + 0;
					renderer->indices[indexCount++] = firstVertex + 2;
					renderer->indices[indexCount++] = firstVertex + 3;

					drawCallCount++;
					renderPartOfBuffer(renderer, vertexCount, indexCount);
					DEBUG_DRAW_CALL(activeShader->asset->shortName, (item->texture == null) ? nullString : item->texture->shortName,(vertexCount >> 2));
				} break;

				INVALID_DEFAULT_CASE;
			}
		}

		// TODO: Properly clear this once we have multiple chunks!
		buffer->data.used = 0;
	}
	#if 0
		// Initialise stuff using the first item...
		RenderItem_DrawThing *firstItem = buffer->items.items + 0;
		GL_ShaderProgram *activeShader = useShader(renderer, firstItem->shaderID);

		glUniformMatrix4fv(activeShader->uProjectionMatrixLoc, 1, false, buffer->camera.projectionMatrix.flat);
		glUniform1f(activeShader->uScaleLoc, buffer->camera.zoom);

		Asset *texture = firstItem->texture;
		if ((activeShader->uTextureLoc != -1) && (texture != null))
		{
			bindTexture(texture, activeShader->uTextureLoc, 0);
		}

		// ...but we still draw the first item in this loop!
		for (s32 i=0; i < buffer->items.count; i++)
		{
			RenderItem_DrawThing *item = buffer->items.items + i;

			bool shaderChanged = (item->shaderID != renderer->currentShader);
			bool textureChanged = (item->texture != texture);

			// Check to see if we need to start a new batch.
			if (shaderChanged || textureChanged || (vertexCount == RENDER_BATCH_VERTEX_COUNT))
			{
				// Render existing buffer contents
				drawCallCount++;
				renderPartOfBuffer(renderer, vertexCount, indexCount);
				DEBUG_DRAW_CALL(activeShader->asset->shortName, (texture == null) ? nullString : texture->shortName, (vertexCount >> 2));

				{
					DEBUG_BLOCK_T("Start new batch", DCDT_Renderer);

					if (shaderChanged)
					{
						activeShader = useShader(renderer, item->shaderID);

						glUniformMatrix4fv(activeShader->uProjectionMatrixLoc, 1, false, buffer->camera.projectionMatrix.flat);
						glUniform1f(activeShader->uScaleLoc, buffer->camera.zoom);
					}

					texture = item->texture; // We need this set whether we bind the texture or not, otherwise it never gets nulled and we use a separate batch for every zone tile, for example!
					// Bind new texture if this shader uses textures

					if ((textureChanged || shaderChanged)
						&& (activeShader->uTextureLoc != -1)
						&& (texture != null))
					{
						bindTexture(texture, activeShader->uTextureLoc, 0);
					}

					vertexCount = 0;
					indexCount = 0;
				}
			}

			u32 firstVertex = vertexCount;
			renderer->vertices[vertexCount++] = {
				v3(item->rect.x, item->rect.y, item->depth),
				item->color,
				v2(item->uv.x, item->uv.y)
			};
			renderer->vertices[vertexCount++] = {
				v3(item->rect.x + item->rect.size.x, item->rect.y, item->depth),
				item->color,
				v2(item->uv.x + item->uv.w, item->uv.y)
			};
			renderer->vertices[vertexCount++] = {
				v3(item->rect.x + item->rect.size.x, item->rect.y + item->rect.size.y, item->depth),
				item->color,
				v2(item->uv.x + item->uv.w, item->uv.y + item->uv.h)
			};
			renderer->vertices[vertexCount++] = {
				v3(item->rect.x, item->rect.y + item->rect.size.y, item->depth),
				item->color,
				v2(item->uv.x, item->uv.y + item->uv.h)
			};

			renderer->indices[indexCount++] = firstVertex + 0;
			renderer->indices[indexCount++] = firstVertex + 1;
			renderer->indices[indexCount++] = firstVertex + 2;
			renderer->indices[indexCount++] = firstVertex + 0;
			renderer->indices[indexCount++] = firstVertex + 2;
			renderer->indices[indexCount++] = firstVertex + 3;
		}

		// Do one final draw for remaining items
		drawCallCount++;
		renderPartOfBuffer(renderer, vertexCount, indexCount);
		DEBUG_DRAW_CALL(activeShader->asset->shortName, (texture == null) ? nullString : texture->shortName,(vertexCount >> 2));
	}

	clear(&buffer->items);
	#endif
}

static void GL_render(Renderer *renderer)
{
	DEBUG_FUNCTION_T(DCDT_Renderer);

	GL_Renderer *gl = (GL_Renderer *)renderer->platformRenderer;
	
	// Sort sprites
	// {
	// 	DEBUG_BLOCK_T("SORT WORLD BUFFER", DCDT_Renderer);
	// 	sortRenderBuffer(&renderer->worldBuffer);
	// }
	// {
	// 	DEBUG_BLOCK_T("sortRenderBuffer(ui)", DCDT_Renderer);
	// 	sortRenderBuffer(&renderer->uiBuffer);
	// }

#if CHECK_BUFFERS_SORTED
	// Check buffers are sorted
	ASSERT(isBufferSorted(&renderer->worldBuffer)); //World sprites are out of order!
	ASSERT(isBufferSorted(&renderer->uiBuffer)); //UI sprites are out of order!
#endif

	glClear(GL_COLOR_BUFFER_BIT);

	{
		DEBUG_BLOCK_T("renderBuffer(world)", DCDT_Renderer);
		renderBuffer(gl, &renderer->worldBuffer);
	}
	{
		DEBUG_BLOCK_T("renderBuffer(ui)", DCDT_Renderer);
		renderBuffer(gl, &renderer->uiBuffer);
	}
}

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

			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

			glGenBuffers(1, &gl->VBO);
			glBindBuffer(GL_ARRAY_BUFFER, gl->VBO);
			GLint vBufferSizeNeeded = RENDER_BATCH_VERTEX_COUNT * sizeof(gl->vertices[0]);
			glBufferData(GL_ARRAY_BUFFER, vBufferSizeNeeded, null, GL_STATIC_DRAW);

			glGenBuffers(1, &gl->IBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl->IBO);
			GLint iBufferSizeNeeded = RENDER_BATCH_INDEX_COUNT * sizeof(gl->indices[0]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, iBufferSizeNeeded, null, GL_STATIC_DRAW);

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
