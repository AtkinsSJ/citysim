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

void initRenderBuffer(MemoryArena *arena, RenderBuffer *buffer, char *name, uint32 maxItems)
{
	buffer->name = name;
	buffer->items = PushArray(arena, RenderItem, maxItems);
	buffer->itemCount = 0;
	buffer->maxItems = maxItems;
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

void drawRect(RenderBuffer *buffer, RealRect rect, real32 depth, V4 color)
{
	ASSERT(buffer->itemCount < buffer->maxItems, "No room for DrawItem in %s.", buffer->name);

	RenderItem *item = buffer->items + buffer->itemCount++;
	item->rect = rect;
	item->depth = depth;
	item->color = color;
	item->textureRegionID = 0;
}

void drawTextureRegion(RenderBuffer *buffer, uint32 region, RealRect rect, real32 depth, V4 color=makeWhite())
{
	ASSERT(buffer->itemCount < buffer->maxItems, "No room for DrawItem in %s.", buffer->name);

	RenderItem *item = buffer->items + buffer->itemCount++;
	item->rect = rect;
	item->depth = depth;
	item->color = color;
	item->textureRegionID = region;
}