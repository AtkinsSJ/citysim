// render_gl.cpp

GLRenderer *initializeRenderer(MemoryArena *memoryArena, const char *WindowTitle)
{
	GLRenderer *renderer = PushStruct(memoryArena, GLRenderer);
	GLRenderer *Result = renderer;
	renderer->renderArena = allocateSubArena(memoryArena, MB(64));

	TemporaryMemoryArena tempArena = beginTemporaryMemory(memoryArena);

	renderer->worldBuffer.sprites = PushArray(&renderer->renderArena, Sprite, WORLD_SPRITE_MAX);
	renderer->worldBuffer.maxSprites = WORLD_SPRITE_MAX;
	renderer->uiBuffer.sprites    = PushArray(&renderer->renderArena, Sprite, UI_SPRITE_MAX);
	renderer->uiBuffer.maxSprites = UI_SPRITE_MAX;

	// SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL could not be initialised! :(\n %s\n", SDL_GetError());
		Result = null;
	}

	// SDL_image
	uint8 imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags))
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL_image could not be initialised! :(\n %s\n", IMG_GetError());
		Result = null;
	}

	// Use GL3.1 Core
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	// Window
	renderer->window = SDL_CreateWindow(WindowTitle,
					SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
					800, 600, // Initial screen resolution
					SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (renderer->window == NULL)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Window could not be created! :(\n %s", SDL_GetError());
		Result = null;
	}

	// Create context
	renderer->context = SDL_GL_CreateContext(renderer->window);
	if (renderer->context == NULL)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "OpenGL context could not be created! :(\n %s", SDL_GetError());
		Result = null;
	}

	// GLEW
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not initialise GLEW! :(\n %s", glewGetErrorString(glewError));
		Result = null;
	}

	// VSync
	if (SDL_GL_SetSwapInterval(1) < 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not set vsync! :(\n %s", SDL_GetError());
		Result = null;
	}

	// Init OpenGL
	if (!initOpenGL(renderer))
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not initialise OpenGL! :(");
		Result = null;
	}

	// UI Theme!
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

	renderer->theme.font = readBMFont(&renderer->renderArena, &tempArena, "dejavu-20.fnt", renderer);
	renderer->theme.buttonFont = readBMFont(&renderer->renderArena, &tempArena, "dejavu-14.fnt", renderer);

	renderer->theme.cursors[Cursor_Main] = createCursor("cursor_main.png");
	renderer->theme.cursors[Cursor_Build] = createCursor("cursor_build.png");
	renderer->theme.cursors[Cursor_Demolish] = createCursor("cursor_demolish.png");
	renderer->theme.cursors[Cursor_Plant] = createCursor("cursor_plant.png");
	renderer->theme.cursors[Cursor_Harvest] = createCursor("cursor_harvest.png");
	renderer->theme.cursors[Cursor_Hire] = createCursor("cursor_hire.png");
	setCursor(renderer, Cursor_Main);

	// Load textures &c
	if (!loadTextures(renderer))
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not load textures! :(");
		Result = null;
	}

	endTemporaryMemory(&tempArena);

	return Result;
}

GLShaderProgram loadShader(GLRenderer *renderer, char *vertexShaderFilename, char *fragmentShaderFilename)
{
	GLShaderProgram result = {};

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
				printShaderLog(result.vertexShader);
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
				printShaderLog(result.fragmentShader);
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
				SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Unable to link shader program %d!\n", result.shaderProgramID);
				printProgramLog(result.shaderProgramID);
			}
			else
			{
				// Vertex attribute location
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
				result.aUVLoc = glGetAttribLocation(result.shaderProgramID, "aUV");
				if (result.aUVLoc == -1)
				{
					SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "aUV is not a valid glsl program variable!\n");
					result.isValid = false;
				}

				// Uniform locations
				result.uProjectionMatrixLoc = glGetUniformLocation(result.shaderProgramID, "uProjectionMatrix");
				if (result.uProjectionMatrixLoc == -1)
				{
					SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "uProjectionMatrix is not a valid glsl program variable!\n");
					result.isValid = false;
				}
				result.uTextureLoc = glGetUniformLocation(result.shaderProgramID, "uTexture");
				if (result.uTextureLoc == -1)
				{
					SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "uTexture is not a valid glsl program variable!\n");
					result.isValid = false;
				}
			}
		}
	}

	return result;
}

