#pragma once

struct UIButtonStyle
{
	String fontName;
	V4 textColor;
	u32 textAlignment;

	V4 backgroundColor;
	V4 hoverColor;
	V4 pressedColor;

	f32 padding;
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

struct UITooltipStyle
{
	String fontName;
	V4 textColor;

	V4 backgroundColor;
	f32 padding;

	V2 offsetFromCursor;
};

struct UIMessageStyle
{
	String fontName;
	V4 textColor;

	V4 backgroundColor;
	f32 padding;
};

struct UIWindowStyle
{
	f32 titleBarHeight;
	V4 titleBarColor;
	V4 titleBarColorInactive;
	String titleFontName;
	V4 titleColor;
	V4 titleBarButtonHoverColor;

	V4 backgroundColor;
	V4 backgroundColorInactive;

	f32 contentPadding;

	String buttonStyleName;
	String labelStyleName;
};

struct UITheme
{
	// TODO: Remove this?
	V4 overlayColor;

	HashTable<String> fontNamesToAssetNames;

	HashTable<UIButtonStyle>  buttonStyles;
	HashTable<UILabelStyle>   labelStyles;
	HashTable<UITooltipStyle> tooltipStyles;
	HashTable<UIMessageStyle> messageStyles;
	HashTable<UITextBoxStyle> textBoxStyles;
	HashTable<UIWindowStyle>  windowStyles;
};

void initUITheme(UITheme *theme);
void loadUITheme(struct AssetManager *assets, Blob data, struct Asset *asset);

inline UIButtonStyle *findButtonStyle(UITheme *theme, String name)
{
	return find(&theme->buttonStyles, name);
}
inline UILabelStyle *findLabelStyle(UITheme *theme, String name)
{
	return find(&theme->labelStyles, name);
}
inline UITooltipStyle *findTooltipStyle(UITheme *theme, String name)
{
	return find(&theme->tooltipStyles, name);
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
