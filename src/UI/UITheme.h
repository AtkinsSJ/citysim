/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/AssetRef.h>
#include <Gfx/Colour.h>
#include <Gfx/Sprite.h>
#include <IO/Forward.h>
#include <Util/HashTable.h>
#include <Util/Optional.h>
#include <Util/Ref.h>
#include <Util/Variant.h>
#include <Util/Vector.h>

enum class ConsoleLineStyle : u8 {
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
    AssetRef font;
    EnumMap<ConsoleLineStyle, Colour> outputTextColors;

    DrawableStyle background;
    Padding padding;
    s32 contentPadding;

    AssetRef scrollbarStyle;
    AssetRef textInputStyle;
};

struct DropDownListStyle {
    // For now, we'll just piggy-back off of other styles:
    AssetRef buttonStyle; // For the normal state
    AssetRef panelStyle;  // For the drop-down state
};

struct LabelStyle {
    Padding padding;
    DrawableStyle background;

    AssetRef font;
    Colour textColor;
    Alignment textAlignment;
};

struct PanelStyle {
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
    s32 width;

    DrawableStyle background;

    DrawableStyle thumb;
    DrawableStyle thumbHover;
    DrawableStyle thumbPressed;
    DrawableStyle thumbDisabled;
};

struct SliderStyle {
    DrawableStyle track;
    s32 trackThickness;

    DrawableStyle thumb;
    DrawableStyle thumbHover;
    DrawableStyle thumbPressed;
    DrawableStyle thumbDisabled;
    V2I thumbSize;
};

struct TextInputStyle {
    AssetRef font;
    Colour textColor;
    Alignment textAlignment;

    DrawableStyle background;
    Padding padding;

    bool showCaret;
    float caretFlashCycleDuration;
};

struct WindowStyle {
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
    EnumMap<StyleType, bool> existsInStyle;
};

class Style {
public:
    StyleType type;
    String name;

    using PropertyValue = Variant<bool, float, s32, Alignment, Colour, DrawableStyle, Padding, String, V2I>;

    void set_property(StringView property, PropertyValue&& value);

    bool get_bool(StringView property, bool default_value) const;
    float get_float(StringView property, float default_value) const;
    s32 get_s32(StringView property, s32 default_value) const;
    Alignment get_alignment(StringView property, Alignment const& default_value) const;
    AssetRef get_asset_ref(StringView property, AssetType, String const& default_name = "default"_h) const;
    Colour get_colour(StringView property, Colour const& default_value) const;
    DrawableStyle get_drawable_style(StringView property, DrawableStyle const& default_value) const;
    Padding get_padding(StringView property, Padding const& default_value) const;
    String get_string(StringView property, String const& default_value) const;
    V2I get_v2i(StringView property, V2I const& default_value) const;
    Optional<V2I> get_v2i(StringView property) const;

private:
    // FIXME: Remove `mutable` once HashTable is const-correct
    mutable HashTable<PropertyValue> m_properties;

    template<typename T>
    Optional<T> get_property_value(StringView property) const
    {
        if (auto property_value = m_properties.find(property.deprecated_to_string()); property_value.has_value()) {
            if (auto* value = property_value->try_get<T>())
                return *value;
        }

        return {};
    }
};

void initStyleConstants();
void assignStyleProperties(StyleType type, std::initializer_list<String> properties);

ErrorOr<NonnullOwnPtr<Asset>> load_theme(AssetMetadata& metadata, Blob data);

}
