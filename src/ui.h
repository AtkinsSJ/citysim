#pragma once
// ui.h

const f32 uiMessageDisplayTime = 2.0f;
struct UIMessage
{
	String text;
	f32 countdown; // In seconds
};

#include "ui_window.h"

struct UIState
{
	MemoryArena arena;
	RenderBuffer *uiBuffer;
	AssetManager *assets;
	InputState *input;

	s32 textShaderID;
	s32 untexturedShaderID;

	f32 closestDepth;

	String tooltipText;
	UIMessage message;

	// TODO: Replace this with better "this input has already been used" code!
	Array<Rect2> uiRects;

	s32 openMenu;

	String currentCursor;
	bool cursorIsVisible;

	// UI elements that react to the mouse should only do so if this is false - and then
	// they should set it to true. 
	bool mouseInputHandled;

	// Window stuff
	ChunkedArray<Window> openWindows; // Order: index 0 is the top, then each one is below the previous
	bool isDraggingWindow;
	V2 windowDragWindowStartPos;
};

void initUiState(UIState *uiState, RenderBuffer *uiBuffer, AssetManager *assets, InputState *input);
void cacheUIShaders(UIState *uiState, AssetManager *assets);

void setCursor(UIState *uiState, String cursorName);
void setCursor(UIState *uiState, char *cursorName);
void setCursorVisible(UIState *uiState, bool visible);

Rect2 uiText(UIState *uiState, BitmapFont *font, String text, V2 origin, u32 align, f32 depth, V4 color, f32 maxWidth = 0);
bool uiButton(UIState *uiState, String text, Rect2 bounds, f32 depth, bool active=false, SDL_Keycode shortcutKey=SDLK_UNKNOWN, String tooltip=nullString);
bool uiMenuButton(UIState *uiState, String text, Rect2 bounds, f32 depth, s32 menuID, SDL_Keycode shortcutKey=SDLK_UNKNOWN, String tooltip=nullString);
void uiCloseMenus(UIState *uiState);

// NB: `message` is copied into the UIState, so it can be a temporary allocation
void pushUiMessage(UIState *uiState, String message);
void drawUiMessage(UIState *uiState);

void drawScrollBar(RenderBuffer *uiBuffer, V2 topLeft, f32 height, f32 scrollPercent, V2 knobSize, f32 depth, V4 knobColor, s32 shaderID);

void showTooltip(UIState *uiState, WindowProc tooltipProc, void *userData);
// Is this something we should actually expose??? IDK
void basicTooltipWindowProc(WindowContext *context, void *userData);
