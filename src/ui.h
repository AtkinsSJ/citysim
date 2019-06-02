#pragma once
// ui.h

struct Tooltip
{
	bool show;
	String styleName;
	String text;
};

const f32 uiMessageDisplayTime = 2.0f;
const f32 uiMessageBottomMargin = 4;
const f32 uiMessageTextPadding = 4;
struct UiMessage
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

	Tooltip tooltip;
	UiMessage message;

	// TODO: Replace this with better "this input has already been used" code!
	Array<Rect2> uiRects;

	s32 openMenu;

	String currentCursor;
	bool cursorIsVisible;

	// UI elements that react to the mouse should only do so if this is false - and then
	// they should set it to true. 
	bool mouseInputHandled;

	// Window stuff
	ChunkedArray<Window> openWindows;
	bool isDraggingWindow;
	V2 windowDragWindowStartPos;
};

void setCursor(UIState *uiState, String cursorName)
{
	DEBUG_FUNCTION();
	
	Asset *newCursorAsset = getAsset(uiState->assets, AssetType_Cursor, cursorName);
	if (newCursorAsset != null)
	{
		uiState->currentCursor = cursorName;
		SDL_SetCursor(newCursorAsset->cursor.sdlCursor);
	}
}
inline void setCursor(UIState *uiState, char *cursorName)
{
	setCursor(uiState, makeString(cursorName));
}
