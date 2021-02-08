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
	Stack<Rect2I> inputScissorRects;

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

void pushInputScissorRect(UIState *uiState, Rect2I bounds);
void popInputScissorRect(UIState *uiState);
bool isInputScissorActive(UIState *uiState);
// NB: This is safe to call whether or not a scissor is active. It returns
// either the scissor or a functionally infinite rectangle.
Rect2I getInputScissorRect(UIState *uiState);

void addUIRect(UIState *uiState, Rect2I bounds);

Rect2I uiText(RenderBuffer *renderBuffer, BitmapFont *font, String text, V2I origin, u32 align, V4 color, s32 maxWidth = 0);
V2I calculateButtonSize(String text, UIButtonStyle *buttonStyle, s32 maxWidth=0, bool fillWidth=true);
bool uiButton(UIState *uiState, String text, Rect2I bounds, UIButtonStyle *style, ButtonState state = Button_Normal, SDL_Keycode shortcutKey=SDLK_UNKNOWN, String tooltip=nullString);
bool uiMenuButton(UIState *uiState, String text, Rect2I bounds, s32 menuID, UIButtonStyle *style, SDL_Keycode shortcutKey=SDLK_UNKNOWN, String tooltip=nullString);

// NB: `message` is copied into the UIState, so it can be a temporary allocation
void pushUiMessage(UIState *uiState, String message);
void drawUiMessage(UIState *uiState);

void updateScrollbar(UIState *uiState, ScrollbarState *state, s32 contentSize, Rect2I bounds, UIScrollbarStyle *style);
void drawScrollbar(RenderBuffer *uiBuffer, f32 scrollPercent, V2I topLeft, s32 height, V2I knobSize, V4 knobColor, V4 backgroundColor, s8 shaderID);
f32 getScrollbarPercent(ScrollbarState *state, s32 scrollbarHeight); // Percent meaning 0.99 = 99%. (I know that's not a percent, but whatever)

void showTooltip(UIState *uiState, WindowProc tooltipProc, void *userData);
// Is this something we should actually expose??? IDK
void basicTooltipWindowProc(WindowContext *context, void *userData);

void showMenu(UIState *uiState, s32 menuID);
void hideMenus(UIState *uiState);
void toggleMenuVisible(UIState *uiState, s32 menuID);
bool isMenuVisible(UIState *uiState, s32 menuID);
