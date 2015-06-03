#pragma once
// ui.h

struct UiButton {
	Rect rect;
	char *text;
	TTF_Font* font;
	Color textColor;
	Texture textTexture;
	Rect textRect; // NOT relative right now. Maybe will be later?

	Color backgroundColor;
	Color backgroundHoverColor;
};

#include "ui.cpp"