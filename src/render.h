#pragma once

const int FRAMES_PER_SECOND = 60;
const real32 SECONDS_PER_FRAME = 1.0f / 60.0f;
const int MS_PER_FRAME = (1000 / 60); // 60 frames per second

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

// Farming
enum TextureAtlasItem {
	TextureAtlasItem_GroundTile = 0,
	TextureAtlasItem_WaterTile,
	TextureAtlasItem_Field,
	TextureAtlasItem_Crop0_0,
	TextureAtlasItem_Crop0_1,
	TextureAtlasItem_Crop0_2,
	TextureAtlasItem_Crop0_3,
	TextureAtlasItem_Potato,
	TextureAtlasItem_Barn,

	TextureAtlasItemCount
};

// Goblin Fortress
/*
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
*/

struct Texture {
	bool valid;
	SDL_Texture *sdl_texture;
	int32 w, h;
};

struct TextureAtlas {
	Texture texture;
	SDL_Rect rects[TextureAtlasItemCount];
};

struct Renderer {
	SDL_Renderer *sdl_renderer;
	SDL_Window *sdl_window;
	Camera camera;

	TextureAtlas textureAtlas;

	TTF_Font *font;
	TTF_Font *fontLarge;
};

#include "render.cpp"