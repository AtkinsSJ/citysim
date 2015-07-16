// render_gl.cpp

bool initializeRenderer(GLRenderer *renderer, const char *windowTitle)
{
	(*renderer) = {};

	// SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL could not be initialised! :(\n %s\n", SDL_GetError());
		return false;
	}

	// SDL_image
	uint8 imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags))
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL_image could not be initialised! :(\n %s\n", IMG_GetError());
		return false;
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
		return false;
	}

	// Create context
	renderer->context = SDL_GL_CreateContext(renderer->window);
	if (renderer->context == NULL)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "OpenGL context could not be created! :(\n %s", SDL_GetError());
		return false;
	}

	// GLEW
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not initialise GLEW! :(\n %s", glewGetErrorString(glewError));
		return false;
	}

	// VSync
	if (SDL_GL_SetSwapInterval(1) < 0)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not set vsync! :(\n %s", SDL_GetError());
		return false;
	}

	// Init OpenGL
	if (!initOpenGL(renderer))
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not initialise OpenGL! :(");
		return false;
	}

	// Load textures &c
	if (!loadTextures(renderer))
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not load textures! :(");
		return false;
	}

	return true;
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

			"out vec4 vColor;"
			"out vec2 vUV;"

			"uniform mat4 uProjectionMatrix;"

			"void main() {"
				"gl_Position = uProjectionMatrix * vec4( aPosition.xyz, 1 );"
				"vColor = aColor;"
				"vUV = aUV;"
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

			"uniform sampler2D uTexture;"

			"in vec4 vColor;"
			"in vec2 vUV;"

			"out vec4 fragColor;"

			"void main() {"
				"vec4 texel = texture(uTexture, vUV);"
				"fragColor = texel * vColor;"
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

	// Uniform locations
	renderer->uProjectionMatrixLoc = glGetUniformLocation(renderer->shaderProgramID, "uProjectionMatrix");
	if (renderer->uProjectionMatrixLoc == -1)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "uProjectionMatrix is not a valid glsl program variable!\n");
		return false;
	}
	renderer->uTextureLoc = glGetUniformLocation(renderer->shaderProgramID, "uTexture");
	if (renderer->uTextureLoc == -1)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "uTexture is not a valid glsl program variable!\n");
		return false;
	}

	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

	// Create vertex and index buffers
	glGenBuffers(1, &renderer->VBO);
	glGenBuffers(1, &renderer->IBO);

	return true;
}

/* Loads a file and generates an opengl texture for it, returning its ID.
 * If this fails in any way, returns -1
 */
Texture loadTexture(char* filename)
{
	Texture texture = {};

	SDL_Surface *surface = IMG_Load(filename);
	if (!surface)
	{
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR,
			"Failed to load '%s'!\n%s", filename, IMG_GetError());
		texture.valid = false;
	}
	else
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
		for (int i=0; i<pixelCount; i++)
		{
			uint32 pixel = ((uint32*)surface->pixels)[i];
			real32 rr = (real32)(pixel & Rmask) / rRmask;
			real32 rg = (real32)(pixel & Gmask) / rGmask;
			real32 rb = (real32)(pixel & Bmask) / rBmask;
			real32 ra = (real32)(pixel & Amask) / rAmask;

			uint32 r = (uint32)(rr * ra * rRmask) & Rmask;
			uint32 g = (uint32)(rg * ra * rGmask) & Gmask;
			uint32 b = (uint32)(rb * ra * rBmask) & Bmask;
			uint32 a = (uint32)(ra * rAmask) & Amask;

			((uint32*)surface->pixels)[i] = (uint32)r | (uint32)g | (uint32)b | (uint32)a;
		}

		GLuint textureID = 0;

		glGenTextures(1, &textureID);
		if (textureID != -1)
		{
			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

			texture.valid = true;
			texture.id = textureID;
			texture.w = surface->w;
			texture.h = surface->h;
		}
		else
		{
			texture.valid = false;
		}

		SDL_FreeSurface(surface);
	}

	return texture;
}

