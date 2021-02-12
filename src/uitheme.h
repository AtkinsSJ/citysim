#pragma once

// TODO: Maybe rather than putting String names of styles (and fonts) in these, they should
// use a combined name-and-pointer, so that most of the time the pointer is used, but
// whenever the theme is modified the pointers can be re-connected.
// - Sam, 15/01/2020

enum UIStyleType {
	// NB: When changing this, make sure to change the lambdas in findStyle() to match!
	UIStyle_None = 0,
	UIStyle_Button = 1,
	UIStyle_Console,
	UIStyle_Label,
	UIStyle_Panel,
	UIStyle_Scrollbar,
	UIStyle_TextInput,
	UIStyle_Window,
	UIStyleTypeCount
};

struct UIStyleReference
{
	String name;
	UIStyleType styleType;

	void *pointer;

	UIStyleReference(UIStyleType type) : styleType(type) {}
};

enum UIDrawableType
{
	Drawable_None,
	Drawable_Color,
	Drawable_Image,
	Drawable_Gradient,
	Drawable_Ninepatch,
};

struct UIDrawableStyle
{
	UIDrawableStyle(UIDrawableType type)
		: type(type) {}

	UIDrawableStyle() : UIDrawableStyle(Drawable_None) 
	{}

	UIDrawableStyle(V4 color) : UIDrawableStyle(Drawable_Color)
	{
		this->color = color;
	}

	UIDrawableType type;
	union
	{
		V4 color;
	};
};
Maybe<UIDrawableStyle> readBackgroundStyle(struct LineReader *reader);

struct UIButtonStyle
{
	FontReference font;
	V4 textColor;
	u32 textAlignment;

	UIDrawableStyle background;
	UIDrawableStyle hoverBackground;
	UIDrawableStyle pressedBackground;
	UIDrawableStyle disabledBackground;

	s32 padding;
};

struct UILabelStyle
{
	FontReference font;
	V4 textColor;
};

struct UIPanelStyle
{
	s32 margin;
	s32 contentPadding;
	u32 widgetAlignment;
	
	UIDrawableStyle background;

	UIStyleReference buttonStyle    = UIStyleReference(UIStyle_Button);
	UIStyleReference labelStyle     = UIStyleReference(UIStyle_Label);
	UIStyleReference scrollbarStyle = UIStyleReference(UIStyle_Scrollbar);
	UIStyleReference textInputStyle = UIStyleReference(UIStyle_TextInput);
};

struct UITextInputStyle
{
	FontReference font;
	V4 textColor;
	u32 textAlignment;

	UIDrawableStyle background;
	s32 padding;
	
	bool showCaret;
	f32 caretFlashCycleDuration;
};

struct UIScrollbarStyle
{
	UIDrawableStyle background;
	V4 knobColor;
	s32 width;
};

struct UIWindowStyle
{
	s32 titleBarHeight;
	V4 titleBarColor;
	V4 titleBarColorInactive;
	FontReference titleFont;
	V4 titleColor;
	V4 titleBarButtonHoverColor;

	V2I offsetFromMouse;

	UIStyleReference panelStyle = UIStyleReference(UIStyle_Panel);
};

struct UIConsoleStyle
{
	FontReference font;
	V4 outputTextColor[CLS_COUNT];

	UIDrawableStyle background;
	s32 padding;

	UIStyleReference scrollbarStyle = UIStyleReference(UIStyle_Scrollbar);
	UIStyleReference textInputStyle = UIStyleReference(UIStyle_TextInput);
};

enum PropType
{
	PropType_Alignment,
	PropType_Background,
	PropType_Bool,
	PropType_Color,
	PropType_Float,
	PropType_Font,
	PropType_Int,
	PropType_String,
	PropType_Style,
	PropType_V2I,
	PropTypeCount
};

struct UIStyle
{
	UIStyleType type;
	String name;

	// PROPERTIES

	UIDrawableStyle background;
	UIDrawableStyle disabledBackground;
	UIDrawableStyle hoverBackground;
	UIDrawableStyle pressedBackground;

	// Alphabetically ordered, which... probably isn't the best. It's certainly ugly.
	UIStyleReference buttonStyle = UIStyleReference(UIStyle_Button);
	f32 caretFlashCycleDuration;
	u32 widgetAlignment;
	s32 contentPadding;
	FontReference font;
	V4 knobColor;
	UIStyleReference labelStyle = UIStyleReference(UIStyle_Label);
	s32 margin;
	V2I offsetFromMouse;
	V4 overlayColor;
	V4 outputTextColor[CLS_COUNT];
	s32 padding;
	UIStyleReference panelStyle = UIStyleReference(UIStyle_Panel);
	UIStyleReference scrollbarStyle = UIStyleReference(UIStyle_Scrollbar);
	bool showCaret;
	u32 textAlignment;
	V4 textColor;
	UIStyleReference textInputStyle = UIStyleReference(UIStyle_TextInput);
	V4 titleBarButtonHoverColor;
	V4 titleBarColor;
	V4 titleBarColorInactive;
	s32 titleBarHeight;
	V4 titleColor;
	FontReference titleFont;
	s32 width;
};

struct UIStylePack
{
	UIStyle styleByType[UIStyleTypeCount];
};

struct UIProperty
{
	PropType type;
	smm offsetInStyleStruct;
	bool existsInStyle[UIStyleTypeCount];
};

struct UITheme
{
	HashTable<UIProperty> styleProperties;

	// TODO: Remove this!
	V4 overlayColor;

	HashTable<String> fontNamesToAssetNames;

	HashTable<UIButtonStyle>    buttonStyles;
	HashTable<UIConsoleStyle>   consoleStyles;
	HashTable<UILabelStyle>     labelStyles;
	HashTable<UIPanelStyle>     panelStyles;
	HashTable<UIScrollbarStyle> scrollbarStyles;
	HashTable<UITextInputStyle> textInputStyles;
	HashTable<UIWindowStyle>    windowStyles;
};

void initUITheme(UITheme *theme);
void loadUITheme(Blob data, struct Asset *asset);

template <typename T>
T* findStyle(UITheme *theme, UIStyleReference *reference);

template <typename T>
inline T* findStyle(UIStyleReference *reference)
{
	return findStyle<T>(&assets->theme, reference);
}

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
inline UIPanelStyle *findPanelStyle(UITheme *theme, String name)
{
	auto style = find(&theme->panelStyles, name);
	if (style == null) logError("Unable to find panel style '{0}'"_s, {name});
	return style;
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
