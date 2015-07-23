// render_gl.cpp

GLRenderer *initializeRenderer(const char *windowTitle)
{
	TexturesToLoad *texturesToLoad = (TexturesToLoad *) calloc(1, sizeof(TexturesToLoad));
	GLRenderer *renderer = (GLRenderer *)calloc(1, sizeof(GLRenderer));

	renderer->worldBuffer.sprites = (Sprite *)malloc(WORLD_SPRITE_MAX * sizeof(Sprite));
	renderer->worldBuffer.maxSprites = WORLD_SPRITE_MAX;
	renderer->uiBuffer.sprites    = (Sprite *)malloc(UI_SPRITE_MAX * sizeof(Sprite));
	renderer->uiBuffer.maxSprites = UI_SPRITE_MAX;

	// SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL could not be initialised! :(\n %s\n", SDL_GetError());
		return null;
	}

	// SDL_image
	uint8 imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags))
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL_image could not be initialised! :(\n %s\n", IMG_GetError());
		return null;
	}

	// Use GL3.1 Core
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	// Window
	renderer->window = SDL_CreateWindow(windowTitle,
					SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
					800, 600, // Initial screen resolution
					SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (renderer->window == NULL)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Window could not be created! :(\n %s", SDL_GetError());
		return null;
	}

	// Create context
	renderer->context = SDL_GL_CreateContext(renderer->window);
	if (renderer->context == NULL)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "OpenGL context could not be created! :(\n %s", SDL_GetError());
		return null;
	}

	// GLEW
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not initialise GLEW! :(\n %s", glewGetErrorString(glewError));
		return null;
	}

	// VSync
	if (SDL_GL_SetSwapInterval(1) < 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not set vsync! :(\n %s", SDL_GetError());
		return null;
	}

	// Init OpenGL
	if (!initOpenGL(renderer))
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not initialise OpenGL! :(");
		return null;
	}

	// UI Theme!
	renderer->theme.buttonTextColor = {0,0,0,255};
	renderer->theme.buttonBackgroundColor = {255,255,255,255};
	renderer->theme.buttonHoverColor = {192,192,255,255};
	renderer->theme.buttonPressedColor = {128,128,255,255};
	renderer->theme.labelColor = {255,255,255,255};
	renderer->theme.overlayColor = {0,0,0,128};
	renderer->theme.textboxTextColor = {0,0,0,255};
	renderer->theme.textboxBackgroundColor = {255,255,255,255};

	renderer->theme.font = readBMFont("dejavu-20.fnt", texturesToLoad);
	renderer->theme.buttonFont = readBMFont("dejavu-16.fnt", texturesToLoad);

	// Load textures &c
	if (!loadTextures(renderer, texturesToLoad))
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not load textures! :(");
		return null;
	}

	free(texturesToLoad);

	return renderer;
}