bool initOpenGL(GLRenderer *renderer)
{
	bool succeeded = true;
	glEnable(GL_TEXTURE_2D);

	renderer->shaderTextured = loadShader(renderer, "general.vert.gl", "general.frag.gl");
	succeeded = renderer->shaderTextured.isValid;

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	// glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	// glClearColor(0.3176f, 0.6353f, 0.2549f, 1.0f);

	// Create vertex and index buffers
	glGenBuffers(1, &renderer->VBO);
	glGenBuffers(1, &renderer->IBO);

	checkForGLError();

	return succeeded;
}

void assignTextureRegion(GLRenderer *renderer, TextureAtlasItem item, Texture *texture, real32 x, real32 y, real32 w, real32 h)
{
	real32 tw = (real32) texture->w,
		   th = (real32) texture->h;

	renderer->textureAtlas.textureRegions[item] = {
		texture->glTextureID,
		{
#if 1
			x / tw,
			y / th,
			w / tw,
			h / th
#else
			(x + 0.5f) / tw,
			(y + 0.5f) / th,
			(w - 1.0f) / tw,
			(h - 1.0f) / th
#endif
		}
	};
}

Texture *loadTexture(GLRenderer *renderer, char *filename, bool isAlphaPremultiplied)
{
	Texture *texture = renderer->textures + renderer->textureCount++;
	texture->filename = filename;

	SDL_Surface *surface = IMG_Load(filename);
	if (!surface)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR,
			"Failed to load '%s'!\n%s", filename, IMG_GetError());
		texture->isValid = false;
	}
	else
	{
		if (!isAlphaPremultiplied)
		{
			// Premultiply alpha
			uint32 Rmask = surface->format->Rmask,
				   Gmask = surface->format->Gmask,
				   Bmask = surface->format->Bmask,
				   Amask = surface->format->Amask;
			real32 rRmask = (real32)Rmask,
				   rGmask = (real32)Gmask,
				   rBmask = (real32)Bmask,
				   rAmask = (real32)Amask;

			int pixelCount = surface->w * surface->h;
			for (int pixelIndex=0;
				pixelIndex < pixelCount;
				pixelIndex++)
			{
				uint32 pixel = ((uint32*)surface->pixels)[pixelIndex];
				real32 rr = (real32)(pixel & Rmask) / rRmask;
				real32 rg = (real32)(pixel & Gmask) / rGmask;
				real32 rb = (real32)(pixel & Bmask) / rBmask;
				real32 ra = (real32)(pixel & Amask) / rAmask;

				uint32 r = (uint32)(rr * ra * rRmask) & Rmask;
				uint32 g = (uint32)(rg * ra * rGmask) & Gmask;
				uint32 b = (uint32)(rb * ra * rBmask) & Bmask;
				uint32 a = (uint32)(ra * rAmask) & Amask;

				((uint32*)surface->pixels)[pixelIndex] = (uint32)r | (uint32)g | (uint32)b | (uint32)a;
			}
		}

		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &texture->glTextureID);
		glBindTexture(GL_TEXTURE_2D, texture->glTextureID);

		// NB: Here we set the texture filter!!!
	#if 1
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	#else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	#endif

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		texture->isValid = true;
		texture->w = surface->w;
		texture->h = surface->h;

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->w, texture->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

		SDL_FreeSurface(surface);
	}

	return texture;
}

