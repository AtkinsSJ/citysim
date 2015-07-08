#include <SDL.h>
#include <gl/glew.h>
#include <SDL_opengl.h>
#include <gl/glu.h>
#include <stdio.h>

#include "types.h"
#include "matrix4.h"

const int WINDOW_W = 800,
		WINDOW_H = 600;

SDL_Window *gWindow = NULL;


struct GLRenderer {
	SDL_GLContext context;

	GLuint shaderProgramID;
	GLuint VBO,
		   IBO;
	GLint uProjectionMatrixLoc;
	GLint aPositionLoc,
		  aColorLoc;

	Matrix4 projectionMatrix;
};

struct VertexData {
	V3 pos;
	V4 color;
};

bool gRenderQuad = true;

bool initOpenGL(GLRenderer *glRenderer);
void printProgramLog(GLuint program);
void printShaderLog(GLuint shader);

bool init(GLRenderer *glRenderer) {

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
					WINDOW_W, WINDOW_H, // Initial screen resolution
					SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (gWindow == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Window could not be created! :(\n %s", SDL_GetError());
		return false;
	}

	// Create context
	glRenderer->context = SDL_GL_CreateContext(gWindow);
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



bool initOpenGL(GLRenderer *glRenderer) {
	glRenderer->shaderProgramID = glCreateProgram();

	// VERTEX SHADER
	{
		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		const GLchar* vertexShaderSource[] = {
			"#version 150\n"
			"in vec3 aPosition;"
			"in vec4 aColor;"
			"out vec4 vColor;"
			"uniform mat4 uProjectionMatrix;"
			"void main() {"
				"gl_Position = uProjectionMatrix * vec4( aPosition.xyz, 1 );"
				"vColor = aColor;"
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
			"in vec4 vColor;"
			"out vec4 fragColor;"
			"void main() {"
				"fragColor = vColor;"
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

	// Uniform locations
	glRenderer->uProjectionMatrixLoc = glGetUniformLocation(glRenderer->shaderProgramID, "uProjectionMatrix");
	if (glRenderer->uProjectionMatrixLoc == -1) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "uProjectionMatrix is not a valid glsl program variable!\n");
		return false;
	}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// Create VBO
	VertexData vertexData[] = {
		{v3( -10.5f, -10.5f, 0.0f), v4(1.0f, 1.0f, 1.0f, 1.0f)},
		{v3(  10.5f, -10.5f, 1.0f), v4(1.0f, 0.0f, 0.0f, 1.0f)},
		{v3(  10.5f,  10.5f, 2.0f), v4(0.0f, 0.0f, 1.0f, 1.0f)},
		{v3( -10.5f,  10.5f, 3.0f), v4(0.0f, 1.0f, 0.0f, 1.0f)},
	};
	glGenBuffers(1, &glRenderer->VBO);
	glBindBuffer(GL_ARRAY_BUFFER, glRenderer->VBO);
	glBufferData(GL_ARRAY_BUFFER, ArrayCount(vertexData) * sizeof(VertexData), vertexData, GL_STATIC_DRAW);

	// Create IBO
	GLuint indexData[] = { 0, 1, 2, 3 };
	glGenBuffers(1, &glRenderer->IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glRenderer->IBO);
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

void render(GLRenderer *glRenderer) {
	glClear(GL_COLOR_BUFFER_BIT);

	if (gRenderQuad) {
		glUseProgram(glRenderer->shaderProgramID);

		glEnableVertexAttribArray(glRenderer->aPositionLoc);
		glEnableVertexAttribArray(glRenderer->aColorLoc);

		glBindBuffer(GL_ARRAY_BUFFER, glRenderer->VBO);
		glVertexAttribPointer(glRenderer->aPositionLoc, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (GLvoid*)offsetof(VertexData, pos));
		glVertexAttribPointer(glRenderer->aColorLoc, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), (GLvoid*)offsetof(VertexData, color));

		glUniformMatrix4fv(glRenderer->uProjectionMatrixLoc, 1, false, glRenderer->projectionMatrix.flat);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glRenderer->IBO);
		glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, NULL);

		glDisableVertexAttribArray(glRenderer->aPositionLoc);
		glDisableVertexAttribArray(glRenderer->aColorLoc);

		glUseProgram(NULL);
	}
}

int main(int argc, char *argv[]) {

	GLRenderer glRenderer;
	if (!init(&glRenderer)) {
		return 1;
	}
	SDL_StartTextInput();

	bool quit = false;
	SDL_Event event;

	const real32 worldScale = 1.0f / 16.0f;
	real32 halfCamWidth = WINDOW_W * worldScale * 0.5f,
			halfCamHeight = WINDOW_H * worldScale * 0.5f;

	glRenderer.projectionMatrix = orthographicMatrix4(-halfCamWidth, halfCamWidth, -halfCamHeight, halfCamHeight, -100.0f, 100.0f);
	real32 seconds = 0.0f;
	
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

#if 0
		seconds = SDL_GetTicks() / 1000.0f;
		glRenderer.projectionMatrix = identityMatrix4();
		scale(&glRenderer.projectionMatrix, v3(sin(seconds) * 0.5f + 1.0f, cos(seconds) * 0.5f + 1.0f, 1.0f) );
		rotateZ(&glRenderer.projectionMatrix, seconds);
		translate(&glRenderer.projectionMatrix, v3(sin(seconds) / 3.0f, cos(seconds) / 3.0f, 0) );
#endif

		//Render quad
		render(&glRenderer);
		SDL_GL_SwapWindow( gWindow );
	}
	
	SDL_StopTextInput();
	glDeleteProgram( glRenderer.shaderProgramID );
	SDL_DestroyWindow( gWindow );
	SDL_Quit();

	return 0;
}