bool loadTextures(GLRenderer *renderer)
{
	Texture texCombinedPng = loadTexture("combined.png");
	if (!texCombinedPng.valid)
	{
		return false;
	}

	renderer->texture = texCombinedPng.id;

	const real32 w1 = 16.0f / 128.0f,
				w2 = w1 *2,
				w3 = w1 *3,
				w4 = w1 *4;
	
	renderer->textureRegions[TextureAtlasItem_GroundTile] 	= {texCombinedPng.id, { 0,  0, w1, w1}};
	renderer->textureRegions[TextureAtlasItem_WaterTile]  	= {texCombinedPng.id, {w1,  0, w1, w1}};
	renderer->textureRegions[TextureAtlasItem_ForestTile] 	= {texCombinedPng.id, {w2,  0, w1, w1}};

	renderer->textureRegions[TextureAtlasItem_Field] 		= {texCombinedPng.id, {w4,  0, w4, w4}};
	renderer->textureRegions[TextureAtlasItem_Crop0_0] 		= {texCombinedPng.id, { 0, w1, w1, w1}};
	renderer->textureRegions[TextureAtlasItem_Crop0_1] 		= {texCombinedPng.id, {w1, w1, w1, w1}};
	renderer->textureRegions[TextureAtlasItem_Crop0_2] 		= {texCombinedPng.id, {w2, w1, w1, w1}};
	renderer->textureRegions[TextureAtlasItem_Crop0_3] 		= {texCombinedPng.id, {w3, w1, w1, w1}};
	renderer->textureRegions[TextureAtlasItem_Potato] 		= {texCombinedPng.id, { 0, w2, w1, w1}};
	renderer->textureRegions[TextureAtlasItem_Barn] 		= {texCombinedPng.id, { 0, w4, w4, w4}};
	renderer->textureRegions[TextureAtlasItem_House] 		= {texCombinedPng.id, {w4, w4, w4, w4}};

	return true;
}

void freeRenderer(GLRenderer *renderer)
{
	glDeleteProgram( renderer->shaderProgramID );

	TTF_CloseFont(renderer->theme.font);
	TTF_CloseFont(renderer->theme.buttonFont);

	SDL_DestroyWindow( renderer->window );

	TTF_Quit();
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
			printf( "%s\n", infoLog );
		}
		
		//Deallocate string
		delete[] infoLog;
	}
	else
	{
		printf( "Name %d is not a program\n", program );
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
			printf( "%s\n", infoLog );
		}

		//Deallocate string
		delete[] infoLog;
	}
	else
	{
		printf( "Name %d is not a shader\n", shader );
	}
}

SDL_Cursor *createCursor(char *path)
{
	SDL_Surface *cursorSurface = IMG_Load(path);
	SDL_Cursor *cursor = SDL_CreateColorCursor(cursorSurface, 0, 0);
	SDL_FreeSurface(cursorSurface);

	return cursor;
}

