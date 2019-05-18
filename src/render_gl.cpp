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

static inline void GL_checkForError()
{
	GLenum errorCode = glGetError();
	ASSERT(errorCode == 0, "GL Error {0}: {1}", {formatInt(errorCode), makeString((char *)gluErrorString(errorCode))});
}

// Disable GL error checking for release version.
// #define GL_checkForError() 

static bool compileShader(GL_ShaderProgram *glShader, Shader *shaderProgram, GL_ShaderPart shaderPart)
{
	bool result = false;

	GLuint shaderID = glCreateShader(shaderPart);
	defer { glDeleteShader(shaderID); };

	String source   = nullString;
	String filename = nullString;
	switch (shaderPart)
	{
		case GL_ShaderPart_Vertex:
		{
			source = shaderProgram->vertexShader;
			filename = shaderProgram->vertexShaderFilename;
		} break;

		case GL_ShaderPart_Fragment:
		{
			source = shaderProgram->fragmentShader;
			filename = shaderProgram->fragmentShaderFilename;
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

		logError("Unable to compile shader {0}, \'{1}\'! ({2})", {formatInt(shaderID), filename, infoLog});
	}

	return result;
}

static void loadShaderAttrib(GL_ShaderProgram *glShader, char *attribName, int *attribLocation)
{
	*attribLocation = glGetAttribLocation(glShader->shaderProgramID, attribName);
	if (*attribLocation == -1)
	{
		logWarn("Shader #{0} does not contain requested variable {1}", {formatInt(glShader->type), makeString(attribName)});
	}
}

static void loadShaderUniform(GL_ShaderProgram *glShader, char *uniformName, int *uniformLocation)
{
	*uniformLocation = glGetUniformLocation(glShader->shaderProgramID, uniformName);
	if (*uniformLocation == -1)
	{
		logWarn("Shader #{0} does not contain requested uniform {1}", {formatInt(glShader->type), makeString(uniformName)});
	}
}

static void loadShaderProgram(Asset *asset, GL_ShaderProgram *glShader)
{
	glShader->shaderProgramID = glCreateProgram();

	if (glShader->shaderProgramID)
	{
		bool isVertexShaderCompiled   = compileShader(glShader, &asset->shader, GL_ShaderPart_Vertex);
		bool isFragmentShaderCompiled = compileShader(glShader, &asset->shader, GL_ShaderPart_Fragment);

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
	GL_Renderer *gl = (GL_Renderer *)renderer->platformRenderer;

	// Textures
	for (auto it = iterate(&assets->assetsByName[AssetType_Texture]);
		!it.isDone;
		next(&it))
	{
		Asset *asset = *get(it);
		Texture *texture = &asset->texture;

		glGenTextures(1, &texture->gl.glTextureID);
		texture->gl.isLoaded = false;
	}

	// Shaders
	for (auto it = iterate(&assets->assetsByName[AssetType_Shader]);
		!it.isDone;
		next(&it))
	{
		Asset *asset = *get(it);

		GL_ShaderProgram *shader = &gl->shaders[asset->shader.shaderType];
		shader->type = asset->shader.shaderType;
		shader->asset = asset;

		loadShaderProgram(asset, shader);
		ASSERT(shader->isValid, "Failed to load shader '{0}' into OpenGL.", {asset->shortName});
	}
}

static void GL_unloadAssets(Renderer *renderer, AssetManager *assets)
{
	GL_Renderer *gl = (GL_Renderer *)renderer->platformRenderer;

	// Textures
	for (auto it = iterate(&assets->assetsByName[AssetType_Texture]);
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
	for (u32 shaderID=0; shaderID < ShaderCount; shaderID++)
	{
		GL_ShaderProgram *shader = gl->shaders + shaderID;
		glDeleteProgram(shader->shaderProgramID);
		*shader = {};
	}
}

void useShader(GL_Renderer *renderer, ShaderType shaderType)
{
	DEBUG_FUNCTION();
	ASSERT(shaderType >= 0 && shaderType < ShaderCount, "Invalid shader!");

	if (renderer->currentShader != shaderType)
	{
		GL_ShaderProgram *targetShader = renderer->shaders + shaderType;
		if (targetShader->isValid)
		{
			glUseProgram(targetShader->shaderProgramID);
			renderer->currentShader = shaderType;
		}
		else
		{
			ASSERT(false, "Attempting to use a shader that isn't loaded!");
		}
	}
}

void bindTexture(Asset *asset, s32 uniformID, u32 textureSlot=0)
{
	DEBUG_FUNCTION();
	ASSERT(asset != null, "Attempted to bind a null texture asset!");

	Texture *texture = &asset->texture;

	glEnable(GL_TEXTURE_2D);
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
		ASSERT(asset->state == AssetState_Loaded, "Attempted to bind an unloaded texture '{0}'!", {asset->shortName});
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->surface->w, texture->surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->surface->pixels);
		texture->gl.isLoaded = true;
		GL_checkForError();
	}

	glUniform1i(uniformID, textureSlot);
}

void renderPartOfBuffer(GL_Renderer *renderer, u32 vertexCount, u32 indexCount)
{
	DEBUG_FUNCTION();
	GL_ShaderProgram *activeShader = renderer->shaders + renderer->currentShader;

	// Fill VBO
	glBindBuffer(GL_ARRAY_BUFFER, renderer->VBO);
	ASSERT(vertexCount <= RENDER_BATCH_VERTEX_COUNT, "Tried to render too many vertices at once!");
	GLint vBufferSizeNeeded = vertexCount * sizeof(renderer->vertices[0]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vBufferSizeNeeded, renderer->vertices);
	GL_checkForError();

	// Fill IBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->IBO);
	ASSERT(indexCount <= RENDER_BATCH_INDEX_COUNT, "Tried to render too many indices at once!");
	GLint iBufferSizeNeeded = indexCount * sizeof(renderer->indices[0]);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, iBufferSizeNeeded, renderer->indices);
	GL_checkForError();

	glEnableVertexAttribArray(activeShader->aPositionLoc);
	glEnableVertexAttribArray(activeShader->aColorLoc);
	GL_checkForError();

	if (activeShader->aUVLoc != -1)
	{
		glEnableVertexAttribArray(activeShader->aUVLoc);
		GL_checkForError();
	}

	glBindBuffer(GL_ARRAY_BUFFER, renderer->VBO);
	GL_checkForError();
	glVertexAttribPointer(activeShader->aPositionLoc, 3, GL_FLOAT, GL_FALSE, sizeof(GL_VertexData), (GLvoid*)offsetof(GL_VertexData, pos));
	GL_checkForError();
	glVertexAttribPointer(activeShader->aColorLoc,    4, GL_FLOAT, GL_FALSE, sizeof(GL_VertexData), (GLvoid*)offsetof(GL_VertexData, color));
	GL_checkForError();

	if (activeShader->aUVLoc != -1)
	{
		glVertexAttribPointer(activeShader->aUVLoc,   2, GL_FLOAT, GL_FALSE, sizeof(GL_VertexData), (GLvoid*)offsetof(GL_VertexData, uv));
		GL_checkForError();
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->IBO);
	GL_checkForError();
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, NULL);
	GL_checkForError();

	glDisableVertexAttribArray(activeShader->aPositionLoc);
	glDisableVertexAttribArray(activeShader->aColorLoc);
	if (activeShader->aUVLoc != -1)
	{
		glDisableVertexAttribArray(activeShader->aUVLoc);
	}
	GL_checkForError();
}

