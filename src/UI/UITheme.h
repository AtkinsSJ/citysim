/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/AssetRef.h>
#include <Assets/Sprite.h>
#include <Gfx/Colour.h>
#include <IO/Forward.h>
#include <Util/HashTable.h>
#include <Util/Optional.h>
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

namespace UI {

// FIXME: This should go somewhere else probably.
enum class DrawableType : u8 {
    None,
    Color,
    Gradient,
    Ninepatch,
    Sprite,
};

struct DrawableStyle {
    DrawableType type;
    Colour color;

    union {
        struct {
            Colour color00;
            Colour color01;
            Colour color10;
            Colour color11;
        } gradient {};

        AssetRef ninepatch;

        SpriteRef sprite;
    };

    // METHODS
    bool hasFixedSize();
    V2I getSize(); // NB: Returns 0 for sizeless types
};
Optional<DrawableStyle> readDrawableStyle(LineReader* reader);

enum class StyleType : u8 {
    None,
    Button,
    Checkbox,
    Console,
    DropDownList,
    Label,
    Panel,
    RadioButton,
    Scrollbar,
    Slider,
    TextInput,
    Window,
    COUNT
};

struct ButtonStyle {
    String name;

    AssetRef font;
    Colour textColor;
    Alignment textAlignment;

    Padding padding;
    s32 contentPadding; // Between icons and content

    DrawableStyle startIcon;
    Alignment startIconAlignment;

    DrawableStyle endIcon;
    Alignment endIconAlignment;

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
        EnumMap<ConsoleLineStyle, Colour> outputTextColors;

        struct {
            // Must match the order in ConsoleLineStyle!
            Colour outputTextColor;
            Colour outputTextColorInputEcho;
            Colour outputTextColorError;
            Colour outputTextColorSuccess;
            Colour outputTextColorWarning;
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
    Colour textColor;
    Alignment textAlignment;
};

struct PanelStyle {
    String name;

    Padding padding;
    s32 contentPadding;
    Alignment widgetAlignment;

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
    Colour textColor;
    Alignment textAlignment;

    DrawableStyle background;
    Padding padding;

    bool showCaret;
    float caretFlashCycleDuration;
};

struct WindowStyle {
    String name;

    AssetRef titleLabelStyle;
    s32 titleBarHeight;
    Colour titleBarColor;
    Colour titleBarColorInactive;
    Colour titleBarButtonHoverColor;

    V2I offsetFromMouse;

    AssetRef panelStyle;
};

enum class PropType : u8 {
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
    COUNT
};

struct Property {
    PropType type;
    smm offsetInStyleStruct;
    EnumMap<StyleType, bool> existsInStyle;
};

struct Style {
    StyleType type;
    String name;

    // PROPERTIES
    Optional<Padding> padding;
    Optional<s32> contentPadding;
    Optional<V2I> offsetFromMouse;
    Optional<s32> width;
    Optional<Alignment> widgetAlignment;
    Optional<V2I> size;

    Optional<DrawableStyle> background;
    Optional<DrawableStyle> backgroundDisabled;
    Optional<DrawableStyle> backgroundHover;
    Optional<DrawableStyle> backgroundPressed;

    Optional<String> buttonStyle;
    Optional<String> checkboxStyle;
    Optional<String> dropDownListStyle;
    Optional<String> labelStyle;
    Optional<String> panelStyle;
    Optional<String> radioButtonStyle;
    Optional<String> scrollbarStyle;
    Optional<String> sliderStyle;
    Optional<String> textInputStyle;

    Optional<DrawableStyle> startIcon;
    Optional<Alignment> startIconAlignment;

    Optional<DrawableStyle> endIcon;
    Optional<Alignment> endIconAlignment;

    Optional<bool> showCaret;
    Optional<float> caretFlashCycleDuration;

    Optional<DrawableStyle> track;
    Optional<s32> trackThickness;
    Optional<DrawableStyle> thumb;
    Optional<DrawableStyle> thumbHover;
    Optional<DrawableStyle> thumbPressed;
    Optional<DrawableStyle> thumbDisabled;
    Optional<V2I> thumbSize;

    Optional<Colour> overlayColor;

    Optional<AssetRef> font;
    Optional<Alignment> textAlignment;
    Optional<Colour> textColor;

    // Window
    Optional<String> titleLabelStyle;
    Optional<Colour> titleBarButtonHoverColor;
    Optional<Colour> titleBarColor;
    Optional<Colour> titleBarColorInactive;
    Optional<s32> titleBarHeight;

    // Checkbox specific
    Optional<V2I> checkSize;
    Optional<DrawableStyle> check;
    Optional<DrawableStyle> checkHover;
    Optional<DrawableStyle> checkPressed;
    Optional<DrawableStyle> checkDisabled;

    // Console
    Optional<Colour> outputTextColor;
    Optional<Colour> outputTextColorInputEcho;
    Optional<Colour> outputTextColorError;
    Optional<Colour> outputTextColorSuccess;
    Optional<Colour> outputTextColorWarning;

    // Radio button
    Optional<V2I> dotSize;
    Optional<DrawableStyle> dot;
    Optional<DrawableStyle> dotHover;
    Optional<DrawableStyle> dotPressed;
    Optional<DrawableStyle> dotDisabled;
};

inline HashTable<Property> styleProperties;
inline HashTable<StyleType> styleTypesByName;
void initStyleConstants();
void assignStyleProperties(StyleType type, std::initializer_list<String> properties);

}

void loadUITheme(Blob data, Asset* asset);
