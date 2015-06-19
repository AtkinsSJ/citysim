
SDL_Cursor *createCursor(char *path) {
	SDL_Surface *cursorSurface = IMG_Load(path);
	SDL_Cursor *cursor = SDL_CreateColorCursor(cursorSurface, 0, 0);
	SDL_FreeSurface(cursorSurface);

	return cursor;
}

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
		texture.filename = path;

		if (!texture.valid) {
			SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to create texture from image at '%s': %s\n", path, SDL_GetError());
		}
		
		SDL_FreeSurface(loadedSurface);
	}

	return texture;
}

Texture renderText(Renderer *renderer, TTF_Font *font, char *text, Color color) {
	SDL_Surface *textSurface = TTF_RenderUTF8_Blended(font, text, color);
	Texture texture = textureFromSurface(renderer, textSurface);
	SDL_FreeSurface(textSurface);

	return texture;
}

void freeTexture(Texture *texture) {
	if (texture->valid) {
		SDL_DestroyTexture(texture->sdl_texture);
	}
	texture = {};
}

void setTextureRegion(Renderer *renderer, TextureAtlasItem index, Texture *texture, Rect rect) {
	TextureRegion *region = renderer->regions + index;
	region->texture = texture;
	region->rect = rect;
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
	renderer->textures[0] = loadTexture(renderer, "combined.png");
	Texture *textureCombinedPng = &renderer->textures[0];
	if (!textureCombinedPng->valid) {
		return false;
	}
	renderer->textures[1] = loadTexture(renderer, "farmer.png");
	Texture *textureFarmerPng = &renderer->textures[1];
	if (!textureFarmerPng->valid) {
		return false;
	}
	renderer->textures[2] = loadTexture(renderer, "icons.png");
	Texture *textureIconsPng = &renderer->textures[2];
	if (!textureIconsPng->valid) {
		return false;
	}

	// Farming
	const int w1 = TILE_WIDTH;
	const int w2 = w1 * 2;
	const int w3 = w1 * 3;
	const int w4 = w1 * 4;

	setTextureRegion(renderer, TextureAtlasItem_GroundTile,	textureCombinedPng, { 0,  0, w1, w1});
	setTextureRegion(renderer, TextureAtlasItem_WaterTile, 	textureCombinedPng, {w1,  0, w1, w1});
	setTextureRegion(renderer, TextureAtlasItem_Field, 		textureCombinedPng, {w4,  0, w4, w4});
	setTextureRegion(renderer, TextureAtlasItem_Crop0_0, 	textureCombinedPng, { 0, w1, w1, w1});
	setTextureRegion(renderer, TextureAtlasItem_Crop0_1, 	textureCombinedPng, {w1, w1, w1, w1});
	setTextureRegion(renderer, TextureAtlasItem_Crop0_2, 	textureCombinedPng, {w2, w1, w1, w1});
	setTextureRegion(renderer, TextureAtlasItem_Crop0_3, 	textureCombinedPng, {w3, w1, w1, w1});
	setTextureRegion(renderer, TextureAtlasItem_Potato, 	textureCombinedPng, { 0, w2, w1, w1});
	setTextureRegion(renderer, TextureAtlasItem_Barn, 		textureCombinedPng, { 0, w4, w4, w4});
	setTextureRegion(renderer, TextureAtlasItem_House, 		textureCombinedPng, {w4, w4, w4, w4});

	setTextureRegion(renderer, TextureAtlasItem_Farmer_Stand,  textureFarmerPng, { 0,  0,  8,  8});
	setTextureRegion(renderer, TextureAtlasItem_Farmer_Walk0,  textureFarmerPng, { 8,  0,  8,  8});
	setTextureRegion(renderer, TextureAtlasItem_Farmer_Walk1,  textureFarmerPng, {16,  0,  8,  8});
	setTextureRegion(renderer, TextureAtlasItem_Farmer_Hold,   textureFarmerPng, { 0,  8,  8,  8});
	setTextureRegion(renderer, TextureAtlasItem_Farmer_Carry0, textureFarmerPng, { 8,  8,  8,  8});
	setTextureRegion(renderer, TextureAtlasItem_Farmer_Carry1, textureFarmerPng, {16,  8,  8,  8});

	setTextureRegion(renderer, TextureAtlasItem_Icon_Planting, 	textureIconsPng, { 0,  0, 32, 32});
	setTextureRegion(renderer, TextureAtlasItem_Icon_Harvesting,textureIconsPng, {32,  0, 32, 32});

	// Goblin Fortress
	/*
	renderer->textureAtlas.rects[TextureAtlasItem_GroundTile] = {0,0,w1,w1};
	renderer->textureAtlas.rects[TextureAtlasItem_WaterTile] = {w1,0,w1,w1};
	renderer->textureAtlas.rects[TextureAtlasItem_Butcher] = {w1*3,w1*5,w1*2,w1*2};
	renderer->textureAtlas.rects[TextureAtlasItem_Hovel] = {w1,w1,w1,w1};
	renderer->textureAtlas.rects[TextureAtlasItem_Paddock] = {0,w1*2,w1*3,w1*3};
	renderer->textureAtlas.rects[TextureAtlasItem_Pit] = {w1*3,0,w1*5,w1*5};
	renderer->textureAtlas.rects[TextureAtlasItem_Road] = {0,w1,w1,w1};
	renderer->textureAtlas.rects[TextureAtlasItem_Goblin] = {w1,w1*5,w1*2,w1*2};
	renderer->textureAtlas.rects[TextureAtlasItem_Goat] = {0,w1*5,w1,w1*2};
	*/

	// Load font
	renderer->font = TTF_OpenFont("OpenSans-Regular.ttf", 12);
	renderer->fontLarge = TTF_OpenFont("OpenSans-Regular.ttf", 16);
	if (!renderer->font || !renderer->fontLarge) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Font could not be loaded. :(\n %s", TTF_GetError());
		return false;
	}

	return true;
}