static void renderBuffer(GL_Renderer *renderer, RenderBuffer *buffer)
{
	DEBUG_FUNCTION();

	// Fill VBO
	u32 vertexCount = 0;
	u32 indexCount = 0;
	u32 drawCallCount = 0;

	if (buffer->items.count > 0)
	{
		Asset *texture = null;

		for (s32 i=0; i < buffer->items.count; i++)
		{
			RenderItem *item = pointerTo(&buffer->items, i);

			bool shaderChanged = (item->shaderID != renderer->currentShader);
			bool textureChanged = (item->texture != texture);

			// Check to see if we need to start a new batch.
			if ((i == 0) || shaderChanged || textureChanged || (vertexCount == RENDER_BATCH_VERTEX_COUNT))
			{
				// Render existing buffer contents
				if (vertexCount > 0)
				{
					drawCallCount++;
					renderPartOfBuffer(renderer, vertexCount, indexCount);
				}

				{
					DEBUG_BLOCK("Start new batch");
					useShader(renderer, item->shaderID);
					GL_ShaderProgram *activeShader = renderer->shaders + renderer->currentShader;

					if (i==0 || shaderChanged)
					{
						glUniformMatrix4fv(activeShader->uProjectionMatrixLoc, 1, false, buffer->camera.projectionMatrix.flat);

						glUniform1f(activeShader->uScaleLoc, buffer->camera.zoom);
					}

					texture = item->texture; // We need this set whether we bind the texture or not, otherwise it never gets nulled and we use a separate batch for every zone tile, for example!
					// Bind new texture if this shader uses textures

					if ((i == 0 || textureChanged || shaderChanged)
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
	}

	DEBUG_RENDER_BUFFER(buffer, drawCallCount);

	clear(&buffer->items);
}

static void GL_render(Renderer *renderer)
{
	DEBUG_FUNCTION();

	GL_Renderer *gl = (GL_Renderer *)renderer->platformRenderer;
	
	// Sort sprites
	// {
	// 	DEBUG_BLOCK("SORT WORLD BUFFER");
	// 	sortRenderBuffer(&renderer->worldBuffer);
	// }
	{
		DEBUG_BLOCK("SORT UI BUFFER");
		sortRenderBuffer(&renderer->uiBuffer);
	}

#if CHECK_BUFFERS_SORTED
	// Check buffers are sorted
	ASSERT(isBufferSorted(&renderer->worldBuffer), "World sprites are out of order!");
	ASSERT(isBufferSorted(&renderer->uiBuffer), "UI sprites are out of order!");
#endif

	glClear(GL_COLOR_BUFFER_BIT);
	GL_checkForError();
	glEnable(GL_BLEND);
	GL_checkForError();
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	GL_checkForError();

	glEnable(GL_TEXTURE_2D);
	GL_checkForError();

	gl->currentShader = Shader_Invalid;

	{
		// DEBUG_BLOCK("DRAW WORLD BUFFER");
		renderBuffer(gl, &renderer->worldBuffer);
	}
	{
		// DEBUG_BLOCK("DRAW UI BUFFER");
		renderBuffer(gl, &renderer->uiBuffer);
	}

	glUseProgram(NULL);
	GL_checkForError();
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

		// VSync
		if (succeeded && SDL_GL_SetSwapInterval(1) < 0)
		{
			logCritical("Could not set vsync! :(\n %s", {makeString(SDL_GetError())});
			succeeded = false;
		}

		// Init OpenGL
		if (succeeded)
		{
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

			glGenBuffers(1, &gl->VBO);
			glBindBuffer(GL_ARRAY_BUFFER, gl->VBO);
			GLint vBufferSizeNeeded = RENDER_BATCH_VERTEX_COUNT * sizeof(gl->vertices[0]);
			glBufferData(GL_ARRAY_BUFFER, vBufferSizeNeeded, null, GL_STATIC_DRAW);

			glGenBuffers(1, &gl->IBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl->IBO);
			GLint iBufferSizeNeeded = RENDER_BATCH_INDEX_COUNT * sizeof(gl->indices[0]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, iBufferSizeNeeded, null, GL_STATIC_DRAW);

			GL_checkForError();
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