bool initOpenGL(GLRenderer *renderer)
{
	renderer->shaderProgramID = glCreateProgram();

	glEnable(GL_TEXTURE_2D);

	// VERTEX SHADER
	{
		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		const GLchar* vertexShaderSource[] = {
			"#version 150\n"

			"in vec3 aPosition;"
			"in vec4 aColor;"
			"in vec2 aUV;"
			"in int aTextureID;"

			"out vec4 vColor;"
			"out vec2 vUV;"
			"out flat int vTextureID;"

			"uniform mat4 uProjectionMatrix;"

			"void main() {"
				"gl_Position = uProjectionMatrix * vec4( aPosition.xyz, 1 );"
				"vColor = aColor;"
				"vUV = aUV;"
				"vTextureID = aTextureID;"
			"}"
		};

		glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
		glCompileShader(vertexShader);

		GLint vShaderCompiled = GL_FALSE;
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vShaderCompiled);
		if (vShaderCompiled != GL_TRUE)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Unable to compile vertex shader %d!\n", vertexShader);
			printShaderLog(vertexShader);
			return false;
		}
		glAttachShader(renderer->shaderProgramID, vertexShader);
		glDeleteShader(vertexShader);
	}

	// FRAGMENT SHADER
	{
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		const GLchar* fragmentShaderSource[] = {
			"#version 150\n"

			"uniform sampler2DArray uTextures;"

			"in vec4 vColor;"
			"in vec2 vUV;"
			"in flat int vTextureID;"

			"out vec4 fragColor;"

			"void main() {"
				"fragColor = vColor;"
				"if (vTextureID != -1) {"
					"vec4 texel = texture(uTextures, vec3(vUV, vTextureID));"
					"fragColor *= texel;"
				"}"
			"}"
		};
		glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
		glCompileShader(fragmentShader);

		GLint fShaderCompiled = GL_FALSE;
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fShaderCompiled);
		if (fShaderCompiled != GL_TRUE)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Unable to compile fragment shader %d!\n", fragmentShader);
			printShaderLog(fragmentShader);
			return false;
		}
		glAttachShader(renderer->shaderProgramID, fragmentShader);
		glDeleteShader(fragmentShader);
	}

	// Link shader program
	glLinkProgram(renderer->shaderProgramID);
	GLint programSuccess = GL_FALSE;
	glGetProgramiv(renderer->shaderProgramID, GL_LINK_STATUS, &programSuccess);
	if (programSuccess != GL_TRUE)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Unable to link shader program %d!\n", renderer->shaderProgramID);
		printProgramLog(renderer->shaderProgramID);
		return false;
	}

	// Vertex attribute location
	renderer->aPositionLoc = glGetAttribLocation(renderer->shaderProgramID, "aPosition");
	if (renderer->aPositionLoc == -1)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "aPosition is not a valid glsl program variable!\n");
		return false;
	}
	renderer->aColorLoc = glGetAttribLocation(renderer->shaderProgramID, "aColor");
	if (renderer->aColorLoc == -1)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "aColor is not a valid glsl program variable!\n");
		return false;
	}
	renderer->aUVLoc = glGetAttribLocation(renderer->shaderProgramID, "aUV");
	if (renderer->aUVLoc == -1)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "aUV is not a valid glsl program variable!\n");
		return false;
	}
	renderer->aTextureIDLoc = glGetAttribLocation(renderer->shaderProgramID, "aTextureID");
	if (renderer->aTextureIDLoc == -1)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "aTextureID is not a valid glsl program variable!\n");
		return false;
	}

	// Uniform locations
	renderer->uProjectionMatrixLoc = glGetUniformLocation(renderer->shaderProgramID, "uProjectionMatrix");
	if (renderer->uProjectionMatrixLoc == -1)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "uProjectionMatrix is not a valid glsl program variable!\n");
		return false;
	}
	renderer->uTexturesLoc = glGetUniformLocation(renderer->shaderProgramID, "uTextures");
	if (renderer->uTexturesLoc == -1)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "uTextures is not a valid glsl program variable!\n");
		return false;
	}

	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

	// Create vertex and index buffers
	glGenBuffers(1, &renderer->VBO);
	glGenBuffers(1, &renderer->IBO);

	checkForGLError();

	return true;
}

