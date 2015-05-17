
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