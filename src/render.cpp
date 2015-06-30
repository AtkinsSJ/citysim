
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

bool initializeRenderer(Renderer *renderer, char *windowTitle) {

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
	renderer->sdl_window = SDL_CreateWindow(windowTitle,
					SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
					800, 600, // Initial screen resolution
					SDL_WINDOW_SHOWN);
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
	renderer->textures[3] = loadTexture(renderer, "farming-logo.png");
	Texture *textureLogoPng = &renderer->textures[3];
	if (!textureLogoPng->valid) {
		return false;
	}

	// Farming
	const int w1 = TILE_WIDTH;
	const int w2 = w1 * 2;
	const int w3 = w1 * 3;
	const int w4 = w1 * 4;

	setTextureRegion(renderer, TextureAtlasItem_GroundTile,	textureCombinedPng, { 0,  0, w1, w1});
	setTextureRegion(renderer, TextureAtlasItem_WaterTile, 	textureCombinedPng, {w1,  0, w1, w1});
	setTextureRegion(renderer, TextureAtlasItem_ForestTile, textureCombinedPng, {w2,  0, w1, w1});
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

	setTextureRegion(renderer, TextureAtlasItem_Farmer_Harvest0, textureFarmerPng, { 0, 16,  8,  8});
	setTextureRegion(renderer, TextureAtlasItem_Farmer_Harvest1, textureFarmerPng, { 8, 16,  8,  8});
	setTextureRegion(renderer, TextureAtlasItem_Farmer_Harvest2, textureFarmerPng, {16, 16,  8,  8});
	setTextureRegion(renderer, TextureAtlasItem_Farmer_Harvest3, textureFarmerPng, {24, 16,  8,  8});

	setTextureRegion(renderer, TextureAtlasItem_Farmer_Plant0, textureFarmerPng, { 0, 24,  8,  8});
	setTextureRegion(renderer, TextureAtlasItem_Farmer_Plant1, textureFarmerPng, { 8, 24,  8,  8});
	setTextureRegion(renderer, TextureAtlasItem_Farmer_Plant2, textureFarmerPng, {16, 24,  8,  8});
	setTextureRegion(renderer, TextureAtlasItem_Farmer_Plant3, textureFarmerPng, {24, 24,  8,  8});

	setTextureRegion(renderer, TextureAtlasItem_Icon_Planting, 	textureIconsPng, { 0,  0, 32, 32});
	setTextureRegion(renderer, TextureAtlasItem_Icon_Harvesting,textureIconsPng, {32,  0, 32, 32});

	setTextureRegion(renderer, TextureAtlasItem_Menu_Logo, textureLogoPng, {0,  0, 499, 154});

	Animation *animation;
	
	animation = renderer->animations + Animation_Farmer_Stand;
	animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Stand;

	animation = renderer->animations + Animation_Farmer_Walk;
	animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Walk0;
	animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Walk1;
	
	animation = renderer->animations + Animation_Farmer_Hold;
	animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Hold;

	animation = renderer->animations + Animation_Farmer_Carry;
	animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Carry0;
	animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Carry1;

	animation = renderer->animations + Animation_Farmer_Harvest;
	animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Harvest0;
	animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Harvest1;
	animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Harvest2;
	animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Harvest3;

	animation = renderer->animations + Animation_Farmer_Plant;
	animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Plant0;
	animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Plant1;
	animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Plant2;
	animation->frames[animation->frameCount++] = TextureAtlasItem_Farmer_Plant3;

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

	// UI theme
	renderer->theme.buttonTextColor = {0,0,0,255};
	renderer->theme.buttonBackgroundColor = {255,255,255,255};
	renderer->theme.buttonHoverColor = {192,192,255,255};
	renderer->theme.buttonPressedColor = {128,128,255,255};
	renderer->theme.labelColor = {255,255,255,255};
	renderer->theme.overlayColor = {0,0,0,128};
	renderer->theme.textboxTextColor = {0,0,0,255};
	renderer->theme.textboxBackgroundColor = {255,255,255,255};

	// Load font
	renderer->theme.font = TTF_OpenFont("OpenSans-Regular.ttf", 16);
	renderer->theme.buttonFont = TTF_OpenFont("OpenSans-Regular.ttf", 12);
	if (!renderer->theme.font || !renderer->theme.buttonFont) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Font could not be loaded. :(\n %s", TTF_GetError());
		return false;
	}

	return true;
}

void freeRenderer(Renderer *renderer) {

	for (int i=0; i < ArrayCount(renderer->textures); i++) {
		freeTexture(&renderer->textures[i]);
	}

	TTF_CloseFont(renderer->theme.font);
	TTF_CloseFont(renderer->theme.buttonFont);

	SDL_DestroyRenderer(renderer->sdl_renderer);
	SDL_DestroyWindow(renderer->sdl_window);

	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

/**
 * Takes x and y in screen space, and returns a position in world-tile space.
 */
inline V2 screenPosToWorldPos(Coord pos, Camera *camera) {
	return {(pos.x - camera->windowWidth/2 + camera->pos.x) / (camera->zoom * TILE_WIDTH),
			(pos.y - camera->windowHeight/2 + camera->pos.y) / (camera->zoom * TILE_HEIGHT)};
}
void centreCameraOnPosition(Camera *camera, V2 position) {
	camera->pos = v2(
		(position.x * camera->zoom * TILE_WIDTH),
		(position.y * camera->zoom * TILE_HEIGHT)
	);
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

void drawWorldRect(Renderer *renderer, Rect worldRect, Color color) {

	// World->screen code, copied from above. (Yay copypasta!)
	const real32 camLeft = renderer->camera.pos.x - (renderer->camera.windowWidth * 0.5f),
				 camTop = renderer->camera.pos.y - (renderer->camera.windowHeight * 0.5f);

	const real32 tileWidth = TILE_WIDTH * renderer->camera.zoom,
				tileHeight = TILE_HEIGHT * renderer->camera.zoom;

	Rect drawRect = rectXYWH(
		(int)((worldRect.x * tileWidth) - camLeft),
		(int)((worldRect.y * tileHeight) - camTop),
		(int)(worldRect.w * tileWidth),
		(int)(worldRect.h * tileHeight)
	);

	SDL_SetRenderDrawColor(renderer->sdl_renderer, color.r, color.g, color.b, color.a);
	SDL_SetRenderDrawBlendMode(renderer->sdl_renderer, SDL_BLENDMODE_BLEND);

	SDL_RenderFillRect(renderer->sdl_renderer, &drawRect.sdl_rect);
}

////////////////////////////////////////////////////////////////////
//                          ANIMATIONS!                           //
////////////////////////////////////////////////////////////////////
void setAnimation(Animator *animator, Renderer *renderer, AnimationID animationID, bool restart = false) {
	Animation *anim = renderer->animations + animationID;
	// We do nothing if the animation is already playing
	if (restart || animator->animation != anim) {
		animator->animation = anim;
		animator->currentFrame = 0;
		animator->frameCounter = 0.0f;
	}
}

void drawAnimator(Renderer *renderer, Animator *animator, real32 daysPerFrame, V2 worldTilePosition, Color *color = 0) {
	animator->frameCounter += daysPerFrame * animationFramesPerDay;
	while (animator->frameCounter >= 1) {
		int32 framesElapsed = (int)animator->frameCounter;
		animator->currentFrame = (animator->currentFrame + framesElapsed) % animator->animation->frameCount;
		animator->frameCounter -= framesElapsed;
	}
	drawAtWorldPos(renderer, animator->animation->frames[animator->currentFrame], worldTilePosition, color);
}