void assignTextureRegion(GLRenderer *renderer, TextureAtlasItem item, Texture *texture, real32 x, real32 y, real32 w, real32 h)
{
	real32 tw = (real32) texture->w,
		   th = (real32) texture->h;

	renderer->textureRegions[item] = {
		texture->id,
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

bool loadTextures(GLRenderer *renderer, TexturesToLoad *texturesToLoad)
{
	GLint combinedPngID = pushTextureToLoad(texturesToLoad, "combined.png");
	GLint menuLogoPngID = pushTextureToLoad(texturesToLoad, "farming-logo.png");

	Texture *textures = (Texture *) calloc(texturesToLoad->filenameCount, sizeof(Texture));

	renderer->textureArrayID = 0;
	glGenTextures(1, &renderer->textureArrayID);
	glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->textureArrayID);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA,
		TEXTURE_WIDTH, TEXTURE_HEIGHT, // Size
		texturesToLoad->filenameCount, // Number
		0, GL_RGBA, GL_UNSIGNED_BYTE, null);

	for (GLint i=0;
		i < texturesToLoad->filenameCount;
		i++)
	{
		Texture *texture = textures + i;

		SDL_Surface *surface = IMG_Load(texturesToLoad->filenames[i]);
		if (!surface)
		{
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR,
				"Failed to load '%s'!\n%s", texturesToLoad->filenames[i], IMG_GetError());
			texture->valid = false;
		}
		else
		{
			// Expand surface to be consistent size
			ASSERT((surface->w <= TEXTURE_WIDTH ) && (surface->h <= TEXTURE_HEIGHT),
				"Texture %s is too large! Max size is %d x %d", texturesToLoad->filenames[i], TEXTURE_WIDTH, TEXTURE_HEIGHT);
			if (surface->w < TEXTURE_WIDTH || surface->h < TEXTURE_HEIGHT)
			{
				// Create a new surface that's the right size,
				// blit the smaller one onto it,
				// then store it in *surface.
				SDL_PixelFormat *format = surface->format;
				SDL_Surface *tempSurface = SDL_CreateRGBSurface(0, TEXTURE_WIDTH, TEXTURE_HEIGHT, 32,
					format->Rmask, format->Gmask, format->Bmask, format->Amask);

				SDL_BlitSurface(surface, null, tempSurface, null);

				SDL_FreeSurface(surface);
				surface = tempSurface;
			}

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

			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i,
				surface->w, surface->h, 1, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

			texture->valid = true;
			texture->id = i;
			texture->w = surface->w;
			texture->h = surface->h;

			SDL_FreeSurface(surface);
		}
	}

	Texture *texCombinedPng = textures + combinedPngID;
	Texture *texMenuLogoPng = textures + menuLogoPngID;

	const real32 w1 = 16.0f,
				w2 = w1 *2,
				w3 = w1 *3,
				w4 = w1 *4;
	
	assignTextureRegion(renderer, TextureAtlasItem_GroundTile, 	texCombinedPng,  0,  0, w1, w1);
	assignTextureRegion(renderer, TextureAtlasItem_WaterTile, 	texCombinedPng, w1,  0, w1, w1);
	assignTextureRegion(renderer, TextureAtlasItem_ForestTile, 	texCombinedPng, w2,  0, w1, w1);
	assignTextureRegion(renderer, TextureAtlasItem_Field, 		texCombinedPng, w4,  0, w4, w4);
	assignTextureRegion(renderer, TextureAtlasItem_Crop0_0, 	texCombinedPng,  0, w1, w1, w1);
	assignTextureRegion(renderer, TextureAtlasItem_Crop0_1, 	texCombinedPng, w1, w1, w1, w1);
	assignTextureRegion(renderer, TextureAtlasItem_Crop0_2, 	texCombinedPng, w2, w1, w1, w1);
	assignTextureRegion(renderer, TextureAtlasItem_Crop0_3, 	texCombinedPng, w3, w1, w1, w1);
	assignTextureRegion(renderer, TextureAtlasItem_Potato, 		texCombinedPng,  0, w2, w1, w1);
	assignTextureRegion(renderer, TextureAtlasItem_Barn, 		texCombinedPng,  0, w4, w4, w4);
	assignTextureRegion(renderer, TextureAtlasItem_House, 		texCombinedPng, w4, w4, w4, w4);

	assignTextureRegion(renderer, TextureAtlasItem_Farmer_Stand,  	texCombinedPng, 128 + 0, 64 +  0,  8,  8);
	assignTextureRegion(renderer, TextureAtlasItem_Farmer_Walk0,  	texCombinedPng, 128 + 8, 64 +  0,  8,  8);
	assignTextureRegion(renderer, TextureAtlasItem_Farmer_Walk1,  	texCombinedPng, 128 +16, 64 +  0,  8,  8);
	assignTextureRegion(renderer, TextureAtlasItem_Farmer_Hold,   	texCombinedPng, 128 + 0, 64 +  8,  8,  8);
	assignTextureRegion(renderer, TextureAtlasItem_Farmer_Carry0, 	texCombinedPng, 128 + 8, 64 +  8,  8,  8);
	assignTextureRegion(renderer, TextureAtlasItem_Farmer_Carry1, 	texCombinedPng, 128 +16, 64 +  8,  8,  8);
	assignTextureRegion(renderer, TextureAtlasItem_Farmer_Harvest0, texCombinedPng, 128 + 0, 64 + 16,  8,  8);
	assignTextureRegion(renderer, TextureAtlasItem_Farmer_Harvest1, texCombinedPng, 128 + 8, 64 + 16,  8,  8);
	assignTextureRegion(renderer, TextureAtlasItem_Farmer_Harvest2, texCombinedPng, 128 +16, 64 + 16,  8,  8);
	assignTextureRegion(renderer, TextureAtlasItem_Farmer_Harvest3, texCombinedPng, 128 +24, 64 + 16,  8,  8);
	assignTextureRegion(renderer, TextureAtlasItem_Farmer_Plant0, 	texCombinedPng, 128 + 0, 64 + 24,  8,  8);
	assignTextureRegion(renderer, TextureAtlasItem_Farmer_Plant1, 	texCombinedPng, 128 + 8, 64 + 24,  8,  8);
	assignTextureRegion(renderer, TextureAtlasItem_Farmer_Plant2, 	texCombinedPng, 128 +16, 64 + 24,  8,  8);
	assignTextureRegion(renderer, TextureAtlasItem_Farmer_Plant3, 	texCombinedPng, 128 +24, 64 + 24,  8,  8);

	assignTextureRegion(renderer, TextureAtlasItem_Icon_Planting, 	texCombinedPng, 128 +  0, 0, 32, 32);
	assignTextureRegion(renderer, TextureAtlasItem_Icon_Harvesting, texCombinedPng, 128 + 32, 0, 32, 32);

	// Logo!
	assignTextureRegion(renderer, TextureAtlasItem_Menu_Logo, 	texMenuLogoPng, 0, 0, 499, 154);

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

	free(textures);
	checkForGLError();
	return true;
}

