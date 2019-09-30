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

void initCamera(Camera *camera, V2 size, f32 sizeRatio, f32 nearClippingPlane, f32 farClippingPlane, V2 position)
{
	camera->sizeRatio = sizeRatio;
	camera->size = size * sizeRatio;
	camera->pos = position;
	camera->zoom = 1.0f;
	camera->nearClippingPlane = nearClippingPlane;
	camera->farClippingPlane = farClippingPlane;

	updateCameraMatrix(camera);
}

void initRenderer(MemoryArena *renderArena, SDL_Window *window)
{
	renderer->window = window;

	renderer->renderBufferChunkSize = KB(64);
	initPool<RenderBufferChunk>(&renderer->chunkPool, renderArena, &allocateRenderBufferChunk, renderer);

	initRenderBuffer(renderArena, &renderer->worldBuffer,        "WorldBuffer",        &renderer->chunkPool);
	initRenderBuffer(renderArena, &renderer->worldOverlayBuffer, "WorldOverlayBuffer", &renderer->chunkPool);
	initRenderBuffer(renderArena, &renderer->uiBuffer,           "UIBuffer",           &renderer->chunkPool);
	initRenderBuffer(renderArena, &renderer->debugBuffer,        "DebugBuffer",        &renderer->chunkPool);

	// Hide cursor until stuff loads
	setCursorVisible(false);
}

void render()
{
	linkRenderBufferToNext(&renderer->worldBuffer, &renderer->worldOverlayBuffer);
	linkRenderBufferToNext(&renderer->worldOverlayBuffer, &renderer->uiBuffer);
	linkRenderBufferToNext(&renderer->uiBuffer, &renderer->debugBuffer);

	renderer->render(renderer->worldBuffer.firstChunk);

	// Return the chunks to the pool
	// NB: We can't easily shortcut this by just modifying the first/last pointers,
	// because the nextChunk list is different from the pool's list!
	// Originally I thought it's simpler to keep them separate, but that's actually making
	// me more confused.
	// Anyway, the number of RenderBufferChunks is quite low, so for now this isn't a real bottleneck.
	// - Sam, 26/07/2019
	if (renderer->worldBuffer.currentChunk != null)
	{
		for (RenderBufferChunk *chunk = renderer->worldBuffer.firstChunk;
			chunk != null;
			chunk = chunk->nextChunk)
		{
			chunk->used = 0;
			addItemToPool(chunk, &renderer->chunkPool);
		}
	}

	// Individual clean-up
	clearRenderBuffer(&renderer->worldBuffer);
	clearRenderBuffer(&renderer->worldOverlayBuffer);
	clearRenderBuffer(&renderer->uiBuffer);
	clearRenderBuffer(&renderer->debugBuffer);
}

void rendererLoadAssets()
{
	renderer->loadAssets();

	// Cache the shader IDs so we don't have to do so many hash lookups
	renderer->shaderIds.pixelArt   = getShader("pixelart.glsl"_s)->rendererShaderID;
	renderer->shaderIds.text       = getShader("textured.glsl"_s)->rendererShaderID;
	renderer->shaderIds.untextured = getShader("untextured.glsl"_s)->rendererShaderID;

	if (!isEmpty(renderer->currentCursorName))
	{
		setCursor(renderer->currentCursorName);
	}
}

void rendererUnloadAssets()
{
	if (renderer->systemWaitCursor == null)
	{
		renderer->systemWaitCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
	}
	SDL_SetCursor(renderer->systemWaitCursor);
	renderer->unloadAssets();
}

void freeRenderer()
{
	if (renderer->systemWaitCursor != null)
	{
		SDL_FreeCursor(renderer->systemWaitCursor);
		renderer->systemWaitCursor = null;
	}
	renderer->free();
}

void onWindowResized(s32 w, s32 h)
{
	renderer->windowResized(w, h);
	V2 windowSize = v2(w, h);

	renderer->worldCamera.size = windowSize * renderer->worldCamera.sizeRatio;

	renderer->uiCamera.size = windowSize * renderer->uiCamera.sizeRatio;
	renderer->uiCamera.pos = renderer->uiCamera.size * 0.5f;
}

