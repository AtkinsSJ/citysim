#pragma once
// ui.h

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

struct RenderBuffer;
struct BitmapFont;
struct Sprite;

namespace UI
{
	// Annoyingly, we have to predeclare all the style types. Yay for C++
	// (I tried rearranging our includes, it got messy fast.)
	// I guess the alternative is switching to an individual-files build. Might be a good idea.
	struct ButtonStyle;
	struct CheckboxStyle;
	struct DropDownListStyle;
	struct LabelStyle;
	struct PanelStyle;
	struct RadioButtonStyle;
	struct ScrollbarStyle;
	struct SliderStyle;
	struct TextInputStyle;

	struct TextInput;
	struct Panel;
	struct Window;
	struct WindowContext;

	struct ScrollbarState
	{
		Orientation orientation;
		s32 contentSize;
		f32 scrollPercent;

		s32 mouseWheelStepSize;
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

	typedef void (*WindowProc)(WindowContext*, void*);

	struct UIState
	{

		// UI elements that react to the mouse should only do so if this is false - and then
		// they should set it to true. 
		bool mouseInputHandled;
		Stack<Rect2I> inputScissorRects;

		// Dragging stuff
		void *currentDragObject;
		V2I dragObjectStartPos;

		String tooltipText;

		// TODO: Replace this with better "this input has already been used" code!
		ChunkedArray<Rect2I> uiRects;

		s32 openMenu;
		ScrollbarState openMenuScrollbar;

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
		bool isAPauseWindowOpen; // Do any open windows have the WindowFlags::Pause flag?
	};
	UIState uiState;

	// Cached input values
	// These are updated in startFrame()
	V2I windowSize;
	V2I mousePos;
	V2I mouseClickStartPos;

// METHODS

	void init(MemoryArena *arena);
	void startFrame();
	void endFrame();

	// Input
	bool isMouseInputHandled();
	void markMouseInputHandled();
	bool isMouseInUIBounds(Rect2I bounds);
	bool isMouseInUIBounds(Rect2I bounds, V2I mousePos);
	bool justClickedOnUI(Rect2I bounds);

	struct WidgetMouseState
	{
		bool isHovered;
		bool isPressed;
	};
	WidgetMouseState getWidgetMouseState(Rect2I bounds);

	// Dragging!
	// After calling startDragging() with a start position, you can repeatedly call
	// getDraggingObjectPos() to find out where the object should be now. You'll have to
	// clamp the position yourself before using it.
	// The *object pointer can be anything that uniquely identifies what you want to drag.
	// (eg, in Sliders, we are using the *currentValue pointer.)
	// NB: The drag is automatically ended as soon as MouseButton_Left is released, hence
	// there is no stopDragging() function.
	bool isDragging(void *object);
	void startDragging(void *object, V2I objectPos);
	V2I getDraggingObjectPos();

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
	bool mouseIsWithinUIRects();

	// Text
	[[deprecated]]
	Rect2I drawText(RenderBuffer *renderBuffer, BitmapFont *font, String text, V2I origin, u32 align, V4 color, s32 maxWidth = 0);

	// Buttons
	V2I calculateButtonSize(String text, ButtonStyle *style = null, s32 maxWidth = 0, bool fillWidth = true);
	V2I calculateButtonSize(V2I contentSize, ButtonStyle *style = null, s32 maxWidth = 0, bool fillWidth = true);
	Rect2I calculateButtonContentBounds(Rect2I bounds, ButtonStyle *style = null);
	bool putButton(Rect2I bounds, ButtonStyle *style = null, ButtonState state = Button_Normal, RenderBuffer *renderBuffer = null, String tooltip = nullString);
	bool putTextButton(String text, Rect2I bounds, ButtonStyle *style = null, ButtonState state = Button_Normal, RenderBuffer *renderBuffer = null, String tooltip=nullString);
	bool putImageButton(Sprite *sprite, Rect2I bounds, ButtonStyle *style = null, ButtonState state = Button_Normal, RenderBuffer *renderBuffer = null, String tooltip=nullString);

	// Checkboxes
	V2I calculateCheckboxSize(CheckboxStyle *style = null);
	void putCheckbox(bool *checked, Rect2I bounds, CheckboxStyle *style = null, bool isDisabled = false, RenderBuffer *renderBuffer = null);

	// Drop-down lists
	template <typename T>
	V2I calculateDropDownListSize(Array<T> *listOptions, String (*getDisplayName)(T *data), DropDownListStyle *style = null, s32 maxWidth = 0, bool fillWidth = true);
	template <typename T>
	void putDropDownList(Array<T> *listOptions, s32 *currentSelection, String (*getDisplayName)(T *data), Rect2I bounds, DropDownListStyle *style = null, RenderBuffer *renderBuffer = null);
	void openDropDownList(void *pointer);
	void closeDropDownList();
	bool isDropDownListOpen(void *pointer);

	// Labels
	V2I calculateLabelSize(String text, LabelStyle *style = null, s32 maxWidth = 0, bool fillWidth = true);
	void putLabel(String text, Rect2I bounds, LabelStyle *style = null, RenderBuffer *renderBuffer = null);

	// Menus
	void showMenu(s32 menuID);
	void hideMenus();
	void toggleMenuVisible(s32 menuID);
	bool isMenuVisible(s32 menuID);
	ScrollbarState *getMenuScrollbar();

	// Radio Buttons
	V2I calculateRadioButtonSize(RadioButtonStyle *style = null);
	void putRadioButton(s32 *selectedValue, s32 value, Rect2I bounds, RadioButtonStyle *style = null, bool isDisabled = false, RenderBuffer *renderBuffer = null);

	// Scrollbars
	void initScrollbar(ScrollbarState *state, Orientation orientation, s32 mouseWheelStepSize = 64);
	// NB: When the viewport is larger than the content, there's no thumb rect so nothing is returned
	Maybe<Rect2I> getScrollbarThumbBounds(ScrollbarState *state, Rect2I scrollbarBounds, ScrollbarStyle *style);
	void putScrollbar(ScrollbarState *state, s32 contentSize, Rect2I bounds, ScrollbarStyle *style = null, bool isDisabled = false, RenderBuffer *renderBuffer = null);
	s32 getScrollbarContentOffset(ScrollbarState *state, s32 scrollbarSize);

	// Sliders
	// NB: fillSpace only applies to the "length" of the slider, not its "thickness"
	V2I calculateSliderSize(Orientation orientation, SliderStyle *style = null, V2I availableSpace = {}, bool fillSpace = false);
	void putSlider(f32 *currentValue, f32 minValue, f32 maxValue, Orientation orientation, Rect2I bounds, SliderStyle *style = null, bool isDisabled = false, RenderBuffer *renderBuffer = null, bool snapToWholeNumbers = false);
	void putSlider(s32 *currentValue, s32 minValue, s32 maxValue, Orientation orientation, Rect2I bounds, SliderStyle *style = null, bool isDisabled = false, RenderBuffer *renderBuffer = null);

	// TextInputs
	bool putTextInput(TextInput *textInput, Rect2I bounds, TextInputStyle *style = null, RenderBuffer *renderBuffer = null);

	// Toasts
	// NB: `message` is copied into the UIState, so it can be a temporary allocation
	void pushToast(String message);
	void drawToast();

	// Tooltip
	void showTooltip(String text);
	void showTooltip(WindowProc tooltipProc, void *userData = null);
	void basicTooltipWindowProc(WindowContext *context, void *userData);
}
