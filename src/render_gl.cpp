// render_gl.cpp

bool initializeRenderer(GLRenderer *glRenderer, const char *windowTitle) {

	*glRenderer = {};

	// SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL could not be initialised! :(\n %s\n", SDL_GetError());
		return false;
	}

	// SDL_image
	uint8 imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags)) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL_image could not be initialised! :(\n %s\n", IMG_GetError());
		return false;
	}

	// Use GL3.1 Core
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	// Window
	glRenderer->window = SDL_CreateWindow(windowTitle,
					SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
					800, 600, // Initial screen resolution
					SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (glRenderer->window == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Window could not be created! :(\n %s", SDL_GetError());
		return false;
	}

	// Create context
	glRenderer->context = SDL_GL_CreateContext(glRenderer->window);
	if (glRenderer->context == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "OpenGL context could not be created! :(\n %s", SDL_GetError());
		return false;
	}

	// GLEW
	glewExperimental = GL_TRUE;
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not initialise GLEW! :(\n %s", glewGetErrorString(glewError));
		return false;
	}

	// VSync
	if (SDL_GL_SetSwapInterval(1) < 0) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not set vsync! :(\n %s", SDL_GetError());
		return false;
	}

	// Init OpenGL
	if (!initOpenGL(glRenderer)) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not initialise OpenGL! :(");
		return false;
	}

	return true;
}

void freeRenderer(GLRenderer *glRenderer) {
	glDeleteProgram( glRenderer->shaderProgramID );

	TTF_CloseFont(glRenderer->theme.font);
	TTF_CloseFont(glRenderer->theme.buttonFont);

	SDL_DestroyWindow( glRenderer->window );

	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

bool initOpenGL(GLRenderer *glRenderer) {
	glRenderer->shaderProgramID = glCreateProgram();

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
		if (vShaderCompiled != GL_TRUE) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Unable to compile vertex shader %d!\n", vertexShader);
			printShaderLog(vertexShader);
			return false;
		}
		glAttachShader(glRenderer->shaderProgramID, vertexShader);
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
		if (fShaderCompiled != GL_TRUE) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Unable to compile fragment shader %d!\n", fragmentShader);
			printShaderLog(fragmentShader);
			return false;
		}
		glAttachShader(glRenderer->shaderProgramID, fragmentShader);
		glDeleteShader(fragmentShader);
	}

	// Link shader program
	glLinkProgram(glRenderer->shaderProgramID);
	GLint programSuccess = GL_FALSE;
	glGetProgramiv(glRenderer->shaderProgramID, GL_LINK_STATUS, &programSuccess);
	if (programSuccess != GL_TRUE) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Unable to link shader program %d!\n", glRenderer->shaderProgramID);
		printProgramLog(glRenderer->shaderProgramID);
		return false;
	}

	// Vertex attribute location
	glRenderer->aPositionLoc = glGetAttribLocation(glRenderer->shaderProgramID, "aPosition");
	if (glRenderer->aPositionLoc == -1) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "aPosition is not a valid glsl program variable!\n");
		return false;
	}
	glRenderer->aColorLoc = glGetAttribLocation(glRenderer->shaderProgramID, "aColor");
	if (glRenderer->aColorLoc == -1) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "aColor is not a valid glsl program variable!\n");
		return false;
	}
	glRenderer->aUVLoc = glGetAttribLocation(glRenderer->shaderProgramID, "aUV");
	if (glRenderer->aUVLoc == -1) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "aUV is not a valid glsl program variable!\n");
		return false;
	}

	// Uniform locations
	glRenderer->uProjectionMatrixLoc = glGetUniformLocation(glRenderer->shaderProgramID, "uProjectionMatrix");
	if (glRenderer->uProjectionMatrixLoc == -1) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "uProjectionMatrix is not a valid glsl program variable!\n");
		return false;
	}
	glRenderer->uTextureLoc = glGetUniformLocation(glRenderer->shaderProgramID, "uTexture");
	if (glRenderer->uTextureLoc == -1) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "uTexture is not a valid glsl program variable!\n");
		return false;
	}

	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

	// Create vertex and index buffers
	glGenBuffers(1, &glRenderer->VBO);
	glGenBuffers(1, &glRenderer->IBO);

	return true;
}

void printProgramLog( GLuint program ) {
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

void printShaderLog( GLuint shader ) {
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

SDL_Cursor *createCursor(char *path) {
	SDL_Surface *cursorSurface = IMG_Load(path);
	SDL_Cursor *cursor = SDL_CreateColorCursor(cursorSurface, 0, 0);
	SDL_FreeSurface(cursorSurface);

	return cursor;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// FIXME! ///////////////////////////////////////////////////////////////////////////////////////
// Below are old functions that I need to keep temporarily so I can just get things compiling! //
/////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Takes x and y in screen space, and returns a position in world-tile space.
 */
inline V2 screenPosToWorldPos(Coord pos, Camera *camera) {
	return {0,0};
}
void centreCameraOnPosition(Camera *camera, V2 position) {}
inline Coord tilePosition(V2 worldPixelPos) {
	return {(int)floor(worldPixelPos.x),
			(int)floor(worldPixelPos.y)};
}

void clearToBlack(GLRenderer *renderer) {}

void drawAtScreenPos(GLRenderer *renderer, TextureAtlasItem textureAtlasItem, Coord position) {}

void drawAtWorldPos(GLRenderer *renderer, TextureAtlasItem textureAtlasItem, V2 worldTilePosition,
	Color *color=0) {}

void drawWorldRect(GLRenderer *renderer, Rect worldRect, Color color) {}

////////////////////////////////////////////////////////////////////
//                          ANIMATIONS!                           //
////////////////////////////////////////////////////////////////////
void setAnimation(Animator *animator, GLRenderer *renderer, AnimationID animationID, bool restart = false) {}

void drawAnimator(GLRenderer *renderer, Animator *animator, real32 daysPerFrame, V2 worldTilePosition, Color *color = 0) {}