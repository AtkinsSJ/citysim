#pragma once
// ui.h

struct UiLabel {
	Rect rect;
	char *text;
	TTF_Font *font;
	Color color;
	Texture texture;
};

struct UiButton {
	Rect rect;

	UiLabel text;

	Color backgroundColor;
	Color backgroundHoverColor;
	Color backgroundPressedColor;

	bool mouseOver;
	bool clickStarted;
	bool active;
};


#include "ui.cpp"