void drawSprite(GLRenderer *renderer, TextureAtlasItem textureAtlasItem,
				V2 position, V2 size, Color *color)
{
	if (renderer->spriteBuffer.count >= ArrayCount(renderer->spriteBuffer.sprites))
	{
		printf("Too many sprites!\n");
		return;
	}

	V4 drawColor;
	if (color)
	{
		drawColor = v4(*color);
	} else {
		drawColor = v4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	renderer->spriteBuffer.sprites[renderer->spriteBuffer.count++] = {
		textureAtlasItem, position, size, 0, drawColor
	};
}


void render(GLRenderer *renderer)
{
	// Sort sprites
	// sortSpriteBuffer(&spriteBuffer);

	// Fill VBO
	renderer->vertexCount = 0;
	renderer->indexCount = 0;
	for (uint32 i=0; i < renderer->spriteBuffer.count; i++)
	{
		int firstVertex = renderer->vertexCount;
		Sprite *sprite = renderer->spriteBuffer.sprites + i;
		TextureRegion *region = renderer->textureRegions + sprite->textureAtlasItem;

		V2 halfSize = sprite->size / 2.0f;

		renderer->vertices[renderer->vertexCount++] = {
			v3( sprite->pos.x - halfSize.x, sprite->pos.y - halfSize.y, sprite->depth),
			sprite->color,
			v2(region->bounds.x, region->bounds.y)
		};
		renderer->vertices[renderer->vertexCount++] = {
			v3( sprite->pos.x + halfSize.x, sprite->pos.y - halfSize.y, sprite->depth),
			sprite->color,
			v2(region->bounds.x + region->bounds.w, region->bounds.y)
		};
		renderer->vertices[renderer->vertexCount++] = {
			v3( sprite->pos.x + halfSize.x, sprite->pos.y + halfSize.y, sprite->depth),
			sprite->color,
			v2(region->bounds.x + region->bounds.w, region->bounds.y + region->bounds.h)
		};
		renderer->vertices[renderer->vertexCount++] = {
			v3( sprite->pos.x - halfSize.x, sprite->pos.y + halfSize.y, sprite->depth),
			sprite->color,
			v2(region->bounds.x, region->bounds.y + region->bounds.h)
		};

		renderer->indices[renderer->indexCount++] = firstVertex + 0;
		renderer->indices[renderer->indexCount++] = firstVertex + 1;
		renderer->indices[renderer->indexCount++] = firstVertex + 2;
		renderer->indices[renderer->indexCount++] = firstVertex + 0;
		renderer->indices[renderer->indexCount++] = firstVertex + 2;
		renderer->indices[renderer->indexCount++] = firstVertex + 3;
	}
	
	glBindBuffer(GL_ARRAY_BUFFER, renderer->VBO);
	glBufferData(GL_ARRAY_BUFFER, renderer->vertexCount * sizeof(VertexData), renderer->vertices, GL_STATIC_DRAW);

	// Fill IBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, renderer->indexCount * sizeof(GLuint), renderer->indices, GL_STATIC_DRAW);

	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(renderer->shaderProgramID);

	// Bind the texture to TEXTURE0, then set 0 as the uniform
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, renderer->texture);
	glUniform1i(renderer->uTextureLoc, 0);

	glEnableVertexAttribArray(renderer->aPositionLoc);
	glEnableVertexAttribArray(renderer->aColorLoc);
	glEnableVertexAttribArray(renderer->aUVLoc);

	glBindBuffer(GL_ARRAY_BUFFER, renderer->VBO);
	glVertexAttribPointer(renderer->aPositionLoc, 	3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (GLvoid*)offsetof(VertexData, pos));
	glVertexAttribPointer(renderer->aColorLoc,		4, GL_FLOAT, GL_FALSE, sizeof(VertexData), (GLvoid*)offsetof(VertexData, color));
	glVertexAttribPointer(renderer->aUVLoc, 		2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (GLvoid*)offsetof(VertexData, uv));

	glUniformMatrix4fv(renderer->uProjectionMatrixLoc, 1, false, renderer->projectionMatrix.flat);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer->IBO);
	glDrawElements(GL_TRIANGLES, renderer->indexCount, GL_UNSIGNED_INT, NULL);

	glDisableVertexAttribArray(renderer->aPositionLoc);
	glDisableVertexAttribArray(renderer->aColorLoc);
	glDisableVertexAttribArray(renderer->aUVLoc);

	glUseProgram(NULL);

	SDL_GL_SwapWindow( renderer->window );
	SDL_Log("Drew %d sprites this frame.", renderer->spriteBuffer.count);
	renderer->spriteBuffer.count = 0;
}

V2 unproject(GLRenderer *renderer, V2 pos)
{
	// This is all wrong. ALL WRONG! D:
	Matrix4 mat = inverse(&renderer->projectionMatrix);

	V4 pos4 = {pos.x, pos.y, 1.0f, 1.0f};

	V4 unprojected = mat * pos4;

	return v2(unprojected.x, unprojected.y);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// FIXME! ///////////////////////////////////////////////////////////////////////////////////////
// Below are old functions that I need to keep temporarily so I can just get things compiling! //
/////////////////////////////////////////////////////////////////////////////////////////////////

void centreCameraOnPosition(Camera *camera, V2 position) {}
inline Coord tilePosition(V2 worldPixelPos) {
	return {(int)floor(worldPixelPos.x),
			(int)floor(worldPixelPos.y)};
}

void drawAtScreenPos(GLRenderer *renderer, TextureAtlasItem textureAtlasItem, Coord position) {}

void drawAtWorldPos(GLRenderer *renderer, TextureAtlasItem textureAtlasItem, V2 worldTilePosition,
	Color *color=0) {}

void drawWorldRect(GLRenderer *renderer, Rect worldRect, Color color) {}

////////////////////////////////////////////////////////////////////
//                          ANIMATIONS!                           //
////////////////////////////////////////////////////////////////////
void setAnimation(Animator *animator, GLRenderer *renderer, AnimationID animationID, bool restart = false) {}

void drawAnimator(GLRenderer *renderer, Animator *animator, real32 daysPerFrame, V2 worldTilePosition, Color *color = 0) {}