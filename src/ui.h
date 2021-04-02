#pragma once
// ui.h

enum UIStyleType {
	// NB: When changing this, make sure to change the lambdas in findStyle() to match!
	UIStyle_None = 0,
	UIStyle_Button = 1,
	UIStyle_Console,
	UIStyle_Label,
	UIStyle_Panel,
	UIStyle_Scrollbar,
	UIStyle_TextInput,
	UIStyle_Window,
	UIStyleTypeCount
};

struct UIStyleRef
{
	String name;
	UIStyleType styleType;

	void *pointer = null;
	u32 pointerRetrievedTicks = 0;

	UIStyleRef() {}
	UIStyleRef(UIStyleType type) : styleType(type) {}
	UIStyleRef(UIStyleType type, String name) : styleType(type), name(name) {}
};

const f32 TOAST_APPEAR_TIME    = 0.2f;
const f32 TOAST_DISPLAY_TIME   = 2.0f;
const f32 TOAST_DISAPPEAR_TIME = 0.2f;
const s32 MAX_TOAST_LENGTH = 1024;
struct Toast
{
	f32 duration;
	f32 time; // In seconds, from 0 to duration

	String text;
	char _chars[MAX_TOAST_LENGTH];
};

struct ScrollbarState
{
	bool isHorizontal;
	s32 contentSize;
	f32 scrollPercent;

	bool isDraggingThumb;
	f32 thumbDragStartPercent;

	s32 mouseWheelStepSize;
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

struct UIState
{
	String tooltipText;

	// TODO: Replace this with better "this input has already been used" code!
	ChunkedArray<Rect2I> uiRects;

	s32 openMenu;
	ScrollbarState openMenuScrollbar;

	// UI elements that react to the mouse should only do so if this is false - and then
	// they should set it to true. 
	bool mouseInputHandled;
	Stack<Rect2I> inputScissorRects;

	// Toast stuff
	Queue<Toast> toasts;

	// Window stuff
	ChunkedArray<struct Window> openWindows; // Order: index 0 is the top, then each one is below the previous
	Set<s32> windowsToClose;
	bool isDraggingWindow;
	V2I windowDragWindowStartPos;
	bool isAPauseWindowOpen; // Do any open windows have the WinFlag_Pause flag?
};

struct RenderBuffer;
struct BitmapFont;
struct UIButtonStyle;
struct UIPanel;
struct UIPanelStyle;
struct UIScrollbarStyle;
struct WindowContext;

typedef void (*WindowProc)(WindowContext*, void*);

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
V2I calculateButtonSize(String text, UIButtonStyle *buttonStyle, s32 maxWidth = 0, bool fillWidth = true);
V2I calculateButtonSize(V2I contentSize, UIButtonStyle *buttonStyle, s32 maxWidth = 0, bool fillWidth = true);
bool uiButton(UIState *uiState, String text, Rect2I bounds, UIButtonStyle *style, ButtonState state = Button_Normal, SDL_Keycode shortcutKey=SDLK_UNKNOWN, String tooltip=nullString);
bool uiMenuButton(UIState *uiState, String text, Rect2I bounds, s32 menuID, UIButtonStyle *style, SDL_Keycode shortcutKey=SDLK_UNKNOWN, String tooltip=nullString);

// NB: `message` is copied into the UIState, so it can be a temporary allocation
void pushToast(UIState *uiState, String message);
void drawToast(UIState *uiState);

void initScrollbar(ScrollbarState *state, bool isHorizontal, s32 mouseWheelStepSize = 64);
// NB: When the viewport is larger than the content, there's no thumb rect so nothing is returned
Maybe<Rect2I> getScrollbarThumbBounds(ScrollbarState *state, Rect2I scrollbarBounds, UIScrollbarStyle *style);
void updateScrollbar(UIState *uiState, ScrollbarState *state, s32 contentSize, Rect2I bounds, UIScrollbarStyle *style);
void drawScrollbar(RenderBuffer *uiBuffer, ScrollbarState *state, Rect2I bounds, UIScrollbarStyle *style);
s32 getScrollbarContentOffset(ScrollbarState *state, s32 scrollbarSize);

void showTooltip(UIState *uiState, WindowProc tooltipProc, void *userData = null);
// Is this something we should actually expose??? IDK
void basicTooltipWindowProc(WindowContext *context, void *userData);

void showMenu(UIState *uiState, s32 menuID);
void hideMenus(UIState *uiState);
void toggleMenuVisible(UIState *uiState, s32 menuID);
bool isMenuVisible(UIState *uiState, s32 menuID);
