#pragma once
// ui.h

enum Alignment {
	ALIGN_LEFT = 1,
	ALIGN_H_CENTER = 2,
	ALIGN_RIGHT = 4,

	ALIGN_H = ALIGN_LEFT | ALIGN_H_CENTER | ALIGN_RIGHT,

	ALIGN_TOP = 8,
	ALIGN_V_CENTER = 16,
	ALIGN_BOTTOM = 32,
	
	ALIGN_V = ALIGN_TOP | ALIGN_V_CENTER | ALIGN_BOTTOM,

	ALIGN_CENTER = ALIGN_H_CENTER | ALIGN_V_CENTER,
};

struct UiLabel {
	V2 origin;
	int32 align; // See Alignment enum

	RealRect _rect;
	char *text;
	TTF_Font *font;
	Color color;
	//Texture texture;
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