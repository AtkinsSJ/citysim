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

void setCameraPos(Camera *camera, V2 position, f32 zoom)
{
	camera->pos = position;
	camera->zoom = snapZoomLevel(zoom);

	updateCameraMatrix(camera);
}

inline f32 snapZoomLevel(f32 zoom)
{
	return (f32) clamp(round_f32(10 * zoom) * 0.1f, 0.1f, 10.0f);
}

void initRenderer(MemoryArena *renderArena, SDL_Window *window)
{
	renderer->window = window;

	renderer->renderBufferChunkSize = KB(64);
	initPool<RenderBufferChunk>(&renderer->chunkPool, renderArena, &allocateRenderBufferChunk, renderer);

	initPool<RenderBuffer>(&renderer->renderBufferPool, renderArena,
		[](MemoryArena *arena, void *) -> RenderBuffer*
		{
			RenderBuffer *buffer = allocateStruct<RenderBuffer>(arena);
			initRenderBuffer(arena, buffer, nullString, &renderer->chunkPool);
			return buffer;
		}, renderer);
	initRenderBuffer(renderArena, &renderer->worldBuffer,        "WorldBuffer"_s,        &renderer->chunkPool);
	initRenderBuffer(renderArena, &renderer->worldOverlayBuffer, "WorldOverlayBuffer"_s, &renderer->chunkPool);
	initRenderBuffer(renderArena, &renderer->uiBuffer,           "UIBuffer"_s,           &renderer->chunkPool);
	initRenderBuffer(renderArena, &renderer->windowBuffer,       "WindowBuffer"_s,       &renderer->chunkPool);
	initRenderBuffer(renderArena, &renderer->debugBuffer,        "DebugBuffer"_s,        &renderer->chunkPool);

	renderer->renderBuffers = allocateArray<RenderBuffer *>(renderArena, 5);
	renderer->renderBuffers.append(&renderer->worldBuffer);
	renderer->renderBuffers.append(&renderer->worldOverlayBuffer);
	renderer->renderBuffers.append(&renderer->uiBuffer);
	renderer->renderBuffers.append(&renderer->windowBuffer);
	renderer->renderBuffers.append(&renderer->debugBuffer);

	// Hide cursor until stuff loads
	setCursorVisible(false);
}

void render()
{
	DEBUG_POOL(&renderer->renderBufferPool, "renderBufferPool");
	DEBUG_POOL(&renderer->chunkPool, "renderChunkPool");

	renderer->render(renderer->renderBuffers);

	for (s32 i = 0; i < renderer->renderBuffers.count; i++)
	{
		clearRenderBuffer(renderer->renderBuffers[i]);
	}
}