bool loadTextures(GLRenderer *renderer)
{
	bool successfullyLoadedAllTextures = true;

	Texture *texCombined = loadTexture(renderer, "combined.png", false);
	if (!texCombined->isValid)
	{
		successfullyLoadedAllTextures = false;
	}

	Texture *texMenuLogo = loadTexture(renderer, "farming-logo.png", false);
	if (!texMenuLogo->isValid)
	{
		successfullyLoadedAllTextures = false;
	}

	if (successfullyLoadedAllTextures)
	{
		const real32 w1 = 16.0f,
					w2 = w1 *2,
					w3 = w1 *3,
					w4 = w1 *4;
		
		assignTextureRegion(renderer, TextureAtlasItem_GroundTile, 	texCombined,  0,  0, w1, w1);
		assignTextureRegion(renderer, TextureAtlasItem_WaterTile, 	texCombined, w1,  0, w1, w1);
		assignTextureRegion(renderer, TextureAtlasItem_ForestTile, 	texCombined, w2,  0, w1, w1);
		assignTextureRegion(renderer, TextureAtlasItem_Path, 		texCombined, w3,  0, w1, w1);
		assignTextureRegion(renderer, TextureAtlasItem_Field, 		texCombined, w4,  0, w4, w4);
		assignTextureRegion(renderer, TextureAtlasItem_Crop0_0, 	texCombined,  0, w1, w1, w1);
		assignTextureRegion(renderer, TextureAtlasItem_Crop0_1, 	texCombined, w1, w1, w1, w1);
		assignTextureRegion(renderer, TextureAtlasItem_Crop0_2, 	texCombined, w2, w1, w1, w1);
		assignTextureRegion(renderer, TextureAtlasItem_Crop0_3, 	texCombined, w3, w1, w1, w1);
		assignTextureRegion(renderer, TextureAtlasItem_Potato, 		texCombined,  0, w2, w1, w1);
		assignTextureRegion(renderer, TextureAtlasItem_Barn, 		texCombined,  0, w4, w4, w4);
		assignTextureRegion(renderer, TextureAtlasItem_House, 		texCombined, w4, w4, w4, w4);

		assignTextureRegion(renderer, TextureAtlasItem_Farmer_Stand,  	texCombined, 128 + 0, 64 +  0,  8,  8);
		assignTextureRegion(renderer, TextureAtlasItem_Farmer_Walk0,  	texCombined, 128 + 8, 64 +  0,  8,  8);
		assignTextureRegion(renderer, TextureAtlasItem_Farmer_Walk1,  	texCombined, 128 +16, 64 +  0,  8,  8);
		assignTextureRegion(renderer, TextureAtlasItem_Farmer_Hold,   	texCombined, 128 + 0, 64 +  8,  8,  8);
		assignTextureRegion(renderer, TextureAtlasItem_Farmer_Carry0, 	texCombined, 128 + 8, 64 +  8,  8,  8);
		assignTextureRegion(renderer, TextureAtlasItem_Farmer_Carry1, 	texCombined, 128 +16, 64 +  8,  8,  8);
		assignTextureRegion(renderer, TextureAtlasItem_Farmer_Harvest0, texCombined, 128 + 0, 64 + 16,  8,  8);
		assignTextureRegion(renderer, TextureAtlasItem_Farmer_Harvest1, texCombined, 128 + 8, 64 + 16,  8,  8);
		assignTextureRegion(renderer, TextureAtlasItem_Farmer_Harvest2, texCombined, 128 +16, 64 + 16,  8,  8);
		assignTextureRegion(renderer, TextureAtlasItem_Farmer_Harvest3, texCombined, 128 +24, 64 + 16,  8,  8);
		assignTextureRegion(renderer, TextureAtlasItem_Farmer_Plant0, 	texCombined, 128 + 0, 64 + 24,  8,  8);
		assignTextureRegion(renderer, TextureAtlasItem_Farmer_Plant1, 	texCombined, 128 + 8, 64 + 24,  8,  8);
		assignTextureRegion(renderer, TextureAtlasItem_Farmer_Plant2, 	texCombined, 128 +16, 64 + 24,  8,  8);
		assignTextureRegion(renderer, TextureAtlasItem_Farmer_Plant3, 	texCombined, 128 +24, 64 + 24,  8,  8);

		assignTextureRegion(renderer, TextureAtlasItem_Icon_Planting, 	texCombined, 128 +  0, 0, 32, 32);
		assignTextureRegion(renderer, TextureAtlasItem_Icon_Harvesting, texCombined, 128 + 32, 0, 32, 32);

		// Logo!
		assignTextureRegion(renderer, TextureAtlasItem_Menu_Logo, 	texMenuLogo, 0, 0, 499, 154);

		Animation *animation;
		
		animation = renderer->animations + Animation_Farmer_Stand;
		animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Stand;

		animation = renderer->animations + Animation_Farmer_Walk;
		animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Walk0;
		animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Walk1;
		
		animation = renderer->animations + Animation_Farmer_Hold;
		animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Hold;

		animation = renderer->animations + Animation_Farmer_Carry;
		animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Carry0;
		animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Carry1;

		animation = renderer->animations + Animation_Farmer_Harvest;
		animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Harvest0;
		animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Harvest1;
		animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Harvest2;
		animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Harvest3;

		animation = renderer->animations + Animation_Farmer_Plant;
		animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Plant0;
		animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Plant1;
		animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Plant2;
		animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Plant3;
	}

	return successfullyLoadedAllTextures;
}

