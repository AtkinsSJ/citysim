#pragma once

inline f32 depthFromY(f32 y)
{
	return (y * 0.1f);
}
inline f32 depthFromY(u32 y)
{
	return depthFromY((f32)y);
}
inline f32 depthFromY(s32 y)
{
	return depthFromY((f32)y);
}

void initRenderBuffer(MemoryArena *arena, RenderBuffer *buffer, char *name, u32 initialSize)
{
	buffer->name = pushString(arena, name);
	initialiseArray(&buffer->items, initialSize);
}

void initRenderer(Renderer *renderer, MemoryArena *renderArena, SDL_Window *window)
{
	renderer->window = window;

	initRenderBuffer(renderArena, &renderer->worldBuffer, "WorldBuffer", 16384);
	initRenderBuffer(renderArena, &renderer->uiBuffer,    "UIBuffer",    16384);
}

void freeRenderer(Renderer *renderer)
{
	renderer->free(renderer);
}

void onWindowResized(Renderer *renderer, s32 w, s32 h)
{
	renderer->windowResized(w, h);
	renderer->worldBuffer.camera.size = v2((f32)w / TILE_SIZE, (f32)h / TILE_SIZE);

	renderer->uiBuffer.camera.size = v2(w, h);
	renderer->uiBuffer.camera.pos = renderer->uiBuffer.camera.size * 0.5f;
}

void resizeWindow(Renderer *renderer, s32 w, s32 h)
{
	SDL_RestoreWindow(renderer->window);
	SDL_SetWindowSize(renderer->window, w, h);

	// NB: Because InputState relies on SDL_WINDOWEVENT_RESIZED events,
	// it simplifies things massively if we generate one ourselves.
	SDL_Event resizeEvent = {};
	resizeEvent.type = SDL_WINDOWEVENT;
	resizeEvent.window.event = SDL_WINDOWEVENT_RESIZED;
	resizeEvent.window.data1 = w;
	resizeEvent.window.data2 = h;
	SDL_PushEvent(&resizeEvent);

	onWindowResized(renderer, w, h);
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
	f32 camHalfWidth = camera->size.x * 0.5f / camera->zoom;
	f32 camHalfHeight = camera->size.y * 0.5f / camera->zoom;
	camera->projectionMatrix = orthographicMatrix4(
		camera->pos.x - camHalfWidth, camera->pos.x + camHalfWidth,
		camera->pos.y - camHalfHeight, camera->pos.y + camHalfHeight,
		-10000.0f, 10000.0f
	);
}

inline RenderItem makeRenderItem(Rect2 rect, f32 depth, TextureRegionID textureRegionID, V4 color)
{
	RenderItem item = {};
	item.rect = rect;
	item.depth = depth;
	item.color = color;
	item.textureRegionID = textureRegionID;
	
	return item;
}

void drawRect(RenderBuffer *buffer, Rect2 rect, f32 depth, V4 color)
{
	append(&buffer->items, makeRenderItem(rect, depth, 0, color));
}

void drawTextureRegion(RenderBuffer *buffer, TextureRegionID region, Rect2 rect, f32 depth, V4 color=makeWhite())
{
	append(&buffer->items, makeRenderItem(rect, depth, region, color));
}

void drawRenderItem(RenderBuffer *buffer, RenderItem *item, V2 offsetP, f32 depthOffset)
{
	RenderItem *dest = appendBlank(&buffer->items);

	dest->rect = offset(item->rect, offsetP);
	dest->depth = item->depth + depthOffset;
	dest->color = item->color;
	dest->textureRegionID = item->textureRegionID;
}

s32 compareRenderItems(RenderItem *a, RenderItem *b)
{
	// NB: Hmmm. Having to convert float -> int might be a problem.
	return (s32)(a->depth - b->depth);
}

void sortRenderBuffer(RenderBuffer *buffer)
{
	DEBUG_FUNCTION();
	sortInPlace(&buffer->items, compareRenderItems);
}

#if CHECK_BUFFERS_SORTED
bool isBufferSorted(RenderBuffer *buffer)
{
	bool isSorted = true;
	f32 lastDepth = f32Min;
	for (u32 i=0; i < buffer->itemCount; i++)
	{
		if (lastDepth > buffer->items[i].depth)
		{
			isSorted = false;
			break;
		}
		lastDepth = buffer->items[i].depth;
	}
	return isSorted;
}
#endif

ShaderProgramType getDesiredShader(RenderItem *item)
{
	ShaderProgramType result = ShaderProgram_Untextured;
	if (item->textureRegionID != 0)
	{
		result = ShaderProgram_Textured;
	}

	return result;
}