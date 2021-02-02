#pragma once

// TODO: Maybe rather than putting String names of styles (and fonts) in these, they should
// use a combined name-and-pointer, so that most of the time the pointer is used, but
// whenever the theme is modified the pointers can be re-connected.
// - Sam, 15/01/2020

enum SectionType {
	Section_None = 0,
	Section_Button = 1,
	Section_Console,
	Section_Label,
	Section_UIMessage,
	Section_PopupMenu,
	Section_Scrollbar,
	Section_TextInput,
	Section_Window,
	SectionTypeCount
};

struct UIStyleReference
{
	String name;
	SectionType styleType;

	void *pointer;

	UIStyleReference(SectionType type) : styleType(type) {}
};

struct UIButtonStyle
{
	FontReference font;
	V4 textColor;
	u32 textAlignment;

	V4 backgroundColor;
	V4 hoverBackgroundColor;
	V4 pressedBackgroundColor;
	V4 disabledBackgroundColor;

	s32 padding;
};

struct UILabelStyle
{
	FontReference font;
	V4 textColor;
};

struct UIMessageStyle
{
	FontReference font;
	V4 textColor;

	V4 backgroundColor;
	s32 padding;
};

struct UIPopupMenuStyle
{
	s32 margin;
	s32 contentPadding;
	V4 backgroundColor;

	UIStyleReference buttonStyle = UIStyleReference(Section_Button);
	UIStyleReference scrollbarStyle = UIStyleReference(Section_Scrollbar);
};

struct UITextInputStyle
{
	FontReference font;
	V4 textColor;
	u32 textAlignment;

	V4 backgroundColor;
	s32 padding;
	
	bool showCaret;
	f32 caretFlashCycleDuration;
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
	FontReference titleFont;
	V4 titleColor;
	V4 titleBarButtonHoverColor;

	V4 backgroundColor;
	V4 backgroundColorInactive;

	s32 margin;
	s32 contentPadding;

	V2I offsetFromMouse;

	UIStyleReference buttonStyle = UIStyleReference(Section_Button);
	UIStyleReference labelStyle = UIStyleReference(Section_Label);
	UIStyleReference scrollbarStyle = UIStyleReference(Section_Scrollbar);
};

struct UIConsoleStyle
{
	FontReference font;
	V4 outputTextColor[CLS_COUNT];

	V4 backgroundColor;
	s32 padding;

	UIStyleReference scrollbarStyle = UIStyleReference(Section_Scrollbar);
	UIStyleReference textInputStyle = UIStyleReference(Section_TextInput);
};

enum PropType
{
	PropType_Alignment,
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
	SectionType type;
	String name;

	// PROPERTIES
	V4 backgroundColorInactive;
	V4 backgroundColor;
	UIStyleReference buttonStyle = UIStyleReference(Section_Button);
	f32 caretFlashCycleDuration;
	s32 contentPadding;
	V4 disabledBackgroundColor;
	FontReference font;
	V4 hoverBackgroundColor;
	V4 knobColor;
	UIStyleReference labelStyle = UIStyleReference(Section_Label);
	s32 margin;
	V2I offsetFromMouse;
	V4 overlayColor;
	V4 outputTextColor[CLS_COUNT];
	s32 padding;
	V4 pressedBackgroundColor;
	UIStyleReference scrollbarStyle = UIStyleReference(Section_Scrollbar);
	bool showCaret;
	u32 textAlignment;
	V4 textColor;
	UIStyleReference textInputStyle = UIStyleReference(Section_TextInput);
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
	UIStyle styleByType[SectionTypeCount];
};

struct UIProperty
{
	PropType type;
	smm offsetInStyleStruct;
	bool existsInStyle[SectionTypeCount];
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
	HashTable<UIMessageStyle>   messageStyles;
	HashTable<UIPopupMenuStyle> popupMenuStyles;
	HashTable<UIScrollbarStyle> scrollbarStyles;
	HashTable<UITextInputStyle> textInputStyles;
	HashTable<UIWindowStyle>    windowStyles;
};

void initUITheme(UITheme *theme);
void loadUITheme(Blob data, struct Asset *asset);

template <typename T>
T* findStyle(UITheme *theme, UIStyleReference *reference);

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
inline UIPopupMenuStyle *findPopupMenuStyle(UITheme *theme, String name)
{
	return find(&theme->popupMenuStyles, name);
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