void freeRenderer(GLRenderer *renderer)
{
	glDeleteProgram( renderer->shaderProgramID );

	SDL_DestroyWindow( renderer->window );

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

void drawQuad(GLRenderer *renderer, bool isUI, RealRect rect, real32 depth,
				GLint textureID, RealRect uv, Color *color)
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

	V4 drawColor;
	if (color)
	{
		drawColor = v4(color);
	} else {
		drawColor = v4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	buffer->sprites[buffer->spriteCount++] = {
		rect, depth, textureID, uv, drawColor
	};
}

void drawSprite(GLRenderer *renderer, bool isUI, TextureAtlasItem textureAtlasItem,
				V2 position, V2 size, Color *color)
{
	TextureRegion *region = renderer->textureRegions + textureAtlasItem;
	GLint textureID = (textureAtlasItem > 0) ? region->textureID : TEXTURE_ID_NONE;

	drawQuad(renderer, isUI, rectCentreSize(position, size), 0, textureID, region->uv, color);
}

void drawRect(GLRenderer *renderer, bool isUI, RealRect rect, Color *color)
{
	drawQuad(renderer, isUI, rect, 0, TEXTURE_ID_NONE, {}, color);
}

void _renderBuffer(GLRenderer *renderer, RenderBuffer *buffer)
{
	// Fill VBO
	uint32 vertexCount = 0;
	uint32 indexCount = 0;
	for (uint32 i=0; i < buffer->spriteCount; i++)
	{
		int firstVertex = vertexCount;
		Sprite *sprite = buffer->sprites + i;

		renderer->vertices[vertexCount++] = {
			v3( sprite->rect.x, sprite->rect.y, sprite->depth),
			sprite->color,
			v2(sprite->uv.x, sprite->uv.y),
			sprite->textureID
		};
		renderer->vertices[vertexCount++] = {
			v3( sprite->rect.x + sprite->rect.size.x, sprite->rect.y, sprite->depth),
			sprite->color,
			v2(sprite->uv.x + sprite->uv.w, sprite->uv.y),
			sprite->textureID
		};
		renderer->vertices[vertexCount++] = {
			v3( sprite->rect.x + sprite->rect.size.x, sprite->rect.y + sprite->rect.size.y, sprite->depth),
			sprite->color,
			v2(sprite->uv.x + sprite->uv.w, sprite->uv.y + sprite->uv.h),
			sprite->textureID
		};
		renderer->vertices[vertexCount++] = {
			v3( sprite->rect.x, sprite->rect.y + sprite->rect.size.y, sprite->depth),
			sprite->color,
			v2(sprite->uv.x, sprite->uv.y + sprite->uv.h),
			sprite->textureID
		};

		renderer->indices[indexCount++] = firstVertex + 0;
		renderer->indices[indexCount++] = firstVertex + 1;
		renderer->indices[indexCount++] = firstVertex + 2;
		renderer->indices[indexCount++] = firstVertex + 0;
		renderer->indices[indexCount++] = firstVertex + 2;
		renderer->indices[indexCount++] = firstVertex + 3;
	}
	
	glBindBuffer(GL_ARRAY_BUFFER, renderer->VBO);
	glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(VertexData), renderer->vertices, GL_STATIC_DRAW);

	// Fill IBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(GLuint), renderer->indices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(renderer->aPositionLoc);
	glEnableVertexAttribArray(renderer->aColorLoc);
	glEnableVertexAttribArray(renderer->aUVLoc);
	glEnableVertexAttribArray(renderer->aTextureIDLoc);

	glBindBuffer(GL_ARRAY_BUFFER, renderer->VBO);
	glVertexAttribPointer(renderer->aPositionLoc, 	3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (GLvoid*)offsetof(VertexData, pos));
	glVertexAttribPointer(renderer->aColorLoc,		4, GL_FLOAT, GL_FALSE, sizeof(VertexData), (GLvoid*)offsetof(VertexData, color));
	glVertexAttribPointer(renderer->aUVLoc, 		2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (GLvoid*)offsetof(VertexData, uv));
	glVertexAttribIPointer(renderer->aTextureIDLoc,	1, GL_INT,   		   sizeof(VertexData), (GLvoid*)offsetof(VertexData, textureID));

	glUniformMatrix4fv(renderer->uProjectionMatrixLoc, 1, false, buffer->projectionMatrix.flat);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->IBO);
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, NULL);

	glDisableVertexAttribArray(renderer->aPositionLoc);
	glDisableVertexAttribArray(renderer->aColorLoc);
	glDisableVertexAttribArray(renderer->aUVLoc);
	glDisableVertexAttribArray(renderer->aTextureIDLoc);

	checkForGLError();

	SDL_Log("Drew %d sprites this frame.", buffer->spriteCount);
	buffer->spriteCount = 0;
}

void render(GLRenderer *renderer)
{
	// Sort sprites
	// sortSpriteBuffer(&spriteBuffer);

	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(renderer->shaderProgramID);

	// Bind the texture array to TEXTURE0, then set 0 as the uniform
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, renderer->textureArrayID);
	glUniform1i(renderer->uTexturesLoc, 0);

	// World sprites
#if 1
	_renderBuffer(renderer, &renderer->worldBuffer);
	_renderBuffer(renderer, &renderer->uiBuffer);
#else
	renderer->worldBuffer.spriteCount = 0;
	renderer->uiBuffer.spriteCount = 0;
#endif

	glUseProgram(NULL);
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

	return result.v2;// + renderer->camera.pos;
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
				V2 worldTilePosition, V2 size, Color *color)
{
	animator->frameCounter += daysPerFrame * animationFramesPerDay;
	while (animator->frameCounter >= 1)
	{
		int32 framesElapsed = (int)animator->frameCounter;
		animator->currentFrame = (animator->currentFrame + framesElapsed) % animator->animation->frameCount;
		animator->frameCounter -= framesElapsed;
	}
	drawSprite(
		renderer,
		isUI,
		animator->animation->frames[animator->currentFrame],
		worldTilePosition,
		size,
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