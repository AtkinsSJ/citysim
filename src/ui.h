#pragma once
// ui.h

struct UiLabel {
	V2 origin;
	int32 align; // See Alignment enum

	// RealRect _rect;
	//Texture texture;

	char *text;
	BitmapFont *font;
	Color color;
	BitmapFontCachedText *cache;
};

struct UiIntLabel {
	UiLabel label;
	int32 *value;
	int32 lastValue;
	char buffer[128];
	char *formatString;
};

struct UiButton {
	RealRect rect;

	UiLabel text;

	Color backgroundColor;
	Color backgroundHoverColor;
	Color backgroundPressedColor;

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

struct UiMessage {
	GLRenderer *renderer;
	RealRect rect;
	Color background;
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