void rendererLoadAssets()
{
	renderer->loadAssets();

	// Cache the shader IDs so we don't have to do so many hash lookups
	renderer->shaderIds.pixelArt   = getShader("pixelart.glsl"_s)->rendererShaderID;
	renderer->shaderIds.text       = getShader("textured.glsl"_s)->rendererShaderID;
	renderer->shaderIds.textured   = getShader("textured.glsl"_s)->rendererShaderID;
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

	// So, I'm not super sure how we want to handle fullscreen. Do we scan the
	// available resolution options and list them to select from? Do we just use
	// the native resolution? Do we use fullscreen or fullscreen-window?
	// 
	// For now, I'm thinking we use the native resolution, as a fullscreen window.
	// - Sam, 20/05/2021
	if (fullscreen)
	{
		// Fullscreen!
		// Grab the desktop resolution to use that
		s32 displayIndex = SDL_GetWindowDisplayIndex(renderer->window);
		SDL_DisplayMode displayMode;
		if (SDL_GetDesktopDisplayMode(displayIndex, &displayMode) == 0)
		{
			SDL_SetWindowDisplayMode(renderer->window, &displayMode);
			SDL_SetWindowFullscreen(renderer->window, SDL_WINDOW_FULLSCREEN_DESKTOP);

			// NB: Because InputState relies on SDL_WINDOWEVENT_RESIZED events,
			// it simplifies things massively if we generate one ourselves.
			SDL_Event resizeEvent = {};
			resizeEvent.type = SDL_WINDOWEVENT;
			resizeEvent.window.event = SDL_WINDOWEVENT_RESIZED;
			resizeEvent.window.data1 = displayMode.w;
			resizeEvent.window.data2 = displayMode.h;
			SDL_PushEvent(&resizeEvent);

			onWindowResized(displayMode.w, displayMode.h);
		}
		else
		{
			logError("Failed to get desktop display mode: {0}"_s, {makeString(SDL_GetError())});
		}
	}
	else
	{
		// Window!
		SDL_SetWindowSize(renderer->window, w, h);
		SDL_SetWindowFullscreen(renderer->window, 0);

		// Centre it
		s32 displayIndex = SDL_GetWindowDisplayIndex(renderer->window);
		SDL_Rect displayBounds;
		if (SDL_GetDisplayBounds(displayIndex, &displayBounds) == 0)
		{
			s32 leftBorder, rightBorder, topBorder, bottomBorder;
			SDL_GetWindowBordersSize(renderer->window, &topBorder, &leftBorder, &bottomBorder, &rightBorder);

			s32 windowW = w + leftBorder + rightBorder;
			s32 windowH = h + topBorder + bottomBorder;
			s32 windowLeft = (displayBounds.w - windowW) / 2;
			s32 windowTop  = (displayBounds.h - windowH) / 2;

			SDL_SetWindowPosition(renderer->window, displayBounds.x + windowLeft, displayBounds.y + windowTop);
		}
		else
		{
			logError("Failed to get display bounds: {0}"_s, {makeString(SDL_GetError())});

			// As a backup, just centre it on the main display
			SDL_SetWindowPosition(renderer->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		}

		// NB: Because InputState relies on SDL_WINDOWEVENT_RESIZED events,
		// it simplifies things massively if we generate one ourselves.
		SDL_Event resizeEvent = {};
		resizeEvent.type = SDL_WINDOWEVENT;
		resizeEvent.window.event = SDL_WINDOWEVENT_RESIZED;
		resizeEvent.window.data1 = w;
		resizeEvent.window.data2 = h;
		SDL_PushEvent(&resizeEvent);

		onWindowResized(w, h);
	}
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

void initRenderBuffer(MemoryArena *arena, RenderBuffer *buffer, String name, Pool<RenderBufferChunk> *chunkPool)
{
	*buffer = {};

	buffer->name = pushString(arena, name);
	hashString(&buffer->name);

	buffer->hasRangeReserved = false;
	buffer->scissorCount = 0;

	buffer->chunkPool = chunkPool;

	buffer->firstChunk = null;
	buffer->currentChunk = null;

	buffer->currentShader = -1;
	buffer->currentTexture = null;
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

	for (RenderBufferChunk *chunk = buffer->firstChunk;
		chunk != null;
		chunk = chunk->nextChunk)
	{
		chunk->used = 0;
		addItemToPool(&renderer->chunkPool, chunk);
	}

	buffer->firstChunk = null;
	buffer->currentChunk = null;
}

inline RenderBuffer *getTemporaryRenderBuffer(String name)
{
	RenderBuffer *result = getItemFromPool(&renderer->renderBufferPool);

	// We only use this for debugging, and it's a leak, so limit it to debug builds
	if (globalDebugState != null)
	{
		result->name = pushString(&globalDebugState->debugArena, name);
	}

	return result;
}

void returnTemporaryRenderBuffer(RenderBuffer *buffer)
{
	buffer->name = nullString;
	clearRenderBuffer(buffer);
	addItemToPool(&renderer->renderBufferPool, buffer);
}

void transferRenderBufferData(RenderBuffer *buffer, RenderBuffer *targetBuffer)
{
	if (buffer->currentChunk != null)
	{
		// If the target is empty, we can take a shortcut and just copy the pointers
		if (targetBuffer->currentChunk == null)
		{
			targetBuffer->firstChunk = buffer->firstChunk;
			targetBuffer->currentChunk = buffer->currentChunk;
		}
		else
		{
			ASSERT((targetBuffer->currentChunk->size - targetBuffer->currentChunk->used) > sizeof(RenderItemType)); // Need space for the next-chunk message
			appendRenderItemType(targetBuffer, RenderItemType_NextMemoryChunk);
			targetBuffer->currentChunk->nextChunk = buffer->firstChunk;
			buffer->firstChunk->prevChunk = targetBuffer->currentChunk;
			targetBuffer->currentChunk = buffer->currentChunk;
		}
	}

	buffer->firstChunk = null;
	buffer->currentChunk = null;
}

inline void appendRenderItemType(RenderBuffer *buffer, RenderItemType type)
{
	*(RenderItemType *)(buffer->currentChunk->memory + buffer->currentChunk->used) = type;
	buffer->currentChunk->used += sizeof(RenderItemType);
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
RenderItemAndData<T> appendRenderItem(RenderBuffer *buffer, RenderItemType type, smm dataSize)
{
	RenderItemAndData<T> result = {};

	u8 *bufferData = appendRenderItemInternal(buffer, type, sizeof(T) + dataSize, 0);
	result.item = (T *) bufferData;
	result.data = bufferData + sizeof(T);

	return result;
}

template<typename T>
inline T *readRenderItem(RenderBufferChunk *renderBufferChunk, smm *pos)
{
	T *item = (T *)(renderBufferChunk->memory + *pos);
	*pos += sizeof(T);
	return item;
}

template<typename T>
inline T *readRenderData(RenderBufferChunk *renderBufferChunk, smm *pos, s32 count)
{
	T *item = (T *)(renderBufferChunk->memory + *pos);
	*pos += sizeof(T) * count;
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
	ASSERT(texture->state == AssetState_Loaded);
	
	if (buffer->currentTexture != texture)
	{
		RenderItem_SetTexture *textureItem = appendRenderItem<RenderItem_SetTexture>(buffer, RenderItemType_SetTexture);
		*textureItem = {};
		textureItem->texture = texture;

		buffer->currentTexture = texture;
	}
}

void addSetTextureRaw(RenderBuffer *buffer, s32 width, s32 height, u8 bytesPerPixel, u8 *pixels)
{
	buffer->currentTexture = null;

	smm pixelDataSize = (width * height * bytesPerPixel);
	auto itemAndData = appendRenderItem<RenderItem_SetTexture>(buffer, RenderItemType_SetTexture, pixelDataSize);

	RenderItem_SetTexture *textureItem = itemAndData.item;
	*textureItem = {};
	textureItem->texture = null;
	textureItem->width = width;
	textureItem->height = height;
	textureItem->bytesPerPixel = bytesPerPixel;

	copyMemory(pixels, itemAndData.data, pixelDataSize);
}

void addSetPalette(RenderBuffer *buffer, s32 paletteSize, V4 *palette)
{
	auto itemAndData = appendRenderItem<RenderItem_SetPalette>(buffer, RenderItemType_SetPalette, sizeof(V4) * paletteSize);

	itemAndData.item->paletteSize = paletteSize;

	copyMemory(palette, (V4*) itemAndData.data, paletteSize);
}

void addClear(RenderBuffer *buffer, V4 clearColor)
{
	RenderItem_Clear *clear = appendRenderItem<RenderItem_Clear>(buffer, RenderItemType_Clear);
	clear->clearColor = clearColor;
}

void addBeginScissor(RenderBuffer *buffer, Rect2I bounds)
{
	ASSERT(bounds.w >= 0);
	ASSERT(bounds.h >= 0);

	RenderItem_BeginScissor *scissor = appendRenderItem<RenderItem_BeginScissor>(buffer, RenderItemType_BeginScissor);

	// We have to flip the bounds rectangle vertically because OpenGL has the origin in the bottom-left,
	// whereas our system uses the top-left!

	bounds.y = inputState->windowHeight - bounds.y - bounds.h;

	// Crop to window bounds
	scissor->bounds = intersect(bounds, irectXYWH(0, 0, inputState->windowWidth, inputState->windowHeight));

	ASSERT(scissor->bounds.w >= 0);
	ASSERT(scissor->bounds.h >= 0);

	buffer->scissorCount++;
}

void addEndScissor(RenderBuffer *buffer)
{
	ASSERT(buffer->scissorCount > 0);
	RenderItem_EndScissor *scissor = appendRenderItem<RenderItem_EndScissor>(buffer, RenderItemType_EndScissor);
	scissor = scissor; // Unused
	buffer->scissorCount--;
}

void drawSingleSprite(RenderBuffer *buffer, Sprite *sprite, Rect2 bounds, s8 shaderID, V4 color)
{
	addSetShader(buffer, shaderID);
	addSetTexture(buffer, sprite->texture);

	RenderItem_DrawSingleRect *rect = appendRenderItem<RenderItem_DrawSingleRect>(buffer, RenderItemType_DrawSingleRect);

	rect->bounds = bounds;
	rect->color00 = color;
	rect->color01 = color;
	rect->color10 = color;
	rect->color11 = color;
	rect->uv = sprite->uv;
}

inline void drawSingleRect(RenderBuffer *buffer, Rect2 bounds, s8 shaderID, V4 color)
{
	drawSingleRect(buffer, bounds, shaderID, color, color, color, color);
}

inline void drawSingleRect(RenderBuffer *buffer, Rect2I bounds, s8 shaderID, V4 color)
{
	drawSingleRect(buffer, rect2(bounds), shaderID, color, color, color, color);
}

void drawSingleRect(RenderBuffer *buffer, Rect2 bounds, s8 shaderID, V4 color00, V4 color01, V4 color10, V4 color11)
{
	addSetShader(buffer, shaderID);

	RenderItem_DrawSingleRect *rect = appendRenderItem<RenderItem_DrawSingleRect>(buffer, RenderItemType_DrawSingleRect);

	rect->bounds = bounds;
	rect->color00 = color00;
	rect->color01 = color01;
	rect->color10 = color10;
	rect->color11 = color11;
	rect->uv = rectXYWH(0.0f, 0.0f, 1.0f, 1.0f);
}

inline void drawSingleRect(RenderBuffer *buffer, Rect2I bounds, s8 shaderID, V4 color00, V4 color01, V4 color10, V4 color11)
{
	drawSingleRect(buffer, rect2(bounds), shaderID, color00, color01, color10, color11);
}

DrawRectPlaceholder appendDrawRectPlaceholder(RenderBuffer *buffer, s8 shaderID, bool hasTexture)
{
	addSetShader(buffer, shaderID);

	DrawRectPlaceholder result = {};

	if (hasTexture)
	{
		RenderItem_SetTexture *textureItem = appendRenderItem<RenderItem_SetTexture>(buffer, RenderItemType_SetTexture);
		*textureItem = {};

		// We need to clear this, because we don't know what this texture will be, so any following draw calls
		// have to assume that they need to set their texture
		buffer->currentTexture = null;

		result.setTexture = textureItem;
	}

	result.drawRect = appendRenderItem<RenderItem_DrawSingleRect>(buffer, RenderItemType_DrawSingleRect);

	return result;
}

inline void fillDrawRectPlaceholder(DrawRectPlaceholder *placeholder, Rect2 bounds, V4 color)
{
	fillDrawRectPlaceholder(placeholder, bounds, color, color, color, color);
}

inline void fillDrawRectPlaceholder(DrawRectPlaceholder *placeholder, Rect2I bounds, V4 color)
{
	fillDrawRectPlaceholder(placeholder, rect2(bounds), color, color, color, color);
}

void fillDrawRectPlaceholder(DrawRectPlaceholder *placeholder, Rect2 bounds, V4 color00, V4 color01, V4 color10, V4 color11)
{
	RenderItem_DrawSingleRect *rect = placeholder->drawRect;

	rect->bounds = bounds;
	rect->color00 = color00;
	rect->color01 = color01;
	rect->color10 = color10;
	rect->color11 = color11;
	rect->uv = rectXYWH(0.0f, 0.0f, 1.0f, 1.0f);
}

inline void fillDrawRectPlaceholder(DrawRectPlaceholder *placeholder, Rect2I bounds, V4 color00, V4 color01, V4 color10, V4 color11)
{
	fillDrawRectPlaceholder(placeholder, rect2(bounds), color00, color01, color10, color11);
}

void fillDrawRectPlaceholder(DrawRectPlaceholder *placeholder, Rect2 bounds, Sprite *sprite, V4 color)
{
	ASSERT(placeholder->setTexture != null);

	placeholder->setTexture->texture = sprite->texture;

	RenderItem_DrawSingleRect *rect = placeholder->drawRect;
	rect->bounds = bounds;
	rect->color00 = color;
	rect->color01 = color;
	rect->color10 = color;
	rect->color11 = color;
	rect->uv = sprite->uv;
}

void drawNinepatch(RenderBuffer *buffer, Rect2I bounds, s8 shaderID, Ninepatch *ninepatch, V4 color)
{
	DrawRectsGroup *group = beginRectsGroupInternal(buffer, ninepatch->texture, shaderID, 9);

	// NB: See comments in the Ninepatch struct for how we could avoid doing the  UV calculations repeatedly,
	// and why we haven't done so.

	f32 x0 = (f32)(bounds.x);
	f32 x1 = (f32)(bounds.x + ninepatch->pu1 - ninepatch->pu0);
	f32 x2 = (f32)(bounds.x + bounds.w - (ninepatch->pu3 - ninepatch->pu2));
	f32 x3 = (f32)(bounds.x + bounds.w);

	f32 y0 = (f32)(bounds.y);
	f32 y1 = (f32)(bounds.y + ninepatch->pv1 - ninepatch->pv0);
	f32 y2 = (f32)(bounds.y + bounds.h - (ninepatch->pv3 - ninepatch->pv2));
	f32 y3 = (f32)(bounds.y + bounds.h);

	// top left
	addRectInternal(group, rectMinMax(x0, y0, x1, y1), color, rectMinMax(ninepatch->u0, ninepatch->v0, ninepatch->u1, ninepatch->v1));

	// top
	addRectInternal(group, rectMinMax(x1, y0, x2, y1), color, rectMinMax(ninepatch->u1, ninepatch->v0, ninepatch->u2, ninepatch->v1));

	// top-right
	addRectInternal(group, rectMinMax(x2, y0, x3, y1), color, rectMinMax(ninepatch->u2, ninepatch->v0, ninepatch->u3, ninepatch->v1));

	// middle left
	addRectInternal(group, rectMinMax(x0, y1, x1, y2), color, rectMinMax(ninepatch->u0, ninepatch->v1, ninepatch->u1, ninepatch->v2));

	// middle
	addRectInternal(group, rectMinMax(x1, y1, x2, y2), color, rectMinMax(ninepatch->u1, ninepatch->v1, ninepatch->u2, ninepatch->v2));

	// middle-right
	addRectInternal(group, rectMinMax(x2, y1, x3, y2), color, rectMinMax(ninepatch->u2, ninepatch->v1, ninepatch->u3, ninepatch->v2));

	// bottom left
	addRectInternal(group, rectMinMax(x0, y2, x1, y3), color, rectMinMax(ninepatch->u0, ninepatch->v2, ninepatch->u1, ninepatch->v3));

	// bottom
	addRectInternal(group, rectMinMax(x1, y2, x2, y3), color, rectMinMax(ninepatch->u1, ninepatch->v2, ninepatch->u2, ninepatch->v3));

	// bottom-right
	addRectInternal(group, rectMinMax(x2, y2, x3, y3), color, rectMinMax(ninepatch->u2, ninepatch->v2, ninepatch->u3, ninepatch->v3));


	endRectsGroup(group);
}

DrawNinepatchPlaceholder appendDrawNinepatchPlaceholder(RenderBuffer *buffer, Asset *texture, s8 shaderID)
{
	DrawNinepatchPlaceholder placeholder = {};

	DrawRectsGroup *rectsGroup = beginRectsGroupTextured(buffer, texture, shaderID, 9);
	placeholder.firstRect = reserve(rectsGroup, 9);
	endRectsGroup(rectsGroup);

	return placeholder;
}

void fillDrawNinepatchPlaceholder(DrawNinepatchPlaceholder *placeholder, Rect2I bounds, Ninepatch *ninepatch, V4 color)
{
	// @Copypasta drawNinepatch()
	// NB: See comments in the Ninepatch struct for how we could avoid doing the  UV calculations repeatedly,
	// and why we haven't done so.

	f32 x0 = (f32)(bounds.x);
	f32 x1 = (f32)(bounds.x + ninepatch->pu1 - ninepatch->pu0);
	f32 x2 = (f32)(bounds.x + bounds.w - (ninepatch->pu3 - ninepatch->pu2));
	f32 x3 = (f32)(bounds.x + bounds.w);

	f32 y0 = (f32)(bounds.y);
	f32 y1 = (f32)(bounds.y + ninepatch->pv1 - ninepatch->pv0);
	f32 y2 = (f32)(bounds.y + bounds.h - (ninepatch->pv3 - ninepatch->pv2));
	f32 y3 = (f32)(bounds.y + bounds.h);

	RenderItem_DrawRects_Item *rect = placeholder->firstRect;

	// top left
	rect->bounds = rectMinMax(x0, y0, x1, y1);
	rect->color = color;
	rect->uv = rectMinMax(ninepatch->u0, ninepatch->v0, ninepatch->u1, ninepatch->v1);
	rect++;

	// top
	rect->bounds = rectMinMax(x1, y0, x2, y1);
	rect->color = color;
	rect->uv = rectMinMax(ninepatch->u1, ninepatch->v0, ninepatch->u2, ninepatch->v1);
	rect++;

	// top-right
	rect->bounds = rectMinMax(x2, y0, x3, y1);
	rect->color = color;
	rect->uv = rectMinMax(ninepatch->u2, ninepatch->v0, ninepatch->u3, ninepatch->v1);
	rect++;

	// middle left
	rect->bounds = rectMinMax(x0, y1, x1, y2);
	rect->color = color;
	rect->uv = rectMinMax(ninepatch->u0, ninepatch->v1, ninepatch->u1, ninepatch->v2);
	rect++;

	// middle
	rect->bounds = rectMinMax(x1, y1, x2, y2);
	rect->color = color;
	rect->uv = rectMinMax(ninepatch->u1, ninepatch->v1, ninepatch->u2, ninepatch->v2);
	rect++;

	// middle-right
	rect->bounds = rectMinMax(x2, y1, x3, y2);
	rect->color = color;
	rect->uv = rectMinMax(ninepatch->u2, ninepatch->v1, ninepatch->u3, ninepatch->v2);
	rect++;

	// bottom left
	rect->bounds = rectMinMax(x0, y2, x1, y3);
	rect->color = color;
	rect->uv = rectMinMax(ninepatch->u0, ninepatch->v2, ninepatch->u1, ninepatch->v3);
	rect++;

	// bottom
	rect->bounds = rectMinMax(x1, y2, x2, y3);
	rect->color = color;
	rect->uv = rectMinMax(ninepatch->u1, ninepatch->v2, ninepatch->u2, ninepatch->v3);
	rect++;

	// bottom-right
	rect->bounds = rectMinMax(x2, y2, x3, y3);
	rect->color = color;
	rect->uv = rectMinMax(ninepatch->u2, ninepatch->v2, ninepatch->u3, ninepatch->v3);
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

RenderItem_DrawRects_Item *reserve(DrawRectsGroup *group, s32 count)
{
	ASSERT((group->currentSubGroup->header->count + count) <= group->currentSubGroup->maxCount);

	RenderItem_DrawRects_Item *result = group->currentSubGroup->first + group->currentSubGroup->header->count;

	group->currentSubGroup->header->count = (u8)(group->currentSubGroup->header->count + count);
	group->count += count;

	return result;
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

void drawGrid(RenderBuffer *buffer, Rect2 bounds, s32 gridW, s32 gridH, u8 *grid, u16 paletteSize, V4 *palette)
{
	DEBUG_FUNCTION_T(DCDT_Renderer);

	addSetShader(buffer, renderer->shaderIds.paletted);

	addSetTextureRaw(buffer, gridW, gridH, 1, grid);
	addSetPalette(buffer, paletteSize, palette);

	RenderItem_DrawSingleRect *rect = appendRenderItem<RenderItem_DrawSingleRect>(buffer, RenderItemType_DrawSingleRect);
	rect->bounds = bounds;
	rect->color00 = makeWhite();
	rect->color01 = makeWhite();
	rect->color10 = makeWhite();
	rect->color11 = makeWhite();
	rect->uv = rectXYWH(0, 0, 1, 1);
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
