#pragma once

inline real32 depthFromY(real32 y)
{
	return (y * 0.1f);
}
inline real32 depthFromY(uint32 y)
{
	return depthFromY((real32)y);
}
inline real32 depthFromY(int32 y)
{
	return depthFromY((real32)y);
}

SDL_Cursor *createCursor(char *path)
{
	SDL_Surface *cursorSurface = IMG_Load(path);
	SDL_Cursor *cursor = SDL_CreateColorCursor(cursorSurface, 0, 0);
	SDL_FreeSurface(cursorSurface);

	return cursor;
}

void setCursor(UiTheme *theme, Cursor cursor)
{
	SDL_SetCursor(theme->cursors[cursor]);
}

// Screen -> scene space
V2 unproject(Camera *camera, V2 screenPos)
{
	V2 result = {};

	V4 screenPosV4 = v4(screenPos.x, screenPos.y, 0.0f, 1.0f);

	// Convert into scene space
	V4 unprojected = inverse(&camera->projectionMatrix) * screenPosV4;
	result = unprojected.xy;

	return result;
}

void updateCameraMatrix(Camera *camera)
{
	real32 camHalfWidth = camera->windowWidth * 0.5f * camera->zoom;
	real32 camHalfHeight = camera->windowHeight * 0.5f * camera->zoom;
	camera->projectionMatrix = orthographicMatrix4(
		camera->pos.x - camHalfWidth, camera->pos.x + camHalfWidth,
		camera->pos.y - camHalfHeight, camera->pos.y + camHalfHeight,
		-1000.0f, 1000.0f
	);
}