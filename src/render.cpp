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

	initRenderBuffer(renderArena, &renderer->worldBuffer,        "WorldBuffer",        KB(64));
	initRenderBuffer(renderArena, &renderer->worldOverlayBuffer, "WorldOverlayBuffer", KB(64));
	initRenderBuffer(renderArena, &renderer->uiBuffer,           "UIBuffer",           KB(64));
	initRenderBuffer(renderArena, &renderer->debugBuffer,        "DebugBuffer",        KB(64));
}

void rendererLoadAssets(Renderer *renderer, AssetManager *assets)
{
	renderer->loadAssets(renderer, assets);

	// Cache the shader IDs so we don't have to do so many hash lookups
	renderer->shaderIds.pixelArt   = getShader(assets, makeString("pixelart.glsl"  ))->rendererShaderID;
	renderer->shaderIds.text       = getShader(assets, makeString("textured.glsl"  ))->rendererShaderID;
	renderer->shaderIds.untextured = getShader(assets, makeString("untextured.glsl"))->rendererShaderID;
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
			buffer->currentChunk->next = null;

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

template<typename T>
inline T *appendRenderItem(RenderBuffer *buffer, RenderItemType type)
{
	u8 *data = appendRenderItemInternal(buffer, type, sizeof(T), 0);
	return (T*) data;
}

void addSetShader(RenderBuffer *buffer, s8 shaderID)
{
	RenderItem_SetShader *shaderItem = appendRenderItem<RenderItem_SetShader>(buffer, RenderItemType_SetShader);
	*shaderItem = {};
	shaderItem->shaderID = shaderID;
}

void addSetTexture(RenderBuffer *buffer, Asset *texture)
{
	RenderItem_SetTexture *textureItem = appendRenderItem<RenderItem_SetTexture>(buffer, RenderItemType_SetTexture);
	*textureItem = {};
	textureItem->texture = texture;
}

void drawSingleSprite(RenderBuffer *buffer, Sprite *sprite, Rect2 bounds, s8 shaderID, V4 color)
{
	ASSERT(!buffer->hasRangeReserved);

	addSetShader(buffer, shaderID);
	addSetTexture(buffer, sprite->texture);

	RenderItem_DrawSingleRect *rect = appendRenderItem<RenderItem_DrawSingleRect>(buffer, RenderItemType_DrawSingleRect);

	rect->bounds = bounds;
	rect->color = color;
	rect->uv = sprite->uv;
}

void drawSingleRect(RenderBuffer *buffer, Rect2 bounds, s8 shaderID, V4 color)
{
	ASSERT(!buffer->hasRangeReserved);

	addSetShader(buffer, shaderID);

	RenderItem_DrawSingleRect *rect = appendRenderItem<RenderItem_DrawSingleRect>(buffer, RenderItemType_DrawSingleRect);

	rect->bounds = bounds;
	rect->color = color;
	rect->uv = {};
}

RenderItem_DrawSingleRect *appendDrawRectPlaceholder(RenderBuffer *buffer, s8 shaderID)
{
	ASSERT(!buffer->hasRangeReserved);

	addSetShader(buffer, shaderID);

	RenderItem_DrawSingleRect *rect = (RenderItem_DrawSingleRect *) appendRenderItemInternal(buffer, RenderItemType_DrawSingleRect, sizeof(RenderItem_DrawSingleRect), 0);

	return rect;
}

void fillDrawRectPlaceholder(RenderItem_DrawSingleRect *placeholder, Rect2 bounds, V4 color)
{
	placeholder->bounds = bounds;
	placeholder->color = color;
	placeholder->uv = {};
}

DrawRectsGroup *beginRectsGroupInternal(RenderBuffer *buffer, Asset *texture, s8 shaderID, s32 maxCount)
{
	ASSERT(!buffer->hasRangeReserved); //Can't reserve a range while a range is already reserved!

	addSetShader(buffer, shaderID);
	if (texture != null) addSetTexture(buffer, texture);

	DrawRectsGroup *result = allocateStruct<DrawRectsGroup>(globalFrameTempArena);
	*result = {};

	result->buffer = buffer;
	result->count = 0;
	result->maxCount = maxCount;
	result->texture = texture;

	result->firstSubGroup = beginRectsSubGroup(result);
	result->currentSubGroup = &result->firstSubGroup;

	return result;
}

DrawRectsGroup *beginRectsGroupTextured(RenderBuffer *buffer, Asset *texture, s8 shaderID, s32 maxCount)
{
	return beginRectsGroupInternal(buffer, texture, shaderID, maxCount);
}

inline DrawRectsGroup *beginRectsGroupUntextured(RenderBuffer *buffer, s8 shaderID, s32 maxCount)
{
	return beginRectsGroupInternal(buffer, null, shaderID, maxCount);
}

inline DrawRectsGroup *beginRectsGroupForText(RenderBuffer *buffer, BitmapFont *font, s8 shaderID, s32 maxCount)
{
	return beginRectsGroupInternal(buffer, font->texture, shaderID, maxCount);
}

DrawRectsSubGroup beginRectsSubGroup(DrawRectsGroup *group)
{
	DrawRectsSubGroup result = {};

	s32 subGroupItemCount = min(group->maxCount - group->count, (s32) maxRenderItemsPerGroup);
	ASSERT(subGroupItemCount > 0 && subGroupItemCount <= maxRenderItemsPerGroup);

	smm reservedSize = sizeof(RenderItem_DrawRects_Item) * subGroupItemCount;
	u8 *data = appendRenderItemInternal(group->buffer, RenderItemType_DrawRects, sizeof(RenderItem_DrawRects), reservedSize);
	group->buffer->hasRangeReserved = true;

	result.header = (RenderItem_DrawRects *) data;
	result.first  = (RenderItem_DrawRects_Item *) (data + sizeof(RenderItem_DrawRects));
	result.maxCount = subGroupItemCount;

	*result.header = {};
	result.header->count = 0;

	return result;
}

void endCurrentSubGroup(DrawRectsGroup *group)
{
	ASSERT(group->buffer->hasRangeReserved); //Attempted to finish a range while a range is not reserved!
	group->buffer->hasRangeReserved = false;

	group->buffer->currentChunk->used += group->currentSubGroup->header->count * sizeof(RenderItem_DrawRects_Item);
}

void addRectInternal(DrawRectsGroup *group, Rect2 bounds, V4 color, Rect2 uv)
{
	ASSERT(group->count < group->maxCount);

	if (group->currentSubGroup->header->count == group->currentSubGroup->maxCount)
	{
		endCurrentSubGroup(group);
		DrawRectsSubGroup *prevSubGroup = group->currentSubGroup;
		group->currentSubGroup = allocateStruct<DrawRectsSubGroup>(globalFrameTempArena);
		*group->currentSubGroup = beginRectsSubGroup(group);
		prevSubGroup->next = group->currentSubGroup;
		group->currentSubGroup->prev = prevSubGroup;
	}
	ASSERT(group->currentSubGroup->header->count < group->currentSubGroup->maxCount);

	RenderItem_DrawRects_Item *item = group->currentSubGroup->first + group->currentSubGroup->header->count++;
	item->bounds = bounds;
	item->color = color;
	item->uv = uv;

	group->count++;
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
	ASSERT(group->texture == sprite->texture);

	addRectInternal(group, bounds, color, sprite->uv);
}

void offsetRange(DrawRectsGroup *group, s32 startIndex, s32 endIndexInclusive, f32 offsetX, f32 offsetY)
{
	ASSERT(startIndex >= 0 && startIndex < group->count);
	ASSERT(endIndexInclusive >= 0 && endIndexInclusive < group->count);
	ASSERT(startIndex <= endIndexInclusive);

	s32 debugItemsUpdated = 0;

	DrawRectsSubGroup *subGroup = &group->firstSubGroup;
	s32 firstIndexInSubGroup = 0;
	// Find the first subgroup that's involved
	while (firstIndexInSubGroup + subGroup->header->count < startIndex)
	{
		firstIndexInSubGroup += subGroup->header->count;
		subGroup = subGroup->next;
	}

	// Update the items in the range
	while (firstIndexInSubGroup <= endIndexInclusive)
	{
		s32 index = startIndex - firstIndexInSubGroup;
		if (index < 0) index = 0;
		for (;
			(index <= endIndexInclusive - firstIndexInSubGroup) && (index < subGroup->header->count);
			index++)
		{
			RenderItem_DrawRects_Item *item = subGroup->first + index;
			item->bounds.x += offsetX;
			item->bounds.y += offsetY;
			debugItemsUpdated++;
		}

		firstIndexInSubGroup += subGroup->header->count;
		subGroup = subGroup->next;
	}

	ASSERT(debugItemsUpdated == (endIndexInclusive - startIndex) + 1);
}

void endRectsGroup(DrawRectsGroup *group)
{
	endCurrentSubGroup(group);
}
