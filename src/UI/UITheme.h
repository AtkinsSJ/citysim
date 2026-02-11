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

class ButtonStyle final : public Asset {
public:
    ButtonStyle(
        AssetRef font,
        Colour text_color,
        Alignment text_alignment,
        Padding padding,
        s32 content_padding,
        DrawableStyle start_icon,
        Alignment start_icon_alignment,
        DrawableStyle end_icon,
        Alignment end_icon_alignment,
        DrawableStyle background,
        DrawableStyle background_hover,
        DrawableStyle background_pressed,
        DrawableStyle background_disabled);

    virtual ~ButtonStyle() override = default;
    virtual void unload(AssetMetadata&) override { }

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

class CheckboxStyle final : public Asset {
public:
    CheckboxStyle(
        Padding padding,
        DrawableStyle background,
        DrawableStyle background_hover,
        DrawableStyle background_pressed,
        DrawableStyle background_disabled,
        V2I check_size,
        DrawableStyle check,
        DrawableStyle check_hover,
        DrawableStyle check_pressed,
        DrawableStyle check_disabled);
    virtual ~CheckboxStyle() override = default;
    virtual void unload(AssetMetadata&) override { }

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

class ConsoleStyle final : public Asset {
public:
    ConsoleStyle(
        AssetRef font,
        EnumMap<ConsoleLineStyle, Colour> output_text_colors,
        DrawableStyle background,
        Padding padding,
        s32 content_padding,
        AssetRef scrollbar_style,
        AssetRef text_input_style);
    virtual ~ConsoleStyle() override = default;
    virtual void unload(AssetMetadata&) override { }

    AssetRef font;
    EnumMap<ConsoleLineStyle, Colour> outputTextColors;

    DrawableStyle background;
    Padding padding;
    s32 contentPadding;

    AssetRef scrollbarStyle;
    AssetRef textInputStyle;
};

class DropDownListStyle final : public Asset {
public:
    DropDownListStyle(AssetRef button_style, AssetRef panel_style);
    virtual ~DropDownListStyle() override = default;
    virtual void unload(AssetMetadata&) override { }

    // For now, we'll just piggy-back off of other styles:
    AssetRef buttonStyle; // For the normal state
    AssetRef panelStyle;  // For the drop-down state
};

class LabelStyle final : public Asset {
public:
    LabelStyle(
        Padding padding,
        DrawableStyle background,
        AssetRef font,
        Colour text_color,
        Alignment text_alignment);
    virtual ~LabelStyle() override = default;
    virtual void unload(AssetMetadata&) override { }

    Padding padding;
    DrawableStyle background;

    AssetRef font;
    Colour textColor;
    Alignment textAlignment;
};

class PanelStyle final : public Asset {
public:
    PanelStyle(
        Padding padding,
        s32 content_padding,
        Alignment widget_alignment,
        DrawableStyle background,
        AssetRef button_style,
        AssetRef checkbox_style,
        AssetRef drop_down_list_style,
        AssetRef label_style,
        AssetRef radio_button_style,
        AssetRef scrollbar_style,
        AssetRef slider_style,
        AssetRef text_input_style);
    virtual ~PanelStyle() override = default;
    virtual void unload(AssetMetadata&) override { }

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

class RadioButtonStyle final : public Asset {
public:
    RadioButtonStyle(
        V2I size,
        DrawableStyle background,
        DrawableStyle background_hover,
        DrawableStyle background_pressed,
        DrawableStyle background_disabled,
        V2I dot_size,
        DrawableStyle dot,
        DrawableStyle dot_hover,
        DrawableStyle dot_pressed,
        DrawableStyle dot_disabled);
    virtual ~RadioButtonStyle() override = default;
    virtual void unload(AssetMetadata&) override { }

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

class ScrollbarStyle final : public Asset {
public:
    ScrollbarStyle(
        s32 width,
        DrawableStyle background,
        DrawableStyle thumb,
        DrawableStyle thumb_hover,
        DrawableStyle thumb_pressed,
        DrawableStyle thumb_disabled);
    virtual ~ScrollbarStyle() override = default;
    virtual void unload(AssetMetadata&) override { }

    s32 width;

    DrawableStyle background;

    DrawableStyle thumb;
    DrawableStyle thumbHover;
    DrawableStyle thumbPressed;
    DrawableStyle thumbDisabled;
};

class SliderStyle final : public Asset {
public:
    SliderStyle(
        DrawableStyle track,
        s32 track_thickness,
        DrawableStyle thumb,
        DrawableStyle thumb_hover,
        DrawableStyle thumb_pressed,
        DrawableStyle thumb_disabled,
        V2I thumb_size);
    virtual ~SliderStyle() override = default;
    virtual void unload(AssetMetadata&) override { }

    DrawableStyle track;
    s32 trackThickness;

    DrawableStyle thumb;
    DrawableStyle thumbHover;
    DrawableStyle thumbPressed;
    DrawableStyle thumbDisabled;
    V2I thumbSize;
};

class TextInputStyle final : public Asset {
public:
    TextInputStyle(
        AssetRef font,
        Colour text_color,
        Alignment text_alignment,
        DrawableStyle background,
        Padding padding,
        bool show_caret,
        float caret_flash_cycle_duration);
    virtual ~TextInputStyle() override = default;
    virtual void unload(AssetMetadata&) override { }

    AssetRef font;
    Colour textColor;
    Alignment textAlignment;

    DrawableStyle background;
    Padding padding;

    bool showCaret;
    float caretFlashCycleDuration;
};

class WindowStyle final : public Asset {
public:
    WindowStyle(
        AssetRef title_label_style,
        s32 title_bar_height,
        Colour title_bar_color,
        Colour title_bar_color_inactive,
        Colour title_bar_button_hover_color,
        V2I offset_from_mouse,
        AssetRef panel_style);
    virtual ~WindowStyle() override = default;
    virtual void unload(AssetMetadata&) override { }

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
