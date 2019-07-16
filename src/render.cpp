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

void initRenderer(Renderer *renderer, MemoryArena *renderArena, SDL_Window *window)
{
	renderer->window = window;

	initRenderBuffer(renderArena, &renderer->worldBuffer, "WorldBuffer", MB(1));
	initRenderBuffer(renderArena, &renderer->uiBuffer,    "UIBuffer",    MB(1));
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

inline void makeRenderItem(RenderItem_DrawThing *result, Rect2 rect, f32 depth, Asset *texture, Rect2 uv, s32 shaderID, V4 color)
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

RenderItem_DrawThing *reserveRenderItemRange(RenderBuffer *buffer, s32 count)
{
	// TODO: This all needs a real go-over!
	ASSERT(!buffer->hasRangeReserved); //Can't reserve a range while a range is already reserved!

	smm size = count * (sizeof(RenderItemType) + sizeof(RenderItem_DrawThing));

	// TODO: Expand to make room.
	// Make sure there's space for the item range and a "go to next thing" item
	ASSERT(buffer->currentChunk->size - buffer->currentChunk->used >= size + (smm)sizeof(RenderItemType));

	buffer->hasRangeReserved = true;
	buffer->reservedRangeSize = size;

	smm stride = sizeof(RenderItemType) + sizeof(RenderItem_DrawThing);
	u8 *base = buffer->currentChunk->memory + buffer->currentChunk->used;
	for (s32 i = 0; i < count; i++)
	{
		*(RenderItemType *)(base + (i * stride)) = RenderItemType_DrawThing;
	}

	RenderItem_DrawThing *result = (RenderItem_DrawThing *)(buffer->currentChunk->memory + buffer->currentChunk->used + sizeof(RenderItemType));

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
	buffer->currentChunk->used += itemsAdded * (sizeof(RenderItemType) + sizeof(RenderItem_DrawThing));
}

void applyOffsetToRenderItems(RenderItem_DrawThing *firstItem, RenderItem_DrawThing *lastItem, f32 offsetX, f32 offsetY)
{
	smm stride = sizeof(RenderItem_DrawThing) + sizeof(RenderItemType);

	RenderItem_DrawThing *it = firstItem;
	while (it <= lastItem)
	{
		it->rect.x += offsetX;
		it->rect.y += offsetY;

		it = (RenderItem_DrawThing *)( (u8*)it + stride );
	}
}

void initRenderBuffer(MemoryArena *arena, RenderBuffer *buffer, char *name, smm initialSize)
{
	buffer->name = pushString(arena, name);
	buffer->hasRangeReserved = false;

	buffer->arena = arena;
	buffer->minimumChunkSize = initialSize;

	buffer->firstChunk.size = initialSize;
	buffer->firstChunk.used = 0;
	buffer->firstChunk.memory = (u8*)allocate(arena, buffer->firstChunk.size);

	buffer->currentChunk = &buffer->firstChunk;
	buffer->firstFreeChunk = null;
}

// NB: reservedSize is for extra data that you want to make sure there is room for,
// but you're not allocating it right away. Make sure not to do any other allocations
// in between, and make sure to allocate the space you used when you're done!
u8 *appendRenderItemInternal(RenderBuffer *buffer, RenderItemType type, smm size, smm reservedSize)
{
	ASSERT(!buffer->hasRangeReserved); //Can't append renderitems while a range is reserved!

	smm totalSizeRequired = (smm)(sizeof(RenderItemType) + size + reservedSize + sizeof(RenderItemType));

	if ((buffer->currentChunk->size - buffer->currentChunk->used) <= totalSizeRequired)
	{
		// Out of room! Push a "go to next chunk" item and append some more memory
		
		ASSERT((buffer->currentChunk->size - buffer->currentChunk->used) > sizeof(RenderItemType)); // Need space for the next-chunk message

		*(RenderItemType *)(buffer->currentChunk->memory + buffer->currentChunk->used) = RenderItemType_NextMemoryChunk;
		buffer->currentChunk->used += sizeof(RenderItemType);

		if (buffer->firstFreeChunk != null)
		{
			buffer->currentChunk->next = buffer->firstFreeChunk;
			buffer->currentChunk = buffer->firstFreeChunk;
			buffer->currentChunk->used = 0;
			buffer->firstFreeChunk = buffer->firstFreeChunk->next;

			// We'd BETTER have space to actually allocate this thing in the chunk!
			ASSERT((buffer->currentChunk->size - buffer->currentChunk->used) >= totalSizeRequired);
		}
		else
		{
			// ALLOCATE
			RenderBufferChunk *newChunk = null;
			// The *2 is somewhat arbitrary, but we want to avoid chunks that are only large enough for this item,
			// because that could lead to fragmentation issues, or getting a chunk out of the free list which is
			// too small to hold whatever we want to put in it!
			// - Sam, 13/07/2019
			smm newChunkSize = max(totalSizeRequired * 2, buffer->minimumChunkSize);
			newChunk = (RenderBufferChunk *)allocate(buffer->arena, newChunkSize + sizeof(RenderBufferChunk));
			newChunk->size = newChunkSize;
			newChunk->used = 0;
			newChunk->memory = (u8*)(newChunk + 1);

			buffer->currentChunk->next = newChunk;
			buffer->currentChunk = newChunk;

			// We'd BETTER have space to actually allocate this thing in the chunk!
			ASSERT((buffer->currentChunk->size - buffer->currentChunk->used) >= totalSizeRequired);
		}
	}

	*(RenderItemType *)(buffer->currentChunk->memory + buffer->currentChunk->used) = type;
	buffer->currentChunk->used += sizeof(RenderItemType);

	u8 *result = (buffer->currentChunk->memory + buffer->currentChunk->used);
	buffer->currentChunk->used += size;
	return result;
}

DrawRectsGroup beginRectsGroup(RenderBuffer *buffer, s32 shaderID, s32 maxCount)
{
	ASSERT(!buffer->hasRangeReserved); //Can't reserve a range while a range is already reserved!

	DrawRectsGroup result = {};

	result.buffer = buffer;

	smm reservedSize = sizeof(RenderItem_DrawRects_Item) * maxCount;
	u8 *data = appendRenderItemInternal(buffer, RenderItemType_DrawRects, sizeof(RenderItem_DrawRects), reservedSize);
	buffer->hasRangeReserved = true;

	result.header = (RenderItem_DrawRects *) data;
	result.first  = (RenderItem_DrawRects_Item *) (data + sizeof(RenderItem_DrawRects));
	result.maxCount = maxCount;

	*result.header = {};
	result.header->shaderID = shaderID;
	result.header->count = 0;

	return result;
}

DrawRectsGroup beginRectsGroupForText(RenderBuffer *buffer, s32 shaderID, BitmapFont *font, s32 maxCount)
{
	ASSERT(!buffer->hasRangeReserved); //Can't reserve a range while a range is already reserved!

	DrawRectsGroup result = {};

	result.buffer = buffer;

	smm reservedSize = sizeof(RenderItem_DrawRects_Item) * maxCount;
	u8 *data = appendRenderItemInternal(buffer, RenderItemType_DrawRects, sizeof(RenderItem_DrawRects), reservedSize);
	buffer->hasRangeReserved = true;

	result.header = (RenderItem_DrawRects *) data;
	result.first  = (RenderItem_DrawRects_Item *) (data + sizeof(RenderItem_DrawRects));
	result.maxCount = maxCount;

	result.header->shaderID = shaderID;
	result.header->texture = font->texture;
	result.header->count = 0;

	return result;
}

void addRectInternal(DrawRectsGroup *group, Rect2 bounds, V4 color, Rect2 uv)
{
	ASSERT(group->header->count < group->maxCount);

	RenderItem_DrawRects_Item *item = group->first + group->header->count++;
	item->bounds = bounds;
	item->color = color;
	item->uv = uv;
}

inline void addUntexturedRect(DrawRectsGroup *group, Rect2 bounds, V4 color)
{
	addRectInternal(group, bounds, color, rectXYWH(0,0,0,0));
}

inline void addGlyphRect(DrawRectsGroup *group, BitmapFontGlyph *glyph, V2 position, V4 color)
{
	Rect2 bounds = rectXYWH(position.x + glyph->xOffset, position.y + glyph->yOffset, glyph->width, glyph->height);
	addRectInternal(group, bounds, color, glyph->uv);
}

inline void addSpriteRect(DrawRectsGroup *group, Sprite *sprite, Rect2 bounds, V4 color)
{
	if (group->header->count == 0)
	{
		group->header->texture = sprite->texture;
	}
	else
	{
		ASSERT(group->header->texture == sprite->texture);
	}

	addRectInternal(group, bounds, color, sprite->uv);
}

void offsetRange(DrawRectsGroup *group, s32 startIndex, s32 endIndexInclusive, f32 offsetX, f32 offsetY)
{
	ASSERT(startIndex >= 0 && startIndex < group->header->count);
	ASSERT(endIndexInclusive >= 0 && endIndexInclusive < group->header->count);
	ASSERT(startIndex <= endIndexInclusive);

	for (RenderItem_DrawRects_Item *item = group->first + startIndex;
		item <= group->first + endIndexInclusive;
		item++)
	{
		item->bounds.x += offsetX;
		item->bounds.y += offsetY;
	}
}

void endRectsGroup(DrawRectsGroup *group)
{
	ASSERT(group->buffer->hasRangeReserved); //Attempted to finish a range while a range is not reserved!
	group->buffer->hasRangeReserved = false;

	group->buffer->currentChunk->used += group->header->count * sizeof(RenderItem_DrawRects_Item);
}
