#pragma once
// ui.h

const f32 uiMessageDisplayTime = 2.0f;
struct UIMessage
{
	String text;
	f32 countdown; // In seconds
};

struct ScrollbarState
{
	s32 contentSize;
	s32 scrollPosition;
};

enum ButtonState
{
	Button_Normal,
	Button_Disabled,
	Button_Active,
};
inline ButtonState buttonIsActive(bool isActive)
{
	return isActive ? Button_Active : Button_Normal;
}

#include "ui_panel.h"
#include "ui_window.h"

struct UIState
{
	String tooltipText;
	UIMessage message;

	// TODO: Replace this with better "this input has already been used" code!
	ChunkedArray<Rect2I> uiRects;

	s32 openMenu;
	ScrollbarState openMenuScrollbar;

	// UI elements that react to the mouse should only do so if this is false - and then
	// they should set it to true. 
	bool mouseInputHandled;
	bool isInputScissorActive;
	Rect2I inputScissorBounds;

	// Window stuff
	ChunkedArray<Window> openWindows; // Order: index 0 is the top, then each one is below the previous
	bool isDraggingWindow;
	V2I windowDragWindowStartPos;
	bool isAPauseWindowOpen; // Do any open windows have the WinFlag_Pause flag?
};

void initUIState(UIState *uiState, MemoryArena *arena);

bool isMouseInUIBounds(UIState *uiState, Rect2I bounds);
bool isMouseInUIBounds(UIState *uiState, Rect2I bounds, V2 mousePos);
bool justClickedOnUI(UIState *uiState, Rect2I bounds);

Rect2I uiText(RenderBuffer *renderBuffer, BitmapFont *font, String text, V2I origin, u32 align, V4 color, s32 maxWidth = 0);
V2I calculateButtonSize(String text, UIButtonStyle *buttonStyle, s32 maxWidth=0, bool fillWidth=true);
bool uiButton(UIState *uiState, String text, Rect2I bounds, UIButtonStyle *style, ButtonState state = Button_Normal, SDL_Keycode shortcutKey=SDLK_UNKNOWN, String tooltip=nullString);
bool uiMenuButton(UIState *uiState, String text, Rect2I bounds, s32 menuID, UIButtonStyle *style, SDL_Keycode shortcutKey=SDLK_UNKNOWN, String tooltip=nullString);
void uiCloseMenus(UIState *uiState);

// NB: `message` is copied into the UIState, so it can be a temporary allocation
void pushUiMessage(UIState *uiState, String message);
void drawUiMessage(UIState *uiState);

void updateScrollbar(UIState *uiState, ScrollbarState *state, s32 contentSize, Rect2I bounds, UIScrollbarStyle *style);
void drawScrollbar(RenderBuffer *uiBuffer, f32 scrollPercent, V2I topLeft, s32 height, V2I knobSize, V4 knobColor, V4 backgroundColor, s8 shaderID);
f32 getScrollbarPercent(ScrollbarState *state, s32 scrollbarHeight); // Percent meaning 0.99 = 99%. (I know that's not a percent, but whatever)

void showTooltip(UIState *uiState, WindowProc tooltipProc, void *userData);
// Is this something we should actually expose??? IDK
void basicTooltipWindowProc(WindowContext *context, void *userData);


struct PopupMenu
{
	UIPopupMenuStyle *style;
	UIButtonStyle *buttonStyle;
	UIScrollbarStyle *scrollbarStyle;

	V2I origin;
	s32 width;
	s32 maxHeight;

	RenderItem_DrawSingleRect *backgroundRect;

	s32 currentYOffset;

	// TODO: Maximum height, with a scrollbar if it's too big.
	// This means, storing a scroll-position for each menu somehow...
	// Or, well, we could get away with one scroll position stored for whichever
	// menu is active, and then set the position to 0 when a menu is shown. Or
	// when it's hidden, actually!!! That'd work. Awesome.
};

PopupMenu beginPopupMenu(UIState *uiState, s32 x, s32 y, s32 width, s32 maxHeight, UIPopupMenuStyle *style);
bool popupMenuButton(UIState *uiState, PopupMenu *menu, String text, UIButtonStyle *style, ButtonState state = Button_Normal);
void endPopupMenu(UIState *uiState, PopupMenu *menu);
