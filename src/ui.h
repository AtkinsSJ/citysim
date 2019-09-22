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
	ChunkedArray<Rect2I> uiRects;

	s32 openMenu;

	// UI elements that react to the mouse should only do so if this is false - and then
	// they should set it to true. 
	bool mouseInputHandled;

	// Window stuff
	ChunkedArray<Window> openWindows; // Order: index 0 is the top, then each one is below the previous
	bool isDraggingWindow;
	V2I windowDragWindowStartPos;
};

void initUIState(UIState *uiState, MemoryArena *arena);

Rect2I uiText(RenderBuffer *renderBuffer, BitmapFont *font, String text, V2I origin, u32 align, V4 color, s32 maxWidth = 0);
bool uiButton(UIState *uiState, String text, Rect2I bounds, bool active=false, SDL_Keycode shortcutKey=SDLK_UNKNOWN, String tooltip=nullString);
bool uiMenuButton(UIState *uiState, String text, Rect2I bounds, s32 menuID, SDL_Keycode shortcutKey=SDLK_UNKNOWN, String tooltip=nullString);
void uiCloseMenus(UIState *uiState);

// NB: `message` is copied into the UIState, so it can be a temporary allocation
void pushUiMessage(UIState *uiState, String message);
void drawUiMessage(UIState *uiState);

void drawScrollBar(RenderBuffer *uiBuffer, V2I topLeft, s32 height, f32 scrollPercent, V2I knobSize, V4 knobColor, s8 shaderID);

void showTooltip(UIState *uiState, WindowProc tooltipProc, void *userData);
// Is this something we should actually expose??? IDK
void basicTooltipWindowProc(WindowContext *context, void *userData);

struct PopupMenu
{
	V2I origin;
	s32 width;

 	// TODO: Put these in a style!
 	// {
	s32 padding;
	V4 backgroundColor;
	// }

	RenderItem_DrawSingleRect *backgroundRect;

	s32 currentYOffset;

	// TODO: Maximum height, with a scrollbar if it's too big.
	// This means, storing a scroll-position for each menu somehow...
	// Or, well, we could get away with one scroll position stored for whichever
	// menu is active, and then set the position to 0 when a menu is shown. Or
	// when it's hidden, actually!!! That'd work. Awesome.
};

PopupMenu beginPopupMenu(s32 x, s32 y, s32 width, V4 backgroundColor);
bool popupMenuButton(UIState *uiState, PopupMenu *menu, String text, bool isActive);
void endPopupMenu(UIState *uiState, PopupMenu *menu);
