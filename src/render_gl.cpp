// render_gl.cpp

void GL_freeRenderer(GL_Renderer *renderer)
{
	SDL_GL_DeleteContext(renderer->context);
}

void GL_windowResized(GL_Renderer *renderer, int32 newWidth, int32 newHeight)
{
	glViewport(0, 0, newWidth, newHeight);
}

void GL_printProgramLog(TemporaryMemoryArena *tempMemory, GLuint program)
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

void GL_printShaderLog(TemporaryMemoryArena *tempMemory, GLuint shader)
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

GL_ShaderProgram GL_loadShader(GL_Renderer *renderer, char *vertexShaderFilename, char *fragmentShaderFilename)
{
	GL_ShaderProgram result = {};

	result.vertexShaderFilename = vertexShaderFilename;
	result.fragmentShaderFilename = fragmentShaderFilename;

	result.isVertexShaderCompiled = GL_FALSE;
	result.isFragmentShaderCompiled = GL_FALSE;

	result.shaderProgramID = glCreateProgram();

	if (result.shaderProgramID)
	{
		// VERTEX SHADER
		{
			TemporaryMemoryArena tempArena = beginTemporaryMemory(&renderer->renderArena);

			result.vertexShader = glCreateShader(GL_VERTEX_SHADER);
			GLchar *shaderData[1] = {(GLchar*) readFileAsString(&tempArena, vertexShaderFilename)};
			glShaderSource(result.vertexShader, 1, shaderData, NULL);
			glCompileShader(result.vertexShader);

			GLint isCompiled = GL_FALSE;
			glGetShaderiv(result.vertexShader, GL_COMPILE_STATUS, &isCompiled);
			result.isVertexShaderCompiled = (isCompiled == GL_TRUE);

			if (result.isVertexShaderCompiled)
			{
				glAttachShader(result.shaderProgramID, result.vertexShader);
				glDeleteShader(result.vertexShader);
			}
			else
			{
				SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Unable to compile vertex shader %d, %s!\n", result.vertexShader, result.vertexShaderFilename);
				GL_printShaderLog(&tempArena, result.vertexShader);
			}

			endTemporaryMemory(&tempArena);
		}

		// FRAGMENT SHADER
		{
			TemporaryMemoryArena tempArena = beginTemporaryMemory(&renderer->renderArena);

			result.fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
			GLchar *shaderData[1] = {(GLchar*) readFileAsString(&tempArena, fragmentShaderFilename)};
			glShaderSource(result.fragmentShader, 1, shaderData, NULL);
			glCompileShader(result.fragmentShader);

			GLint isCompiled = GL_FALSE;
			glGetShaderiv(result.fragmentShader, GL_COMPILE_STATUS, &isCompiled);
			result.isFragmentShaderCompiled = (isCompiled == GL_TRUE);

			if (result.isFragmentShaderCompiled)
			{
				glAttachShader(result.shaderProgramID, result.fragmentShader);
				glDeleteShader(result.fragmentShader);
			}
			else
			{
				SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Unable to compile fragment shader %d!\n", result.fragmentShader);
				GL_printShaderLog(&tempArena, result.fragmentShader);
			}

			endTemporaryMemory(&tempArena);
		}

		// Link shader program
		if (result.isVertexShaderCompiled && result.isFragmentShaderCompiled)
		{
			glLinkProgram(result.shaderProgramID);
			GLint programSuccess = GL_FALSE;
			glGetProgramiv(result.shaderProgramID, GL_LINK_STATUS, &programSuccess);
			result.isValid = (programSuccess == GL_TRUE);

			if (!result.isValid)
			{
				TemporaryMemoryArena tempArena = beginTemporaryMemory(&renderer->renderArena);
				SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Unable to link shader program %d!\n", result.shaderProgramID);
				GL_printProgramLog(&tempArena, result.shaderProgramID);
				endTemporaryMemory(&tempArena);
			}
			else
			{
				// Common vertex attributes
				result.aPositionLoc = glGetAttribLocation(result.shaderProgramID, "aPosition");
				if (result.aPositionLoc == -1)
				{
					SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "aPosition is not a valid glsl program variable!\n");
					result.isValid = false;
				}
				result.aColorLoc = glGetAttribLocation(result.shaderProgramID, "aColor");
				if (result.aColorLoc == -1)
				{
					SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "aColor is not a valid glsl program variable!\n");
					result.isValid = false;
				}

				// Optional vertex attributes
				result.aUVLoc = glGetAttribLocation(result.shaderProgramID, "aUV");

				// Common uniforms
				result.uProjectionMatrixLoc = glGetUniformLocation(result.shaderProgramID, "uProjectionMatrix");
				if (result.uProjectionMatrixLoc == -1)
				{
					SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "uProjectionMatrix is not a valid glsl program variable!\n");
					result.isValid = false;
				}

				// Optional uniforms
				result.uTextureLoc = glGetUniformLocation(result.shaderProgramID, "uTexture");
			}
		}
	}

	return result;
}