void resizeWindow(s32 w, s32 h, bool fullscreen)
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

	onWindowResized(newW, newH);
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

void setCursor(String cursorName)
{
	DEBUG_FUNCTION();
	
	Asset *newCursorAsset = getAsset(AssetType_Cursor, cursorName);
	if (newCursorAsset != null)
	{
		renderer->currentCursorName = cursorName;
		SDL_SetCursor(newCursorAsset->cursor.sdlCursor);
	}
}

void setCursorVisible(bool visible)
{
	renderer->cursorIsVisible = visible;
	SDL_ShowCursor(visible ? 1 : 0);
}

void initRenderBuffer(MemoryArena *arena, RenderBuffer *buffer, char *name, Pool<RenderBufferChunk> *chunkPool)
{
	*buffer = {};

	buffer->name = pushString(arena, name);
	hashString(&buffer->name);

	buffer->renderProfileName = pushString(arena, myprintf("render({0})"_s, {buffer->name}));
	hashString(&buffer->renderProfileName);

	buffer->hasRangeReserved = false;
	buffer->hasScissorEnabled = false;

	buffer->arena = arena; // @Cleanup: Don't need this now we're using the pool?
	buffer->chunkPool = chunkPool;

	buffer->firstChunk = null;
	buffer->currentChunk = null;

	buffer->currentShader = -1;
	buffer->currentTexture = null;

	// TODO: @Speed: This should probably be delayed until we first try to append an item to this buffer.
	// Right now, we're reallocating a chunk we just freed, and (potentially) only to hold this one marker!
	// See also below
	RenderItem_SectionMarker *bufferStart = appendRenderItem<RenderItem_SectionMarker>(buffer, RenderItemType_SectionMarker);
	bufferStart->name = buffer->name;
	bufferStart->renderProfileName = buffer->renderProfileName;
}

RenderBufferChunk *allocateRenderBufferChunk(MemoryArena *arena, void *userData)
{
	smm newChunkSize = ((Renderer*)userData)->renderBufferChunkSize;
	RenderBufferChunk *result = (RenderBufferChunk *)allocate(arena, newChunkSize + sizeof(RenderBufferChunk));
	result->size = newChunkSize;
	result->used = 0;
	result->memory = (u8*)(result + 1);

	return result;
}

void clearRenderBuffer(RenderBuffer *buffer)
{
	buffer->currentShader = -1;
	buffer->currentTexture = null;

	buffer->firstChunk = null;
	buffer->currentChunk = null;
	
	// TODO: @Speed: See above
	RenderItem_SectionMarker *bufferStart = appendRenderItem<RenderItem_SectionMarker>(buffer, RenderItemType_SectionMarker);
	bufferStart->name = buffer->name;
	bufferStart->renderProfileName = buffer->renderProfileName;
}

inline void appendRenderItemType(RenderBuffer *buffer, RenderItemType type)
{
	*(RenderItemType *)(buffer->currentChunk->memory + buffer->currentChunk->used) = type;
	buffer->currentChunk->used += sizeof(RenderItemType);
}

void linkRenderBufferToNext(RenderBuffer *buffer, RenderBuffer *nextBuffer)
{
	// TODO: @Speed: Could optimise this by skipping empty buffers, so we just jump straight
	// to the nextBuffer. However, right now every buffer has a chunk in which contains its name,
	// so currentChunk is never null anyway.
	// - Sam, 26/07/2019
	ASSERT(buffer->currentChunk != null);
	ASSERT((buffer->currentChunk->size - buffer->currentChunk->used) > sizeof(RenderItemType)); // Need space for the next-chunk message
	appendRenderItemType(buffer, RenderItemType_NextMemoryChunk);

	// Add to the renderbuffer
	buffer->currentChunk->nextChunk = nextBuffer->firstChunk;
	nextBuffer->firstChunk->prevChunk = buffer->currentChunk;
}

