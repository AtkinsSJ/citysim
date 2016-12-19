// render_gl.cpp

void GL_freeRenderer(GL_Renderer *renderer)
{
	SDL_GL_DeleteContext(renderer->context);
}

void GL_windowResized(int32 newWidth, int32 newHeight)
{
	glViewport(0, 0, newWidth, newHeight);
}

void GL_printProgramLog(TemporaryMemory *tempMemory, GLuint program)
{
	//Make sure name is shader
	if( glIsProgram( program ) )
	{
		//Program log length
		int infoLogLength = 0;
		int maxLength = 0;
		
		//Get info string length
		glGetProgramiv( program, GL_INFO_LOG_LENGTH, &maxLength );
		
		//Allocate string
		char* infoLog = PushArray(tempMemory, char, maxLength);
		
		//Get info log
		glGetProgramInfoLog( program, maxLength, &infoLogLength, infoLog );
		if( infoLogLength > 0 )
		{
			//Print Log
			SDL_Log( "%s\n", infoLog );
		}
	}
	else
	{
		SDL_Log( "Name %d is not a program\n", program );
	}
}

void GL_printShaderLog(TemporaryMemory *tempMemory, GLuint shader)
{
	//Make sure name is shader
	if( glIsShader( shader ) )
	{
		//Shader log length
		int infoLogLength = 0;
		int maxLength = 0;
		
		//Get info string length
		glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &maxLength );
		
		//Allocate string
		char* infoLog = PushArray(tempMemory, char, maxLength);
		
		//Get info log
		glGetShaderInfoLog( shader, maxLength, &infoLogLength, infoLog );
		if( infoLogLength > 0 )
		{
			//Print Log
			SDL_Log( "%s\n", infoLog );
		}
	}
	else
	{
		SDL_Log( "Name %d is not a shader\n", shader );
	}
}

bool GL_compileShader(GL_Renderer *renderer, GL_ShaderProgram *shaderProgram, GL_ShaderType shaderType,
	                  ShaderProgram *shaderAsset)
{
	bool result = false;
	char **shaderSource = 0;
	char *filename = 0;

	switch (shaderType)
	{
		case GL_ShaderType_Fragment:
		{
			shaderSource = &shaderAsset->fragShader;
			filename = shaderAsset->fragFilename;
		} break;
		case GL_ShaderType_Geometry:
		{
			ASSERT(false, "Geometry shaders are not implemented!");
		} break;
		case GL_ShaderType_Vertex:
		{
			shaderSource = &shaderAsset->vertShader;
			filename = shaderAsset->vertFilename;
		} break;
	}

	ASSERT(shaderSource && filename, "Failed to select a shader!");


	GLuint shaderID = glCreateShader(shaderType);
	glShaderSource(shaderID, 1, shaderSource, NULL);
	glCompileShader(shaderID);

	GLint isCompiled = GL_FALSE;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &isCompiled);
	result = (isCompiled == GL_TRUE);

	if (result)
	{
		glAttachShader(shaderProgram->shaderProgramID, shaderID);
		glDeleteShader(shaderID);
	}
	else
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Unable to compile vertex shader %d, %s!\n",
			            shaderID, filename);
		TemporaryMemory tempArena = beginTemporaryMemory(&renderer->renderArena);
		GL_printShaderLog(&tempArena, shaderID);
		endTemporaryMemory(&tempArena);
	}

	return result;
}

