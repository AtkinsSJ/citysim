
bool initializeRenderer(Renderer *renderer) {

	(*renderer) = {};

	// SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL could not be initialised! :(\n %s\n", SDL_GetError());
		return false;
	}

	// SDL_image
	uint8 imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags)) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL_image could not be initialised! :(\n %s\n", IMG_GetError());
		return false;
	}

	// Window
	renderer->sdl_window = SDL_CreateWindow("Impressionable",
					SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
					SCREEN_WIDTH, SCREEN_HEIGHT,
					SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );
	if (renderer->sdl_window == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Window could not be created! :(\n %s", SDL_GetError());
		return false;
	}

	// Renderer
	renderer->sdl_renderer = SDL_CreateRenderer(renderer->sdl_window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer->sdl_renderer == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Renderer could not be created! :(\n %s", SDL_GetError());
		return null;
	}
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	return true;
}

void freeRenderer(Renderer *renderer) {
	SDL_DestroyRenderer(renderer->sdl_renderer);
	SDL_DestroyWindow(renderer->sdl_window);

	IMG_Quit();
	SDL_Quit();
}

/**
 * Takes x and y in screen space, and returns a position in world-tile space.
 */
inline V2 screenPosToWorldPos(int32 x, int32 y, Camera &camera) {
	return {(x - camera.windowWidth/2 + camera.pos.x) / (camera.zoom * TILE_WIDTH),
			(y - camera.windowHeight/2 + camera.pos.y) / (camera.zoom * TILE_HEIGHT)};
}
inline Coord tilePosition(V2 worldPixelPos) {
	return {(int)floor(worldPixelPos.x),
			(int)floor(worldPixelPos.y)};
}

void drawAtWorldPos(SDL_Renderer *&renderer, Camera &camera, TextureAtlas &textureAtlas, TextureAtlasItem textureAtlasItem, Coord worldTilePosition, Color *color=0) {
	
	const real32 camLeft = camera.pos.x - (camera.windowWidth * 0.5f),
				 camTop = camera.pos.y - (camera.windowHeight * 0.5f);

	const real32 tileWidth = TILE_WIDTH * camera.zoom,
				tileHeight = TILE_HEIGHT * camera.zoom;

	SDL_Rect *sourceRect = &textureAtlas.rects[textureAtlasItem];
	SDL_Rect destRect = {
		(int)((worldTilePosition.x * tileWidth) - camLeft),
		(int)((worldTilePosition.y * tileHeight) - camTop),
		(int)(sourceRect->w * camera.zoom),
		(int)(sourceRect->h * camera.zoom)
	};

	if (color) {
		SDL_SetTextureColorMod(textureAtlas.texture, color->r, color->g, color->b);
		SDL_SetTextureAlphaMod(textureAtlas.texture, color->a);

		SDL_RenderCopy(renderer, textureAtlas.texture, sourceRect, &destRect);

		SDL_SetTextureColorMod(textureAtlas.texture, 255, 255, 255);
		SDL_SetTextureAlphaMod(textureAtlas.texture, 255);
	} else {
		SDL_RenderCopy(renderer, textureAtlas.texture, sourceRect, &destRect);
	}
}

SDL_Texture* loadTexture(SDL_Renderer *renderer, char *path) {
	SDL_Texture *texture = NULL;

	SDL_Surface *loadedSurface = IMG_Load(path);
	if (loadedSurface == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to load image at '%s': %s\n", path, SDL_GetError());
	} else {
		texture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
		if (texture == NULL) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create texture from image at '%s': %s\n", path, SDL_GetError());
		}

		SDL_FreeSurface(loadedSurface);
	}

	return texture;
}