#include <SDL.h>
#include <gl/glew.h>
#include <SDL_opengl.h>
#include <gl/glu.h>
#include <stdio.h>
#include <SDL_image.h>

#include "types.h"
#include "matrix4.h"

const int WINDOW_W = 800,
		WINDOW_H = 600;

SDL_Window *gWindow = NULL;


struct VertexData {
	V3 pos;
	V4 color;
	V2 uv;
};

struct GLRenderer {
	SDL_GLContext context;

	GLuint shaderProgramID;
	GLuint VBO,
		   IBO;
	GLint uProjectionMatrixLoc,
		  uTextureLoc;
	GLint aPositionLoc,
		  aColorLoc,
		  aUVLoc;

	Matrix4 projectionMatrix;

	GLuint texture;
	GLenum textureFormat;

	VertexData vertices[1024];
	uint32 vertexCount;
	GLuint indices[1024];
	uint32 indexCount;
};

struct Sprite {
	V2 pos;
	V2 size;
	real32 depth; // Positive is forwards from the camera
};

struct SpriteBuffer {
	Sprite sprites[1024];
	uint32 spriteCount;
};

bool initOpenGL(GLRenderer *glRenderer);
void printProgramLog(GLuint program);
void printShaderLog(GLuint shader);

bool init(GLRenderer *glRenderer) {

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

void renderSprite(GLRenderer *glRenderer, Sprite *sprite) {
	int firstVertex = glRenderer->vertexCount;

	V2 halfSize = sprite->size / 2.0f;

	glRenderer->vertices[glRenderer->vertexCount++] = {v3( sprite->pos.x - halfSize.x, sprite->pos.y - halfSize.y, sprite->depth), v4(1.0f, 1.0f, 1.0f, 1.0f), v2(0.0f, 0.0f)};
	glRenderer->vertices[glRenderer->vertexCount++] = {v3( sprite->pos.x + halfSize.x, sprite->pos.y - halfSize.y, sprite->depth), v4(1.0f, 1.0f, 1.0f, 1.0f), v2(1.0f, 0.0f)};
	glRenderer->vertices[glRenderer->vertexCount++] = {v3( sprite->pos.x + halfSize.x, sprite->pos.y + halfSize.y, sprite->depth), v4(1.0f, 1.0f, 1.0f, 1.0f), v2(1.0f, 1.0f)};
	glRenderer->vertices[glRenderer->vertexCount++] = {v3( sprite->pos.x - halfSize.x, sprite->pos.y + halfSize.y, sprite->depth), v4(1.0f, 1.0f, 1.0f, 1.0f), v2(0.0f, 1.0f)};

	glRenderer->indices[glRenderer->indexCount++] = firstVertex + 0;
	glRenderer->indices[glRenderer->indexCount++] = firstVertex + 1;
	glRenderer->indices[glRenderer->indexCount++] = firstVertex + 2;
	glRenderer->indices[glRenderer->indexCount++] = firstVertex + 0;
	glRenderer->indices[glRenderer->indexCount++] = firstVertex + 2;
	glRenderer->indices[glRenderer->indexCount++] = firstVertex + 3;
}

void render(GLRenderer *glRenderer) {
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(glRenderer->shaderProgramID);

	// Bind the texture to TEXTURE0, then set 0 as the uniform
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, glRenderer->texture);
	glUniform1i(glRenderer->uTextureLoc, 0);

	glEnableVertexAttribArray(glRenderer->aPositionLoc);
	glEnableVertexAttribArray(glRenderer->aColorLoc);
	glEnableVertexAttribArray(glRenderer->aUVLoc);

	glBindBuffer(GL_ARRAY_BUFFER, glRenderer->VBO);
	glVertexAttribPointer(glRenderer->aPositionLoc, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (GLvoid*)offsetof(VertexData, pos));
	glVertexAttribPointer(glRenderer->aColorLoc, 	4, GL_FLOAT, GL_FALSE, sizeof(VertexData), (GLvoid*)offsetof(VertexData, color));
	glVertexAttribPointer(glRenderer->aUVLoc, 		2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (GLvoid*)offsetof(VertexData, uv));

	glUniformMatrix4fv(glRenderer->uProjectionMatrixLoc, 1, false, glRenderer->projectionMatrix.flat);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glRenderer->IBO);
	glDrawElements(GL_TRIANGLES, glRenderer->indexCount, GL_UNSIGNED_INT, NULL);

	glDisableVertexAttribArray(glRenderer->aPositionLoc);
	glDisableVertexAttribArray(glRenderer->aColorLoc);
	glDisableVertexAttribArray(glRenderer->aUVLoc);

	glUseProgram(NULL);

	SDL_GL_SwapWindow( gWindow );
}

