/*
 * Copyright (c) 2017-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "../line_reader.h"
#include <Assets/AssetManager.h>
#include <UI/UITheme.h>
#include <Util/Deferred.h>

namespace UI {

Maybe<DrawableStyle> readDrawableStyle(LineReader* reader)
{
    String typeName = readToken(reader);
    Maybe<DrawableStyle> result = makeFailure<DrawableStyle>();

    if (equals(typeName, "none"_s)) {
        DrawableStyle drawable = {};
        drawable.type = DrawableType::None;

        result = makeSuccess(drawable);
    } else if (equals(typeName, "color"_s)) {
        Maybe<V4> color = readColor(reader);
        if (color.isValid) {
            DrawableStyle drawable = {};
            drawable.type = DrawableType::Color;
            drawable.color = color.value;

            result = makeSuccess(drawable);
        }
    } else if (equals(typeName, "gradient"_s)) {
        Maybe<V4> color00 = readColor(reader);
        Maybe<V4> color01 = readColor(reader);
        Maybe<V4> color10 = readColor(reader);
        Maybe<V4> color11 = readColor(reader);

        if (color00.isValid && color01.isValid && color10.isValid && color11.isValid) {
            DrawableStyle drawable = {};
            drawable.type = DrawableType::Gradient;
            drawable.gradient.color00 = color00.value;
            drawable.gradient.color01 = color01.value;
            drawable.gradient.color10 = color10.value;
            drawable.gradient.color11 = color11.value;

            result = makeSuccess(drawable);
        }
    } else if (equals(typeName, "ninepatch"_s)) {
        String ninepatchName = readToken(reader);

        Maybe<V4> color = readColor(reader, true);

        DrawableStyle drawable = {};
        drawable.type = DrawableType::Ninepatch;
        drawable.color = color.orDefault(makeWhite());
        drawable.ninepatch = getAssetRef(AssetType::Ninepatch, ninepatchName);

        result = makeSuccess(drawable);
    } else if (equals(typeName, "sprite"_s)) {
        String spriteName = readToken(reader);

        Maybe<V4> color = readColor(reader, true);

        DrawableStyle drawable = {};
        drawable.type = DrawableType::Sprite;
        drawable.color = color.orDefault(makeWhite());
        drawable.sprite = getSpriteRef(spriteName, 0);

        result = makeSuccess(drawable);
    } else {
        error(reader, "Unrecognised background type '{0}'"_s, { typeName });
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
    initHashTable(&styleProperties, 0.75f, 256);
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

    initHashTable(&styleTypesByName, 0.75f, 256);
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
        Property* property = styleProperties.find(*propName).orDefault(nullptr);
        property->existsInStyle[type] = true;
    }
}

}

void loadUITheme(Blob data, Asset* asset)
{
    LineReader reader = readLines(asset->shortName, data);

    HashTable<EnumMap<UI::StyleType, UI::Style>> styles;
    initHashTable(&styles);

    HashTable<String> fontNamesToAssetNames;
    initHashTable(&fontNamesToAssetNames);

    Deferred defer_free_hash_tables = [&styles, &fontNamesToAssetNames] {
        freeHashTable(&styles);
        freeHashTable(&fontNamesToAssetNames);
    };

    EnumMap<UI::StyleType, s32> style_count;

    String currentSection = nullString;
    UI::Style* target = nullptr;

    while (loadNextLine(&reader)) {
        String firstWord = readToken(&reader);

        if (firstWord.chars[0] == ':') {
            // define an item
            ++firstWord.chars;
            --firstWord.length;
            currentSection = firstWord;

            if (equals(firstWord, "Font"_s)) {
                target = nullptr;
                String fontName = readToken(&reader);
                String fontFilename = getRemainderOfLine(&reader);

                if (!isEmpty(fontName) && !isEmpty(fontFilename)) {
                    Asset* fontAsset = addAsset(AssetType::BitmapFont, fontFilename);
                    fontNamesToAssetNames.put(fontName, fontAsset->shortName);
                } else {
                    error(&reader, "Invalid font declaration: '{0}'"_s, { getLine(&reader) });
                }
            } else {
                // Create a new style entry if the name matches a style type
                Maybe<UI::StyleType> foundStyleType = UI::styleTypesByName.findValue(firstWord);
                if (foundStyleType.isValid) {
                    UI::StyleType styleType = foundStyleType.value;

                    String name = intern(&asset_manager().assetStrings, readToken(&reader));

                    auto& pack = *styles.findOrAdd(name);
                    target = &pack[styleType];
                    *target = {};
                    target->name = name;
                    target->type = styleType;

                    style_count[styleType]++;
                } else {
                    error(&reader, "Unrecognized command: '{0}'"_s, { firstWord });
                }
            }
        } else {
            // Properties of the item
            // These are arranged alphabetically
            if (equals(firstWord, "extends"_s)) {
                // Clones an existing style
                String parentStyle = readToken(&reader);
                auto parentPack = styles.find(parentStyle);
                if (!parentPack.isValid) {
                    error(&reader, "Unable to find style named '{0}'"_s, { parentStyle });
                } else {
                    UI::Style const& parent = (*parentPack.value)[target->type];
                    // For undefined styles, the parent struct will be all nulls, so the type will not match
                    if (parent.type != target->type) {
                        error(&reader, "Attempting to extend a style of the wrong type."_s);
                    } else {
                        String name = target->name;
                        *target = parent;
                        target->name = name;
                    }
                }
            } else {
                // Check our properties map for a match
                UI::Property* property = UI::styleProperties.find(firstWord).orDefault(nullptr);
                if (property) {
                    if (property->existsInStyle[target->type]) {
                        switch (property->type) {
                        case UI::PropType::Alignment: {
                            Maybe<u32> value = readAlignment(&reader);
                            if (value.isValid) {
                                UI::setPropertyValue<u32>(target, property, value.value);
                            }
                        } break;

                        case UI::PropType::Bool: {
                            Maybe<bool> value = readBool(&reader);
                            if (value.isValid) {
                                UI::setPropertyValue<bool>(target, property, value.value);
                            }
                        } break;

                        case UI::PropType::Color: {
                            Maybe<V4> value = readColor(&reader);
                            if (value.isValid) {
                                UI::setPropertyValue<V4>(target, property, value.value);
                            }
                        } break;

                        case UI::PropType::Drawable: {
                            Maybe<UI::DrawableStyle> value = UI::readDrawableStyle(&reader);
                            if (value.isValid) {
                                UI::setPropertyValue<UI::DrawableStyle>(target, property, value.value);
                            }
                        } break;

                        case UI::PropType::Float: {
                            Maybe<f64> value = readFloat(&reader);
                            if (value.isValid) {
                                UI::setPropertyValue<f32>(target, property, (f32)value.value);
                            }
                        } break;

                        case UI::PropType::Font: {
                            String value = intern(&asset_manager().assetStrings, readToken(&reader));
                            Maybe<String> fontFilename = fontNamesToAssetNames.findValue(value);
                            if (fontFilename.isValid) {
                                AssetRef fontRef = getAssetRef(AssetType::BitmapFont, fontFilename.value);
                                UI::setPropertyValue<AssetRef>(target, property, fontRef);
                            } else {
                                error(&reader, "Unrecognised font name '{0}'. Make sure to declare the :Font before it is used!"_s, { value });
                            }
                        } break;

                        case UI::PropType::Int: {
                            Maybe<s32> value = readInt<s32>(&reader);
                            if (value.isValid) {
                                UI::setPropertyValue<s32>(target, property, value.value);
                            }
                        } break;

                        case UI::PropType::Padding: {
                            Maybe<Padding> value = readPadding(&reader);
                            if (value.isValid) {
                                UI::setPropertyValue<Padding>(target, property, value.value);
                            }
                        } break;

                        case UI::PropType::Style: // NB: Style names are just Strings now
                        case UI::PropType::String: {
                            String value = intern(&asset_manager().assetStrings, readToken(&reader));
                            // Strings are read directly, so we don't need an if(valid) check
                            UI::setPropertyValue<String>(target, property, value);
                        } break;

                        case UI::PropType::V2I: {
                            Maybe<s32> offsetX = readInt<s32>(&reader);
                            Maybe<s32> offsetY = readInt<s32>(&reader);
                            if (offsetX.isValid && offsetY.isValid) {
                                V2I vector = v2i(offsetX.value, offsetY.value);
                                UI::setPropertyValue<V2I>(target, property, vector);
                            }
                        } break;

                        default:
                            logCritical("Invalid property type for '{0}'"_s, { firstWord });
                        }
                    } else {
                        error(&reader, "Property '{0}' is not allowed in '{1}'"_s, { firstWord, currentSection });
                    }
                } else {
                    error(&reader, "Unrecognized property '{0}'"_s, { firstWord });
                }
            }
        }
    }

    // Actually write out the styles into the UITheme

    s32 totalStyleCount = 0;
    for (s32 i = 0; i < to_underlying(UI::StyleType::COUNT); i++) {
        totalStyleCount += style_count[i];
    }
    allocateChildren(asset, totalStyleCount);

    // Some default values to use
    V4 transparent = color255(0, 0, 0, 0);
    V4 white = makeWhite();
    AssetRef defaultFont = getAssetRef(AssetType::BitmapFont, nullString);
    String defaultStyleName = "default"_h;

    for (auto it = styles.iterate(); it.hasNext(); it.next()) {
        auto* stylePack = it.get();
        for (s32 sectionType = 1; sectionType < to_underlying(UI::StyleType::COUNT); sectionType++) {
            UI::Style* style = &(*stylePack)[sectionType];
            // For undefined styles, the parent struct will be all nulls, so the type will not match
            // FIXME: This seems really sketchy.
            if (to_underlying(style->type) == sectionType) {
                switch (style->type) {
                case UI::StyleType::Button: {
                    Asset* childAsset = addAsset(AssetType::ButtonStyle, style->name, 0);
                    addChildAsset(asset, childAsset);

                    UI::ButtonStyle* button = &childAsset->buttonStyle;
                    button->name = style->name;

                    button->font = style->font.orDefault(defaultFont);
                    button->textColor = style->textColor.orDefault(white);
                    button->textAlignment = style->textAlignment.orDefault(ALIGN_TOP | ALIGN_LEFT);

                    button->padding = style->padding.value;
                    button->contentPadding = style->contentPadding.value;

                    button->startIcon = style->startIcon.value;
                    button->startIconAlignment = style->startIconAlignment.orDefault(ALIGN_TOP | ALIGN_LEFT);
                    button->endIcon = style->endIcon.value;
                    button->endIconAlignment = style->endIconAlignment.orDefault(ALIGN_TOP | ALIGN_RIGHT);

                    button->background = style->background.value;
                    button->backgroundHover = style->backgroundHover.orDefault(button->background);
                    button->backgroundPressed = style->backgroundPressed.orDefault(button->background);
                    button->backgroundDisabled = style->backgroundDisabled.orDefault(button->background);

                    if (!button->startIcon.hasFixedSize()) {
                        error(&reader, "Start icon for button '{0}' has no fixed size. Defaulting to 0 x 0"_s, { button->name });
                    }
                    if (!button->endIcon.hasFixedSize()) {
                        error(&reader, "End icon for button '{0}' has no fixed size. Defaulting to 0 x 0"_s, { button->name });
                    }
                } break;

                case UI::StyleType::Checkbox: {
                    Asset* childAsset = addAsset(AssetType::CheckboxStyle, style->name, 0);
                    addChildAsset(asset, childAsset);

                    UI::CheckboxStyle* checkbox = &childAsset->checkboxStyle;
                    checkbox->name = style->name;

                    checkbox->padding = style->padding.value;

                    checkbox->background = style->background.value;
                    checkbox->backgroundHover = style->backgroundHover.orDefault(checkbox->background);
                    checkbox->backgroundPressed = style->backgroundPressed.orDefault(checkbox->background);
                    checkbox->backgroundDisabled = style->backgroundDisabled.orDefault(checkbox->background);

                    checkbox->check = style->check.value;
                    checkbox->checkHover = style->checkHover.orDefault(checkbox->check);
                    checkbox->checkPressed = style->checkPressed.orDefault(checkbox->check);
                    checkbox->checkDisabled = style->checkDisabled.orDefault(checkbox->check);

                    // Default checkSize to the check image's size if it has one
                    if (style->checkSize.isValid) {
                        checkbox->checkSize = style->checkSize.value;
                    } else if (checkbox->check.hasFixedSize()) {
                        checkbox->checkSize = checkbox->check.getSize();
                    } else {
                        error(&reader, "Check for checkbox '{0}' has no fixed size, and no checkSize was provided. Defaulting to 0 x 0"_s, { checkbox->name });
                    }
                } break;

                case UI::StyleType::Console: {
                    Asset* childAsset = addAsset(AssetType::ConsoleStyle, style->name, 0);
                    addChildAsset(asset, childAsset);

                    UI::ConsoleStyle* console = &childAsset->consoleStyle;
                    console->name = style->name;

                    console->font = style->font.orDefault(defaultFont);

                    console->outputTextColor = style->outputTextColor.orDefault(white);
                    console->outputTextColorInputEcho = style->outputTextColorInputEcho.orDefault(console->outputTextColor);
                    console->outputTextColorError = style->outputTextColorError.orDefault(console->outputTextColor);
                    console->outputTextColorWarning = style->outputTextColorWarning.orDefault(console->outputTextColor);
                    console->outputTextColorSuccess = style->outputTextColorSuccess.orDefault(console->outputTextColor);

                    console->background = style->background.value;
                    console->padding = style->padding.value;
                    console->contentPadding = style->contentPadding.value;

                    console->scrollbarStyle = getAssetRef(AssetType::ScrollbarStyle, style->scrollbarStyle.orDefault(defaultStyleName));
                    console->textInputStyle = getAssetRef(AssetType::TextInputStyle, style->textInputStyle.orDefault(defaultStyleName));
                } break;

                case UI::StyleType::DropDownList: {
                    Asset* childAsset = addAsset(AssetType::DropDownListStyle, style->name, 0);
                    addChildAsset(asset, childAsset);

                    UI::DropDownListStyle* ddl = &childAsset->dropDownListStyle;
                    ddl->name = style->name;

                    ddl->buttonStyle = getAssetRef(AssetType::ButtonStyle, style->buttonStyle.orDefault(defaultStyleName));
                    ddl->panelStyle = getAssetRef(AssetType::PanelStyle, style->panelStyle.orDefault(defaultStyleName));
                } break;

                case UI::StyleType::Label: {
                    Asset* childAsset = addAsset(AssetType::LabelStyle, style->name, 0);
                    addChildAsset(asset, childAsset);

                    UI::LabelStyle* label = &childAsset->labelStyle;
                    label->name = style->name;

                    label->padding = style->padding.value;
                    label->background = style->background.value;
                    label->font = style->font.orDefault(defaultFont);
                    label->textColor = style->textColor.orDefault(white);
                    label->textAlignment = style->textAlignment.orDefault(ALIGN_TOP | ALIGN_LEFT);
                } break;

                case UI::StyleType::Panel: {
                    Asset* childAsset = addAsset(AssetType::PanelStyle, style->name, 0);
                    addChildAsset(asset, childAsset);

                    UI::PanelStyle* panel = &childAsset->panelStyle;
                    panel->name = style->name;

                    panel->padding = style->padding.value;
                    panel->contentPadding = style->contentPadding.value;
                    panel->widgetAlignment = style->widgetAlignment.orDefault(ALIGN_TOP | ALIGN_EXPAND_H);
                    panel->background = style->background.value;

                    panel->buttonStyle = getAssetRef(AssetType::ButtonStyle, style->buttonStyle.orDefault(defaultStyleName));
                    panel->checkboxStyle = getAssetRef(AssetType::CheckboxStyle, style->checkboxStyle.orDefault(defaultStyleName));
                    panel->dropDownListStyle = getAssetRef(AssetType::DropDownListStyle, style->dropDownListStyle.orDefault(defaultStyleName));
                    panel->labelStyle = getAssetRef(AssetType::LabelStyle, style->labelStyle.orDefault(defaultStyleName));
                    panel->radioButtonStyle = getAssetRef(AssetType::RadioButtonStyle, style->radioButtonStyle.orDefault(defaultStyleName));
                    panel->scrollbarStyle = getAssetRef(AssetType::ScrollbarStyle, style->scrollbarStyle.orDefault(defaultStyleName));
                    panel->sliderStyle = getAssetRef(AssetType::SliderStyle, style->sliderStyle.orDefault(defaultStyleName));
                    panel->textInputStyle = getAssetRef(AssetType::TextInputStyle, style->textInputStyle.orDefault(defaultStyleName));
                } break;

                case UI::StyleType::RadioButton: {
                    Asset* childAsset = addAsset(AssetType::RadioButtonStyle, style->name, 0);
                    addChildAsset(asset, childAsset);

                    UI::RadioButtonStyle* radioButton = &childAsset->radioButtonStyle;
                    radioButton->name = style->name;

                    radioButton->size = style->size.value;
                    radioButton->background = style->background.value;
                    radioButton->backgroundDisabled = style->backgroundDisabled.orDefault(radioButton->background);
                    radioButton->backgroundHover = style->backgroundHover.orDefault(radioButton->background);
                    radioButton->backgroundPressed = style->backgroundPressed.orDefault(radioButton->background);

                    radioButton->dotSize = style->dotSize.value;
                    radioButton->dot = style->dot.value;
                    radioButton->dotDisabled = style->dotDisabled.orDefault(radioButton->dot);
                    radioButton->dotHover = style->dotHover.orDefault(radioButton->dot);
                    radioButton->dotPressed = style->dotPressed.orDefault(radioButton->dot);
                } break;

                case UI::StyleType::Scrollbar: {
                    Asset* childAsset = addAsset(AssetType::ScrollbarStyle, style->name, 0);
                    addChildAsset(asset, childAsset);

                    UI::ScrollbarStyle* scrollbar = &childAsset->scrollbarStyle;
                    scrollbar->name = style->name;

                    scrollbar->background = style->background.value;
                    scrollbar->thumb = style->thumb.value;
                    scrollbar->thumbDisabled = style->thumbDisabled.orDefault(scrollbar->thumb);
                    scrollbar->thumbHover = style->thumbHover.orDefault(scrollbar->thumb);
                    scrollbar->thumbPressed = style->thumbPressed.orDefault(scrollbar->thumb);
                    scrollbar->width = style->width.orDefault(8);
                } break;

                case UI::StyleType::Slider: {
                    Asset* childAsset = addAsset(AssetType::SliderStyle, style->name, 0);
                    addChildAsset(asset, childAsset);

                    UI::SliderStyle* slider = &childAsset->sliderStyle;
                    slider->name = style->name;

                    slider->track = style->track.value;
                    slider->trackThickness = style->trackThickness.orDefault(3);
                    slider->thumb = style->thumb.value;
                    slider->thumbDisabled = style->thumbDisabled.orDefault(slider->thumb);
                    slider->thumbHover = style->thumbHover.orDefault(slider->thumb);
                    slider->thumbPressed = style->thumbPressed.orDefault(slider->thumb);
                    slider->thumbSize = style->thumbSize.orDefault(v2i(8, 8));
                } break;

                case UI::StyleType::TextInput: {
                    Asset* childAsset = addAsset(AssetType::TextInputStyle, style->name, 0);
                    addChildAsset(asset, childAsset);

                    UI::TextInputStyle* textInput = &childAsset->textInputStyle;
                    textInput->name = style->name;

                    textInput->font = style->font.orDefault(defaultFont);
                    textInput->textColor = style->textColor.orDefault(white);
                    textInput->textAlignment = style->textAlignment.orDefault(ALIGN_TOP | ALIGN_LEFT);

                    textInput->background = style->background.value;
                    textInput->padding = style->padding.value;

                    textInput->showCaret = style->showCaret.orDefault(true);
                    textInput->caretFlashCycleDuration = style->caretFlashCycleDuration.orDefault(1.0f);
                } break;

                case UI::StyleType::Window: {
                    Asset* childAsset = addAsset(AssetType::WindowStyle, style->name, 0);
                    addChildAsset(asset, childAsset);

                    UI::WindowStyle* window = &childAsset->windowStyle;
                    window->name = style->name;

                    window->titleBarHeight = style->titleBarHeight.orDefault(16);
                    window->titleBarColor = style->titleBarColor.orDefault(color255(128, 128, 128, 255));
                    window->titleBarColorInactive = style->titleBarColorInactive.orDefault(window->titleBarColor);
                    window->titleBarButtonHoverColor = style->titleBarButtonHoverColor.orDefault(transparent);

                    window->titleLabelStyle = getAssetRef(AssetType::LabelStyle, style->titleLabelStyle.orDefault(defaultStyleName));

                    window->offsetFromMouse = style->offsetFromMouse.value;

                    window->panelStyle = getAssetRef(AssetType::PanelStyle, style->panelStyle.orDefault(defaultStyleName));
                } break;

                    INVALID_DEFAULT_CASE;
                }
            }
        }
    }
}
