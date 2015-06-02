#pragma once

const real32 SECONDS_PER_FRAME = 1.0f / 60.0f;
const int MS_PER_FRAME = (1000 / 60); // 60 frames per second

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

const int TILE_WIDTH = 16,
			TILE_HEIGHT = 16;
const int CAMERA_MARGIN = 20;

struct Camera {
	int32 windowWidth, windowHeight;
	V2 pos; // Centre of screen
	real32 zoom; // 1 = normal, 2 = things appear twice their size, etc.
};
const real32 CAMERA_PAN_SPEED = 0.5f; // Measured in camera-widths per second
const int CAMERA_EDGE_SCROLL_MARGIN = 8;

enum TextureAtlasItem {
	TextureAtlasItem_GroundTile = 0,
	TextureAtlasItem_WaterTile,
	TextureAtlasItem_Butcher,
	TextureAtlasItem_Hovel,
	TextureAtlasItem_Paddock,
	TextureAtlasItem_Pit,
	TextureAtlasItem_Road,
	TextureAtlasItem_Goblin,
	TextureAtlasItem_Goat,

	TextureAtlasItemCount
};
struct TextureAtlas {
	SDL_Texture *texture;
	SDL_Rect rects[TextureAtlasItemCount];
};

struct Renderer {
	SDL_Renderer *sdl_renderer;
	SDL_Window *sdl_window;
	Camera camera;

	TextureAtlas textureAtlas;
};

#include "render.cpp"