void sortSpriteBuffer(SpriteBuffer *spriteBuffer) {

}

int main(int argc, char *argv[]) {

	GLRenderer glRenderer = {};
	if (!init(&glRenderer)) {
		return 1;
	}

	// Load a texture
	{
		SDL_Surface *surface = IMG_Load("combined.png");
		if (!surface) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to load png!\n%s", IMG_GetError());
			return 1;
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

		for (int i=0; i<surface->w * surface->h; i++) {
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

		glGenTextures(1, &glRenderer.texture);
		glBindTexture(GL_TEXTURE_2D, glRenderer.texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
		SDL_FreeSurface(surface);
	}

	SDL_StartTextInput();

	bool quit = false;
	SDL_Event event;

	const real32 worldScale = 1.0f / 16.0f;
	real32 halfCamWidth = WINDOW_W * worldScale * 0.5f,
			halfCamHeight = WINDOW_H * worldScale * 0.5f;

	glRenderer.projectionMatrix = orthographicMatrix4(-halfCamWidth, halfCamWidth, -halfCamHeight, halfCamHeight, -1000.0f, 1000.0f);
	real32 seconds = 0.0f;

	SpriteBuffer spriteBuffer = {};
	
	while( !quit )
	{
		while( SDL_PollEvent( &event ) != 0 )
		{
			switch (event.type)
			{
				case SDL_QUIT: {
					quit = true;
				} break;
			}
		}

#if 0
		seconds = SDL_GetTicks() / 1000.0f;
		glRenderer.projectionMatrix = orthographicMatrix4(-halfCamWidth, halfCamWidth, -halfCamHeight, halfCamHeight, -100.0f, 100.0f);
		scale(&glRenderer.projectionMatrix, v3(sin(seconds) * 0.5f + 1.0f, cos(seconds) * 0.5f + 1.0f, 1.0f) );
		rotateZ(&glRenderer.projectionMatrix, -seconds);
		translate(&glRenderer.projectionMatrix, v3(sin(seconds) / 3.0f, cos(seconds) / 3.0f, 0) );
#endif

		// Generate sprites
		spriteBuffer.spriteCount = 0;
		spriteBuffer.sprites[spriteBuffer.spriteCount++] = {v2(1.0f,1.0f), v2(10.0f, 10.0f), 1.0f};
		spriteBuffer.sprites[spriteBuffer.spriteCount++] = {v2(2.0f,2.0f), v2( 9.0f,  9.0f), 2.0f};
		spriteBuffer.sprites[spriteBuffer.spriteCount++] = {v2(3.0f,3.0f), v2( 8.0f,  8.0f), 3.0f};
		spriteBuffer.sprites[spriteBuffer.spriteCount++] = {v2(4.0f,4.0f), v2( 7.0f,  7.0f), 4.0f};
		spriteBuffer.sprites[spriteBuffer.spriteCount++] = {v2(5.0f,5.0f), v2( 6.0f,  6.0f), 5.0f};

		// Sort sprites
		sortSpriteBuffer(&spriteBuffer);

		// Fill VBO
		glRenderer.vertexCount = 0;
		glRenderer.indexCount = 0;
		for (uint32 i=0; i < spriteBuffer.spriteCount; i++) {
			renderSprite(&glRenderer, spriteBuffer.sprites + i);
		}
		
		glBindBuffer(GL_ARRAY_BUFFER, glRenderer.VBO);
		glBufferData(GL_ARRAY_BUFFER, glRenderer.vertexCount * sizeof(VertexData), glRenderer.vertices, GL_STATIC_DRAW);

		// Fill IBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glRenderer.IBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, glRenderer.indexCount * sizeof(GLuint), glRenderer.indices, GL_STATIC_DRAW);

		//Render quad
		render(&glRenderer);
	}
	
	SDL_StopTextInput();
	glDeleteProgram( glRenderer.shaderProgramID );
	SDL_DestroyWindow( gWindow );
	IMG_Quit();
	SDL_Quit();

	return 0;
}