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

void initRenderBuffer(MemoryArena *arena, RenderBuffer *buffer, char *name, uint32 maxItems)
{
	buffer->name = name;
	buffer->items = PushArray(arena, RenderItem, maxItems);
	buffer->itemCount = 0;
	buffer->maxItems = maxItems;
}

void initRenderer(Renderer *renderer, MemoryArena *renderArena, SDL_Window *window)
{
	renderer->window = window;

	initRenderBuffer(renderArena, &renderer->worldBuffer, "WorldBuffer", WORLD_SPRITE_MAX);
	initRenderBuffer(renderArena, &renderer->uiBuffer, "UIBuffer", UI_SPRITE_MAX);
}

void resizeWindow(Renderer *renderer, int32 w, int32 h)
{
	SDL_SetWindowSize(renderer->window, w, h);
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
	real32 camHalfWidth = camera->size.x * 0.5f / camera->zoom;
	real32 camHalfHeight = camera->size.y * 0.5f / camera->zoom;
	camera->projectionMatrix = orthographicMatrix4(
		camera->pos.x - camHalfWidth, camera->pos.x + camHalfWidth,
		camera->pos.y - camHalfHeight, camera->pos.y + camHalfHeight,
		-1000.0f, 1000.0f
	);
}

inline RenderItem makeRenderItem(RealRect rect, real32 depth, uint32 textureRegionID, V4 color)
{
	RenderItem item = {};
	item.rect = rect;
	item.depth = depth;
	item.color = color;
	item.textureRegionID = textureRegionID;
	
	return item;
}

void drawRect(RenderBuffer *buffer, RealRect rect, real32 depth, V4 color)
{
	ASSERT(buffer->itemCount < buffer->maxItems, "No room for DrawItem in %s.", buffer->name);

	buffer->items[buffer->itemCount++] = makeRenderItem(rect, depth, 0, color);
}

void drawTextureRegion(RenderBuffer *buffer, uint32 region, RealRect rect, real32 depth, V4 color=makeWhite())
{
	ASSERT(buffer->itemCount < buffer->maxItems, "No room for DrawItem in %s.", buffer->name);

	buffer->items[buffer->itemCount++] = makeRenderItem(rect, depth, region, color);
}

void drawRenderItem(RenderBuffer *buffer, RenderItem *item, V2 offsetP, real32 depthOffset)
{
	RenderItem *dest = buffer->items + buffer->itemCount++;

	dest->rect = offset(item->rect, offsetP);
	dest->depth = item->depth + depthOffset;
	dest->color = item->color;
	dest->textureRegionID = item->textureRegionID;
}