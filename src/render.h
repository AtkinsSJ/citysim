#pragma once

// General rendering code.

const f32 SECONDS_PER_FRAME = 1.0f / 60.0f;

struct Camera
{
	V2 pos; // Centre of camera, in camera units
	V2 size; // Size of camera, in camera units
	f32 sizeRatio; // Size of window is multiplied by this to produce the camera's size
	f32 zoom; // 1 = normal, 2 = things appear twice their size, etc.
	Matrix4 projectionMatrix;

	// NB: We don't use depth anywhere any more, but these do get used in generating the
	// projection matrix. - Sam, 26/07/2019
	f32 nearClippingPlane;
	f32 farClippingPlane;

	V2 mousePos;
};

enum RenderItemType
{
	RenderItemType_NextMemoryChunk,

	RenderItemType_SectionMarker,

	RenderItemType_SetCamera,
	RenderItemType_SetShader,
	RenderItemType_SetTexture,

	RenderItemType_Clear,

	RenderItemType_BeginScissor,
	RenderItemType_EndScissor,

	RenderItemType_DrawSingleRect,
	RenderItemType_DrawRects,
	RenderItemType_DrawGrid,
	RenderItemType_DrawRings,
};

struct RenderItem_SectionMarker
{
	String name;
	String renderProfileName;
};

struct RenderItem_SetCamera
{
	Camera *camera;
};

struct RenderItem_SetShader
{
	s8 shaderID;
};

struct RenderItem_SetTexture
{
	Asset *texture;
};

struct RenderItem_Clear
{
	V4 clearColor;
};

struct RenderItem_BeginScissor
{
	Rect2I bounds;
};
struct RenderItem_EndScissor
{
};

struct RenderItem_DrawSingleRect
{
	Rect2 bounds;
	V4 color;
	Rect2 uv; // in (0 to 1) space
};

const u8 maxRenderItemsPerGroup = u8Max;
struct RenderItem_DrawRects
{
	u8 count;
};
struct RenderItem_DrawRects_Item
{
	Rect2 bounds;
	V4 color;
	Rect2 uv;
};

struct DrawRectsSubGroup
{
	RenderItem_DrawRects *header;
	RenderItem_DrawRects_Item *first;
	s32 maxCount;

	DrawRectsSubGroup *prev;
	DrawRectsSubGroup *next;
};

// Think of this like a "handle". It has data inside but you shouldn't touch it from user code!
struct DrawRectsGroup
{
	RenderBuffer *buffer;

	DrawRectsSubGroup firstSubGroup;
	DrawRectsSubGroup *currentSubGroup;

	s32 count;
	s32 maxCount;

	Asset *texture;
};

struct RenderItem_DrawRings
{
	u8 count;
};
struct RenderItem_DrawRings_Item
{
	V2 centre;
	f32 radius;
	f32 thickness;
	V4 color;
};

struct DrawRingsSubGroup
{
	RenderItem_DrawRings *header;
	RenderItem_DrawRings_Item *first;
	s32 maxCount;

	DrawRingsSubGroup *prev;
	DrawRingsSubGroup *next;
};

// Think of this like a "handle". It has data inside but you shouldn't touch it from user code!
struct DrawRingsGroup
{
	RenderBuffer *buffer;

	DrawRingsSubGroup firstSubGroup;
	DrawRingsSubGroup *currentSubGroup;

	s32 count;
	s32 maxCount;
};

struct RenderItem_DrawGrid
{
	Rect2 bounds; // Rendering position of the grid as a whole
	u16 paletteSize;
	s32 gridW, gridH; // Dimensions of the array
};

struct RenderBufferChunk : PoolItem
{
	smm size;
	smm used;
	u8 *memory;

	// TODO: @Size: Could potentially re-use the pointers from the PoolItem, if we wanted.
	RenderBufferChunk *prevChunk;
	RenderBufferChunk *nextChunk;
};

struct RenderBuffer
{
	String name;
	String renderProfileName;
	
	MemoryArena *arena;
	RenderBufferChunk *firstChunk;
	RenderBufferChunk *currentChunk;
	Pool<RenderBufferChunk> *chunkPool;

	// Transient stuff
	Camera *currentCamera;
	bool hasRangeReserved;
	smm reservedRangeSize;
	bool hasScissorEnabled;

	s8 currentShader;
	Asset *currentTexture;
};

struct Renderer
{
	MemoryArena renderArena;
	
	SDL_Window *window;

	Camera worldCamera;
	Camera uiCamera;

	// Cursor stuff
	String currentCursorName;
	bool cursorIsVisible;
	SDL_Cursor *systemWaitCursor;

	RenderBuffer worldBuffer;
	RenderBuffer worldOverlayBuffer;
	RenderBuffer uiBuffer;
	RenderBuffer debugBuffer;

	smm renderBufferChunkSize;
	Pool<RenderBufferChunk> chunkPool;

	// Not convinced this is the best way of doing it, but it's better than what we had before!
	// Really, we do want to have this stuff in code, because it's accessed a LOT and we don't
	// want to be doing a million hashtable lookups all the time. It does feel like a hack, but
	// practicality is more important than perfection!
	// - Sam, 23/07/2019
	struct
	{
		s8 pixelArt;
		s8 text;
		s8 untextured;
	} shaderIds;