void freeRenderer(GLRenderer *renderer)
{
	SDL_GL_DeleteContext(renderer->context);
	SDL_DestroyWindow(renderer->window);

	IMG_Quit();
	SDL_Quit();
}

void printProgramLog( GLuint program )
{
	//Make sure name is shader
	if( glIsProgram( program ) )
	{
		//Program log length
		int infoLogLength = 0;
		int maxLength = infoLogLength;
		
		//Get info string length
		glGetProgramiv( program, GL_INFO_LOG_LENGTH, &maxLength );
		
		//Allocate string
		char* infoLog = new char[ maxLength ];
		
		//Get info log
		glGetProgramInfoLog( program, maxLength, &infoLogLength, infoLog );
		if( infoLogLength > 0 )
		{
			//Print Log
			SDL_Log( "%s\n", infoLog );
		}
		
		//Deallocate string
		delete[] infoLog;
	}
	else
	{
		SDL_Log( "Name %d is not a program\n", program );
	}
}

void printShaderLog( GLuint shader )
{
	//Make sure name is shader
	if( glIsShader( shader ) )
	{
		//Shader log length
		int infoLogLength = 0;
		int maxLength = infoLogLength;
		
		//Get info string length
		glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &maxLength );
		
		//Allocate string
		char* infoLog = new char[ maxLength ];
		
		//Get info log
		glGetShaderInfoLog( shader, maxLength, &infoLogLength, infoLog );
		if( infoLogLength > 0 )
		{
			//Print Log
			SDL_Log( "%s\n", infoLog );
		}

		//Deallocate string
		delete[] infoLog;
	}
	else
	{
		SDL_Log( "Name %d is not a shader\n", shader );
	}
}

SDL_Cursor *createCursor(char *path)
{
	SDL_Surface *cursorSurface = IMG_Load(path);
	SDL_Cursor *cursor = SDL_CreateColorCursor(cursorSurface, 0, 0);
	SDL_FreeSurface(cursorSurface);

	return cursor;
}

void setCursor(GLRenderer *renderer, Cursor cursor)
{
	SDL_SetCursor(renderer->theme.cursors[cursor]);
}

void drawSprite(GLRenderer *renderer, bool isUI, Sprite *sprite, V3 offset)
{
	RenderBuffer *buffer;
	if (isUI)
	{
		buffer = &renderer->uiBuffer;
	}
	else
	{
		buffer = &renderer->worldBuffer;
	}

	if (buffer->spriteCount >= buffer->maxSprites)
	{
		SDL_Log("Too many %s sprites!", isUI ? "UI" : "world");
		return;
	}

	Sprite *bufferSprite = buffer->sprites + buffer->spriteCount++;
	*bufferSprite = *sprite;
	bufferSprite->rect.pos += offset.xy;
	bufferSprite->depth += offset.z;
}

void drawQuad(GLRenderer *renderer, bool isUI, RealRect rect, real32 depth,
				GLint textureID, RealRect uv, V4 color)
{
	RenderBuffer *buffer;
	if (isUI)
	{
		buffer = &renderer->uiBuffer;
	}
	else
	{
		buffer = &renderer->worldBuffer;
	}

	if (buffer->spriteCount >= buffer->maxSprites)
	{
		SDL_Log("Too many %s sprites!", isUI ? "UI" : "world");
		return;
	}

	buffer->sprites[buffer->spriteCount++] = {
		rect, depth, textureID, uv, color
	};
}

