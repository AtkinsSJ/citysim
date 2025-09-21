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

template<typename T>
static void setPropertyValue(Style* style, Property* property, T value)
{
    *((Optional<T>*)((u8*)(style) + property->offsetInStyleStruct)) = move(value);
}

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

void initStyleConstants()
{
#define PROP(name, _type)                                     \
    {                                                         \
        Property property = {};                               \
        property.type = PropType::_type;                      \
        property.offsetInStyleStruct = offsetof(Style, name); \
        styleProperties.put(makeString(#name), property);     \
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

        if (firstWord.chars[0] == ':') {
            // define an item
            ++firstWord.chars;
            --firstWord.length;
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
                Optional<UI::StyleType> foundStyleType = UI::styleTypesByName.findValue(firstWord);
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
                                UI::setPropertyValue(target, property, value.release_value());
                            }
                        } break;

                        case UI::PropType::Bool: {
                            if (auto value = reader.read_bool(); value.has_value()) {
                                UI::setPropertyValue(target, property, value.release_value());
                            }
                        } break;

                        case UI::PropType::Color: {
                            if (Optional value = Colour::read(reader); value.has_value()) {
                                UI::setPropertyValue(target, property, value.release_value());
                            }
                        } break;

                        case UI::PropType::Drawable: {
                            Optional<UI::DrawableStyle> value = UI::readDrawableStyle(&reader);
                            if (value.has_value()) {
                                UI::setPropertyValue(target, property, value.release_value());
                            }
                        } break;

                        case UI::PropType::Float: {
                            if (auto value = reader.read_float(); value.has_value()) {
                                UI::setPropertyValue(target, property, value.release_value());
                            }
                        } break;

                        case UI::PropType::Font: {
                            String value = intern(&asset_manager().assetStrings, reader.next_token());
                            Optional<String> fontFilename = fontNamesToAssetNames.findValue(value);
                            if (fontFilename.has_value()) {
                                AssetRef fontRef = AssetRef { AssetType::BitmapFont, fontFilename.value() };
                                UI::setPropertyValue(target, property, fontRef);
                            } else {
                                reader.error("Unrecognised font name '{0}'. Make sure to declare the :Font before it is used!"_s, { value });
                            }
                        } break;

                        case UI::PropType::Int: {
                            if (auto value = reader.read_int<s32>(); value.has_value()) {
                                UI::setPropertyValue(target, property, value.release_value());
                            }
                        } break;

                        case UI::PropType::Padding: {
                            if (auto value = Padding::read(reader); value.has_value()) {
                                UI::setPropertyValue(target, property, value.release_value());
                            }
                        } break;

                        case UI::PropType::Style: // NB: Style names are just Strings now
                        case UI::PropType::String: {
                            String value = intern(&asset_manager().assetStrings, reader.next_token());
                            // Strings are read directly, so we don't need an if(valid) check
                            UI::setPropertyValue(target, property, value);
                        } break;

                        case UI::PropType::V2I: {
                            auto offsetX = reader.read_int<s32>();
                            auto offsetY = reader.read_int<s32>();
                            if (offsetX.has_value() && offsetY.has_value()) {
                                V2I vector = v2i(offsetX.value(), offsetY.value());
                                UI::setPropertyValue(target, property, vector);
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
    AssetRef defaultFont = AssetRef { AssetType::BitmapFont, nullString };
    String defaultStyleName = "default"_h;

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

                    button->font = style->font.value_or(defaultFont);
                    button->textColor = style->textColor.value_or(white);
                    button->textAlignment = style->textAlignment.value_or({ HAlign::Left, VAlign::Top });

                    button->padding = style->padding.value_or({});
                    button->contentPadding = style->contentPadding.value_or({});

                    button->startIcon = style->startIcon.value_or({});
                    button->startIconAlignment = style->startIconAlignment.value_or({ HAlign::Left, VAlign::Top });
                    button->endIcon = style->endIcon.value_or({});
                    button->endIconAlignment = style->endIconAlignment.value_or({ HAlign::Right, VAlign::Top });

                    button->background = style->background.value_or({});
                    button->backgroundHover = style->backgroundHover.value_or(button->background);
                    button->backgroundPressed = style->backgroundPressed.value_or(button->background);
                    button->backgroundDisabled = style->backgroundDisabled.value_or(button->background);

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

                    checkbox->padding = style->padding.value_or({});

                    checkbox->background = style->background.value_or({});
                    checkbox->backgroundHover = style->backgroundHover.value_or(checkbox->background);
                    checkbox->backgroundPressed = style->backgroundPressed.value_or(checkbox->background);
                    checkbox->backgroundDisabled = style->backgroundDisabled.value_or(checkbox->background);

                    checkbox->check = style->check.value_or({});
                    checkbox->checkHover = style->checkHover.value_or(checkbox->check);
                    checkbox->checkPressed = style->checkPressed.value_or(checkbox->check);
                    checkbox->checkDisabled = style->checkDisabled.value_or(checkbox->check);

                    // Default checkSize to the check image's size if it has one
                    if (style->checkSize.has_value()) {
                        checkbox->checkSize = style->checkSize.value_or({});
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

                    console->font = style->font.value_or(defaultFont);

                    console->outputTextColor = style->outputTextColor.value_or(white);
                    console->outputTextColorInputEcho = style->outputTextColorInputEcho.value_or(console->outputTextColor);
                    console->outputTextColorError = style->outputTextColorError.value_or(console->outputTextColor);
                    console->outputTextColorWarning = style->outputTextColorWarning.value_or(console->outputTextColor);
                    console->outputTextColorSuccess = style->outputTextColorSuccess.value_or(console->outputTextColor);

                    console->background = style->background.value_or({});
                    console->padding = style->padding.value_or({});
                    console->contentPadding = style->contentPadding.value_or({});

                    console->scrollbarStyle = AssetRef { AssetType::ScrollbarStyle, style->scrollbarStyle.value_or(defaultStyleName) };
                    console->textInputStyle = AssetRef { AssetType::TextInputStyle, style->textInputStyle.value_or(defaultStyleName) };
                } break;

                case UI::StyleType::DropDownList: {
                    Asset* childAsset = addAsset(AssetType::DropDownListStyle, style->name, {});
                    addChildAsset(asset, childAsset);

                    UI::DropDownListStyle* ddl = &childAsset->dropDownListStyle;
                    ddl->name = style->name;

                    ddl->buttonStyle = AssetRef { AssetType::ButtonStyle, style->buttonStyle.value_or(defaultStyleName) };
                    ddl->panelStyle = AssetRef { AssetType::PanelStyle, style->panelStyle.value_or(defaultStyleName) };
                } break;

                case UI::StyleType::Label: {
                    Asset* childAsset = addAsset(AssetType::LabelStyle, style->name, {});
                    addChildAsset(asset, childAsset);

                    UI::LabelStyle* label = &childAsset->labelStyle;
                    label->name = style->name;

                    label->padding = style->padding.value_or({});
                    label->background = style->background.value_or({});
                    label->font = style->font.value_or(defaultFont);
                    label->textColor = style->textColor.value_or(white);
                    label->textAlignment = style->textAlignment.value_or({ HAlign::Left, VAlign::Top });
                } break;

                case UI::StyleType::Panel: {
                    Asset* childAsset = addAsset(AssetType::PanelStyle, style->name, {});
                    addChildAsset(asset, childAsset);

                    UI::PanelStyle* panel = &childAsset->panelStyle;
                    panel->name = style->name;

                    panel->padding = style->padding.value_or({});
                    panel->contentPadding = style->contentPadding.value_or({});
                    panel->widgetAlignment = style->widgetAlignment.value_or({ HAlign::Fill, VAlign::Top });
                    panel->background = style->background.value_or({});

                    panel->buttonStyle = AssetRef { AssetType::ButtonStyle, style->buttonStyle.value_or(defaultStyleName) };
                    panel->checkboxStyle = AssetRef { AssetType::CheckboxStyle, style->checkboxStyle.value_or(defaultStyleName) };
                    panel->dropDownListStyle = AssetRef { AssetType::DropDownListStyle, style->dropDownListStyle.value_or(defaultStyleName) };
                    panel->labelStyle = AssetRef { AssetType::LabelStyle, style->labelStyle.value_or(defaultStyleName) };
                    panel->radioButtonStyle = AssetRef { AssetType::RadioButtonStyle, style->radioButtonStyle.value_or(defaultStyleName) };
                    panel->scrollbarStyle = AssetRef { AssetType::ScrollbarStyle, style->scrollbarStyle.value_or(defaultStyleName) };
                    panel->sliderStyle = AssetRef { AssetType::SliderStyle, style->sliderStyle.value_or(defaultStyleName) };
                    panel->textInputStyle = AssetRef { AssetType::TextInputStyle, style->textInputStyle.value_or(defaultStyleName) };
                } break;

                case UI::StyleType::RadioButton: {
                    Asset* childAsset = addAsset(AssetType::RadioButtonStyle, style->name, {});
                    addChildAsset(asset, childAsset);

                    UI::RadioButtonStyle* radioButton = &childAsset->radioButtonStyle;
                    radioButton->name = style->name;

                    radioButton->size = style->size.value_or({});
                    radioButton->background = style->background.value_or({});
                    radioButton->backgroundDisabled = style->backgroundDisabled.value_or(radioButton->background);
                    radioButton->backgroundHover = style->backgroundHover.value_or(radioButton->background);
                    radioButton->backgroundPressed = style->backgroundPressed.value_or(radioButton->background);

                    radioButton->dotSize = style->dotSize.value_or({});
                    radioButton->dot = style->dot.value_or({});
                    radioButton->dotDisabled = style->dotDisabled.value_or(radioButton->dot);
                    radioButton->dotHover = style->dotHover.value_or(radioButton->dot);
                    radioButton->dotPressed = style->dotPressed.value_or(radioButton->dot);
                } break;

                case UI::StyleType::Scrollbar: {
                    Asset* childAsset = addAsset(AssetType::ScrollbarStyle, style->name, {});
                    addChildAsset(asset, childAsset);

                    UI::ScrollbarStyle* scrollbar = &childAsset->scrollbarStyle;
                    scrollbar->name = style->name;

                    scrollbar->background = style->background.value_or({});
                    scrollbar->thumb = style->thumb.value_or({});
                    scrollbar->thumbDisabled = style->thumbDisabled.value_or(scrollbar->thumb);
                    scrollbar->thumbHover = style->thumbHover.value_or(scrollbar->thumb);
                    scrollbar->thumbPressed = style->thumbPressed.value_or(scrollbar->thumb);
                    scrollbar->width = style->width.value_or(8);
                } break;

                case UI::StyleType::Slider: {
                    Asset* childAsset = addAsset(AssetType::SliderStyle, style->name, {});
                    addChildAsset(asset, childAsset);

                    UI::SliderStyle* slider = &childAsset->sliderStyle;
                    slider->name = style->name;

                    slider->track = style->track.value_or({});
                    slider->trackThickness = style->trackThickness.value_or(3);
                    slider->thumb = style->thumb.value_or({});
                    slider->thumbDisabled = style->thumbDisabled.value_or(slider->thumb);
                    slider->thumbHover = style->thumbHover.value_or(slider->thumb);
                    slider->thumbPressed = style->thumbPressed.value_or(slider->thumb);
                    slider->thumbSize = style->thumbSize.value_or(v2i(8, 8));
                } break;

                case UI::StyleType::TextInput: {
                    Asset* childAsset = addAsset(AssetType::TextInputStyle, style->name, {});
                    addChildAsset(asset, childAsset);

                    UI::TextInputStyle* textInput = &childAsset->textInputStyle;
                    textInput->name = style->name;

                    textInput->font = style->font.value_or(defaultFont);
                    textInput->textColor = style->textColor.value_or(white);
                    textInput->textAlignment = style->textAlignment.value_or({ HAlign::Left, VAlign::Top });

                    textInput->background = style->background.value_or({});
                    textInput->padding = style->padding.value_or({});

                    textInput->showCaret = style->showCaret.value_or(true);
                    textInput->caretFlashCycleDuration = style->caretFlashCycleDuration.value_or(1.0f);
                } break;

                case UI::StyleType::Window: {
                    Asset* childAsset = addAsset(AssetType::WindowStyle, style->name, {});
                    addChildAsset(asset, childAsset);

                    UI::WindowStyle* window = &childAsset->windowStyle;
                    window->name = style->name;

                    window->titleBarHeight = style->titleBarHeight.value_or(16);
                    window->titleBarColor = style->titleBarColor.value_or(Colour::from_rgb_255(128, 128, 128, 255));
                    window->titleBarColorInactive = style->titleBarColorInactive.value_or(window->titleBarColor);
                    window->titleBarButtonHoverColor = style->titleBarButtonHoverColor.value_or(transparent);

                    window->titleLabelStyle = AssetRef { AssetType::LabelStyle, style->titleLabelStyle.value_or(defaultStyleName) };

                    window->offsetFromMouse = style->offsetFromMouse.value_or({});

                    window->panelStyle = AssetRef { AssetType::PanelStyle, style->panelStyle.value_or(defaultStyleName) };
                } break;

                    INVALID_DEFAULT_CASE;
                }
            }
        }
    }
}