bool GL_loadShaderProgram(GL_Renderer *renderer, AssetManager *assets, ShaderProgramType shaderProgramID)
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
		isVertexShaderCompiled = GL_compileShader(renderer, glShader, GL_ShaderType_Vertex, shaderAsset);
		isFragmentShaderCompiled = GL_compileShader(renderer, glShader, GL_ShaderType_Fragment, shaderAsset);

		// Link shader program
		if (isVertexShaderCompiled && isFragmentShaderCompiled)
		{
			glLinkProgram(glShader->shaderProgramID);
			GLint programSuccess = GL_FALSE;
			glGetProgramiv(glShader->shaderProgramID, GL_LINK_STATUS, &programSuccess);
			glShader->isValid = (programSuccess == GL_TRUE);

			if (!glShader->isValid)
			{
				TemporaryMemory tempArena = beginTemporaryMemory(&renderer->renderArena);
				SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Unable to link shader program %d!\n", glShader->shaderProgramID);
				GL_printProgramLog(&tempArena, glShader->shaderProgramID);
				endTemporaryMemory(&tempArena);
			}
			else
			{
				// Common vertex attributes
				glShader->aPositionLoc = glGetAttribLocation(glShader->shaderProgramID, "aPosition");
				if (glShader->aPositionLoc == -1)
				{
					SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "aPosition is not a valid glsl program variable!\n");
					glShader->isValid = false;
				}
				glShader->aColorLoc = glGetAttribLocation(glShader->shaderProgramID, "aColor");
				if (glShader->aColorLoc == -1)
				{
					SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "aColor is not a valid glsl program variable!\n");
					glShader->isValid = false;
				}

				// Optional vertex attributes
				glShader->aUVLoc = glGetAttribLocation(glShader->shaderProgramID, "aUV");

				// Common uniforms
				glShader->uProjectionMatrixLoc = glGetUniformLocation(glShader->shaderProgramID, "uProjectionMatrix");
				if (glShader->uProjectionMatrixLoc == -1)
				{
					SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "uProjectionMatrix is not a valid glsl program variable!\n");
					glShader->isValid = false;
				}

				// Optional uniforms
				glShader->uTextureLoc = glGetUniformLocation(glShader->shaderProgramID, "uTexture");
			}
		}
		result = glShader->isValid;
	}

	return result;
}

void GL_loadAssets(GL_Renderer *renderer, AssetManager *assets)
{
	// Textures
	renderer->textureCount = assets->textureCount;
	for (uint32 i=0; i<renderer->textureCount; i++)
	{
		if (i == 0)
		{
			renderer->textureInfo[i].glTextureID = 0;
			renderer->textureInfo[i].isLoaded = true;
		}
		else
		{
			glGenTextures(1, &renderer->textureInfo[i].glTextureID);
			renderer->textureInfo[i].isLoaded = false;
		}
	}

	// Shaders
	for (uint32 shaderID=0; shaderID < ShaderProgramCount; shaderID++)
	{
		bool shaderLoaded = GL_loadShaderProgram(renderer, assets, (ShaderProgramType)shaderID);
		ASSERT(shaderLoaded, "Failed to load shader %d into OpenGL.", shaderID);
	}
}

void GL_unloadAssets(GL_Renderer *renderer)
{
	// Textures
	for (uint32 i=1; i<renderer->textureCount; i++)
	{
		GL_TextureInfo *info = renderer->textureInfo + i;
		if (info->isLoaded)
		{
			glDeleteTextures(1, &info->glTextureID);
			info->glTextureID = 0;
			info->isLoaded = false;
		}
	}
	renderer->textureCount = 0;

	// Shaders
	renderer->currentShader = -1;
	for (uint32 shaderID=0; shaderID < ShaderProgramCount; shaderID++)
	{
		GL_ShaderProgram *shader = renderer->shaders + shaderID;
		glDeleteProgram(shader->shaderProgramID);
		*shader = {};
	}
}

GL_Renderer *GL_initializeRenderer(SDL_Window *window, AssetManager *assets)
{
	GL_Renderer *renderer;
	bootstrapArena(GL_Renderer, renderer, renderArena);
	bool succeeded = (renderer != 0);

	if (succeeded)
	{
		initRenderer(&renderer->renderer, &renderer->renderArena);

		renderer->window = window;

		// Use GL3.1 Core
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		// Create context
		renderer->context = SDL_GL_CreateContext(renderer->window);
		if (renderer->context == NULL)
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
				GL_loadAssets(renderer, assets);
			}

			if (succeeded)
			{
				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

				glGenBuffers(1, &renderer->VBO);
				glGenBuffers(1, &renderer->IBO);

				GL_checkForError();
			}

			if (!succeeded)
			{
				SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not initialise OpenGL! :(");
			}
		}

		if (!succeeded)
		{
			GL_freeRenderer(renderer);
			renderer = 0;
		}
	}

	return renderer;
}

inline GL_ShaderProgram *getActiveShader(GL_Renderer *renderer)
{
	ASSERT(renderer->currentShader >= 0 && renderer->currentShader < ShaderProgramCount, "Invalid shader!");
	GL_ShaderProgram *activeShader = renderer->shaders + renderer->currentShader;
	ASSERT(activeShader->isValid, "Shader not properly loaded!");

	return activeShader;
}

