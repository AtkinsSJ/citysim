#pragma once
// ui.h

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

struct WidgetMouseState
{
	bool isHovered;
	bool isPressed;
};

struct RenderBuffer;
struct BitmapFont;
struct Sprite;
struct TextInput;
struct Window;

// Annoyingly, we have to predeclare all the style types. Yay for C++
// (I tried rearranging our includes, it got messy fast.)
// I guess the alternative is switching to an individual-files build. Might be a good idea.
struct UIButtonStyle;
struct UICheckboxStyle;
struct UIDropDownListStyle;
struct UIPanel;
struct UIPanelStyle;
struct UIScrollbarStyle;
struct UITextInputStyle;
struct WindowContext;

typedef void (*WindowProc)(WindowContext*, void*);

void initScrollbar(ScrollbarState *state, bool isHorizontal, s32 mouseWheelStepSize = 64);
// NB: When the viewport is larger than the content, there's no thumb rect so nothing is returned
Maybe<Rect2I> getScrollbarThumbBounds(ScrollbarState *state, Rect2I scrollbarBounds, UIScrollbarStyle *style);
void updateScrollbar(ScrollbarState *state, s32 contentSize, Rect2I bounds, UIScrollbarStyle *style);
void drawScrollbar(RenderBuffer *uiBuffer, ScrollbarState *state, Rect2I bounds, UIScrollbarStyle *style);
s32 getScrollbarContentOffset(ScrollbarState *state, s32 scrollbarSize);

namespace UI
{
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

		// DropDownList stuff
		void *openDropDownList; // We use the pointer to the options array
		ScrollbarState openDropDownListScrollbar;
		RenderBuffer *openDropDownListRenderBuffer;

		// Toast stuff
		Queue<Toast> toasts;

		// Window stuff
		ChunkedArray<Window> openWindows; // Order: index 0 is the top, then each one is below the previous
		Set<s32> windowsToClose;
		Set<s32> windowsToMakeActive;
		bool isDraggingWindow;
		V2I windowDragWindowStartPos;
		bool isAPauseWindowOpen; // Do any open windows have the WinFlag_Pause flag?
	};
	UIState uiState;

	void init(MemoryArena *arena);
	void startFrame();
	void endFrame();

	// Input
	bool isMouseInputHandled();
	void markMouseInputHandled();
	bool isMouseInUIBounds(Rect2I bounds);
	bool isMouseInUIBounds(Rect2I bounds, V2 mousePos);
	bool justClickedOnUI(Rect2I bounds);

	WidgetMouseState getWidgetMouseState(Rect2I bounds);

	// Input Scissor
	void pushInputScissorRect(Rect2I bounds);
	void popInputScissorRect();
	bool isInputScissorActive();
	// NB: This is safe to call whether or not a scissor is active. It returns
	// either the scissor or a functionally infinite rectangle.
	Rect2I getInputScissorRect();

	// UI Rects
	// (Rectangle areas which block the mouse from interacting with the game
	// world. eg, stops you clicking through windows)
	void addUIRect(Rect2I bounds);
	bool mouseIsWithinUIRects(V2 mousePos);

	// Text
	Rect2I drawText(RenderBuffer *renderBuffer, BitmapFont *font, String text, V2I origin, u32 align, V4 color, s32 maxWidth = 0);

	// Buttons
	V2I calculateButtonSize(String text, UIButtonStyle *buttonStyle, s32 maxWidth = 0, bool fillWidth = true);
	V2I calculateButtonSize(V2I contentSize, UIButtonStyle *buttonStyle, s32 maxWidth = 0, bool fillWidth = true);
	bool putButton(Rect2I bounds, UIButtonStyle *style, ButtonState state = Button_Normal, RenderBuffer *renderBuffer = null, bool invisible = false, String tooltip = nullString);
	bool putTextButton(String text, Rect2I bounds, UIButtonStyle *style, ButtonState state = Button_Normal, RenderBuffer *renderBuffer = null, bool invisible = false, String tooltip=nullString);
	bool putImageButton(Sprite *sprite, Rect2I bounds, UIButtonStyle *style, ButtonState state = Button_Normal, RenderBuffer *renderBuffer = null, bool invisible = false, String tooltip=nullString);

	// Checkboxes
	V2I calculateCheckboxSize(UICheckboxStyle *style);
	void putCheckbox(bool *checked, Rect2I bounds, UICheckboxStyle *style, bool isDisabled = false, RenderBuffer *renderBuffer = null, bool invisible = false);

	// Drop-down lists
	template <typename T>
	V2I calculateDropDownListSize(Array<T> *listOptions, String (*getDisplayName)(T *data), UIDropDownListStyle *style, s32 maxWidth = 0, bool fillWidth = true);
	template <typename T>
	void putDropDownList(Array<T> *listOptions, s32 *currentSelection, String (*getDisplayName)(T *data), Rect2I bounds, UIDropDownListStyle *style, RenderBuffer *renderBuffer = null);

	// Menus
	void showMenu(s32 menuID);
	void hideMenus();
	void toggleMenuVisible(s32 menuID);
	bool isMenuVisible(s32 menuID);
	ScrollbarState *getMenuScrollbar();

	// TextInputs
	bool putTextInput(TextInput *textInput, UITextInputStyle *style, Rect2I bounds, RenderBuffer *renderBuffer = null, bool invisible = false);

	// Toasts
	// NB: `message` is copied into the UIState, so it can be a temporary allocation
	void pushToast(String message);
	void drawToast();

	// Tooltip
	void showTooltip(String text);
	void showTooltip(WindowProc tooltipProc, void *userData = null);
	void basicTooltipWindowProc(WindowContext *context, void *userData);
}
