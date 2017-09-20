// render_gl.cpp

void GL_freeRenderer(Renderer *renderer)
{
	GL_Renderer *gl = (GL_Renderer *)renderer->platformRenderer;
	SDL_GL_DeleteContext(gl->context);
}

void GL_windowResized(int32 newWidth, int32 newHeight)
{
	glViewport(0, 0, newWidth, newHeight);
}

static void logGLError(GLenum errorCode)
{
	if (errorCode != GL_NO_ERROR)
	{
		logError("{0}", {stringFromChars((char*)gluErrorString(errorCode))});
	}
}

static void checkForError()
{
	GLenum errorCode = glGetError();
	ASSERT(errorCode == 0, "GL Error %d: %s", errorCode, gluErrorString(errorCode));
}

static bool compileShader(GL_ShaderProgram *shaderProgram, GL_ShaderType shaderType, ShaderProgram *shaderAsset)
{
	bool result = false;
	String *shaderSource = null;
	String filename = {};

	switch (shaderType)
	{
		case GL_ShaderType_Fragment:
		{
			shaderSource = &shaderAsset->fragShader;
			filename = shaderAsset->fragFilename;
		} break;
		case GL_ShaderType_Vertex:
		{
			shaderSource = &shaderAsset->vertShader;
			filename = shaderAsset->vertFilename;
		} break;

		INVALID_DEFAULT_CASE;
	}

	ASSERT(shaderSource && filename.length, "Failed to select a shader!");

	GLuint shaderID = glCreateShader(shaderType);
	DEFER(glDeleteShader(shaderID));

	glShaderSource(shaderID, 1, &shaderSource->chars, &shaderSource->length);
	glCompileShader(shaderID);

	GLint isCompiled = GL_FALSE;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &isCompiled);
	result = (isCompiled == GL_TRUE);

	if (result)
	{
		glAttachShader(shaderProgram->shaderProgramID, shaderID);
	}
	else
	{
		// Print a big error message about this!
		int logMaxLength = 0;
		
		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logMaxLength);
		String infoLog = newString(globalFrameTempArena, logMaxLength);
		glGetShaderInfoLog(shaderID, logMaxLength, &infoLog.length, infoLog.chars);

		if (infoLog.length == 0)
		{
			infoLog = stringFromChars("No error log provided by OpenGL. Sad panda.");
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
		logWarn("Shader #{0} does not contain requested variable {1}", {formatInt(glShader->shaderProgramID), stringFromChars(attribName)});
	}
}

static void loadShaderUniform(GL_ShaderProgram *glShader, char *uniformName, int *uniformLocation)
{
	*uniformLocation = glGetUniformLocation(glShader->shaderProgramID, uniformName);
	if (*uniformLocation == -1)
	{
		logWarn("Shader #{0} does not contain requested uniform {1}", {formatInt(glShader->shaderProgramID), stringFromChars(uniformName)});
	}
}

static bool loadShaderProgram(GL_Renderer *renderer, AssetManager *assets, ShaderProgramType shaderProgramID)
{
	bool result = false;

	ShaderProgram *shaderAsset = getShaderProgram(assets, shaderProgramID);
	ASSERT(shaderAsset->state == AssetState_Loaded, "Shader asset %d not loaded!", shaderProgramID);

	GL_ShaderProgram *glShader = renderer->shaders + shaderProgramID;
	glShader->assetID = shaderProgramID;

	bool isVertexShaderCompiled = GL_FALSE;
	bool isFragmentShaderCompiled = GL_FALSE;

	glShader->shaderProgramID = glCreateProgram();

	if (glShader->shaderProgramID)
	{
		// VERTEX SHADER
		isVertexShaderCompiled = compileShader(glShader, GL_ShaderType_Vertex, shaderAsset);
		isFragmentShaderCompiled = compileShader(glShader, GL_ShaderType_Fragment, shaderAsset);

		// Link shader program
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
				String infoLog = newString(globalFrameTempArena, logMaxLength);
				glGetProgramInfoLog(glShader->shaderProgramID, logMaxLength, &infoLog.length, infoLog.chars);

				if (infoLog.length == 0)
				{
					infoLog = stringFromChars("No error log provided by OpenGL. Sad panda.");
				}

				logError("Unable to link shader program {0}! ({1})", {formatInt(glShader->shaderProgramID), infoLog});
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
			}
		}
		result = glShader->isValid;
	}
	else
	{
		logGLError(glGetError());
	}

	return result;
}