void drawTextureAtlasItem(GLRenderer *renderer, bool isUI, TextureAtlasItem textureAtlasItem,
				V2 position, V2 size, real32 depth, V4 color)
{
	TextureRegion *region = renderer->textureAtlas.textureRegions + textureAtlasItem;
	GLint textureID = (textureAtlasItem > 0) ? region->textureID : TEXTURE_ID_NONE;

	drawQuad(renderer, isUI, rectCentreSize(position, size), depth, textureID, region->uv, color);
}

void drawRect(GLRenderer *renderer, bool isUI, RealRect rect, real32 depth, V4 color)
{
	drawQuad(renderer, isUI, rect, depth, TEXTURE_ID_NONE, {}, color);
}

void renderPartOfBuffer(GLRenderer *renderer, uint32 vertexCount, uint32 indexCount)
{
	glBindBuffer(GL_ARRAY_BUFFER, renderer->VBO);
	checkForGLError();
	glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(VertexData), renderer->vertices, GL_STATIC_DRAW);
	checkForGLError();

	// Fill IBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->IBO);
	checkForGLError();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(GLuint), renderer->indices, GL_STATIC_DRAW);
	checkForGLError();

	glEnableVertexAttribArray(renderer->shaderTextured.aPositionLoc);
	checkForGLError();
	glEnableVertexAttribArray(renderer->shaderTextured.aColorLoc);
	checkForGLError();
	glEnableVertexAttribArray(renderer->shaderTextured.aUVLoc);
	checkForGLError();

	glBindBuffer(GL_ARRAY_BUFFER, renderer->VBO);
	checkForGLError();
	glVertexAttribPointer(renderer->shaderTextured.aPositionLoc, 	3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (GLvoid*)offsetof(VertexData, pos));
	checkForGLError();
	glVertexAttribPointer(renderer->shaderTextured.aColorLoc,		4, GL_FLOAT, GL_FALSE, sizeof(VertexData), (GLvoid*)offsetof(VertexData, color));
	checkForGLError();
	glVertexAttribPointer(renderer->shaderTextured.aUVLoc, 		2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (GLvoid*)offsetof(VertexData, uv));
	checkForGLError();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->IBO);
	checkForGLError();
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, NULL);
	checkForGLError();

	glDisableVertexAttribArray(renderer->shaderTextured.aPositionLoc);
	checkForGLError();
	glDisableVertexAttribArray(renderer->shaderTextured.aColorLoc);
	checkForGLError();
	glDisableVertexAttribArray(renderer->shaderTextured.aUVLoc);

	checkForGLError();
}

