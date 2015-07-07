#include <SDL.h>
#include <gl/glew.h>
#include <SDL_opengl.h>
#include <gl/glu.h>
#include <stdio.h>

SDL_Window *gWindow = NULL;
SDL_GLContext gContext;

GLuint gProgramID = 0;
GLint gVertextPos2DLocation = -1;
GLuint gVBO = 0;
GLuint gIBO = 0;
bool gRenderQuad = true;

bool initOpenGL();
void printProgramLog(GLuint program);
void printShaderLog(GLuint shader);

bool init() {

	// SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL could not be initialised! :(\n %s\n", SDL_GetError());
		return false;
	}

	// Use GL3.1 Core
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	// Window
	gWindow = SDL_CreateWindow("OpenGL test",
					SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
					800, 600, // Initial screen resolution
					SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (gWindow == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Window could not be created! :(\n %s", SDL_GetError());
		return false;
	}

	// Create context
	gContext = SDL_GL_CreateContext(gWindow);
	if (gContext == NULL) {
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
	if (!initOpenGL()) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Could not initialise OpenGL! :(");
		return false;
	}

	return true;
}

bool initOpenGL() {
	gProgramID = glCreateProgram();

	// VERTEX SHADER
	{
		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		const GLchar* vertexShaderSource[] = {
			"#version 140\n"
			"in vec2 LVertexPos2D;"
			"void main() {"
				"gl_Position = vec4( LVertexPos2D.x, LVertexPos2D.y, 0, 1 );"
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
		glAttachShader(gProgramID, vertexShader);
	}
	
	// FRAGMENT SHADER
	{
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		const GLchar* fragmentShaderSource[] = {
			"#version 140\n"
			"out vec4 LFragment;"
			"void main() {"
				"LFragment = vec4( 1.0, 1.0, 1.0, 1.0 );"
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
		glAttachShader(gProgramID, fragmentShader);
	}

	// Link shader program
	glLinkProgram(gProgramID);
	GLint programSuccess = GL_FALSE;
	glGetProgramiv(gProgramID, GL_LINK_STATUS, &programSuccess);
	if (programSuccess != GL_TRUE) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Unable to link shader program %d!\n", gProgramID);
		printProgramLog(gProgramID);
		return false;
	}

	// Vertex attribute location
	gVertextPos2DLocation = glGetAttribLocation(gProgramID, "LVertexPos2D");
	if (gVertextPos2DLocation == -1) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "LVertexPos2D is not a valid glsl program variable!\n");
		return false;
	}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// Create VBO
	GLfloat vertexData[] = {
		 -0.5f, -0.5f,
		  0.5f, -0.5f,
		  0.5f,  0.5f,
		 -0.5f,  0.5f
	};
	glGenBuffers(1, &gVBO);
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glBufferData(GL_ARRAY_BUFFER, 2 * 4 * sizeof(GLfloat), vertexData, GL_STATIC_DRAW);

	// Create IBO
	GLuint indexData[] = { 0, 1, 2, 3 };
	glGenBuffers(1, &gIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), indexData, GL_STATIC_DRAW);

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

void render() {
	glClear(GL_COLOR_BUFFER_BIT);

	if (gRenderQuad) {
		glUseProgram(gProgramID);

		glEnableVertexAttribArray(gVertextPos2DLocation);

		glBindBuffer(GL_ARRAY_BUFFER, gVBO);
		glVertexAttribPointer(gVertextPos2DLocation, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), NULL);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
		glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, NULL);

		glDisableVertexAttribArray(gVertextPos2DLocation);

		glUseProgram(NULL);
	}
}

int main(int argc, char *argv[]) {

	if (!init()) {
		return 1;
	}
	SDL_StartTextInput();

	bool quit = false;
	SDL_Event event;
	
	while( !quit )
	{
		while( SDL_PollEvent( &event ) != 0 )
		{
			switch (event.type)
			{
				case SDL_QUIT: {
					quit = true;
				} break;

				case SDL_TEXTINPUT: {
					if( event.text.text[ 0 ] == 'q' )
					{
						gRenderQuad = !gRenderQuad;
					}
				} break;
			}
		}

		//Render quad
		render();
		SDL_GL_SwapWindow( gWindow );
	}
	
	SDL_StopTextInput();
	glDeleteProgram( gProgramID );
	SDL_DestroyWindow( gWindow );
	SDL_Quit();

	return 0;
}