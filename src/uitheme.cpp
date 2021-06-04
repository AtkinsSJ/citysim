#pragma once

namespace UI
{
	Maybe<DrawableStyle> readDrawableStyle(LineReader *reader)
	{
		String typeName = readToken(reader);
		Maybe<DrawableStyle> result = makeFailure<DrawableStyle>();

		if (equals(typeName, "none"_s))
		{
			DrawableStyle drawable = {};
			drawable.type = Drawable_None;

			result = makeSuccess(drawable);
		}
		else if (equals(typeName, "color"_s))
		{
			Maybe<V4> color = readColor(reader);
			if (color.isValid)
			{
				DrawableStyle drawable = {};
				drawable.type = Drawable_Color;
				drawable.color = color.value;

				result = makeSuccess(drawable);
			}
		}
		else if (equals(typeName, "gradient"_s))
		{
			Maybe<V4> color00 = readColor(reader);
			Maybe<V4> color01 = readColor(reader);
			Maybe<V4> color10 = readColor(reader);
			Maybe<V4> color11 = readColor(reader);

			if (color00.isValid && color01.isValid && color10.isValid && color11.isValid)
			{
				DrawableStyle drawable = {};
				drawable.type = Drawable_Gradient;
				drawable.gradient.color00 = color00.value;
				drawable.gradient.color01 = color01.value;
				drawable.gradient.color10 = color10.value;
				drawable.gradient.color11 = color11.value;

				result = makeSuccess(drawable);
			}
		}
		else if (equals(typeName, "ninepatch"_s))
		{
			String ninepatchName = readToken(reader);

			Maybe<V4> color = readColor(reader, true);
			
			DrawableStyle drawable = {};
			drawable.type = Drawable_Ninepatch;
			drawable.color = color.orDefault(makeWhite());
			drawable.ninepatch = getAssetRef(AssetType_Ninepatch, ninepatchName);

			result = makeSuccess(drawable);
		}
		else if (equals(typeName, "sprite"_s))
		{
			String spriteName = readToken(reader);

			Maybe<V4> color = readColor(reader, true);

			DrawableStyle drawable = {};
			drawable.type = Drawable_Sprite;
			drawable.color = color.orDefault(makeWhite());
			drawable.sprite = getSpriteRef(spriteName, 0);

			result = makeSuccess(drawable);
		}
		else
		{
			error(reader, "Unrecognised background type '{0}'"_s, {typeName});
		}

		return result;
	}

	inline bool DrawableStyle::hasFixedSize()
	{
		return (type == Drawable_None || type == Drawable_Sprite);
	}

	V2I DrawableStyle::getSize()
	{
		V2I result = {};

		switch (type)
		{
			case Drawable_Sprite:
			{
				Sprite *theSprite = getSprite(&sprite);
				result.x = theSprite->pixelWidth;
				result.y = theSprite->pixelHeight;
			} break;
		}

		return result;
	}