GL_Renderer *GL_initializeRenderer(MemoryArena *memoryArena, SDL_Window *window, AssetManager *assets)
{
	GL_Renderer *renderer = PushStruct(memoryArena, GL_Renderer);
	bool succeeded = (renderer != 0);

	if (succeeded)
	{
		renderer->renderArena = allocateSubArena(memoryArena, MB(64));
		TemporaryMemoryArena tempArena = beginTemporaryMemory(memoryArena);

		renderer->window = window;

		initRenderBuffer(&renderer->renderArena, &renderer->worldBuffer, "WorldBuffer", WORLD_SPRITE_MAX);
		initRenderBuffer(&renderer->renderArena, &renderer->uiBuffer, "UIBuffer", UI_SPRITE_MAX);

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
			renderer->shaders[ShaderProgram_Textured] = GL_loadShader(renderer, "textured.vert.gl", "textured.frag.gl");
			succeeded = renderer->shaders[ShaderProgram_Textured].isValid;

			if (succeeded)
			{
				renderer->shaders[ShaderProgram_Untextured] = GL_loadShader(renderer, "untextured.vert.gl", "untextured.frag.gl");
				succeeded = renderer->shaders[ShaderProgram_Untextured].isValid;
			}

			if (succeeded)
			{
				// Textures
				for (uint32 i=0; i<assets->textureCount; i++)
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
			}

			if (succeeded)
			{
				glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
				// glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
				// glClearColor(0.3176f, 0.6353f, 0.2549f, 1.0f);

				// Create vertex and index buffers
				glGenBuffers(1, &renderer->VBO);
				glGenBuffers(1, &renderer->IBO);

				checkForGLError();
			}

			if (!succeeded)
			{
				SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not initialise OpenGL! :(");
			}
		}

		// UI Theme!
		// TODO: Move this!
		renderer->theme.buttonTextColor 		= color255(   0,   0,   0, 255 );
		renderer->theme.buttonBackgroundColor 	= color255( 255, 255, 255, 255 );
		renderer->theme.buttonHoverColor 		= color255( 192, 192, 255, 255 );
		renderer->theme.buttonPressedColor 		= color255( 128, 128, 255, 255 );

		renderer->theme.labelColor 				= color255( 255, 255, 255, 255 );
		renderer->theme.overlayColor 			= color255(   0,   0,   0, 128 );

		renderer->theme.textboxBackgroundColor 	= color255( 255, 255, 255, 255 );
		renderer->theme.textboxTextColor 		= color255(   0,   0,   0, 255 );
		
		renderer->theme.tooltipBackgroundColor	= color255(   0,   0,   0, 128 );
		renderer->theme.tooltipColorNormal 		= color255( 255, 255, 255, 255 );
		renderer->theme.tooltipColorBad 		= color255( 255,   0,   0, 255 );

		renderer->theme.font = FontAssetType_Main;
		renderer->theme.buttonFont = FontAssetType_Buttons;
		
		// renderer->theme.font = readBMFont(&renderer->renderArena, &tempArena, "dejavu-20.fnt", renderer);
		// renderer->theme.buttonFont = readBMFont(&renderer->renderArena, &tempArena, "dejavu-14.fnt", renderer);

		renderer->theme.cursors[Cursor_Main] = createCursor("cursor_main.png");
		renderer->theme.cursors[Cursor_Build] = createCursor("cursor_build.png");
		renderer->theme.cursors[Cursor_Demolish] = createCursor("cursor_demolish.png");
		renderer->theme.cursors[Cursor_Plant] = createCursor("cursor_plant.png");
		renderer->theme.cursors[Cursor_Harvest] = createCursor("cursor_harvest.png");
		renderer->theme.cursors[Cursor_Hire] = createCursor("cursor_hire.png");
		setCursor(&renderer->theme, Cursor_Main);

		endTemporaryMemory(&tempArena);

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
	ASSERT(renderer->currentShader >= 0 && renderer->currentShader < ShaderProgram_Count, "Invalid shader!");
	GL_ShaderProgram *activeShader = renderer->shaders + renderer->currentShader;
	ASSERT(activeShader->isValid, "Shader not properly loaded!");

	return activeShader;
}

void renderPartOfBuffer(GL_Renderer *renderer, uint32 vertexCount, uint32 indexCount)
{
	GL_ShaderProgram *activeShader = getActiveShader(renderer);

	glBindBuffer(GL_ARRAY_BUFFER, renderer->VBO);
	checkForGLError();
	glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(GL_VertexData), renderer->vertices, GL_STATIC_DRAW);
	checkForGLError();

	// Fill IBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->IBO);
	checkForGLError();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(GLuint), renderer->indices, GL_STATIC_DRAW);
	checkForGLError();

	glEnableVertexAttribArray(activeShader->aPositionLoc);
	checkForGLError();
	glEnableVertexAttribArray(activeShader->aColorLoc);
	checkForGLError();

	if (activeShader->aUVLoc != -1)
	{
		glEnableVertexAttribArray(activeShader->aUVLoc);
		checkForGLError();
	}

	glBindBuffer(GL_ARRAY_BUFFER, renderer->VBO);
	checkForGLError();
	glVertexAttribPointer(activeShader->aPositionLoc, 	3, GL_FLOAT, GL_FALSE, sizeof(GL_VertexData), (GLvoid*)offsetof(GL_VertexData, pos));
	checkForGLError();
	glVertexAttribPointer(activeShader->aColorLoc,		4, GL_FLOAT, GL_FALSE, sizeof(GL_VertexData), (GLvoid*)offsetof(GL_VertexData, color));
	checkForGLError();
	if (activeShader->aUVLoc != -1)
	{
		glVertexAttribPointer(activeShader->aUVLoc, 		2, GL_FLOAT, GL_FALSE, sizeof(GL_VertexData), (GLvoid*)offsetof(GL_VertexData, uv));
		checkForGLError();
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->IBO);
	checkForGLError();
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, NULL);
	checkForGLError();

	glDisableVertexAttribArray(activeShader->aPositionLoc);
	checkForGLError();
	glDisableVertexAttribArray(activeShader->aColorLoc);
	checkForGLError();
	if (activeShader->aUVLoc != -1)
	{
		glDisableVertexAttribArray(activeShader->aUVLoc);
		checkForGLError();
	}
}