void renderBuffer(GLRenderer *renderer, RenderBuffer *buffer)
{
	// Fill VBO
	uint32 vertexCount = 0;
	uint32 indexCount = 0;
	GLint boundTextureID = TEXTURE_ID_INVALID;

	uint32 drawCallCount = 0;

	for (uint32 i=0; i < buffer->spriteCount; i++)
	{
		Sprite *sprite = buffer->sprites + i;

		if (sprite->textureID != boundTextureID)
		{
			// Render existing buffer contents
			if (vertexCount)
			{
				drawCallCount++;
				renderPartOfBuffer(renderer, vertexCount, indexCount);
			}

			// Bind new texture
			glActiveTexture(GL_TEXTURE0);
			checkForGLError();
			glBindTexture(GL_TEXTURE_2D, sprite->textureID);
			checkForGLError();

			glUniformMatrix4fv(renderer->shaderTextured.uProjectionMatrixLoc, 1, false, buffer->projectionMatrix.flat);
			checkForGLError();
			glUniform1i(renderer->shaderTextured.uTextureLoc, 0);
			checkForGLError();

			vertexCount = 0;
			indexCount = 0;
			boundTextureID = sprite->textureID;
		}

		int firstVertex = vertexCount;

		renderer->vertices[vertexCount++] = {
			v3( sprite->rect.x, sprite->rect.y, sprite->depth),
			sprite->color,
			v2(sprite->uv.x, sprite->uv.y)
		};
		renderer->vertices[vertexCount++] = {
			v3( sprite->rect.x + sprite->rect.size.x, sprite->rect.y, sprite->depth),
			sprite->color,
			v2(sprite->uv.x + sprite->uv.w, sprite->uv.y)
		};
		renderer->vertices[vertexCount++] = {
			v3( sprite->rect.x + sprite->rect.size.x, sprite->rect.y + sprite->rect.size.y, sprite->depth),
			sprite->color,
			v2(sprite->uv.x + sprite->uv.w, sprite->uv.y + sprite->uv.h)
		};
		renderer->vertices[vertexCount++] = {
			v3( sprite->rect.x, sprite->rect.y + sprite->rect.size.y, sprite->depth),
			sprite->color,
			v2(sprite->uv.x, sprite->uv.y + sprite->uv.h)
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

	SDL_Log("Drew %d sprites this frame, with %d draw calls.", buffer->spriteCount, drawCallCount);
	buffer->spriteCount = 0;
}

void sortSpriteBuffer(RenderBuffer *buffer)
{
	// This is an implementation of the 'comb sort' algorithm, low to high

	uint32 gap = buffer->spriteCount;
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
			i + gap <= buffer->spriteCount;
			i++)
		{
			if (buffer->sprites[i].depth > buffer->sprites[i+gap].depth)
			{
				Sprite temp = buffer->sprites[i];
				buffer->sprites[i] = buffer->sprites[i+gap];
				buffer->sprites[i+gap] = temp;

				swapped = true;
			}
		}
	}
}

bool isBufferSorted(RenderBuffer *buffer)
{
	bool isSorted = true;
	real32 lastDepth = real32Min;
	for (uint32 i=0; i<=buffer->spriteCount; i++)
	{
		if (lastDepth > buffer->sprites[i].depth)
		{
			isSorted = false;
			break;
		}
		lastDepth = buffer->sprites[i].depth;
	}
	return isSorted;
}

void render(GLRenderer *renderer)
{
	// Sort sprites
	sortSpriteBuffer(&renderer->worldBuffer);
	sortSpriteBuffer(&renderer->uiBuffer);

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

	glUseProgram(renderer->shaderTextured.shaderProgramID);
	checkForGLError();

	renderBuffer(renderer, &renderer->worldBuffer);
	renderBuffer(renderer, &renderer->uiBuffer);

	glUseProgram(NULL);
	checkForGLError();
	SDL_Log("End of frame.");
	SDL_GL_SwapWindow( renderer->window );
}

V2 unproject(GLRenderer *renderer, V2 pos)
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

////////////////////////////////////////////////////////////////////
//                          ANIMATIONS!                           //
////////////////////////////////////////////////////////////////////
void setAnimation(Animator *animator, GLRenderer *renderer, AnimationID animationID,
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

void drawAnimator(GLRenderer *renderer, bool isUI, Animator *animator, real32 daysPerFrame,
				V2 worldTilePosition, V2 size, real32 depth, V4 color)
{
	animator->frameCounter += daysPerFrame * animationFramesPerDay;
	while (animator->frameCounter >= 1)
	{
		int32 framesElapsed = (int)animator->frameCounter;
		animator->currentFrame = (animator->currentFrame + framesElapsed) % animator->animation->frameCount;
		animator->frameCounter -= framesElapsed;
	}
	drawTextureAtlasItem(
		renderer,
		isUI,
		animator->animation->frames[animator->currentFrame],
		worldTilePosition,
		size,
		depth,
		color
	);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// FIXME! ///////////////////////////////////////////////////////////////////////////////////////
// Below are old functions that I need to keep temporarily so I can just get things compiling! //
/////////////////////////////////////////////////////////////////////////////////////////////////

inline Coord tilePosition(V2 worldPixelPos) {
	return {(int)floor(worldPixelPos.x),
			(int)floor(worldPixelPos.y)};
}