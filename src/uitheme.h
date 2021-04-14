#pragma once

enum UIDrawableType
{
	Drawable_None,
	Drawable_Color,
	Drawable_Gradient,
	Drawable_Ninepatch,
	Drawable_Sprite,
};

struct UIDrawableStyle
{
	UIDrawableType type;
	V4 color;

	union
	{
		struct {
			V4 color00;
			V4 color01;
			V4 color10;
			V4 color11;
		} gradient;

		AssetRef ninepatch;

		SpriteRef sprite;
	};
};
Maybe<UIDrawableStyle> readDrawableStyle(struct LineReader *reader);

struct UIButtonStyle
{
	String name;

	AssetRef font;
	V4 textColor;
	u32 textAlignment;

	UIDrawableStyle background;
	UIDrawableStyle hoverBackground;
	UIDrawableStyle pressedBackground;
	UIDrawableStyle disabledBackground;

	s32 padding;
};

enum ConsoleLineStyleID
{
	CLS_Default,
	CLS_InputEcho,
	CLS_Success,
	CLS_Warning,
	CLS_Error,

	CLS_COUNT
};

struct UIConsoleStyle
{
	String name;
	
	AssetRef font;
	V4 outputTextColor[CLS_COUNT];

	UIDrawableStyle background;
	s32 padding;

	AssetRef scrollbarStyle;
	AssetRef textInputStyle;
};

struct UILabelStyle
{
	String name;
	
	AssetRef font;
	V4 textColor;
};

struct UIPanelStyle
{
	String name;
	
	s32 margin;
	s32 contentPadding;
	u32 widgetAlignment;
	
	UIDrawableStyle background;

	AssetRef buttonStyle;
	AssetRef labelStyle;
	AssetRef scrollbarStyle;
	AssetRef textInputStyle;
};

struct UIScrollbarStyle
{
	String name;
	
	UIDrawableStyle background;
	UIDrawableStyle thumb;
	s32 width;
};

struct UITextInputStyle
{
	String name;
	
	AssetRef font;
	V4 textColor;
	u32 textAlignment;

	UIDrawableStyle background;
	s32 padding;
	
	bool showCaret;
	f32 caretFlashCycleDuration;
};

struct UIWindowStyle
{
	String name;
	
	s32 titleBarHeight;
	V4 titleBarColor;
	V4 titleBarColorInactive;
	AssetRef titleFont;
	V4 titleColor;
	V4 titleBarButtonHoverColor;

	V2I offsetFromMouse;

	AssetRef panelStyle;
};

enum PropType
{
	PropType_Alignment,
	PropType_Bool,
	PropType_Color,
	PropType_Drawable,
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
	String buttonStyle;
	f32 caretFlashCycleDuration;
	u32 widgetAlignment;
	s32 contentPadding;
	AssetRef font;
	UIDrawableStyle thumb;
	String labelStyle;
	s32 margin;
	V2I offsetFromMouse;
	V4 overlayColor;
	V4 outputTextColor[CLS_COUNT];
	s32 padding;
	String panelStyle;
	String scrollbarStyle;
	bool showCaret;
	u32 textAlignment;
	V4 textColor;
	String textInputStyle;
	V4 titleBarButtonHoverColor;
	V4 titleBarColor;
	V4 titleBarColorInactive;
	s32 titleBarHeight;
	V4 titleColor;
	AssetRef titleFont;
	s32 width;
};

struct UIProperty
{
	PropType type;
	smm offsetInStyleStruct;
	bool existsInStyle[UIStyleTypeCount];
};

HashTable<UIProperty> uiStyleProperties;
void initUIStyleProperties();

void loadUITheme(Blob data, struct Asset *asset);

template <typename T>
T* findStyle(AssetRef *reference);

template <typename T>
inline T* findStyle(String styleName, AssetRef *defaultStyle)
{
	T *result = null;
	if (!isEmpty(styleName)) result = findStyle<T>(styleName);
	if (result == null)      result = findStyle<T>(defaultStyle);

	return result;
}

template <typename T>
T* findStyle(String styleName);
template <> UIButtonStyle    *findStyle<UIButtonStyle>   (String styleName);
template <> UIConsoleStyle   *findStyle<UIConsoleStyle>  (String styleName);
template <> UILabelStyle     *findStyle<UILabelStyle>    (String styleName);
template <> UIPanelStyle     *findStyle<UIPanelStyle>    (String styleName);
template <> UIScrollbarStyle *findStyle<UIScrollbarStyle>(String styleName);
template <> UITextInputStyle *findStyle<UITextInputStyle>(String styleName);
template <> UIWindowStyle    *findStyle<UIWindowStyle>   (String styleName);
