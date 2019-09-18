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
	String tooltipText;
	UIMessage message;

	// TODO: Replace this with better "this input has already been used" code!
	ChunkedArray<Rect2> uiRects;

	s32 openMenu;

	// UI elements that react to the mouse should only do so if this is false - and then
	// they should set it to true. 
	bool mouseInputHandled;

	// Window stuff
	ChunkedArray<Window> openWindows; // Order: index 0 is the top, then each one is below the previous
	bool isDraggingWindow;
	V2 windowDragWindowStartPos;
};

void initUIState(UIState *uiState, MemoryArena *arena);

Rect2 uiText(RenderBuffer *renderBuffer, BitmapFont *font, String text, V2 origin, u32 align, V4 color, f32 maxWidth = 0);
bool uiButton(UIState *uiState, String text, Rect2 bounds, bool active=false, SDL_Keycode shortcutKey=SDLK_UNKNOWN, String tooltip=nullString);
bool uiMenuButton(UIState *uiState, String text, Rect2 bounds, s32 menuID, SDL_Keycode shortcutKey=SDLK_UNKNOWN, String tooltip=nullString);
void uiCloseMenus(UIState *uiState);

// NB: `message` is copied into the UIState, so it can be a temporary allocation
void pushUiMessage(UIState *uiState, String message);
void drawUiMessage(UIState *uiState);

void drawScrollBar(RenderBuffer *uiBuffer, V2 topLeft, f32 height, f32 scrollPercent, V2 knobSize, V4 knobColor, s8 shaderID);

void showTooltip(UIState *uiState, WindowProc tooltipProc, void *userData);
// Is this something we should actually expose??? IDK
void basicTooltipWindowProc(WindowContext *context, void *userData);

struct PopupMenu
{
	V2 origin;
	f32 width;

 	// TODO: Put these in a style!
 	// {
	f32 padding;
	V4 backgroundColor;
	// }

	RenderItem_DrawSingleRect *backgroundRect;

	f32 currentYOffset;

	// TODO: Maximum height, with a scrollbar if it's too big.
	// This means, storing a scroll-position for each menu somehow...
	// Or, well, we could get away with one scroll position stored for whichever
	// menu is active, and then set the position to 0 when a menu is shown. Or
	// when it's hidden, actually!!! That'd work. Awesome.
};

PopupMenu beginPopupMenu(f32 x, f32 y, f32 width, V4 backgroundColor);
bool popupMenuButton(UIState *uiState, PopupMenu *menu, String text, bool isActive);
void endPopupMenu(UIState *uiState, PopupMenu *menu);
