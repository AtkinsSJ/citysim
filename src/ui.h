#pragma once
// ui.h

struct UiLabel {
	V2 origin;
	int32 align; // See Alignment enum

	char *text;
	BitmapFont *font;
	V4 color;
	BitmapFontCachedText *cache;

	bool hasBackground;
	V4 backgroundColor;
	real32 backgroundPadding;
};

struct UiButton {
	RealRect rect;

	UiLabel text;

	V4 backgroundColor;
	V4 backgroundHoverColor;
	V4 backgroundPressedColor;

	SDL_Scancode shortcutKey;
	char *tooltip;

	bool mouseOver;
	bool clickStarted;
	bool justClicked;
	bool active;
};

struct UiButtonGroup {
	int32 buttonCount;
	UiButton buttons[8]; // TODO: Decide how many buttons we need
	UiButton *activeButton;
};

const real32 uiMessageBottomMargin = 4,
			uiMessageTextPadding = 4;
struct UiMessage {
	GLRenderer *renderer;
	RealRect rect;
	V4 background;
	UiLabel label;
	int32 messageCountdown; // In milliseconds
};

struct Tooltip {
	UiLabel label;
	bool show;
	V2 offsetFromCursor;
	char buffer[128];
};
void showTooltip(Tooltip *tooltip, GLRenderer *renderer, char *text);

#include "ui.cpp"