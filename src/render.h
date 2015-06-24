#pragma once

const int FRAMES_PER_SECOND = 60;
const real32 SECONDS_PER_FRAME = 1.0f / 60.0f;
const int MS_PER_FRAME = (1000 / 60); // 60 frames per second

const int TILE_WIDTH = 16,
			TILE_HEIGHT = 16;
const int CAMERA_MARGIN = 20;
const bool canZoom = false;

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
	TextureAtlasItem_House,

	TextureAtlasItem_Farmer_Stand,
	TextureAtlasItem_Farmer_Walk0,
	TextureAtlasItem_Farmer_Walk1,
	TextureAtlasItem_Farmer_Hold,
	TextureAtlasItem_Farmer_Carry0,
	TextureAtlasItem_Farmer_Carry1,
	TextureAtlasItem_Farmer_Harvest0,
	TextureAtlasItem_Farmer_Harvest1,
	TextureAtlasItem_Farmer_Harvest2,
	TextureAtlasItem_Farmer_Harvest3,
	TextureAtlasItem_Farmer_Plant0,
	TextureAtlasItem_Farmer_Plant1,
	TextureAtlasItem_Farmer_Plant2,
	TextureAtlasItem_Farmer_Plant3,

	TextureAtlasItem_Icon_Planting,
	TextureAtlasItem_Icon_Harvesting,

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
	const char* filename;
	SDL_Texture *sdl_texture;
	int32 w, h;
};

struct TextureRegion {
	Texture *texture;
	Rect rect;
};

enum AnimationID {
	Animation_Farmer_Stand,
	Animation_Farmer_Walk,
	Animation_Farmer_Hold,
	Animation_Farmer_Carry,
	Animation_Farmer_Harvest,
	Animation_Farmer_Plant,
	
	Animation_Count,
};

struct Animation {
	TextureAtlasItem frames[16];
	uint32 frameCount;
};

struct Animator {
	Animation *animation;
	uint32 currentFrame;
	real32 frameCounter; // Sub-frame ticks
};
const real32 animationFramesPerDay = 10.0f;

struct Renderer {
	SDL_Renderer *sdl_renderer;
	SDL_Window *sdl_window;
	Camera camera;

	Texture textures[16];
	TextureRegion regions[TextureAtlasItemCount];

	Animation animations[Animation_Count];

	TTF_Font *font;
	TTF_Font *fontLarge;
};

#include "render.cpp"