static void GL_loadAssets(Renderer *renderer, AssetManager *assets)
{
	GL_Renderer *gl = (GL_Renderer *)renderer->platformRenderer;

	// Textures
	gl->textureCount = assets->textureCount;
	for (uint32 i=0; i<gl->textureCount; i++)
	{
		if (i == 0)
		{
			gl->textureInfo[i].glTextureID = 0;
			gl->textureInfo[i].isLoaded = true;
		}
		else
		{
			glGenTextures(1, &gl->textureInfo[i].glTextureID);
			gl->textureInfo[i].isLoaded = false;
		}
	}

	// Shaders
	for (uint32 shaderID=0; shaderID < ShaderProgramCount; shaderID++)
	{
		bool shaderLoaded = loadShaderProgram(gl, assets, (ShaderProgramType)shaderID);
		ASSERT(shaderLoaded, "Failed to load shader %d into OpenGL.", shaderID);
	}
}

static void GL_unloadAssets(Renderer *renderer)
{
	GL_Renderer *gl = (GL_Renderer *)renderer->platformRenderer;

	// Textures
	for (uint32 i=1; i<gl->textureCount; i++)
	{
		GL_TextureInfo *info = gl->textureInfo + i;
		if (info->isLoaded)
		{
			glDeleteTextures(1, &info->glTextureID);
			info->glTextureID = 0;
			info->isLoaded = false;
		}
	}
	gl->textureCount = 0;

	// Shaders
	gl->currentShader = -1;
	for (uint32 shaderID=0; shaderID < ShaderProgramCount; shaderID++)
	{
		GL_ShaderProgram *shader = gl->shaders + shaderID;
		glDeleteProgram(shader->shaderProgramID);
		*shader = {};
	}
}

static GL_ShaderProgram *getActiveShader(GL_Renderer *renderer)
{
	ASSERT(renderer->currentShader >= 0 && renderer->currentShader < ShaderProgramCount, "Invalid shader!");
	GL_ShaderProgram *activeShader = renderer->shaders + renderer->currentShader;
	ASSERT(activeShader->isValid, "Shader not properly loaded!");

	return activeShader;
}

static void renderPartOfBuffer(GL_Renderer *renderer, uint32 vertexCount, uint32 indexCount)
{
	GL_ShaderProgram *activeShader = getActiveShader(renderer);

	glBindBuffer(GL_ARRAY_BUFFER, renderer->VBO);
	glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(GL_VertexData), renderer->vertices, GL_STATIC_DRAW);

	// Fill IBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(GLuint), renderer->indices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(activeShader->aPositionLoc);
	glEnableVertexAttribArray(activeShader->aColorLoc);

	if (activeShader->aUVLoc != -1)
	{
		glEnableVertexAttribArray(activeShader->aUVLoc);
	}

	glBindBuffer(GL_ARRAY_BUFFER, renderer->VBO);
	glVertexAttribPointer(activeShader->aPositionLoc, 3, GL_FLOAT, GL_FALSE, sizeof(GL_VertexData), (GLvoid*)offsetof(GL_VertexData, pos));
	glVertexAttribPointer(activeShader->aColorLoc,    4, GL_FLOAT, GL_FALSE, sizeof(GL_VertexData), (GLvoid*)offsetof(GL_VertexData, color));
	if (activeShader->aUVLoc != -1)
	{
		glVertexAttribPointer(activeShader->aUVLoc,   2, GL_FLOAT, GL_FALSE, sizeof(GL_VertexData), (GLvoid*)offsetof(GL_VertexData, uv));
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->IBO);
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, NULL);

	glDisableVertexAttribArray(activeShader->aPositionLoc);
	glDisableVertexAttribArray(activeShader->aColorLoc);
	if (activeShader->aUVLoc != -1)
	{
		glDisableVertexAttribArray(activeShader->aUVLoc);
	}
}