	// Don't access these directly!
	void *platformRenderer;
	void (*windowResized)(s32, s32);
	void (*render)(RenderBufferChunk *);
	void (*loadAssets)();
	void (*unloadAssets)();
	void (*free)();
};

void initRenderer(MemoryArena *renderArena, SDL_Window *window);
void render();
void rendererLoadAssets();
void rendererUnloadAssets();
void freeRenderer();

void initRenderBuffer(MemoryArena *arena, RenderBuffer *buffer, char *name, Pool<RenderBufferChunk> *chunkPool);
RenderBufferChunk *allocateRenderBufferChunk(MemoryArena *arena, void *userData);
void linkRenderBufferToNext(RenderBuffer *buffer, RenderBuffer *nextBuffer);
void clearRenderBuffer(RenderBuffer *buffer);

void initCamera(Camera *camera, V2 size, f32 sizeRatio, f32 nearClippingPlane, f32 farClippingPlane, V2 position = v2(0,0));
void updateCameraMatrix(Camera *camera);
V2 unproject(Camera *camera, V2 screenPos);

void setCursor(String cursorName);
void setCursor(char *cursorName);
void setCursorVisible(bool visible);

u8* appendRenderItemInternal(RenderBuffer *buffer, RenderItemType type, smm size, smm reservedSize);

template<typename T>
T *appendRenderItem(RenderBuffer *buffer, RenderItemType type);

template<typename T>
T *readRenderItem(RenderBufferChunk *renderBufferChunk, smm *pos);

void addSetCamera(RenderBuffer *buffer, Camera *camera);
void addSetShader(RenderBuffer *buffer, s8 shaderID);
void addSetTexture(RenderBuffer *buffer, Asset *texture);

void addClear(RenderBuffer *buffer, V4 clearColor = {});

// NB: bounds rectangle is in the coordinates of the buffer's camera, to make life easier.
// (glScissor() takes window-pixel coordinates, but the Y is inverted. Bleh.)
void addBeginScissor(RenderBuffer *buffer, Rect2 bounds);
void addEndScissor(RenderBuffer *buffer);

void drawSingleSprite(RenderBuffer *buffer, Sprite *sprite, Rect2 bounds, s8 shaderID, V4 color);
void drawSingleRect(RenderBuffer *buffer, Rect2 bounds, s8 shaderID, V4 color);
void drawSingleRect(RenderBuffer *buffer, Rect2I bounds, s8 shaderID, V4 color);

// For when you want something to appear NOW in the render-order, but you don't know its details until later
RenderItem_DrawSingleRect *appendDrawRectPlaceholder(RenderBuffer *buffer, s8 shaderID);
void fillDrawRectPlaceholder(RenderItem_DrawSingleRect *placeholder, Rect2 bounds, V4 color);
void fillDrawRectPlaceholder(RenderItem_DrawSingleRect *placeholder, Rect2I bounds, V4 color);

// NB: The Rects drawn must all have the same Texture!
DrawRectsGroup *beginRectsGroupInternal(RenderBuffer *buffer, Asset *texture, s8 shaderID, s32 maxCount);
// TODO: Have the shaderID default to a sensible value for these like beginRectsGroupForText
DrawRectsGroup *beginRectsGroupTextured(RenderBuffer *buffer, Asset *texture, s8 shaderID, s32 maxCount);
DrawRectsGroup *beginRectsGroupUntextured(RenderBuffer *buffer, s8 shaderID, s32 maxCount);
// TODO: Have the shaderID be last and default to the standard text shader, so I don't have to always pass it
DrawRectsGroup *beginRectsGroupForText(RenderBuffer *buffer, BitmapFont *font, s8 shaderID, s32 maxCount);
void addRectInternal(DrawRectsGroup *group, Rect2 bounds, V4 color, Rect2 uv);
void addGlyphRect(DrawRectsGroup *state, BitmapFontGlyph *glyph, V2 position, V4 color);
void addSpriteRect(DrawRectsGroup *state, Sprite *sprite, Rect2 bounds, V4 color);
void addUntexturedRect(DrawRectsGroup *group, Rect2 bounds, V4 color);
void offsetRange(DrawRectsGroup *state, s32 startIndex, s32 endIndexInclusive, f32 offsetX, f32 offsetY);
void endRectsGroup(DrawRectsGroup *group);

DrawRectsSubGroup beginRectsSubGroup(DrawRectsGroup *group);
void endCurrentSubGroup(DrawRectsGroup *group);

void drawGrid(RenderBuffer *buffer, Rect2 bounds, s8 shaderID, s32 gridW, s32 gridH, u8 *grid, u16 paletteSize, V4 *palette);

DrawRingsGroup *beginRingsGroup(RenderBuffer *buffer, s32 maxCount);
DrawRingsSubGroup beginRingsSubGroup(DrawRingsGroup *group);
void addRing(DrawRingsGroup *group, V2 centre, f32 radius, f32 thickness, V4 color);
void endRingsGroup(DrawRingsGroup *group);

void resizeWindow(s32 w, s32 h, bool fullscreen);
void onWindowResized(s32 w, s32 h);