// NB: reservedSize is for extra data that you want to make sure there is room for,
// but you're not allocating it right away. Make sure not to do any other allocations
// in between, and make sure to allocate the space you used when you're done!
u8 *appendRenderItemInternal(RenderBuffer *buffer, RenderItemType type, smm size, smm reservedSize)
{
	ASSERT(!buffer->hasRangeReserved); //Can't append renderitems while a range is reserved!

	smm totalSizeRequired = (smm)(sizeof(RenderItemType) + size + reservedSize + sizeof(RenderItemType));

	if (buffer->currentChunk == null || ((buffer->currentChunk->size - buffer->currentChunk->used) <= totalSizeRequired))
	{
		// Out of room! Push a "go to next chunk" item and append some more memory
		if (buffer->currentChunk != null)
		{
			ASSERT((buffer->currentChunk->size - buffer->currentChunk->used) > sizeof(RenderItemType)); // Need space for the next-chunk message
			appendRenderItemType(buffer, RenderItemType_NextMemoryChunk);
		}

		RenderBufferChunk *newChunk = getItemFromPool(buffer->chunkPool);
		newChunk->used = 0;
		newChunk->prevChunk = null;
		newChunk->nextChunk = null;

		// Add to the renderbuffer
		if (buffer->currentChunk == null)
		{
			buffer->firstChunk = newChunk;
			buffer->currentChunk = newChunk;
		}
		else
		{
			buffer->currentChunk->nextChunk = newChunk;
			newChunk->prevChunk = buffer->currentChunk;
		}

		buffer->currentChunk = newChunk;
	}

	appendRenderItemType(buffer, type);
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

template<typename T>
inline T *readRenderItem(RenderBufferChunk *renderBufferChunk, smm *pos)
{
	T *item = (T *)(renderBufferChunk->memory + *pos);
	*pos += sizeof(T);
	return item;
}

void addSetCamera(RenderBuffer *buffer, Camera *camera)
{
	RenderItem_SetCamera *cameraItem = appendRenderItem<RenderItem_SetCamera>(buffer, RenderItemType_SetCamera);
	cameraItem->camera = camera;

	buffer->currentCamera = camera;
}

void addSetShader(RenderBuffer *buffer, s8 shaderID)
{
	if (buffer->currentShader != shaderID)
	{
		RenderItem_SetShader *shaderItem = appendRenderItem<RenderItem_SetShader>(buffer, RenderItemType_SetShader);
		*shaderItem = {};
		shaderItem->shaderID = shaderID;

		buffer->currentShader = shaderID;
		// NB: Setting the opengl shader loses the texture, so we replicate that here.
		// I suppose in the future we could make sure to bind the texture when the shader changes, but
		// most of the time we want to set a new texture anyway, so that would be a waste of cpu time.
		buffer->currentTexture = null;
	}
}

void addSetTexture(RenderBuffer *buffer, Asset *texture)
{
	if (buffer->currentTexture != texture)
	{
		RenderItem_SetTexture *textureItem = appendRenderItem<RenderItem_SetTexture>(buffer, RenderItemType_SetTexture);
		*textureItem = {};
		textureItem->texture = texture;

		buffer->currentTexture = texture;
	}
}

void addClear(RenderBuffer *buffer, V4 clearColor)
{
	RenderItem_Clear *clear = appendRenderItem<RenderItem_Clear>(buffer, RenderItemType_Clear);
	clear->clearColor = clearColor;
}

void addBeginScissor(RenderBuffer *buffer, Rect2 bounds)
{
	ASSERT(!buffer->hasScissorEnabled);
	RenderItem_BeginScissor *scissor = appendRenderItem<RenderItem_BeginScissor>(buffer, RenderItemType_BeginScissor);

	//
	// Project the bounds using the camera matrix.
	// This is foolish and fraught with peril! GPU scissors only accept axis-aligned rectangles,
	// so if our matrix does something else then we'll have a bad time. (Well, it'll probably
	// look really cool, but I'd rather things only look cool when I tell them to.)
	// If we do want to have wacky-angled or funky-shaped scissor areas, we can use a stencil
	// buffer instead.
	//
	// - Sam, 23/09/2019
	//

	V2 minCorner = bounds.pos;
	V2 maxCorner = bounds.pos + bounds.size;

	V4 projectedMin = buffer->currentCamera->projectionMatrix * v4(minCorner.x, minCorner.y, 0.0f, 1.0f);
	V4 projectedMax = buffer->currentCamera->projectionMatrix * v4(maxCorner.x, maxCorner.y, 0.0f, 1.0f);

	s32 screenWidth  = inputState->windowWidth;
	s32 screenHeight = inputState->windowHeight;

	// At this point we're in -1 to 1 coordinates

	// So, we need to map back into pixel space

	// Also, note that even though these are called min/max, they might be the opposite way around
	// because of the projection matrix! We're just calling them that to match minCorner/maxCorner above.

	s32 screenMinX = floor_s32((projectedMin.x + 1.0f) * (screenWidth  * 0.5f));
	s32 screenMinY = floor_s32((projectedMin.y + 1.0f) * (screenHeight * 0.5f));
	s32 screenMaxX =  ceil_s32((projectedMax.x + 1.0f) * (screenWidth  * 0.5f));
	s32 screenMaxY =  ceil_s32((projectedMax.y + 1.0f) * (screenHeight * 0.5f));

	s32 width  = abs_s32(screenMinX - screenMaxX);
	s32 height = abs_s32(screenMinY - screenMaxY);

	Rect2I resultBounds = irectXYWH(min(screenMinX, screenMaxX), min(screenMinY, screenMaxY), width, height);

	// Crop it to be within the screen bounds
	scissor->bounds = intersect(resultBounds, irectXYWH(0, 0, inputState->windowWidth, inputState->windowHeight));

	buffer->hasScissorEnabled = true;
}

void addEndScissor(RenderBuffer *buffer)
{
	ASSERT(buffer->hasScissorEnabled);
	RenderItem_EndScissor *scissor = appendRenderItem<RenderItem_EndScissor>(buffer, RenderItemType_EndScissor);
	scissor = scissor; // Unused
	buffer->hasScissorEnabled = false;
}

void drawSingleSprite(RenderBuffer *buffer, Sprite *sprite, Rect2 bounds, s8 shaderID, V4 color)
{
	addSetShader(buffer, shaderID);
	addSetTexture(buffer, sprite->texture);

	RenderItem_DrawSingleRect *rect = appendRenderItem<RenderItem_DrawSingleRect>(buffer, RenderItemType_DrawSingleRect);

	rect->bounds = bounds;
	rect->color = color;
	rect->uv = sprite->uv;
}

void drawSingleRect(RenderBuffer *buffer, Rect2 bounds, s8 shaderID, V4 color)
{
	addSetShader(buffer, shaderID);

	RenderItem_DrawSingleRect *rect = appendRenderItem<RenderItem_DrawSingleRect>(buffer, RenderItemType_DrawSingleRect);

	rect->bounds = bounds;
	rect->color = color;
	rect->uv = {};
}

void drawSingleRect(RenderBuffer *buffer, Rect2I bounds, s8 shaderID, V4 color)
{
	addSetShader(buffer, shaderID);

	RenderItem_DrawSingleRect *rect = appendRenderItem<RenderItem_DrawSingleRect>(buffer, RenderItemType_DrawSingleRect);

	rect->bounds = rectXYWHi(bounds.x, bounds.y, bounds.w, bounds.h);
	rect->color = color;
	rect->uv = {};
}

RenderItem_DrawSingleRect *appendDrawRectPlaceholder(RenderBuffer *buffer, s8 shaderID)
{
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

void fillDrawRectPlaceholder(RenderItem_DrawSingleRect *placeholder, Rect2I bounds, V4 color)
{
	placeholder->bounds = rectXYWHi(bounds.x, bounds.y, bounds.w, bounds.h);
	placeholder->color = color;
	placeholder->uv = {};
}

DrawRectsGroup *beginRectsGroupInternal(RenderBuffer *buffer, Asset *texture, s8 shaderID, s32 maxCount)
{
	addSetShader(buffer, shaderID);
	if (texture != null) addSetTexture(buffer, texture);

	DrawRectsGroup *result = allocateStruct<DrawRectsGroup>(tempArena);
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
		group->currentSubGroup = allocateStruct<DrawRectsSubGroup>(tempArena);
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
	DEBUG_FUNCTION_T(DCDT_Renderer);

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

void drawGrid(RenderBuffer *buffer, Rect2 bounds, s8 shaderID, s32 gridW, s32 gridH, u8 *grid, u16 paletteSize, V4 *palette)
{
	DEBUG_FUNCTION_T(DCDT_Renderer);

	addSetShader(buffer, shaderID);

	smm size = sizeof(RenderItem_DrawGrid)
			 + (gridW * gridH * sizeof(grid[0]))
			 + (paletteSize * sizeof(palette[0]));

	u8 *data = appendRenderItemInternal(buffer, RenderItemType_DrawGrid, size, 0);

	RenderItem_DrawGrid *gridItem = (RenderItem_DrawGrid *) data;
	data += sizeof(RenderItem_DrawGrid);
	gridItem->bounds = bounds;
	gridItem->paletteSize = paletteSize;
	gridItem->gridW = gridW;
	gridItem->gridH = gridH;

	u8 *gridData = (u8 *) data;
	data += (gridW * gridH * sizeof(grid[0]));
	copyMemory(grid, gridData, (gridW * gridH));

	V4 *paletteData = (V4 *) data;
	copyMemory(palette, paletteData, paletteSize);
}

DrawRingsGroup *beginRingsGroup(RenderBuffer *buffer, s32 maxCount)
{
	// @Copypasta beginRectsGroupInternal() - maybe factor out common "grouped render items" code?
	addSetShader(buffer, renderer->shaderIds.untextured);

	DrawRingsGroup *result = allocateStruct<DrawRingsGroup>(tempArena);
	*result = {};

	result->buffer = buffer;
	result->count = 0;
	result->maxCount = maxCount;

	result->firstSubGroup = beginRingsSubGroup(result);
	result->currentSubGroup = &result->firstSubGroup;

	return result;
}

DrawRingsSubGroup beginRingsSubGroup(DrawRingsGroup *group)
{
	// @Copypasta beginRectsSubGroup() - maybe factor out common "grouped render items" code?
	DrawRingsSubGroup result = {};

	s32 subGroupItemCount = min(group->maxCount - group->count, (s32) maxRenderItemsPerGroup);
	ASSERT(subGroupItemCount > 0 && subGroupItemCount <= maxRenderItemsPerGroup);

	smm reservedSize = sizeof(RenderItem_DrawRings_Item) * subGroupItemCount;
	u8 *data = appendRenderItemInternal(group->buffer, RenderItemType_DrawRings, sizeof(RenderItem_DrawRings), reservedSize);
	group->buffer->hasRangeReserved = true;

	result.header = (RenderItem_DrawRings *) data;
	result.first  = (RenderItem_DrawRings_Item *) (data + sizeof(RenderItem_DrawRings));
	result.maxCount = subGroupItemCount;

	*result.header = {};
	result.header->count = 0;

	return result;
}

void endCurrentSubGroup(DrawRingsGroup *group)
{
	// @Copypasta endCurrentSubGroup(DrawRectsGroup*) - maybe factor out common "grouped render items" code?
	ASSERT(group->buffer->hasRangeReserved); //Attempted to finish a range while a range is not reserved!
	group->buffer->hasRangeReserved = false;

	group->buffer->currentChunk->used += group->currentSubGroup->header->count * sizeof(RenderItem_DrawRings_Item);
}

void addRing(DrawRingsGroup *group, V2 centre, f32 radius, f32 thickness, V4 color)
{
	// @Copypasta addRectInternal() - maybe factor out common "grouped render items" code?
	ASSERT(group->count < group->maxCount);

	if (group->currentSubGroup->header->count == group->currentSubGroup->maxCount)
	{
		endCurrentSubGroup(group);
		DrawRingsSubGroup *prevSubGroup = group->currentSubGroup;
		group->currentSubGroup = allocateStruct<DrawRingsSubGroup>(tempArena);
		*group->currentSubGroup = beginRingsSubGroup(group);
		prevSubGroup->next = group->currentSubGroup;
		group->currentSubGroup->prev = prevSubGroup;
	}
	ASSERT(group->currentSubGroup->header->count < group->currentSubGroup->maxCount);

	RenderItem_DrawRings_Item *item = group->currentSubGroup->first + group->currentSubGroup->header->count++;
	item->centre = centre;
	item->radius = radius;
	item->thickness = thickness;
	item->color = color;

	group->count++;
}

void endRingsGroup(DrawRingsGroup *group)
{
	endCurrentSubGroup(group);
}
