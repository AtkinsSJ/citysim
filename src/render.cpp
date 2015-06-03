
Texture textureFromSurface(Renderer *renderer, SDL_Surface *surface) {
	Texture texture = {};
	texture.sdl_texture = SDL_CreateTextureFromSurface(renderer->sdl_renderer, surface);
	if (texture.sdl_texture) {
		texture.valid = true;
		SDL_QueryTexture(texture.sdl_texture, null, null, &texture.w, &texture.h);
	}

	return texture;
}

Texture loadTexture(Renderer *renderer, char *path) {

	Texture texture = {};

	SDL_Surface *loadedSurface = IMG_Load(path);
	if (loadedSurface == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to load image at '%s': %s\n", path, SDL_GetError());
	} else {
		texture = textureFromSurface(renderer, loadedSurface);

		if (!texture.valid) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create texture from image at '%s': %s\n", path, SDL_GetError());
		}
		
		SDL_FreeSurface(loadedSurface);
	}

	return texture;
}

void freeTexture(Texture *texture) {
	SDL_DestroyTexture(texture->sdl_texture);
	texture = {};
}

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

	// SDL_ttf
	if (TTF_Init() < 0) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL_ttf could not be initialized! :(\n %s\n", TTF_GetError());
			return false;
	}

	// Window
	renderer->sdl_window = SDL_CreateWindow("Impressionable",
					SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
					800, 600, // Initial screen resolution
					SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );
	if (renderer->sdl_window == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Window could not be created! :(\n %s", SDL_GetError());
		return false;
	}

	// Renderer
	renderer->sdl_renderer = SDL_CreateRenderer(renderer->sdl_window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer->sdl_renderer == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Renderer could not be created! :(\n %s", SDL_GetError());
		return false;
	}
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	// Load texture data
	renderer->textureAtlas = {};
	renderer->textureAtlas.texture = loadTexture(renderer, "combined.png");
	if (!renderer->textureAtlas.texture.valid) {
		return false;
	}
	const int tw = TILE_WIDTH;

	// Farming
	renderer->textureAtlas.rects[TextureAtlasItem_GroundTile] = {0,0,tw,tw};
	renderer->textureAtlas.rects[TextureAtlasItem_WaterTile] = {tw,0,tw,tw};
	renderer->textureAtlas.rects[TextureAtlasItem_Field] = {tw*4,0,tw*4,tw*4};
	renderer->textureAtlas.rects[TextureAtlasItem_Crop] = {0,tw,tw,tw};

	// Goblin Fortress
	/*
	renderer->textureAtlas.rects[TextureAtlasItem_GroundTile] = {0,0,tw,tw};
	renderer->textureAtlas.rects[TextureAtlasItem_WaterTile] = {tw,0,tw,tw};
	renderer->textureAtlas.rects[TextureAtlasItem_Butcher] = {tw*3,tw*5,tw*2,tw*2};
	renderer->textureAtlas.rects[TextureAtlasItem_Hovel] = {tw,tw,tw,tw};
	renderer->textureAtlas.rects[TextureAtlasItem_Paddock] = {0,tw*2,tw*3,tw*3};
	renderer->textureAtlas.rects[TextureAtlasItem_Pit] = {tw*3,0,tw*5,tw*5};
	renderer->textureAtlas.rects[TextureAtlasItem_Road] = {0,tw,tw,tw};
	renderer->textureAtlas.rects[TextureAtlasItem_Goblin] = {tw,tw*5,tw*2,tw*2};
	renderer->textureAtlas.rects[TextureAtlasItem_Goat] = {0,tw*5,tw,tw*2};
	*/

	// Load font
	renderer->font = TTF_OpenFont("OpenSans-Regular.ttf", 24);
	if (!renderer->font) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Font could not be loaded. :(\n %s", TTF_GetError());
		return false;
	}

	return true;
}

void freeRenderer(Renderer *renderer) {

	freeTexture(&renderer->textureAtlas.texture);

	TTF_CloseFont(renderer->font);

	SDL_DestroyRenderer(renderer->sdl_renderer);
	SDL_DestroyWindow(renderer->sdl_window);

	TTF_Quit();
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

void clearToBlack(Renderer *renderer) {
	SDL_SetRenderDrawColor(renderer->sdl_renderer, 0, 0, 0, 255);
	SDL_SetRenderDrawBlendMode(renderer->sdl_renderer, SDL_BLENDMODE_NONE);
	SDL_RenderClear(renderer->sdl_renderer);
}

void drawAtWorldPos(Renderer *renderer, TextureAtlasItem textureAtlasItem, Coord worldTilePosition, Color *color=0) {
	
	const real32 camLeft = renderer->camera.pos.x - (renderer->camera.windowWidth * 0.5f),
				 camTop = renderer->camera.pos.y - (renderer->camera.windowHeight * 0.5f);

	const real32 tileWidth = TILE_WIDTH * renderer->camera.zoom,
				tileHeight = TILE_HEIGHT * renderer->camera.zoom;

	SDL_Rect *sourceRect = &renderer->textureAtlas.rects[textureAtlasItem];
	SDL_Rect destRect = {
		(int)((worldTilePosition.x * tileWidth) - camLeft),
		(int)((worldTilePosition.y * tileHeight) - camTop),
		(int)(sourceRect->w * renderer->camera.zoom),
		(int)(sourceRect->h * renderer->camera.zoom)
	};

	if (color) {
		SDL_SetTextureColorMod(renderer->textureAtlas.texture.sdl_texture, color->r, color->g, color->b);
		SDL_SetTextureAlphaMod(renderer->textureAtlas.texture.sdl_texture, color->a);

		SDL_RenderCopy(renderer->sdl_renderer, renderer->textureAtlas.texture.sdl_texture, sourceRect, &destRect);

		SDL_SetTextureColorMod(renderer->textureAtlas.texture.sdl_texture, 255, 255, 255);
		SDL_SetTextureAlphaMod(renderer->textureAtlas.texture.sdl_texture, 255);
	} else {
		SDL_RenderCopy(renderer->sdl_renderer, renderer->textureAtlas.texture.sdl_texture, sourceRect, &destRect);
	}
}