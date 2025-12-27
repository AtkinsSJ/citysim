/*
 * Copyright (c) 2017-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Assets/AssetManager.h>
#include <IO/LineReader.h>
#include <UI/UITheme.h>

namespace UI {

static HashTable<Property> styleProperties { 256 };
static HashTable<StyleType> styleTypesByName { 256 };

Optional<DrawableStyle> readDrawableStyle(LineReader* reader)
{
    String typeName = reader->next_token();
    Optional<DrawableStyle> result;

    if (typeName == "none"_s) {
        DrawableStyle drawable = {};
        drawable.type = DrawableType::None;

        result = move(drawable);
    } else if (typeName == "color"_s) {
        Optional color = Colour::read(*reader);
        if (color.has_value()) {
            DrawableStyle drawable = {};
            drawable.type = DrawableType::Color;
            drawable.color = color.release_value();

            result = move(drawable);
        }
    } else if (typeName == "gradient"_s) {
        Optional color00 = Colour::read(*reader);
        Optional color01 = Colour::read(*reader);
        Optional color10 = Colour::read(*reader);
        Optional color11 = Colour::read(*reader);

        if (color00.has_value() && color01.has_value() && color10.has_value() && color11.has_value()) {
            DrawableStyle drawable = {};
            drawable.type = DrawableType::Gradient;
            drawable.gradient.color00 = color00.release_value();
            drawable.gradient.color01 = color01.release_value();
            drawable.gradient.color10 = color10.release_value();
            drawable.gradient.color11 = color11.release_value();

            result = move(drawable);
        }
    } else if (typeName == "ninepatch"_s) {
        String ninepatchName = reader->next_token();

        auto color = Colour::read(*reader, LineReader::IsRequired::No);

        DrawableStyle drawable = {};
        drawable.type = DrawableType::Ninepatch;
        drawable.color = color.value_or(Colour::white());
        drawable.ninepatch = AssetRef { AssetType::Ninepatch, intern(&asset_manager().assetStrings, ninepatchName) };

        result = move(drawable);
    } else if (typeName == "sprite"_s) {
        String spriteName = reader->next_token();

        auto color = Colour::read(*reader, LineReader::IsRequired::No);

        DrawableStyle drawable = {};
        drawable.type = DrawableType::Sprite;
        drawable.color = color.value_or(Colour::white());
        drawable.sprite = getSpriteRef(spriteName, 0);

        result = move(drawable);
    } else {
        reader->error("Unrecognised background type '{0}'"_s, { typeName });
    }

    return result;
}

bool DrawableStyle::hasFixedSize()
{
    return (type == DrawableType::None || type == DrawableType::Sprite);
}

V2I DrawableStyle::getSize()
{
    V2I result = {};

    switch (type) {
    case DrawableType::Sprite: {
        Sprite* theSprite = getSprite(&sprite);
        result.x = theSprite->pixelWidth;
        result.y = theSprite->pixelHeight;
    } break;

    default:
        break;
    }

    return result;
}

void Style::set_property(String const& property, PropertyValue&& value)
{
    m_properties.put(property, move(value));
}

bool Style::get_bool(String const& property, bool default_value) const
{
    return get_property_value<bool>(property).value_or(default_value);
}

float Style::get_float(String const& property, float default_value) const
{
    return get_property_value<float>(property).value_or(default_value);
}

s32 Style::get_s32(String const& property, s32 default_value) const
{
    return get_property_value<s32>(property).value_or(default_value);
}

Alignment Style::get_alignment(String const& property, Alignment const& default_value) const
{
    return get_property_value<Alignment>(property).value_or(default_value);
}

AssetRef Style::get_asset_ref(String const& property, AssetType asset_type, String const& default_name) const
{
    if (auto asset_name = get_property_value<String>(property); asset_name.has_value())
        return AssetRef { asset_type, asset_name.release_value() };
    return AssetRef { asset_type, default_name };
}

Colour Style::get_colour(String const& property, Colour const& default_value) const
{
    return get_property_value<Colour>(property).value_or(default_value);
}

DrawableStyle Style::get_drawable_style(String const& property, DrawableStyle const& default_value) const
{
    return get_property_value<DrawableStyle>(property).value_or(default_value);
}

Padding Style::get_padding(String const& property, Padding const& default_value) const
{
    return get_property_value<Padding>(property).value_or(default_value);
}

String Style::get_string(String const& property, String const& default_value) const
{
    return get_property_value<String>(property).value_or(default_value);
}

V2I Style::get_v2i(String const& property, V2I const& default_value) const
{
    return get_v2i(property).value_or(default_value);
}

Optional<V2I> Style::get_v2i(String const& property) const
{
    return get_property_value<V2I>(property);
}

void initStyleConstants()
{
#define PROP(name, _type)                         \
    {                                             \
        Property property = {};                   \
        property.type = PropType::_type;          \
        styleProperties.put(#name##_s, property); \
    }

    PROP(background, Drawable);
    PROP(backgroundDisabled, Drawable);
    PROP(backgroundHover, Drawable);
    PROP(backgroundPressed, Drawable);
    PROP(buttonStyle, Style);
    PROP(caretFlashCycleDuration, Float);
    PROP(check, Drawable);
    PROP(checkDisabled, Drawable);
    PROP(checkHover, Drawable);
    PROP(checkPressed, Drawable);
    PROP(checkboxStyle, Style);
    PROP(checkSize, V2I);
    PROP(dot, Drawable);
    PROP(dotDisabled, Drawable);
    PROP(dotHover, Drawable);
    PROP(dotPressed, Drawable);
    PROP(dotSize, V2I);
    PROP(contentPadding, Int);
    PROP(dropDownListStyle, Style);
    PROP(endIcon, Drawable);
    PROP(endIconAlignment, Alignment);
    PROP(font, Font);
    PROP(labelStyle, Style);
    PROP(offsetFromMouse, V2I);
    PROP(outputTextColor, Color);
    PROP(outputTextColorInputEcho, Color);
    PROP(outputTextColorError, Color);
    PROP(outputTextColorWarning, Color);
    PROP(outputTextColorSuccess, Color);
    PROP(padding, Padding);
    PROP(panelStyle, Style);
    PROP(radioButtonStyle, Style);
    PROP(scrollbarStyle, Style);
    PROP(showCaret, Bool);
    PROP(size, V2I);
    PROP(sliderStyle, Style);
    PROP(startIcon, Drawable);
    PROP(startIconAlignment, Alignment);
    PROP(textAlignment, Alignment);
    PROP(textColor, Color);
    PROP(textInputStyle, Style);
    PROP(thumb, Drawable);
    PROP(thumbDisabled, Drawable);
    PROP(thumbHover, Drawable);
    PROP(thumbPressed, Drawable);
    PROP(titleBarButtonHoverColor, Color);
    PROP(titleBarColor, Color);
    PROP(titleBarColorInactive, Color);
    PROP(titleBarHeight, Int);
    PROP(titleLabelStyle, Style);
    PROP(trackThickness, Int);
    PROP(widgetAlignment, Alignment);
    PROP(width, Int);
    PROP(track, Drawable);
    PROP(thumbSize, V2I);

#undef PROP

    assignStyleProperties(StyleType::Button, {
                                                 "background"_s,
                                                 "backgroundDisabled"_s,
                                                 "backgroundHover"_s,
                                                 "backgroundPressed"_s,
                                                 "contentPadding"_s,
                                                 "endIcon"_s,
                                                 "endIconAlignment"_s,
                                                 "font"_s,
                                                 "padding"_s,
                                                 "startIcon"_s,
                                                 "startIconAlignment"_s,
                                                 "textAlignment"_s,
                                                 "textColor"_s,
                                             });
    assignStyleProperties(StyleType::Checkbox, {
                                                   "background"_s,
                                                   "backgroundDisabled"_s,
                                                   "backgroundHover"_s,
                                                   "backgroundPressed"_s,
                                                   "check"_s,
                                                   "checkDisabled"_s,
                                                   "checkHover"_s,
                                                   "checkSize"_s,
                                                   "checkPressed"_s,
                                                   "dropDownListStyle"_s,
                                                   "padding"_s,
                                               });
    assignStyleProperties(StyleType::Console, {
                                                  "background"_s,
                                                  "font"_s,
                                                  "outputTextColor"_s,
                                                  "outputTextColorInputEcho"_s,
                                                  "outputTextColorError"_s,
                                                  "outputTextColorSuccess"_s,
                                                  "outputTextColorWarning"_s,
                                                  "padding"_s,
                                                  "contentPadding"_s,
                                                  "scrollbarStyle"_s,
                                                  "textInputStyle"_s,
                                              });
    assignStyleProperties(StyleType::DropDownList, {
                                                       "buttonStyle"_s,
                                                       "panelStyle"_s,
                                                   });
    assignStyleProperties(StyleType::Label, {
                                                "background"_s,
                                                "font"_s,
                                                "padding"_s,
                                                "textColor"_s,
                                                "textAlignment"_s,
                                            });
    assignStyleProperties(StyleType::Panel, {
                                                "background"_s,
                                                "buttonStyle"_s,
                                                "checkboxStyle"_s,
                                                "contentPadding"_s,
                                                "dropDownListStyle"_s,
                                                "labelStyle"_s,
                                                "padding"_s,
                                                "radioButtonStyle"_s,
                                                "scrollbarStyle"_s,
                                                "sliderStyle"_s,
                                                "textInputStyle"_s,
                                                "widgetAlignment"_s,
                                            });
    assignStyleProperties(StyleType::RadioButton, {
                                                      "background"_s,
                                                      "backgroundDisabled"_s,
                                                      "backgroundHover"_s,
                                                      "backgroundPressed"_s,
                                                      "dot"_s,
                                                      "dotDisabled"_s,
                                                      "dotHover"_s,
                                                      "dotPressed"_s,
                                                      "dotSize"_s,
                                                      "size"_s,
                                                  });
    assignStyleProperties(StyleType::Scrollbar, {
                                                    "background"_s,
                                                    "thumb"_s,
                                                    "thumbDisabled"_s,
                                                    "thumbHover"_s,
                                                    "thumbPressed"_s,
                                                    "width"_s,
                                                });
    assignStyleProperties(StyleType::Slider, {
                                                 "track"_s,
                                                 "trackThickness"_s,
                                                 "thumb"_s,
                                                 "thumbDisabled"_s,
                                                 "thumbHover"_s,
                                                 "thumbPressed"_s,
                                                 "thumbSize"_s,
                                             });
    assignStyleProperties(StyleType::TextInput, {
                                                    "background"_s,
                                                    "caretFlashCycleDuration"_s,
                                                    "font"_s,
                                                    "padding"_s,
                                                    "showCaret"_s,
                                                    "textAlignment"_s,
                                                    "textColor"_s,
                                                });
    assignStyleProperties(StyleType::Window, {
                                                 "offsetFromMouse"_s,
                                                 "panelStyle"_s,
                                                 "titleBarButtonHoverColor"_s,
                                                 "titleBarColor"_s,
                                                 "titleBarColorInactive"_s,
                                                 "titleBarHeight"_s,
                                                 "titleLabelStyle"_s,
                                             });

    styleTypesByName.put("Button"_s, StyleType::Button);
    styleTypesByName.put("Checkbox"_s, StyleType::Checkbox);
    styleTypesByName.put("Console"_s, StyleType::Console);
    styleTypesByName.put("DropDownList"_s, StyleType::DropDownList);
    styleTypesByName.put("Label"_s, StyleType::Label);
    styleTypesByName.put("Panel"_s, StyleType::Panel);
    styleTypesByName.put("RadioButton"_s, StyleType::RadioButton);
    styleTypesByName.put("Scrollbar"_s, StyleType::Scrollbar);
    styleTypesByName.put("Slider"_s, StyleType::Slider);
    styleTypesByName.put("TextInput"_s, StyleType::TextInput);
    styleTypesByName.put("Window"_s, StyleType::Window);
}

void assignStyleProperties(StyleType type, std::initializer_list<String> properties)
{
    for (String* propName = (String*)properties.begin(); propName < properties.end(); propName++) {
        Property* property = styleProperties.find(*propName).value();
        property->existsInStyle[type] = true;
    }
}

}

void loadUITheme(Blob data, Asset* asset)
{
    LineReader reader { asset->shortName, data };

    HashTable<EnumMap<UI::StyleType, UI::Style>> styles;

    HashTable<String> fontNamesToAssetNames;

    EnumMap<UI::StyleType, s32> style_count;

    String currentSection = nullString;
    UI::Style* target = nullptr;

    while (reader.load_next_line()) {
        String firstWord = reader.next_token();

        if (firstWord.starts_with(':')) {
            // define an item
            firstWord = firstWord.view().substring(1).deprecated_to_string();
            currentSection = firstWord;

            if (firstWord == "Font"_s) {
                target = nullptr;
                String fontName = reader.next_token();
                String fontFilename = reader.remainder_of_current_line();

                if (!fontName.is_empty() && !fontFilename.is_empty()) {
                    Asset* fontAsset = addAsset(AssetType::BitmapFont, fontFilename);
                    fontNamesToAssetNames.put(fontName, fontAsset->shortName);
                } else {
                    reader.error("Invalid font declaration: '{0}'"_s, { reader.current_line() });
                }
            } else {
                // Create a new style entry if the name matches a style type
                Optional<UI::StyleType> foundStyleType = UI::styleTypesByName.find_value(firstWord);
                if (foundStyleType.has_value()) {
                    UI::StyleType styleType = foundStyleType.release_value();

                    String name = intern(&asset_manager().assetStrings, reader.next_token());

                    auto& pack = *styles.findOrAdd(name);
                    target = &pack[styleType];
                    *target = {};
                    target->name = name;
                    target->type = styleType;

                    style_count[styleType]++;
                } else {
                    reader.error("Unrecognized command: '{0}'"_s, { firstWord });
                }
            }
        } else {
            // Properties of the item
            // These are arranged alphabetically
            if (firstWord == "extends"_s) {
                // Clones an existing style
                String parentStyle = reader.next_token();
                auto parentPack = styles.find(parentStyle);
                if (!parentPack.has_value()) {
                    reader.error("Unable to find style named '{0}'"_s, { parentStyle });
                } else {
                    UI::Style const& parent = (*parentPack.value())[target->type];
                    // For undefined styles, the parent struct will be all nulls, so the type will not match
                    if (parent.type != target->type) {
                        reader.error("Attempting to extend a style of the wrong type."_s);
                    } else {
                        String name = target->name;
                        *target = parent;
                        target->name = name;
                    }
                }
            } else {
                // Check our properties map for a match
                UI::Property* property = UI::styleProperties.find(firstWord).value_or(nullptr);
                if (property) {
                    if (property->existsInStyle[target->type]) {
                        switch (property->type) {
                        case UI::PropType::Alignment: {
                            if (auto value = Alignment::read(reader); value.has_value()) {
                                target->set_property(firstWord, value.release_value());
                            }
                        } break;

                        case UI::PropType::Bool: {
                            if (auto value = reader.read_bool(); value.has_value()) {
                                target->set_property(firstWord, value.release_value());
                            }
                        } break;

                        case UI::PropType::Color: {
                            if (Optional value = Colour::read(reader); value.has_value()) {
                                target->set_property(firstWord, value.release_value());
                            }
                        } break;

                        case UI::PropType::Drawable: {
                            Optional<UI::DrawableStyle> value = UI::readDrawableStyle(&reader);
                            if (value.has_value()) {
                                target->set_property(firstWord, value.release_value());
                            }
                        } break;

                        case UI::PropType::Float: {
                            if (auto value = reader.read_float(); value.has_value()) {
                                target->set_property(firstWord, value.release_value());
                            }
                        } break;

                        case UI::PropType::Font: {
                            String value = intern(&asset_manager().assetStrings, reader.next_token());
                            Optional<String> fontFilename = fontNamesToAssetNames.find_value(value);
                            if (fontFilename.has_value()) {
                                target->set_property(firstWord, fontFilename.value());
                            } else {
                                reader.error("Unrecognised font name '{0}'. Make sure to declare the :Font before it is used!"_s, { value });
                            }
                        } break;

                        case UI::PropType::Int: {
                            if (auto value = reader.read_int<s32>(); value.has_value()) {
                                target->set_property(firstWord, value.release_value());
                            }
                        } break;

                        case UI::PropType::Padding: {
                            if (auto value = Padding::read(reader); value.has_value()) {
                                target->set_property(firstWord, value.release_value());
                            }
                        } break;

                        case UI::PropType::Style: // NB: Style names are just Strings now
                        case UI::PropType::String: {
                            String value = intern(&asset_manager().assetStrings, reader.next_token());
                            // Strings are read directly, so we don't need an if(valid) check
                            target->set_property(firstWord, value);
                        } break;

                        case UI::PropType::V2I: {
                            auto offsetX = reader.read_int<s32>();
                            auto offsetY = reader.read_int<s32>();
                            if (offsetX.has_value() && offsetY.has_value()) {
                                V2I vector = v2i(offsetX.value(), offsetY.value());
                                target->set_property(firstWord, vector);
                            }
                        } break;

                        default:
                            logCritical("Invalid property type for '{0}'"_s, { firstWord });
                        }
                    } else {
                        reader.error("Property '{0}' is not allowed in '{1}'"_s, { firstWord, currentSection });
                    }
                } else {
                    reader.error("Unrecognized property '{0}'"_s, { firstWord });
                }
            }
        }
    }

    // Actually write out the styles into the UITheme

    s32 totalStyleCount = 0;
    for (auto style_type : enum_values<UI::StyleType>()) {
        totalStyleCount += style_count[style_type];
    }
    allocateChildren(asset, totalStyleCount);

    // Some default values to use
    auto transparent = Colour::from_rgb_255(0, 0, 0, 0);
    auto white = Colour::white();
    auto default_font_name = ""_h;

    for (auto it = styles.iterate(); it.hasNext(); it.next()) {
        auto* stylePack = it.get();
        for (auto style_type : enum_values<UI::StyleType>()) {
            if (style_type == UI::StyleType::None)
                continue;

            UI::Style* style = &(*stylePack)[style_type];
            // For undefined styles, the parent struct will be all nulls, so the type will not match
            // FIXME: This seems really sketchy.
            if (style->type == style_type) {
                switch (style->type) {
                case UI::StyleType::Button: {
                    Asset* childAsset = addAsset(AssetType::ButtonStyle, style->name, {});
                    addChildAsset(asset, childAsset);

                    UI::ButtonStyle* button = &childAsset->buttonStyle;
                    button->name = style->name;

                    button->font = style->get_asset_ref("font"_h, AssetType::BitmapFont, default_font_name);
                    button->textColor = style->get_colour("textColor"_h, white);
                    button->textAlignment = style->get_alignment("textAlignment"_h, { HAlign::Left, VAlign::Top });

                    button->padding = style->get_padding("padding"_h, {});
                    button->contentPadding = style->get_s32("contentPadding"_h, {});

                    button->startIcon = style->get_drawable_style("startIcon"_h, {});
                    button->startIconAlignment = style->get_alignment("startIconAlignment"_h, { HAlign::Left, VAlign::Top });
                    button->endIcon = style->get_drawable_style("endIcon"_h, {});
                    button->endIconAlignment = style->get_alignment("endIconAlignment"_h, { HAlign::Right, VAlign::Top });

                    button->background = style->get_drawable_style("background"_h, {});
                    button->backgroundHover = style->get_drawable_style("backgroundHover"_h, button->background);
                    button->backgroundPressed = style->get_drawable_style("backgroundPressed"_h, button->background);
                    button->backgroundDisabled = style->get_drawable_style("backgroundDisabled"_h, button->background);

                    if (!button->startIcon.hasFixedSize()) {
                        reader.error("Start icon for button '{0}' has no fixed size. Defaulting to 0 x 0"_s, { button->name });
                    }
                    if (!button->endIcon.hasFixedSize()) {
                        reader.error("End icon for button '{0}' has no fixed size. Defaulting to 0 x 0"_s, { button->name });
                    }
                } break;

                case UI::StyleType::Checkbox: {
                    Asset* childAsset = addAsset(AssetType::CheckboxStyle, style->name, {});
                    addChildAsset(asset, childAsset);

                    UI::CheckboxStyle* checkbox = &childAsset->checkboxStyle;
                    checkbox->name = style->name;

                    checkbox->padding = style->get_padding("padding"_h, {});

                    checkbox->background = style->get_drawable_style("background"_h, {});
                    checkbox->backgroundHover = style->get_drawable_style("backgroundHover"_h, checkbox->background);
                    checkbox->backgroundPressed = style->get_drawable_style("backgroundPressed"_h, checkbox->background);
                    checkbox->backgroundDisabled = style->get_drawable_style("backgroundDisabled"_h, checkbox->background);

                    checkbox->check = style->get_drawable_style("check"_h, {});
                    checkbox->checkHover = style->get_drawable_style("checkHover"_h, checkbox->check);
                    checkbox->checkPressed = style->get_drawable_style("checkPressed"_h, checkbox->check);
                    checkbox->checkDisabled = style->get_drawable_style("checkDisabled"_h, checkbox->check);

                    // Default checkSize to the check image's size if it has one
                    if (auto check_size = style->get_v2i("checkSize"_h); check_size.has_value()) {
                        checkbox->checkSize = check_size.release_value();
                    } else if (checkbox->check.hasFixedSize()) {
                        checkbox->checkSize = checkbox->check.getSize();
                    } else {
                        reader.error("Check for checkbox '{0}' has no fixed size, and no checkSize was provided. Defaulting to 0 x 0"_s, { checkbox->name });
                    }
                } break;

                case UI::StyleType::Console: {
                    Asset* childAsset = addAsset(AssetType::ConsoleStyle, style->name, {});
                    addChildAsset(asset, childAsset);

                    UI::ConsoleStyle* console = &childAsset->consoleStyle;
                    console->name = style->name;

                    console->font = style->get_asset_ref("font"_h, AssetType::BitmapFont, default_font_name);

                    console->outputTextColor = style->get_colour("outputTextColor"_h, white);
                    console->outputTextColorInputEcho = style->get_colour("outputTextColorInputEcho"_h, console->outputTextColor);
                    console->outputTextColorError = style->get_colour("outputTextColorError"_h, console->outputTextColor);
                    console->outputTextColorWarning = style->get_colour("outputTextColorWarning"_h, console->outputTextColor);
                    console->outputTextColorSuccess = style->get_colour("outputTextColorSuccess"_h, console->outputTextColor);

                    console->background = style->get_drawable_style("background"_h, {});
                    console->padding = style->get_padding("padding"_h, {});
                    console->contentPadding = style->get_s32("contentPadding"_h, 0);

                    console->scrollbarStyle = style->get_asset_ref("scrollbarStyle"_h, AssetType::ScrollbarStyle);
                    console->textInputStyle = style->get_asset_ref("textInputStyle"_h, AssetType::TextInputStyle);
                } break;

                case UI::StyleType::DropDownList: {
                    Asset* childAsset = addAsset(AssetType::DropDownListStyle, style->name, {});
                    addChildAsset(asset, childAsset);

                    UI::DropDownListStyle* ddl = &childAsset->dropDownListStyle;
                    ddl->name = style->name;

                    ddl->buttonStyle = style->get_asset_ref("buttonStyle"_h, AssetType::ButtonStyle);
                    ddl->panelStyle = style->get_asset_ref("panelStyle"_h, AssetType::PanelStyle);
                } break;

                case UI::StyleType::Label: {
                    Asset* childAsset = addAsset(AssetType::LabelStyle, style->name, {});
                    addChildAsset(asset, childAsset);

                    UI::LabelStyle* label = &childAsset->labelStyle;
                    label->name = style->name;

                    label->padding = style->get_padding("padding"_h, {});
                    label->background = style->get_drawable_style("background"_h, {});
                    label->font = style->get_asset_ref("font"_h, AssetType::BitmapFont, default_font_name);
                    label->textColor = style->get_colour("textColor"_h, white);
                    label->textAlignment = style->get_alignment("textAlignment"_h, { HAlign::Left, VAlign::Top });
                } break;

                case UI::StyleType::Panel: {
                    Asset* childAsset = addAsset(AssetType::PanelStyle, style->name, {});
                    addChildAsset(asset, childAsset);

                    UI::PanelStyle* panel = &childAsset->panelStyle;
                    panel->name = style->name;

                    panel->padding = style->get_padding("padding"_h, {});
                    panel->contentPadding = style->get_s32("contentPadding"_h, {});
                    panel->widgetAlignment = style->get_alignment("widgetAlignment"_h, { HAlign::Fill, VAlign::Top });
                    panel->background = style->get_drawable_style("background"_h, {});

                    panel->buttonStyle = style->get_asset_ref("buttonStyle"_h, AssetType::ButtonStyle);
                    panel->checkboxStyle = style->get_asset_ref("checkboxStyle"_h, AssetType::CheckboxStyle);
                    panel->dropDownListStyle = style->get_asset_ref("dropDownListStyle"_h, AssetType::DropDownListStyle);
                    panel->labelStyle = style->get_asset_ref("labelStyle"_h, AssetType::LabelStyle);
                    panel->radioButtonStyle = style->get_asset_ref("radioButtonStyle"_h, AssetType::RadioButtonStyle);
                    panel->scrollbarStyle = style->get_asset_ref("scrollbarStyle"_h, AssetType::ScrollbarStyle);
                    panel->sliderStyle = style->get_asset_ref("sliderStyle"_h, AssetType::SliderStyle);
                    panel->textInputStyle = style->get_asset_ref("textInputStyle"_h, AssetType::TextInputStyle);
                } break;

                case UI::StyleType::RadioButton: {
                    Asset* childAsset = addAsset(AssetType::RadioButtonStyle, style->name, {});
                    addChildAsset(asset, childAsset);

                    UI::RadioButtonStyle* radioButton = &childAsset->radioButtonStyle;
                    radioButton->name = style->name;

                    radioButton->size = style->get_v2i("size"_h, {});
                    radioButton->background = style->get_drawable_style("background"_h, {});
                    radioButton->backgroundDisabled = style->get_drawable_style("backgroundDisabled"_h, radioButton->background);
                    radioButton->backgroundHover = style->get_drawable_style("backgroundHover"_h, radioButton->background);
                    radioButton->backgroundPressed = style->get_drawable_style("backgroundPressed"_h, radioButton->background);

                    radioButton->dotSize = style->get_v2i("dotSize"_h, {});
                    radioButton->dot = style->get_drawable_style("dot"_h, {});
                    radioButton->dotDisabled = style->get_drawable_style("dotDisabled"_h, radioButton->dot);
                    radioButton->dotHover = style->get_drawable_style("dotHover"_h, radioButton->dot);
                    radioButton->dotPressed = style->get_drawable_style("dotPressed"_h, radioButton->dot);
                } break;

                case UI::StyleType::Scrollbar: {
                    Asset* childAsset = addAsset(AssetType::ScrollbarStyle, style->name, {});
                    addChildAsset(asset, childAsset);

                    UI::ScrollbarStyle* scrollbar = &childAsset->scrollbarStyle;
                    scrollbar->name = style->name;

                    scrollbar->background = style->get_drawable_style("background"_h, {});
                    scrollbar->thumb = style->get_drawable_style("thumb"_h, {});
                    scrollbar->thumbDisabled = style->get_drawable_style("thumbDisabled"_h, scrollbar->thumb);
                    scrollbar->thumbHover = style->get_drawable_style("thumbHover"_h, scrollbar->thumb);
                    scrollbar->thumbPressed = style->get_drawable_style("thumbPressed"_h, scrollbar->thumb);
                    scrollbar->width = style->get_s32("width"_h, 8);
                } break;

                case UI::StyleType::Slider: {
                    Asset* childAsset = addAsset(AssetType::SliderStyle, style->name, {});
                    addChildAsset(asset, childAsset);

                    UI::SliderStyle* slider = &childAsset->sliderStyle;
                    slider->name = style->name;

                    slider->track = style->get_drawable_style("track"_h, {});
                    slider->trackThickness = style->get_s32("trackThickness"_h, 3);
                    slider->thumb = style->get_drawable_style("thumb"_h, {});
                    slider->thumbDisabled = style->get_drawable_style("thumbDisabled"_h, slider->thumb);
                    slider->thumbHover = style->get_drawable_style("thumbHover"_h, slider->thumb);
                    slider->thumbPressed = style->get_drawable_style("thumbPressed"_h, slider->thumb);
                    slider->thumbSize = style->get_v2i("thumbSize"_h, v2i(8, 8));
                } break;

                case UI::StyleType::TextInput: {
                    Asset* childAsset = addAsset(AssetType::TextInputStyle, style->name, {});
                    addChildAsset(asset, childAsset);

                    UI::TextInputStyle* textInput = &childAsset->textInputStyle;
                    textInput->name = style->name;

                    textInput->font = style->get_asset_ref("font"_h, AssetType::BitmapFont, default_font_name);
                    textInput->textColor = style->get_colour("textColor"_h, white);
                    textInput->textAlignment = style->get_alignment("textAlignment"_h, { HAlign::Left, VAlign::Top });

                    textInput->background = style->get_drawable_style("background"_h, {});
                    textInput->padding = style->get_padding("padding"_h, {});

                    textInput->showCaret = style->get_bool("showCaret"_h, true);
                    textInput->caretFlashCycleDuration = style->get_float("caretFlashCycleDuration"_h, 1.0f);
                } break;

                case UI::StyleType::Window: {
                    Asset* childAsset = addAsset(AssetType::WindowStyle, style->name, {});
                    addChildAsset(asset, childAsset);

                    UI::WindowStyle* window = &childAsset->windowStyle;
                    window->name = style->name;

                    window->titleBarHeight = style->get_s32("titleBarHeight"_h, 16);
                    window->titleBarColor = style->get_colour("titleBarColor"_h, Colour::from_rgb_255(128, 128, 128, 255));
                    window->titleBarColorInactive = style->get_colour("titleBarColorInactive"_h, window->titleBarColor);
                    window->titleBarButtonHoverColor = style->get_colour("titleBarButtonHoverColor"_h, transparent);

                    window->titleLabelStyle = style->get_asset_ref("titleLabelStyle"_h, AssetType::LabelStyle);

                    window->offsetFromMouse = style->get_v2i("offsetFromMouse"_h, {});

                    window->panelStyle = style->get_asset_ref("panelStyle"_h, AssetType::PanelStyle);
                } break;

                    INVALID_DEFAULT_CASE;
                }
            }
        }
    }
}
