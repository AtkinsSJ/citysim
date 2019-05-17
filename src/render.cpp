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

void initCamera(Camera *camera, V2 size, f32 nearClippingPlane, f32 farClippingPlane, V2 position = v2(0,0))
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

inline void makeRenderItem(RenderItem *result, Rect2 rect, f32 depth, Asset *texture, Rect2 uv, V4 color=makeWhite(), ShaderType shaderID = Shader_Invalid)
{
	*result = {};
	result->rect = rect;
	result->depth = depth;
	result->color = color;

	result->texture = texture;
	result->uv = uv;

	if (shaderID == Shader_Invalid)
	{
		result->shaderID = (texture == null) ? Shader_Untextured : Shader_Textured;
	}
	else
	{
		result->shaderID = shaderID;
	}
}

inline void drawRect(RenderBuffer *buffer, Rect2 rect, f32 depth, V4 color, ShaderType shaderID = Shader_Invalid)
{
	makeRenderItem(appendBlank(&buffer->items), rect, depth, null, {}, color, shaderID);
}

inline void drawSprite(RenderBuffer *buffer, Sprite *sprite, Rect2 rect, f32 depth, V4 color=makeWhite(), ShaderType shaderID = Shader_Invalid)
{
	ASSERT(sprite != null, "Attempted to draw a null Sprite!");
	makeRenderItem(appendBlank(&buffer->items), rect, depth, sprite->texture, sprite->uv, color, shaderID);
}

void drawRenderItem(RenderBuffer *buffer, RenderItem *item, V2 offsetP, f32 depthOffset, V4 color)
{
	ASSERT(item != null, "Attempted to draw a null RenderItem!");
	makeRenderItem(appendBlank(&buffer->items), offset(item->rect, offsetP), item->depth + depthOffset, item->texture, item->uv, color, item->shaderID);
}

void drawRenderItem(RenderBuffer *buffer, RenderItem *item)
{
	ASSERT(item != null, "Attempted to draw a null RenderItem!");
	*appendBlank(&buffer->items) = *item;
}

f32 compareRenderItems(RenderItem *a, RenderItem *b)
{
	return (a->depth - b->depth);
}

void sortRenderBuffer(RenderBuffer *buffer)
{
	DEBUG_FUNCTION();
	sortInPlace(&buffer->items, compareRenderItems);
}

#if CHECK_BUFFERS_SORTED
bool isBufferSorted(RenderBuffer *buffer)
{
	DEBUG_FUNCTION();
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
