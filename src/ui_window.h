#pragma once

// NB: A WindowContext only exists for a single call of a WindowProc.
// It isn't kept around between update and render, so attempting to save
// state here that you want to carry over from one to the other will not work!!!
// AKA, I messed up.
struct WindowContext
{
	struct UIState *uiState;
	struct Window *window;
	UIWindowStyle *windowStyle;

	bool doUpdate;
	bool doRender;

	Rect2I totalContentArea;
	Rect2I contentArea;
	V2I currentOffset;
	s32 largestItemWidth;
	s32 largestItemHeightOnLine;
	u32 alignment;
	s32 perItemPadding;

	s32 columnStartOffsetX;
	s32 columnScrollbarWidth;
	struct ScrollbarState *columnScrollbarState;

	// Results
	bool closeRequested;
};

typedef void (*WindowProc)(WindowContext*, void*);

enum WindowFlags
{
	WinFlag_AutomaticHeight = 1 << 0,
	WinFlag_Unique          = 1 << 1, // Only one window with the same WindowProc will be allowed. A new one will replace the old.
	WinFlag_Modal           = 1 << 2,
	WinFlag_Tooltip         = 1 << 3,
	WinFlag_Headless        = 1 << 4,
	WinFlag_ShrinkWidth     = 1 << 5,
};

struct Window
{
	String title;
	u32 flags;

	Rect2I area;
	String styleName;

	WindowProc windowProc;
	void *userData;
	WindowProc onClose;

	bool isInitialised;
	bool wasActiveLastUpdate;
};

//
// PUBLIC
//
void showWindow(UIState *uiState, String title, s32 width, s32 height, V2I position, String styleName, u32 flags, WindowProc windowProc, void *userData, WindowProc onClose=null);



// Columns!
//
// This is very basic right now and probably will need rewriting later to be more comprehensive.
// Start columns with window_beginColumns().
// Then, start each column with window_column(context, width) or window_columnPercent(context, widthPercent)
// For the final column, don't pass a width (or make it 0) and it will fill the remaining space.
// Then, call window_endColumns() to finish.
//
// Internally, this all works by modifying the context->contentArea, which is what the 
// window_label() etc functions use to lay themselves out. So, they don't have to know anything
// about columns or other layout complexities! So that's pretty nice.
void window_beginColumns(WindowContext *context, s32 height=0);
void window_column(WindowContext *context, s32 width=0, ScrollbarState *scrollbar=null);
void window_columnPercent(WindowContext *context, f32 widthPercent, ScrollbarState *scrollbar=null);
Rect2I window_getColumnArea(WindowContext *context);
void window_endColumns(WindowContext *context);
// Internal
void window_completeColumn(WindowContext *context);

Rect2I window_getCurrentLayoutPosition(WindowContext *context);
void window_completeWidget(WindowContext *context, V2I widgetSize);

// Elements
void window_label(WindowContext *context, String text, String styleName = nullString);
/*
 * If you pass textWidth, then the button will be sized as though the text was that size. If you leave
 * it blank (or pass -1 manually) then the button will be automatically sized to wrap the contained text.
 * Either way, it always matches the size vertically.
 */
bool window_button(WindowContext *context, String text, s32 textWidth = -1, bool isActive = false);

// Returns true if RETURN was pressed, same as updateTextInput().
bool window_textInput(WindowContext *context, TextInput *textInput, String styleName = nullString);

void updateWindows(UIState *uiState);
void renderWindows(UIState *uiState);

//
// INTERNAL
//
static void makeWindowActive(UIState *uiState, s32 windowIndex);
static Rect2I getWindowContentArea(Rect2I windowArea, s32 barHeight, s32 contentPadding);

WindowContext makeWindowContext(Window *window, UIWindowStyle *windowStyle, UIState *uiState);
void prepareForUpdate(WindowContext *context);
void prepareForRender(WindowContext *context);