static ShaderProgramType getDesiredShader(RenderItem *item)
{
	ShaderProgramType result = ShaderProgram_Untextured;
	if (item->textureRegionID != 0)
	{
		result = ShaderProgram_Textured;
	}

	return result;
}

static void renderBuffer(GL_Renderer *renderer, AssetManager *assets, RenderBuffer *buffer)
{
	DEBUG_FUNCTION();
	// Fill VBO
	uint32 vertexCount = 0;
	uint32 indexCount = 0;
	GLuint glBoundTextureID = 0; // 0 = none

	uint32 drawCallCount = 0;

	if (buffer->itemCount > 0)
	{
		for (uint32 i=0; i < buffer->itemCount; i++)
		{
			RenderItem *item = buffer->items + i;
			ShaderProgramType desiredShader = getDesiredShader(item);

			TextureRegion *region = getTextureRegion(assets, item->textureRegionID);
			GL_TextureInfo *textureInfo = renderer->textureInfo + region->textureID;

			if ((textureInfo->glTextureID != glBoundTextureID)
				|| (desiredShader != renderer->currentShader))
			{
				// Render existing buffer contents
				if (vertexCount)
				{
					drawCallCount++;
					renderPartOfBuffer(renderer, vertexCount, indexCount);
				}

				renderer->currentShader = desiredShader;
				GL_ShaderProgram *activeShader = getActiveShader(renderer);

				glUseProgram(activeShader->shaderProgramID);
				checkForError();

				glUniformMatrix4fv(activeShader->uProjectionMatrixLoc, 1, false, buffer->camera.projectionMatrix.flat);
				checkForError();

				// Bind new texture if this shader uses textures
				if (activeShader->uTextureLoc != -1)
				{
					glEnable(GL_TEXTURE_2D);
					glActiveTexture(GL_TEXTURE0);
					checkForError();
					glBindTexture(GL_TEXTURE_2D, textureInfo->glTextureID);
					checkForError();

					if (!textureInfo->isLoaded)
					{
						// Load texture into GPU
	#if 0
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	#else
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	#endif

						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

						// Upload texture
						Texture *texture = getTexture(assets, region->textureID);
						ASSERT(texture->state == AssetState_Loaded, "Texture asset not loaded yet!");
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->surface->w, texture->surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->surface->pixels);
						textureInfo->isLoaded = true;
						checkForError();
					}

					glUniform1i(activeShader->uTextureLoc, 0);
					checkForError();
				}

				vertexCount = 0;
				indexCount = 0;
				glBoundTextureID = textureInfo->glTextureID;
			}

			int firstVertex = vertexCount;

			renderer->vertices[vertexCount++] = {
				v3( item->rect.x, item->rect.y, item->depth),
				item->color,
				v2(region->uv.x, region->uv.y)
			};
			renderer->vertices[vertexCount++] = {
				v3( item->rect.x + item->rect.size.x, item->rect.y, item->depth),
				item->color,
				v2(region->uv.x + region->uv.w, region->uv.y)
			};
			renderer->vertices[vertexCount++] = {
				v3( item->rect.x + item->rect.size.x, item->rect.y + item->rect.size.y, item->depth),
				item->color,
				v2(region->uv.x + region->uv.w, region->uv.y + region->uv.h)
			};
			renderer->vertices[vertexCount++] = {
				v3( item->rect.x, item->rect.y + item->rect.size.y, item->depth),
				item->color,
				v2(region->uv.x, region->uv.y + region->uv.h)
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

	buffer->itemCount = 0;
}

static void sortRenderBuffer(RenderBuffer *buffer)
{
	DEBUG_FUNCTION();
	// This is an implementation of the 'comb sort' algorithm, low to high

	uint32 gap = buffer->itemCount;
	real32 shrink = 1.3f;

	bool swapped = false;

	while (gap > 1 || swapped)
	{
		gap = (uint32)((real32)gap / shrink);
		if (gap < 1)
		{
			gap = 1;
		}

		swapped = false;

		// "comb" over the list
		for (uint32 i = 0;
			i + gap < buffer->itemCount; // Here lies the remains of the flicker bug. It was <= not <. /fp
			i++)
		{
			if (buffer->items[i].depth > buffer->items[i+gap].depth)
			{
				RenderItem temp = buffer->items[i];
				buffer->items[i] = buffer->items[i+gap];
				buffer->items[i+gap] = temp;

				swapped = true;
			}
		}
	}
}

#if CHECK_BUFFERS_SORTED
static bool isBufferSorted(RenderBuffer *buffer)
{
	bool isSorted = true;
	real32 lastDepth = real32Min;
	for (uint32 i=0; i<=buffer->itemCount; i++)
	{
		if (lastDepth > buffer->items[i].depth)
		{
			isSorted = false;
			break;
		}
		lastDepth = buffer->items[i].depth;
	}
	return isSorted;
}
#endif

static void GL_render(Renderer *renderer, AssetManager *assets)
{
	DEBUG_FUNCTION();

	GL_Renderer *gl = (GL_Renderer *)renderer->platformRenderer;
	
	// Sort sprites
	sortRenderBuffer(&renderer->worldBuffer);
	sortRenderBuffer(&renderer->uiBuffer);

#if CHECK_BUFFERS_SORTED
	// Check buffers are sorted
	ASSERT(isBufferSorted(&renderer->worldBuffer), "World sprites are out of order!");
	ASSERT(isBufferSorted(&renderer->uiBuffer), "UI sprites are out of order!");
#endif

	glClear(GL_COLOR_BUFFER_BIT);
	checkForError();
	glEnable(GL_BLEND);
	checkForError();
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	checkForError();

	glEnable(GL_TEXTURE_2D);
	checkForError();

	gl->currentShader = ShaderProgram_Invalid;

	renderBuffer(gl, assets, &renderer->worldBuffer);
	renderBuffer(gl, assets, &renderer->uiBuffer);

	glUseProgram(NULL);
	checkForError();
}

Renderer *GL_initializeRenderer(SDL_Window *window)
{
	GL_Renderer *gl;
	bootstrapArena(GL_Renderer, gl, renderer.renderArena);
	bool succeeded = (gl != 0);
	Renderer *renderer = &gl->renderer;

	if (succeeded)
	{
		initRenderer(renderer, &renderer->renderArena, window);

		renderer->platformRenderer = gl;
		renderer->windowResized = &GL_windowResized;
		renderer->render = &GL_render;
		renderer->loadAssets = &GL_loadAssets;
		renderer->unloadAssets = &GL_unloadAssets;
		renderer->free = &GL_freeRenderer;

		// Use GL3.1 Core
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		// Create context
		gl->context = SDL_GL_CreateContext(renderer->window);
		if (gl->context == NULL)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "OpenGL context could not be created! :(\n %s", SDL_GetError());
			succeeded = false;
		}

		// GLEW
		glewExperimental = GL_TRUE;
		GLenum glewError = glewInit();
		if (succeeded && glewError != GLEW_OK)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not initialise GLEW! :(\n %s", glewGetErrorString(glewError));
			succeeded = false;
		}

		// VSync
		if (succeeded && SDL_GL_SetSwapInterval(1) < 0)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not set vsync! :(\n %s", SDL_GetError());
			succeeded = false;
		}

		// Init OpenGL
		if (succeeded)
		{
			if (succeeded)
			{
				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

				glGenBuffers(1, &gl->VBO);
				glGenBuffers(1, &gl->IBO);

				checkForError();
			}

			if (!succeeded)
			{
				SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not initialise OpenGL! :(");
			}
		}

		if (!succeeded)
		{
			GL_freeRenderer(renderer);
			gl = 0;
		}
	}

	return renderer;
}