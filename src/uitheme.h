#pragma once

// TODO: make this an enum class? For the nicer namespacing
enum ConsoleLineStyleID
{
	// Must match the order in ConsoleStyle!
	CLS_Default,
	CLS_InputEcho,
	CLS_Error,
	CLS_Success,
	CLS_Warning,

	CLS_COUNT
};

struct LineReader;

namespace UI
{
	enum DrawableType
	{
		Drawable_None,
		Drawable_Color,
		Drawable_Gradient,
		Drawable_Ninepatch,
		Drawable_Sprite,
	};

	struct DrawableStyle
	{
		DrawableType type;
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

	// METHODS
		bool hasFixedSize();
		V2I getSize(); // NB: Returns 0 for sizeless types
	};
	Maybe<DrawableStyle> readDrawableStyle(LineReader *reader);

	enum StyleType {
		Style_None = 0,
		Style_Button = 1,
		Style_Checkbox,
		Style_Console,
		Style_DropDownList,
		Style_Label,
		Style_Panel,
		Style_Scrollbar,
		Style_Slider,
		Style_TextInput,
		Style_Window,
		StyleTypeCount
	};

	struct ButtonStyle
	{
		String name;

		AssetRef font;
		V4 textColor;
		u32 textAlignment;

		Padding padding;
		s32 contentPadding; // Between icons and content

		DrawableStyle startIcon;
		u32 startIconAlignment;

		DrawableStyle endIcon;
		u32 endIconAlignment;

		DrawableStyle background;
		DrawableStyle backgroundHover;
		DrawableStyle backgroundPressed;
		DrawableStyle backgroundDisabled;
	};

	struct CheckboxStyle
	{
		String name;

		Padding padding;

		DrawableStyle background;
		DrawableStyle backgroundHover;
		DrawableStyle backgroundPressed;
		DrawableStyle backgroundDisabled;

		V2I contentSize;
		DrawableStyle check;
		DrawableStyle checkHover;
		DrawableStyle checkPressed;
		DrawableStyle checkDisabled;
	};

	struct ConsoleStyle
	{
		String name;
		
		AssetRef font;
		union {
			V4 outputTextColors[CLS_COUNT];

			struct {
				// Must match the order in ConsoleLineStyleID!
				V4 outputTextColor;
				V4 outputTextColorInputEcho;
				V4 outputTextColorError;
				V4 outputTextColorSuccess;
				V4 outputTextColorWarning;
			};
		};

		DrawableStyle background;
		Padding padding;

		AssetRef scrollbarStyle;
		AssetRef textInputStyle;
	};

	struct DropDownListStyle
	{
		String name;

		// For now, we'll just piggy-back off of other styles:
		AssetRef buttonStyle; // For the normal state
		AssetRef panelStyle;  // For the drop-down state
	};

	struct LabelStyle
	{
		String name;
		
		AssetRef font;
		V4 textColor;
	};

	struct PanelStyle
	{
		String name;
		
		Padding padding;
		s32 contentPadding;
		u32 widgetAlignment;
		
		DrawableStyle background;

		AssetRef buttonStyle;
		AssetRef checkboxStyle;
		AssetRef dropDownListStyle;
		AssetRef labelStyle;
		AssetRef scrollbarStyle;
		AssetRef sliderStyle;
		AssetRef textInputStyle;
	};

	struct ScrollbarStyle
	{
		String name;

		s32 width;
		
		DrawableStyle background;
		
		DrawableStyle thumb;
		DrawableStyle thumbHover;
		DrawableStyle thumbPressed;
		DrawableStyle thumbDisabled;
	};

	struct SliderStyle
	{
		String name;
		
		DrawableStyle track;
		s32 trackThickness;

		DrawableStyle thumb;
		DrawableStyle thumbHover;
		DrawableStyle thumbPressed;
		DrawableStyle thumbDisabled;
		V2I thumbSize;
	};

	struct TextInputStyle
	{
		String name;
		
		AssetRef font;
		V4 textColor;
		u32 textAlignment;

		DrawableStyle background;
		Padding padding;
		
		bool showCaret;
		f32 caretFlashCycleDuration;
	};

	struct WindowStyle
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

	enum class PropType
	{
		Alignment,
		Bool,
		Color,
		Drawable,
		Float,
		Font,
		Int,
		Padding,
		String,
		Style,
		V2I,
		PropTypeCount
	};

	struct Property
	{
		PropType type;
		smm offsetInStyleStruct;
		bool existsInStyle[StyleTypeCount];
	};

	struct Style
	{
		StyleType type;
		String name;

		// PROPERTIES
		Padding padding;
		s32 contentPadding;
		V2I contentSize;
		V2I offsetFromMouse;
		s32 width;
		u32 widgetAlignment;

		DrawableStyle background;
		DrawableStyle backgroundDisabled;
		DrawableStyle backgroundHover;
		DrawableStyle backgroundPressed;

		String buttonStyle;
		String checkboxStyle;
		String dropDownListStyle;
		String labelStyle;
		String panelStyle;
		String scrollbarStyle;
		String sliderStyle;
		String textInputStyle;

		DrawableStyle startIcon;
		u32 startIconAlignment;
		
		DrawableStyle endIcon;
		u32 endIconAlignment;
		
		bool showCaret;
		f32 caretFlashCycleDuration;

		DrawableStyle track;
		s32 trackThickness;
		DrawableStyle thumb;
		DrawableStyle thumbHover;
		DrawableStyle thumbPressed;
		DrawableStyle thumbDisabled;
		V2I thumbSize;
		
		V4 overlayColor;

		AssetRef font;
		u32 textAlignment;
		V4 textColor;
		
		V4 titleBarButtonHoverColor;
		V4 titleBarColor;
		V4 titleBarColorInactive;
		s32 titleBarHeight;
		V4 titleColor;
		AssetRef titleFont;

		// Checkbox specific
		DrawableStyle check;
		DrawableStyle checkHover;
		DrawableStyle checkPressed;
		DrawableStyle checkDisabled;

		// Console
		V4 outputTextColor;
		V4 outputTextColorInputEcho;
		V4 outputTextColorError;
		V4 outputTextColorSuccess;
		V4 outputTextColorWarning;
	};

	HashTable<Property> styleProperties;
	HashTable<StyleType> styleTypesByName;
	void initStyleConstants();
	void assignStyleProperties(StyleType type, std::initializer_list<String> properties);

	template <typename T>
	void setPropertyValue(Style *style, Property *property, T value);
}

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
template <> UI::ButtonStyle			*findStyle<UI::ButtonStyle>			(String styleName);
template <> UI::CheckboxStyle		*findStyle<UI::CheckboxStyle>		(String styleName);
template <> UI::ConsoleStyle		*findStyle<UI::ConsoleStyle>		(String styleName);
template <> UI::DropDownListStyle 	*findStyle<UI::DropDownListStyle>	(String styleName);
template <> UI::LabelStyle			*findStyle<UI::LabelStyle>			(String styleName);
template <> UI::PanelStyle			*findStyle<UI::PanelStyle>			(String styleName);
template <> UI::ScrollbarStyle		*findStyle<UI::ScrollbarStyle>		(String styleName);
template <> UI::SliderStyle			*findStyle<UI::SliderStyle>			(String styleName);
template <> UI::TextInputStyle		*findStyle<UI::TextInputStyle>		(String styleName);
template <> UI::WindowStyle			*findStyle<UI::WindowStyle>			(String styleName);