void renderPartOfBuffer(GL_Renderer *renderer, uint32 vertexCount, uint32 indexCount)
{
	GL_ShaderProgram *activeShader = getActiveShader(renderer);

	glBindBuffer(GL_ARRAY_BUFFER, renderer->VBO);
	GL_checkForError();
	glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(GL_VertexData), renderer->vertices, GL_STATIC_DRAW);
	GL_checkForError();

	// Fill IBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->IBO);
	GL_checkForError();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(GLuint), renderer->indices, GL_STATIC_DRAW);
	GL_checkForError();

	glEnableVertexAttribArray(activeShader->aPositionLoc);
	GL_checkForError();
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
	GL_checkForError();
	glDisableVertexAttribArray(activeShader->aColorLoc);
	GL_checkForError();
	if (activeShader->aUVLoc != -1)
	{
		glDisableVertexAttribArray(activeShader->aUVLoc);
		GL_checkForError();
	}
}

ShaderProgramType getDesiredShader(RenderItem *item)
{
	ShaderProgramType result = ShaderProgram_Untextured;
	if (item->textureRegionID != 0)
	{
		result = ShaderProgram_Textured;
	}

	return result;
}

void renderBuffer(GL_Renderer *renderer, AssetManager *assets, RenderBuffer *buffer)
{
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
				GL_checkForError();

				glUniformMatrix4fv(activeShader->uProjectionMatrixLoc, 1, false, buffer->camera.projectionMatrix.flat);
				GL_checkForError();

				// Bind new texture if this shader uses textures
				if (activeShader->uTextureLoc != -1)
				{
					glEnable(GL_TEXTURE_2D);
					glActiveTexture(GL_TEXTURE0);
					GL_checkForError();
					glBindTexture(GL_TEXTURE_2D, textureInfo->glTextureID);
					GL_checkForError();

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

						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

						// Upload texture
						Texture *texture = assets->textures + region->textureID;
						ASSERT(texture->state == AssetState_Loaded, "Texture asset not loaded yet!");
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->surface->w, texture->surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->surface->pixels);
						textureInfo->isLoaded = true;
						GL_checkForError();
					}

					glUniform1i(activeShader->uTextureLoc, 0);
					GL_checkForError();
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

	SDL_Log("Drew %d items this frame, with %d draw calls.", buffer->itemCount, drawCallCount);
	buffer->itemCount = 0;
}

void sortRenderBuffer(RenderBuffer *buffer)
{
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
			i + gap <= buffer->itemCount;
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

bool isBufferSorted(RenderBuffer *buffer)
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

void GL_render(GL_Renderer *renderer, AssetManager *assets)
{
	// Sort sprites
	sortRenderBuffer(&renderer->renderer.worldBuffer);
	sortRenderBuffer(&renderer->renderer.uiBuffer);

#if 0
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

	renderer->currentShader = ShaderProgram_Invalid;

	renderBuffer(renderer, assets, &renderer->renderer.worldBuffer);
	renderBuffer(renderer, assets, &renderer->renderer.uiBuffer);

	glUseProgram(NULL);
	GL_checkForError();
	SDL_Log("End of frame.");
	SDL_GL_SwapWindow( renderer->window );
}

////////////////////////////////////////////////////////////////////
//                          ANIMATIONS!                           //
////////////////////////////////////////////////////////////////////
#if 0
void setAnimation(Animator *animator, GL_Renderer *renderer, AnimationID animationID,
					bool restart)
{
	Animation *anim = renderer->animations + animationID;
	// We do nothing if the animation is already playing
	if (restart
	 || animator->animation != anim)
	{
		animator->animation = anim;
		animator->currentFrame = 0;
		animator->frameCounter = 0.0f;
	}
}

void drawAnimator(GL_Renderer *renderer, RenderBuffer *buffer, Animator *animator, real32 daysPerFrame,
				V2 worldTilePosition, V2 size, real32 depth, V4 color)
{
	animator->frameCounter += daysPerFrame * animationFramesPerDay;
	while (animator->frameCounter >= 1)
	{
		int32 framesElapsed = (int)animator->frameCounter;
		animator->currentFrame = (animator->currentFrame + framesElapsed) % animator->animation->frameCount;
		animator->frameCounter -= framesElapsed;
	}
	drawTextureAtlasItem(renderer, buffer, animator->animation->frames[animator->currentFrame],
		                 worldTilePosition, size, depth, color);
}
#endif