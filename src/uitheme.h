#pragma once

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

struct UITextBoxStyle
{
	String fontName;
	V4 textColor;
	V4 backgroundColor;
};

struct UIMessageStyle
{
	String fontName;
	V4 textColor;

	V4 backgroundColor;
	s32 padding;
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
	V4 inputTextColor;
	V4 inputBackgroundColor;
	s32 padding;

	// Scrollbar
	s32 scrollBarWidth;
	V4 scrollBarKnobColor;
};

struct UITheme
{
	// TODO: Remove this?
	V4 overlayColor;

	HashTable<String> fontNamesToAssetNames;

	HashTable<UIButtonStyle>  buttonStyles;
	HashTable<UIConsoleStyle> consoleStyles;
	HashTable<UILabelStyle>   labelStyles;
	HashTable<UIMessageStyle> messageStyles;
	HashTable<UITextBoxStyle> textBoxStyles;
	HashTable<UIWindowStyle>  windowStyles;
};

void initUITheme(UITheme *theme);
void loadUITheme(Blob data, struct Asset *asset);

inline UIButtonStyle *findButtonStyle(UITheme *theme, String name)
{
	return find(&theme->buttonStyles, name);
}
inline UILabelStyle *findLabelStyle(UITheme *theme, String name)
{
	return find(&theme->labelStyles, name);
}
inline UIMessageStyle *findMessageStyle(UITheme *theme, String name)
{
	return find(&theme->messageStyles, name);
}
inline UITextBoxStyle *findTextBoxStyle(UITheme *theme, String name)
{
	return find(&theme->textBoxStyles, name);
}
inline UIWindowStyle *findWindowStyle(UITheme *theme, String name)
{
	return find(&theme->windowStyles, name);
}
inline UIConsoleStyle *findConsoleStyle(UITheme *theme, String name)
{
	return find(&theme->consoleStyles, name);
}