void freeRenderer(Renderer *renderer) {

	for (int i=0; i < ArrayCount(renderer->textures); i++) {
		freeTexture(&renderer->textures[i]);
	}

	TTF_CloseFont(renderer->font);
	TTF_CloseFont(renderer->fontLarge);

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

void drawAtScreenPos(Renderer *renderer, TextureAtlasItem textureAtlasItem, Coord position) {
	TextureRegion *region = &renderer->regions[textureAtlasItem];
	SDL_Rect srcRect = region->rect.sdl_rect;
	SDL_Rect destRect = {position.x, position.y, srcRect.w, srcRect.h};
	SDL_RenderCopy(renderer->sdl_renderer, region->texture->sdl_texture, &srcRect, &destRect);
}

void drawAtWorldPos(Renderer *renderer, TextureAtlasItem textureAtlasItem, V2 worldTilePosition,
	Color *color=0) {
	
	const real32 camLeft = renderer->camera.pos.x - (renderer->camera.windowWidth * 0.5f),
				 camTop = renderer->camera.pos.y - (renderer->camera.windowHeight * 0.5f);

	const real32 tileWidth = TILE_WIDTH * renderer->camera.zoom,
				tileHeight = TILE_HEIGHT * renderer->camera.zoom;

	TextureRegion *region = &renderer->regions[textureAtlasItem];

	SDL_Rect destRect = {
		(int)((worldTilePosition.x * tileWidth) - camLeft),
		(int)((worldTilePosition.y * tileHeight) - camTop),
		(int)(region->rect.w * renderer->camera.zoom),
		(int)(region->rect.h * renderer->camera.zoom)
	};

	if (color) {
		SDL_SetTextureColorMod(region->texture->sdl_texture, color->r, color->g, color->b);
		SDL_SetTextureAlphaMod(region->texture->sdl_texture, color->a);

		SDL_RenderCopy(renderer->sdl_renderer, region->texture->sdl_texture, &region->rect.sdl_rect, &destRect);

		SDL_SetTextureColorMod(region->texture->sdl_texture, 255, 255, 255);
		SDL_SetTextureAlphaMod(region->texture->sdl_texture, 255);
	} else {
		SDL_RenderCopy(renderer->sdl_renderer, region->texture->sdl_texture, &region->rect.sdl_rect, &destRect);
	}
}