void renderBuffer(GL_Renderer *renderer, AssetManager *assets, RenderBuffer *buffer)
{
	// Fill VBO
	uint32 vertexCount = 0;
	uint32 indexCount = 0;
	GLint glBoundTextureID = -1;

	uint32 drawCallCount = 0;

	for (uint32 i=0; i < buffer->itemCount; i++)
	{
		RenderItem *item = buffer->items + i;
		TextureRegion *region = assets->textureRegions + item->textureRegionID;
		GL_TextureInfo *textureInfo = renderer->textureInfo + region->textureID;

		if (textureInfo->glTextureID != glBoundTextureID)
		{
			// Render existing buffer contents
			if (vertexCount)
			{
				drawCallCount++;
				renderPartOfBuffer(renderer, vertexCount, indexCount);
			}

			if (textureInfo->glTextureID == 0)
			{
				renderer->currentShader = ShaderProgram_Untextured;
			}
			else
			{
				renderer->currentShader = ShaderProgram_Textured;
			}

			GL_ShaderProgram *activeShader = getActiveShader(renderer);

			glUseProgram(activeShader->shaderProgramID);
			checkForGLError();

			glUniformMatrix4fv(activeShader->uProjectionMatrixLoc, 1, false, buffer->camera.projectionMatrix.flat);
			checkForGLError();

			// Bind new texture if this shader uses textures
			if (activeShader->uTextureLoc != -1)
			{
				glEnable(GL_TEXTURE_2D);
				glActiveTexture(GL_TEXTURE0);
				checkForGLError();
				glBindTexture(GL_TEXTURE_2D, textureInfo->glTextureID);
				checkForGLError();

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
					checkForGLError();
				}

				glUniform1i(activeShader->uTextureLoc, 0);
				checkForGLError();
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
	sortRenderBuffer(&renderer->worldBuffer);
	sortRenderBuffer(&renderer->uiBuffer);

#if 0
	// Check buffers are sorted
	ASSERT(isBufferSorted(&renderer->worldBuffer), "World sprites are out of order!");
	ASSERT(isBufferSorted(&renderer->uiBuffer), "UI sprites are out of order!");
#endif

	glClear(GL_COLOR_BUFFER_BIT);
	checkForGLError();
	glEnable(GL_BLEND);
	checkForGLError();
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	checkForGLError();

	glEnable(GL_TEXTURE_2D);
	checkForGLError();

	renderer->currentShader = ShaderProgram_Invalid;

	renderBuffer(renderer, assets, &renderer->worldBuffer);
	renderBuffer(renderer, assets, &renderer->uiBuffer);

	glUseProgram(NULL);
	checkForGLError();
	SDL_Log("End of frame.");
	SDL_GL_SwapWindow( renderer->window );
}

#if 0 // Leaving this here until I'm sure the new unproject works.
V2 unproject(GL_Renderer *renderer, V2 pos)
{
	// Normalise to (-1 to 1) coordinates as used by opengl
	V2 windowSize = v2(renderer->worldCamera.windowSize);
	V4 normalised = v4(
		((pos.x * 2.0f) / windowSize.x) - 1.0f,
		((pos.y * -2.0f) + windowSize.y) / windowSize.y,
		0.0f,
		1.0f
	);

	// Convert into world space
	V4 result = inverse(&renderer->worldBuffer.projectionMatrix) * normalised;
	// SDL_Log("upproject: %f, %f", result.x, result.y);

	return result.xy;// + renderer->camera.pos;
}
#endif

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