	void initStyleConstants()
	{
		initHashTable(&styleProperties, 0.75f, 256);
		#define PROP(name, _type) {\
			Property property = {}; \
			property.type = PropType::_type; \
			property.offsetInStyleStruct = offsetof(Style, name); \
			styleProperties.put(makeString(#name), property); \
		}

		PROP(background,				Drawable);
		PROP(backgroundDisabled,		Drawable);
		PROP(backgroundHover,			Drawable);
		PROP(backgroundPressed,			Drawable);
		PROP(buttonStyle,				Style);
		PROP(caretFlashCycleDuration,	Float);
		PROP(check,						Drawable);
		PROP(checkDisabled,				Drawable);
		PROP(checkHover,				Drawable);
		PROP(checkPressed,				Drawable);
		PROP(checkboxStyle,				Style);
		PROP(checkSize,					V2I);
		PROP(dot,						Drawable);
		PROP(dotDisabled,				Drawable);
		PROP(dotHover,					Drawable);
		PROP(dotPressed,				Drawable);
		PROP(dotSize,					V2I);
		PROP(contentPadding,			Int);
		PROP(dropDownListStyle,			Style);
		PROP(endIcon,					Drawable);
		PROP(endIconAlignment,			Alignment);
		PROP(font,						Font);
		PROP(labelStyle,				Style);
		PROP(offsetFromMouse,			V2I);
		PROP(outputTextColor,			Color);
		PROP(outputTextColorInputEcho,	Color);
		PROP(outputTextColorError,		Color);
		PROP(outputTextColorWarning,	Color);
		PROP(outputTextColorSuccess,	Color);
		PROP(padding,					Padding);
		PROP(panelStyle,				Style);
		PROP(radioButtonStyle,			Style);
		PROP(scrollbarStyle,			Style);
		PROP(showCaret,					Bool);
		PROP(size,						V2I);
		PROP(sliderStyle,				Style);
		PROP(startIcon,					Drawable);
		PROP(startIconAlignment,		Alignment);
		PROP(textAlignment,				Alignment);
		PROP(textColor,					Color);
		PROP(textInputStyle,			Style);
		PROP(thumb,						Drawable);
		PROP(thumbDisabled,				Drawable);
		PROP(thumbHover,				Drawable);
		PROP(thumbPressed,				Drawable);
		PROP(titleBarButtonHoverColor,	Color);
		PROP(titleBarColor,				Color);
		PROP(titleBarColorInactive,		Color);
		PROP(titleBarHeight,			Int);
		PROP(titleColor,				Color);
		PROP(titleFont,					Font);
		PROP(trackThickness,			Int);
		PROP(widgetAlignment,			Alignment);
		PROP(width,						Int);
		PROP(track,						Drawable);
		PROP(thumbSize,					V2I);

		#undef PROP

		assignStyleProperties(Style_Button, {
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
		assignStyleProperties(Style_Checkbox, {
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
		assignStyleProperties(Style_Console, {
			"background"_s,
			"font"_s,
			"outputTextColor"_s,
			"outputTextColorInputEcho"_s,
			"outputTextColorError"_s,
			"outputTextColorSuccess"_s,
			"outputTextColorWarning"_s,
			"padding"_s,
			"scrollbarStyle"_s,
			"textInputStyle"_s,
		});
		assignStyleProperties(Style_DropDownList, {
			"buttonStyle"_s,
			"panelStyle"_s,
		});
		assignStyleProperties(Style_Label, {
			"background"_s,
			"font"_s,
			"padding"_s,
			"textColor"_s,
			"textAlignment"_s,
		});
		assignStyleProperties(Style_Panel, {
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
		assignStyleProperties(Style_RadioButton, {
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
		assignStyleProperties(Style_Scrollbar, {
			"background"_s,
			"thumb"_s,
			"thumbDisabled"_s,
			"thumbHover"_s,
			"thumbPressed"_s,
			"width"_s,
		});
		assignStyleProperties(Style_Slider, {
			"track"_s,
			"trackThickness"_s,
			"thumb"_s,
			"thumbDisabled"_s,
			"thumbHover"_s,
			"thumbPressed"_s,
			"thumbSize"_s,
		});
		assignStyleProperties(Style_TextInput, {
			"background"_s,
			"caretFlashCycleDuration"_s,
			"font"_s,
			"padding"_s,
			"showCaret"_s,
			"textAlignment"_s,
			"textColor"_s,
		});
		assignStyleProperties(Style_Window, {
			"offsetFromMouse"_s,
			"panelStyle"_s,
			"titleBarButtonHoverColor"_s,
			"titleBarColor"_s,
			"titleBarColorInactive"_s,
			"titleBarHeight"_s,
			"titleColor"_s,
			"titleFont"_s,
		});

		initHashTable(&styleTypesByName, 0.75f, 256);
		styleTypesByName.put("Button"_s, 		Style_Button);
		styleTypesByName.put("Checkbox"_s,		Style_Checkbox);
		styleTypesByName.put("Console"_s, 		Style_Console);
		styleTypesByName.put("DropDownList"_s, 	Style_DropDownList);
		styleTypesByName.put("Label"_s, 		Style_Label);
		styleTypesByName.put("Panel"_s, 		Style_Panel);
		styleTypesByName.put("RadioButton"_s, 	Style_RadioButton);
		styleTypesByName.put("Scrollbar"_s, 	Style_Scrollbar);
		styleTypesByName.put("Slider"_s, 		Style_Slider);
		styleTypesByName.put("TextInput"_s, 	Style_TextInput);
		styleTypesByName.put("Window"_s, 		Style_Window);
	}

	void assignStyleProperties(StyleType type, std::initializer_list<String> properties)
	{
		for (String *propName = (String*)properties.begin(); propName < properties.end(); propName++)
		{
			Property *property = styleProperties.find(*propName).orDefault(null);
			property->existsInStyle[type] = true;
		}
	}


	template <typename T>
	void setPropertyValue(Style *style, Property *property, T value)
	{
		Maybe<T> newValue = makeSuccess(value);
		*((Maybe<T>*)((u8*)(style) + property->offsetInStyleStruct)) = newValue;
	}
}

void loadUITheme(Blob data, Asset *asset)
{
	LineReader reader = readLines(asset->shortName, data);

	struct StylePack
	{
		UI::Style styleByType[UI::StyleTypeCount];
	};
	HashTable<StylePack> styles;
	initHashTable(&styles);

	HashTable<String> fontNamesToAssetNames;
	initHashTable(&fontNamesToAssetNames);

	defer {
		freeHashTable(&styles);
		freeHashTable(&fontNamesToAssetNames);
	};

	s32 styleCount[UI::StyleTypeCount] = {};

	String currentSection = nullString;
	UI::Style *target = null;

	while (loadNextLine(&reader))
	{
		String firstWord = readToken(&reader);

		if (firstWord.chars[0] == ':')
		{
			// define an item
			++firstWord.chars;
			--firstWord.length;
			currentSection = firstWord;

			if (equals(firstWord, "Font"_s))
			{
				target = null;
				String fontName = readToken(&reader);
				String fontFilename = getRemainderOfLine(&reader);

				if (!isEmpty(fontName) && !isEmpty(fontFilename))
				{
					Asset *fontAsset = addAsset(AssetType_BitmapFont, fontFilename);
					fontNamesToAssetNames.put(fontName, fontAsset->shortName);
				}
				else
				{
					error(&reader, "Invalid font declaration: '{0}'"_s, {getLine(&reader)});
				}
			}
			else
			{
				// Create a new style entry if the name matches a style type
				Maybe<UI::StyleType> foundStyleType = UI::styleTypesByName.findValue(firstWord);
				if (foundStyleType.isValid)
				{
					UI::StyleType styleType = foundStyleType.value;

					String name = intern(&assets->assetStrings, readToken(&reader));

					StylePack *pack = styles.findOrAdd(name);
					target = &pack->styleByType[(s32)styleType];
					*target = {};
					target->name = name;
					target->type = styleType;
					
					styleCount[styleType]++;
				}
				else
				{
					error(&reader, "Unrecognized command: '{0}'"_s, {firstWord});
				}
			}
		}
		else
		{
			// Properties of the item
			// These are arranged alphabetically
			if (equals(firstWord, "extends"_s))
			{
				// Clones an existing style
				String parentStyle = readToken(&reader);
				Maybe<StylePack *> parentPack = styles.find(parentStyle);
				if (!parentPack.isValid)
				{
					error(&reader, "Unable to find style named '{0}'"_s, {parentStyle});
				}
				else
				{
					UI::Style *parent = parentPack.value->styleByType + target->type;
					// For undefined styles, the parent struct will be all nulls, so the type will not match
					if (parent->type != target->type)
					{
						error(&reader, "Attempting to extend a style of the wrong type."_s);
					}
					else
					{
						String name = target->name;
						*target = *parent;
						target->name = name;
					}
				}
			}
			else
			{
				// Check our properties map for a match
				UI::Property *property = UI::styleProperties.find(firstWord).orDefault(null);
				if (property)
				{
					if (property->existsInStyle[target->type])
					{
						switch (property->type)
						{
							case UI::PropType::Alignment: {
								Maybe<u32> value = readAlignment(&reader);
								if (value.isValid)
								{
									UI::setPropertyValue<u32>(target, property, value.value);
								}
							} break;

							case UI::PropType::Bool: {
								Maybe<bool> value = readBool(&reader);
								if (value.isValid)
								{
									UI::setPropertyValue<bool>(target, property, value.value);
								}
							} break;

							case UI::PropType::Color: {
								Maybe<V4> value = readColor(&reader);
								if (value.isValid)
								{
									UI::setPropertyValue<V4>(target, property, value.value);
								}
							} break;

							case UI::PropType::Drawable: {
								Maybe<UI::DrawableStyle> value = UI::readDrawableStyle(&reader);
								if (value.isValid)
								{
									UI::setPropertyValue<UI::DrawableStyle>(target, property, value.value);
								}
							} break;

							case UI::PropType::Float: {
								Maybe<f64> value = readFloat(&reader);
								if (value.isValid)
								{
									UI::setPropertyValue<f32>(target, property, (f32)value.value);
								}
							} break;

							case UI::PropType::Font: {
								String value = intern(&assets->assetStrings, readToken(&reader));
								Maybe<String> fontFilename = fontNamesToAssetNames.findValue(value);
								if (fontFilename.isValid)
								{
									AssetRef fontRef = getAssetRef(AssetType_BitmapFont, fontFilename.value);
									UI::setPropertyValue<AssetRef>(target, property, fontRef);
								}
								else
								{
									error(&reader, "Unrecognised font name '{0}'. Make sure to declare the :Font before it is used!"_s, {value});
								}
							} break;

							case UI::PropType::Int: {
								Maybe<s32> value = readInt<s32>(&reader);
								if (value.isValid)
								{
									UI::setPropertyValue<s32>(target, property, value.value);
								}
							} break;

							case UI::PropType::Padding: {
								Maybe<Padding> value = readPadding(&reader);
								if (value.isValid)
								{
									UI::setPropertyValue<Padding>(target, property, value.value);
								}
							} break;

							case UI::PropType::Style: // NB: Style names are just Strings now
							case UI::PropType::String: {
								String value = intern(&assets->assetStrings, readToken(&reader));
								// Strings are read directly, so we don't need an if(valid) check
								UI::setPropertyValue<String>(target, property, value);
							} break;

							case UI::PropType::V2I: {
								Maybe<s32> offsetX = readInt<s32>(&reader);
								Maybe<s32> offsetY = readInt<s32>(&reader);
								if (offsetX.isValid && offsetY.isValid)
								{
									V2I vector = v2i(offsetX.value, offsetY.value);
									UI::setPropertyValue<V2I>(target, property, vector);
								}
							} break;

							default: logCritical("Invalid property type for '{0}'"_s, {firstWord});
						}
					}
					else
					{
						error(&reader, "Property '{0}' is not allowed in '{1}'"_s, {firstWord, currentSection});
					}
				}
				else
				{
					error(&reader, "Unrecognized property '{0}'"_s, {firstWord});
				}
			}
		}
	}

	// Actually write out the styles into the UITheme

	s32 totalStyleCount = 0;
	for (s32 i=0; i < UI::StyleTypeCount; i++)
	{
		totalStyleCount += styleCount[i];
	}
	allocateChildren(asset, totalStyleCount);

	// Some default values to use
	V4 transparent = color255(0, 0, 0, 0);
	V4 white = makeWhite();
	AssetRef defaultFont = getAssetRef(AssetType_BitmapFont, nullString);
	String defaultStyleName = "default"_h;

	for (auto it = styles.iterate(); it.hasNext(); it.next())
	{
		StylePack *stylePack = it.get();
		for (s32 sectionType = 1; sectionType < UI::StyleTypeCount; sectionType++)
		{
			UI::Style *style = stylePack->styleByType + sectionType;
			// For undefined styles, the parent struct will be all nulls, so the type will not match
			if (style->type == sectionType)
			{
				switch (style->type)
				{
					case UI::Style_Button: {
						Asset *childAsset = addAsset(AssetType_ButtonStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UI::ButtonStyle *button = &childAsset->buttonStyle;
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

						button->background 			= style->background.value;
						button->backgroundHover 	= style->backgroundHover.orDefault(button->background);
						button->backgroundPressed 	= style->backgroundPressed.orDefault(button->background);
						button->backgroundDisabled 	= style->backgroundDisabled.orDefault(button->background);

						if (!button->startIcon.hasFixedSize())
						{
							error(&reader, "Start icon for button '{0}' has no fixed size. Defaulting to 0 x 0"_s, {button->name});
						}
						if (!button->endIcon.hasFixedSize())
						{
							error(&reader, "End icon for button '{0}' has no fixed size. Defaulting to 0 x 0"_s, {button->name});
						}
					} break;

					case UI::Style_Checkbox: {
						Asset *childAsset = addAsset(AssetType_CheckboxStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UI::CheckboxStyle *checkbox = &childAsset->checkboxStyle;
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
						if (style->checkSize.isValid)
						{
							checkbox->checkSize = style->checkSize.value;
						}
						else if (checkbox->check.hasFixedSize())
						{
							checkbox->checkSize = checkbox->check.getSize();
						}
						else
						{
							error(&reader, "Check for checkbox '{0}' has no fixed size, and no checkSize was provided. Defaulting to 0 x 0"_s, {checkbox->name});
						}
					} break;

					case UI::Style_Console: {
						Asset *childAsset = addAsset(AssetType_ConsoleStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UI::ConsoleStyle *console = &childAsset->consoleStyle;
						console->name = style->name;

						console->font = style->font.orDefault(defaultFont);

						console->outputTextColor 			= style->outputTextColor.orDefault(white);
						console->outputTextColorInputEcho 	= style->outputTextColorInputEcho.orDefault(white);
						console->outputTextColorError 		= style->outputTextColorError.orDefault(white);
						console->outputTextColorWarning 	= style->outputTextColorWarning.orDefault(white);
						console->outputTextColorSuccess 	= style->outputTextColorSuccess.orDefault(white);

						console->background = style->background.value;
						console->padding = style->padding.value;

						console->scrollbarStyle = getAssetRef(AssetType_ScrollbarStyle, style->scrollbarStyle.orDefault(defaultStyleName));
						console->textInputStyle = getAssetRef(AssetType_TextInputStyle, style->textInputStyle.orDefault(defaultStyleName));
					} break;

					case UI::Style_DropDownList: {
						Asset *childAsset = addAsset(AssetType_DropDownListStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UI::DropDownListStyle *ddl = &childAsset->dropDownListStyle;
						ddl->name = style->name;

						ddl->buttonStyle = getAssetRef(AssetType_ButtonStyle, style->buttonStyle.orDefault(defaultStyleName));
						ddl->panelStyle = getAssetRef(AssetType_PanelStyle, style->panelStyle.orDefault(defaultStyleName));
					} break;

					case UI::Style_Label: {
						Asset *childAsset = addAsset(AssetType_LabelStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UI::LabelStyle *label = &childAsset->labelStyle;
						label->name = style->name;

						label->padding = style->padding.value;
						label->background = style->background.value;
						label->font = style->font.orDefault(defaultFont);
						label->textColor = style->textColor.orDefault(white);
						label->textAlignment = style->textAlignment.orDefault(ALIGN_TOP | ALIGN_LEFT);
					} break;

					case UI::Style_Panel: {
						Asset *childAsset = addAsset(AssetType_PanelStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UI::PanelStyle *panel = &childAsset->panelStyle;
						panel->name = style->name;

						panel->padding = style->padding.value;
						panel->contentPadding = style->contentPadding.value;
						panel->widgetAlignment = style->widgetAlignment.orDefault(ALIGN_TOP | ALIGN_EXPAND_H);
						panel->background = style->background.value;

						panel->buttonStyle 			= getAssetRef(AssetType_ButtonStyle, style->buttonStyle.orDefault(defaultStyleName));
						panel->checkboxStyle 		= getAssetRef(AssetType_CheckboxStyle, style->checkboxStyle.orDefault(defaultStyleName));
						panel->dropDownListStyle 	= getAssetRef(AssetType_DropDownListStyle, style->dropDownListStyle.orDefault(defaultStyleName));
						panel->labelStyle 			= getAssetRef(AssetType_LabelStyle, style->labelStyle.orDefault(defaultStyleName));
						panel->radioButtonStyle		= getAssetRef(AssetType_RadioButtonStyle, style->radioButtonStyle.orDefault(defaultStyleName));
						panel->scrollbarStyle 		= getAssetRef(AssetType_ScrollbarStyle, style->scrollbarStyle.orDefault(defaultStyleName));
						panel->sliderStyle 			= getAssetRef(AssetType_SliderStyle, style->sliderStyle.orDefault(defaultStyleName));
						panel->textInputStyle 		= getAssetRef(AssetType_TextInputStyle, style->textInputStyle.orDefault(defaultStyleName));
					} break;

					case UI::Style_RadioButton: {
						Asset *childAsset = addAsset(AssetType_RadioButtonStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UI::RadioButtonStyle *radioButton = &childAsset->radioButtonStyle;
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

					case UI::Style_Scrollbar: {
						Asset *childAsset = addAsset(AssetType_ScrollbarStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UI::ScrollbarStyle *scrollbar = &childAsset->scrollbarStyle;
						scrollbar->name = style->name;

						scrollbar->background = style->background.value;
						scrollbar->thumb = style->thumb.value;
						scrollbar->thumbDisabled = style->thumbDisabled.orDefault(scrollbar->thumb);
						scrollbar->thumbHover = style->thumbHover.orDefault(scrollbar->thumb);
						scrollbar->thumbPressed = style->thumbPressed.orDefault(scrollbar->thumb);
						scrollbar->width = style->width.orDefault(8);
					} break;

					case UI::Style_Slider: {
						Asset *childAsset = addAsset(AssetType_SliderStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UI::SliderStyle *slider = &childAsset->sliderStyle;
						slider->name = style->name;

						slider->track = style->track.value;
						slider->trackThickness = style->trackThickness.orDefault(3);
						slider->thumb = style->thumb.value;
						slider->thumbDisabled = style->thumbDisabled.orDefault(slider->thumb);
						slider->thumbHover = style->thumbHover.orDefault(slider->thumb);
						slider->thumbPressed = style->thumbPressed.orDefault(slider->thumb);
						slider->thumbSize = style->thumbSize.orDefault(v2i(8,8));
					} break;

					case UI::Style_TextInput: {
						Asset *childAsset = addAsset(AssetType_TextInputStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UI::TextInputStyle *textInput = &childAsset->textInputStyle;
						textInput->name = style->name;

						textInput->font = style->font.orDefault(defaultFont);
						textInput->textColor = style->textColor.orDefault(white);
						textInput->textAlignment = style->textAlignment.orDefault(ALIGN_TOP | ALIGN_LEFT);

						textInput->background = style->background.value;
						textInput->padding = style->padding.value;
				
						textInput->showCaret = style->showCaret.orDefault(true);
						textInput->caretFlashCycleDuration = style->caretFlashCycleDuration.orDefault(1.0f);
					} break;

					case UI::Style_Window: {
						Asset *childAsset = addAsset(AssetType_WindowStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UI::WindowStyle *window = &childAsset->windowStyle;
						window->name = style->name;

						window->titleBarHeight = style->titleBarHeight.orDefault(16);
						window->titleBarColor = style->titleBarColor.orDefault(color255(128, 128, 128, 255));
						window->titleBarColorInactive = style->titleBarColorInactive.orDefault(window->titleBarColor);
						window->titleFont = style->titleFont.orDefault(defaultFont);
						window->titleColor = style->titleColor.orDefault(white);
						window->titleBarButtonHoverColor = style->titleBarButtonHoverColor.orDefault(transparent);

						window->offsetFromMouse = style->offsetFromMouse.value;

						window->panelStyle = getAssetRef(AssetType_PanelStyle, style->panelStyle.orDefault(defaultStyleName));
					} break;
				}
			}
		}
	}
}
