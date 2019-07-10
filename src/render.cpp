#pragma once

void updateCameraMatrix(Camera *camera)
{
	f32 camHalfWidth = camera->size.x * 0.5f / camera->zoom;
	f32 camHalfHeight = camera->size.y * 0.5f / camera->zoom;
	camera->projectionMatrix = orthographicMatrix4(
		camera->pos.x - camHalfWidth, camera->pos.x + camHalfWidth,
		camera->pos.y - camHalfHeight, camera->pos.y + camHalfHeight,
		camera->nearClippingPlane, camera->farClippingPlane
	);
}

void initCamera(Camera *camera, V2 size, f32 nearClippingPlane, f32 farClippingPlane, V2 position)
{
	camera->size = size;
	camera->pos = position;
	camera->zoom = 1.0f;
	camera->nearClippingPlane = nearClippingPlane;
	camera->farClippingPlane = farClippingPlane;

	updateCameraMatrix(camera);
}

void initRenderBuffer(MemoryArena *arena, RenderBuffer *buffer, char *name, u32 initialSize)
{
	buffer->name = pushString(arena, name);
	buffer->hasRangeReserved = false;
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

void resizeWindow(Renderer *renderer, s32 w, s32 h, bool fullscreen)
{
	SDL_RestoreWindow(renderer->window);

	// There are functions in SDL that appear to get the min/max window size, but they actually
	// return 0 unless we've already told it what they should be, which makes them useless.
	// TODO: Maybe examine the screen resolutions for each available display and make a decent
	// guess from that? It's not a pretty solution though.
	s32 newW = w;
	s32 newH = h;

	SDL_SetWindowSize(renderer->window, newW, newH);

	// NB: SDL_WINDOW_FULLSCREEN_DESKTOP is a fake-fullscreen window, rather than actual fullscreen.
	// I think that's a better option, but it doesn't work properly right now if we request a
	// non-native screen resolution with it!
	// Then again, regular fullscreen does a bunch of flickery stuff in that case, so yeahhhhh
	SDL_SetWindowFullscreen(renderer->window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);

	// Centre the window
	if (!fullscreen)
	{
		SDL_SetWindowPosition(renderer->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}

	// NB: Because InputState relies on SDL_WINDOWEVENT_RESIZED events,
	// it simplifies things massively if we generate one ourselves.
	SDL_Event resizeEvent = {};
	resizeEvent.type = SDL_WINDOWEVENT;
	resizeEvent.window.event = SDL_WINDOWEVENT_RESIZED;
	resizeEvent.window.data1 = newW;
	resizeEvent.window.data2 = newH;
	SDL_PushEvent(&resizeEvent);

	onWindowResized(renderer, newW, newH);
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

inline void makeRenderItem(RenderItem *result, Rect2 rect, f32 depth, Asset *texture, Rect2 uv, s32 shaderID, V4 color)
{
	result->rect = rect;
	result->depth = depth;
	result->color = color;

	result->texture = texture;
	result->uv = uv;
	result->shaderID = shaderID;

	// DEBUG STUFF
#if 0
	if (result->shaderID == 0 && texture != null)
	{
		// We had a weird issue where invalid textures are sometimes reaching the renderer, so we'll try to
		// catch them here instead.

		Asset *min = globalAppState.assets->allAssets.firstChunk.items;
		Asset *max = globalAppState.assets->allAssets.lastChunk->items + globalAppState.assets->allAssets.chunkSize;
		ASSERT(texture >= min && texture <= max); //Attempted to draw using an invalid texture asset pointer!
	}
#endif
}

RenderItem *reserveRenderItemRange(RenderBuffer *buffer, s32 count)
{
	ASSERT(!buffer->hasRangeReserved); //Can't reserve a range while a range is already reserved!

	reserve(&buffer->items, count);
	buffer->hasRangeReserved = true;
	buffer->reservedRangeSize = count;

	RenderItem *result = buffer->items.items + buffer->items.count;

	return result;
}

void finishReservedRenderItemRange(RenderBuffer *buffer, s32 itemsAdded)
{
	ASSERT(buffer->hasRangeReserved); //Attempted to finish a range while a range is not reserved!
	if (itemsAdded > buffer->reservedRangeSize)
	{
		logError("You drew {0} items but only reserved room for {1}!!! This is really bad.", {formatInt(itemsAdded), formatInt(buffer->reservedRangeSize)});
		ASSERT_RELEASE(false);
	}

	buffer->hasRangeReserved = false;
	buffer->items.count += itemsAdded;
}

void applyOffsetToRenderItems(RenderItem *firstItem, RenderItem *lastItem, f32 offsetX, f32 offsetY)
{
	for (RenderItem *it = firstItem; it <= lastItem; it++)
	{
		it->rect.x += offsetX;
		it->rect.y += offsetY;
	}
}

int compareRenderItems(const void *a, const void *b)
{
	f32 depthA = ((RenderItem*)a)->depth;
	f32 depthB = ((RenderItem*)b)->depth;

	if (depthA < depthB) return -1;
	if (depthA > depthB) return 1;
	return 0;
}

void sortRenderBuffer(RenderBuffer *buffer)
{
	DEBUG_FUNCTION_T(DCDT_Renderer);

	qsort(buffer->items.items, buffer->items.count, sizeof(RenderItem), compareRenderItems);
}

#if CHECK_BUFFERS_SORTED
bool isBufferSorted(RenderBuffer *buffer)
{
	DEBUG_FUNCTION_T(DCDT_Debugging);
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
