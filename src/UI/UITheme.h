/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/AssetRef.h>
#include <Assets/Sprite.h>
#include <Util/HashTable.h>
#include <Util/Vector.h>

enum class ConsoleLineStyle : u8 {
    // Must match the order in ConsoleStyle!
    Default,
    InputEcho,
    Error,
    Success,
    Warning,

    COUNT
};

struct LineReader;

namespace UI {

enum DrawableType {
    Drawable_None,
    Drawable_Color,
    Drawable_Gradient,
    Drawable_Ninepatch,
    Drawable_Sprite,
};

struct DrawableStyle {
    DrawableType type;
    V4 color;

    union {
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
Maybe<DrawableStyle> readDrawableStyle(LineReader* reader);

enum StyleType {
    Style_None = 0,
    Style_Button = 1,
    Style_Checkbox,
    Style_Console,
    Style_DropDownList,
    Style_Label,
    Style_Panel,
    Style_RadioButton,
    Style_Scrollbar,
    Style_Slider,
    Style_TextInput,
    Style_Window,
    StyleTypeCount
};

struct ButtonStyle {
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

struct CheckboxStyle {
    String name;

    Padding padding;

    DrawableStyle background;
    DrawableStyle backgroundHover;
    DrawableStyle backgroundPressed;
    DrawableStyle backgroundDisabled;

    V2I checkSize;
    DrawableStyle check;
    DrawableStyle checkHover;
    DrawableStyle checkPressed;
    DrawableStyle checkDisabled;
};

struct ConsoleStyle {
    String name;

    AssetRef font;
    union {
        V4 outputTextColors[to_underlying(ConsoleLineStyle::COUNT)];

        struct {
            // Must match the order in ConsoleLineStyle!
            V4 outputTextColor;
            V4 outputTextColorInputEcho;
            V4 outputTextColorError;
            V4 outputTextColorSuccess;
            V4 outputTextColorWarning;
        };
    };

    DrawableStyle background;
    Padding padding;
    s32 contentPadding;

    AssetRef scrollbarStyle;
    AssetRef textInputStyle;
};

struct DropDownListStyle {
    String name;

    // For now, we'll just piggy-back off of other styles:
    AssetRef buttonStyle; // For the normal state
    AssetRef panelStyle;  // For the drop-down state
};

struct LabelStyle {
    String name;

    Padding padding;
    DrawableStyle background;

    AssetRef font;
    V4 textColor;
    u32 textAlignment;
};

struct PanelStyle {
    String name;

    Padding padding;
    s32 contentPadding;
    u32 widgetAlignment;

    DrawableStyle background;

    AssetRef buttonStyle;
    AssetRef checkboxStyle;
    AssetRef dropDownListStyle;
    AssetRef labelStyle;
    AssetRef radioButtonStyle;
    AssetRef scrollbarStyle;
    AssetRef sliderStyle;
    AssetRef textInputStyle;
};

struct RadioButtonStyle {
    String name;

    V2I size;
    DrawableStyle background;
    DrawableStyle backgroundHover;
    DrawableStyle backgroundPressed;
    DrawableStyle backgroundDisabled;

    V2I dotSize;
    DrawableStyle dot;
    DrawableStyle dotHover;
    DrawableStyle dotPressed;
    DrawableStyle dotDisabled;
};

struct ScrollbarStyle {
    String name;

    s32 width;

    DrawableStyle background;

    DrawableStyle thumb;
    DrawableStyle thumbHover;
    DrawableStyle thumbPressed;
    DrawableStyle thumbDisabled;
};

struct SliderStyle {
    String name;

    DrawableStyle track;
    s32 trackThickness;

    DrawableStyle thumb;
    DrawableStyle thumbHover;
    DrawableStyle thumbPressed;
    DrawableStyle thumbDisabled;
    V2I thumbSize;
};

struct TextInputStyle {
    String name;

    AssetRef font;
    V4 textColor;
    u32 textAlignment;

    DrawableStyle background;
    Padding padding;

    bool showCaret;
    f32 caretFlashCycleDuration;
};

struct WindowStyle {
    String name;

    AssetRef titleLabelStyle;
    s32 titleBarHeight;
    V4 titleBarColor;
    V4 titleBarColorInactive;
    V4 titleBarButtonHoverColor;

    V2I offsetFromMouse;

    AssetRef panelStyle;
};

enum class PropType {
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

struct Property {
    PropType type;
    smm offsetInStyleStruct;
    bool existsInStyle[StyleTypeCount];
};

struct Style {
    StyleType type;
    String name;

    // PROPERTIES
    Maybe<Padding> padding;
    Maybe<s32> contentPadding;
    Maybe<V2I> offsetFromMouse;
    Maybe<s32> width;
    Maybe<u32> widgetAlignment;
    Maybe<V2I> size;

    Maybe<DrawableStyle> background;
    Maybe<DrawableStyle> backgroundDisabled;
    Maybe<DrawableStyle> backgroundHover;
    Maybe<DrawableStyle> backgroundPressed;

    Maybe<String> buttonStyle;
    Maybe<String> checkboxStyle;
    Maybe<String> dropDownListStyle;
    Maybe<String> labelStyle;
    Maybe<String> panelStyle;
    Maybe<String> radioButtonStyle;
    Maybe<String> scrollbarStyle;
    Maybe<String> sliderStyle;
    Maybe<String> textInputStyle;

    Maybe<DrawableStyle> startIcon;
    Maybe<u32> startIconAlignment;

    Maybe<DrawableStyle> endIcon;
    Maybe<u32> endIconAlignment;

    Maybe<bool> showCaret;
    Maybe<f32> caretFlashCycleDuration;

    Maybe<DrawableStyle> track;
    Maybe<s32> trackThickness;
    Maybe<DrawableStyle> thumb;
    Maybe<DrawableStyle> thumbHover;
    Maybe<DrawableStyle> thumbPressed;
    Maybe<DrawableStyle> thumbDisabled;
    Maybe<V2I> thumbSize;

    Maybe<V4> overlayColor;

    Maybe<AssetRef> font;
    Maybe<u32> textAlignment;
    Maybe<V4> textColor;

    // Window
    Maybe<String> titleLabelStyle;
    Maybe<V4> titleBarButtonHoverColor;
    Maybe<V4> titleBarColor;
    Maybe<V4> titleBarColorInactive;
    Maybe<s32> titleBarHeight;

    // Checkbox specific
    Maybe<V2I> checkSize;
    Maybe<DrawableStyle> check;
    Maybe<DrawableStyle> checkHover;
    Maybe<DrawableStyle> checkPressed;
    Maybe<DrawableStyle> checkDisabled;

    // Console
    Maybe<V4> outputTextColor;
    Maybe<V4> outputTextColorInputEcho;
    Maybe<V4> outputTextColorError;
    Maybe<V4> outputTextColorSuccess;
    Maybe<V4> outputTextColorWarning;

    // Radio button
    Maybe<V2I> dotSize;
    Maybe<DrawableStyle> dot;
    Maybe<DrawableStyle> dotHover;
    Maybe<DrawableStyle> dotPressed;
    Maybe<DrawableStyle> dotDisabled;
};

inline HashTable<Property> styleProperties;
inline HashTable<StyleType> styleTypesByName;
void initStyleConstants();
void assignStyleProperties(StyleType type, std::initializer_list<String> properties);

template<typename T>
void setPropertyValue(Style* style, Property* property, T value)
{
    Maybe<T> newValue = makeSuccess(value);
    *((Maybe<T>*)((u8*)(style) + property->offsetInStyleStruct)) = newValue;
}

}

void loadUITheme(Blob data, Asset* asset);
