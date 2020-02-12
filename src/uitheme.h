#pragma once

// TODO: Maybe rather than putting String names of styles (and fonts) in these, they should
// use a combined name-and-pointer, so that most of the time the pointer is used, but
// whenever the theme is modified the pointers can be re-connected.
// - Sam, 15/01/2020

struct UIButtonStyle
{
	String fontName;
	V4 textColor;
	u32 textAlignment;

	V4 backgroundColor;
	V4 hoverColor;
	V4 pressedColor;

	s32 padding;
};

struct UILabelStyle
{
	String fontName;
	V4 textColor;
};

struct UITextInputStyle
{
	String fontName;
	V4 textColor;
	u32 textAlignment;

	V4 backgroundColor;
	s32 padding;
	
	bool showCaret;
	f32 caretFlashCycleDuration;
};

struct UIMessageStyle
{
	String fontName;
	V4 textColor;

	V4 backgroundColor;
	s32 padding;
};

struct UIScrollbarStyle
{
	V4 backgroundColor;
	V4 knobColor;
	s32 width;
};

struct UIWindowStyle
{
	s32 titleBarHeight;
	V4 titleBarColor;
	V4 titleBarColorInactive;
	String titleFontName;
	V4 titleColor;
	V4 titleBarButtonHoverColor;

	V4 backgroundColor;
	V4 backgroundColorInactive;

	s32 contentPadding;

	V2I offsetFromMouse;

	String buttonStyleName;
	String labelStyleName;
};

struct UIConsoleStyle
{
	String fontName;
	V4 outputTextColor[CLS_COUNT];

	V4 backgroundColor;
	s32 padding;

	String scrollbarStyleName;
	String textInputStyleName;
};

struct UITheme
{
	// TODO: Remove this?
	V4 overlayColor;

	HashTable<String> fontNamesToAssetNames;

	HashTable<UIButtonStyle>    buttonStyles;
	HashTable<UIConsoleStyle>   consoleStyles;
	HashTable<UILabelStyle>     labelStyles;
	HashTable<UIMessageStyle>   messageStyles;
	HashTable<UIScrollbarStyle> scrollbarStyles;
	HashTable<UITextInputStyle> textInputStyles;
	HashTable<UIWindowStyle>    windowStyles;
};

void initUITheme(UITheme *theme);
void loadUITheme(Blob data, struct Asset *asset);

inline UIButtonStyle *findButtonStyle(UITheme *theme, String name)
{
	return find(&theme->buttonStyles, name);
}
inline UIConsoleStyle *findConsoleStyle(UITheme *theme, String name)
{
	return find(&theme->consoleStyles, name);
}
inline UILabelStyle *findLabelStyle(UITheme *theme, String name)
{
	return find(&theme->labelStyles, name);
}
inline UIMessageStyle *findMessageStyle(UITheme *theme, String name)
{
	return find(&theme->messageStyles, name);
}
inline UIScrollbarStyle *findScrollbarStyle(UITheme *theme, String name)
{
	return find(&theme->scrollbarStyles, name);
}
inline UITextInputStyle *findTextInputStyle(UITheme *theme, String name)
{
	return find(&theme->textInputStyles, name);
}
inline UIWindowStyle *findWindowStyle(UITheme *theme, String name)
{
	return find(&theme->windowStyles, name);
}
