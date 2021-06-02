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
			property.type = _type; \
			property.offsetInStyleStruct = offsetof(Style, name); \
			styleProperties.put(makeString(#name), property); \
		}

		PROP(background,				PropType_Drawable);
		PROP(backgroundDisabled,		PropType_Drawable);
		PROP(backgroundHover,			PropType_Drawable);
		PROP(backgroundPressed,			PropType_Drawable);
		PROP(buttonStyle,				PropType_Style);
		PROP(caretFlashCycleDuration,	PropType_Float);
		PROP(check,						PropType_Drawable);
		PROP(checkDisabled,				PropType_Drawable);
		PROP(checkHover,				PropType_Drawable);
		PROP(checkPressed,				PropType_Drawable);
		PROP(checkboxStyle,				PropType_Style);
		PROP(contentSize,				PropType_V2I);
		PROP(contentPadding,			PropType_Int);
		PROP(dropDownListStyle,			PropType_Style);
		PROP(endIcon,					PropType_Drawable);
		PROP(endIconAlignment,			PropType_Alignment);
		PROP(font,						PropType_Font);
		PROP(labelStyle,				PropType_Style);
		PROP(margin,					PropType_Int);
		PROP(offsetFromMouse,			PropType_V2I);
		PROP(outputTextColor,			PropType_Color);
		PROP(outputTextColorInputEcho,	PropType_Color);
		PROP(outputTextColorError,		PropType_Color);
		PROP(outputTextColorWarning,	PropType_Color);
		PROP(outputTextColorSuccess,	PropType_Color);
		PROP(padding,					PropType_Int);
		PROP(panelStyle,				PropType_Style);
		PROP(scrollbarStyle,			PropType_Style);
		PROP(showCaret,					PropType_Bool);
		PROP(sliderStyle,				PropType_Style);
		PROP(startIcon,					PropType_Drawable);
		PROP(startIconAlignment,		PropType_Alignment);
		PROP(textAlignment,				PropType_Alignment);
		PROP(textColor,					PropType_Color);
		PROP(textInputStyle,			PropType_Style);
		PROP(thumb,						PropType_Drawable);
		PROP(thumbDisabled,				PropType_Drawable);
		PROP(thumbHover,				PropType_Drawable);
		PROP(thumbPressed,				PropType_Drawable);
		PROP(titleBarButtonHoverColor,	PropType_Color);
		PROP(titleBarColor,				PropType_Color);
		PROP(titleBarColorInactive,		PropType_Color);
		PROP(titleBarHeight,			PropType_Int);
		PROP(titleColor,				PropType_Color);
		PROP(titleFont,					PropType_Font);
		PROP(trackThickness,			PropType_Int);
		PROP(widgetAlignment,			PropType_Alignment);
		PROP(width,						PropType_Int);
		PROP(track,						PropType_Drawable);
		PROP(thumbSize,					PropType_V2I);

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
			"contentSize"_s,
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
			"font"_s,
			"textColor"_s,
		});
		assignStyleProperties(Style_Panel, {
			"background"_s,
			"buttonStyle"_s,
			"checkboxStyle"_s,
			"contentPadding"_s,
			"dropDownListStyle"_s,
			"labelStyle"_s,
			"margin"_s,
			"scrollbarStyle"_s,
			"sliderStyle"_s,
			"textInputStyle"_s,
			"widgetAlignment"_s,
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
							case UI::PropType_Alignment: {
								Maybe<u32> value = readAlignment(&reader);
								if (value.isValid)
								{
									*((u32*)((u8*)(target) + property->offsetInStyleStruct)) = value.value;
								}
							} break;

							case UI::PropType_Bool: {
								Maybe<bool> value = readBool(&reader);
								if (value.isValid)
								{
									*((bool*)((u8*)(target) + property->offsetInStyleStruct)) = value.value;
								}
							} break;

							case UI::PropType_Color: {
								Maybe<V4> value = readColor(&reader);
								if (value.isValid)
								{
									*((V4*)((u8*)(target) + property->offsetInStyleStruct)) = value.value;
								}
							} break;

							case UI::PropType_Drawable: {
								Maybe<UI::DrawableStyle> value = UI::readDrawableStyle(&reader);
								if (value.isValid)
								{
									*((UI::DrawableStyle*)((u8*)(target) + property->offsetInStyleStruct)) = value.value;
								}
							} break;

							case UI::PropType_Float: {
								Maybe<f64> value = readFloat(&reader);
								if (value.isValid)
								{
									*((f32*)((u8*)(target) + property->offsetInStyleStruct)) = (f32)(value.value);
								}
							} break;

							case UI::PropType_Font: {
								String value = intern(&assets->assetStrings, readToken(&reader));
								AssetRef *fontRef = ((AssetRef*)((u8*)(target) + property->offsetInStyleStruct));
								*fontRef = {};
								Maybe<String> fontFilename = fontNamesToAssetNames.findValue(value);
								if (fontFilename.isValid)
								{
									*fontRef = getAssetRef(AssetType_BitmapFont, fontFilename.value);
								}
								else
								{
									error(&reader, "Unrecognised font name '{0}'. Make sure to declare the :Font before it is used!"_s, {value});
								}
							} break;

							case UI::PropType_Int: {
								Maybe<s32> value = readInt<s32>(&reader);
								if (value.isValid)
								{
									*((s32*)((u8*)(target) + property->offsetInStyleStruct)) = value.value;
								}
							} break;

							case UI::PropType_Style: // NB: Style names are just Strings now
							case UI::PropType_String: {
								String value = intern(&assets->assetStrings, readToken(&reader));
								// Strings are read directly, so we don't need an if(valid) check
								*((String*)((u8*)(target) + property->offsetInStyleStruct)) = value;
							} break;

							case UI::PropType_V2I: {
								Maybe<s32> offsetX = readInt<s32>(&reader);
								Maybe<s32> offsetY = readInt<s32>(&reader);
								if (offsetX.isValid && offsetY.isValid)
								{
									V2I vector = v2i(offsetX.value, offsetY.value);
									*((V2I*)((u8*)(target) + property->offsetInStyleStruct)) = vector;
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

						button->font = style->font;
						button->textColor = style->textColor;
						button->textAlignment = style->textAlignment;

						button->padding = style->padding;
						button->contentPadding = style->contentPadding;

						button->startIcon = style->startIcon;
						button->startIconAlignment = style->startIconAlignment;
						button->endIcon = style->endIcon;
						button->endIconAlignment = style->endIconAlignment;

						button->background = style->background;
						button->backgroundHover = style->backgroundHover;
						button->backgroundPressed = style->backgroundPressed;
						button->backgroundDisabled = style->backgroundDisabled;

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

						checkbox->padding = style->padding;

						checkbox->background = style->background;
						checkbox->backgroundHover = style->backgroundHover;
						checkbox->backgroundPressed = style->backgroundPressed;
						checkbox->backgroundDisabled = style->backgroundDisabled;

						checkbox->contentSize = style->contentSize;
						checkbox->check = style->check;
						checkbox->checkHover = style->checkHover;
						checkbox->checkPressed = style->checkPressed;
						checkbox->checkDisabled = style->checkDisabled;

						// ASSERT(checkbox->check.hasFixedSize()
						//  && checkbox->checkHover.hasFixedSize()
						//  && checkbox->checkPressed.hasFixedSize()
						//  && checkbox->checkDisabled.hasFixedSize());
					} break;

					case UI::Style_Console: {
						Asset *childAsset = addAsset(AssetType_ConsoleStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UI::ConsoleStyle *console = &childAsset->consoleStyle;
						console->name = style->name;

						console->font = style->font;

						console->outputTextColor 			= style->outputTextColor;
						console->outputTextColorInputEcho 	= style->outputTextColorInputEcho;
						console->outputTextColorError 		= style->outputTextColorError;
						console->outputTextColorWarning 	= style->outputTextColorWarning;
						console->outputTextColorSuccess 	= style->outputTextColorSuccess;

						console->background = style->background;
						console->padding = style->padding;

						console->scrollbarStyle = getAssetRef(AssetType_ScrollbarStyle, style->scrollbarStyle);
						console->textInputStyle = getAssetRef(AssetType_TextInputStyle, style->textInputStyle);
					} break;

					case UI::Style_DropDownList: {
						Asset *childAsset = addAsset(AssetType_DropDownListStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UI::DropDownListStyle *ddl = &childAsset->dropDownListStyle;
						ddl->name = style->name;

						ddl->buttonStyle = getAssetRef(AssetType_ButtonStyle, style->buttonStyle);
						ddl->panelStyle = getAssetRef(AssetType_PanelStyle, style->panelStyle);
					} break;

					case UI::Style_Label: {
						Asset *childAsset = addAsset(AssetType_LabelStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UI::LabelStyle *label = &childAsset->labelStyle;
						label->name = style->name;

						label->font = style->font;
						label->textColor = style->textColor;
					} break;

					case UI::Style_Panel: {
						Asset *childAsset = addAsset(AssetType_PanelStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UI::PanelStyle *panel = &childAsset->panelStyle;
						panel->name = style->name;

						panel->margin = style->margin;
						panel->contentPadding = style->contentPadding;
						panel->widgetAlignment = style->widgetAlignment;
						panel->background = style->background;

						panel->buttonStyle 			= getAssetRef(AssetType_ButtonStyle, style->buttonStyle);
						panel->checkboxStyle 		= getAssetRef(AssetType_CheckboxStyle, style->checkboxStyle);
						panel->dropDownListStyle 	= getAssetRef(AssetType_DropDownListStyle, style->dropDownListStyle);
						panel->labelStyle 			= getAssetRef(AssetType_LabelStyle, style->labelStyle);
						panel->scrollbarStyle 		= getAssetRef(AssetType_ScrollbarStyle, style->scrollbarStyle);
						panel->sliderStyle 			= getAssetRef(AssetType_SliderStyle, style->sliderStyle);
						panel->textInputStyle 		= getAssetRef(AssetType_TextInputStyle, style->textInputStyle);
					} break;

					case UI::Style_Scrollbar: {
						Asset *childAsset = addAsset(AssetType_ScrollbarStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UI::ScrollbarStyle *scrollbar = &childAsset->scrollbarStyle;
						scrollbar->name = style->name;

						scrollbar->background = style->background;
						scrollbar->thumb = style->thumb;
						scrollbar->thumbDisabled = style->thumbDisabled;
						scrollbar->thumbHover = style->thumbHover;
						scrollbar->thumbPressed = style->thumbPressed;
						scrollbar->width = style->width;
					} break;

					case UI::Style_Slider: {
						Asset *childAsset = addAsset(AssetType_SliderStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UI::SliderStyle *slider = &childAsset->sliderStyle;
						slider->name = style->name;

						slider->track = style->track;
						slider->trackThickness = style->trackThickness;
						slider->thumb = style->thumb;
						slider->thumbDisabled = style->thumbDisabled;
						slider->thumbHover = style->thumbHover;
						slider->thumbPressed = style->thumbPressed;
						slider->thumbSize = style->thumbSize;
					} break;

					case UI::Style_TextInput: {
						Asset *childAsset = addAsset(AssetType_TextInputStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UI::TextInputStyle *textInput = &childAsset->textInputStyle;
						textInput->name = style->name;

						textInput->font = style->font;
						textInput->textColor = style->textColor;
						textInput->textAlignment = style->textAlignment;

						textInput->background = style->background;
						textInput->padding = style->padding;
				
						textInput->showCaret = style->showCaret;
						textInput->caretFlashCycleDuration = style->caretFlashCycleDuration;
					} break;

					case UI::Style_Window: {
						Asset *childAsset = addAsset(AssetType_WindowStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UI::WindowStyle *window = &childAsset->windowStyle;
						window->name = style->name;

						window->titleBarHeight = style->titleBarHeight;
						window->titleBarColor = style->titleBarColor;
						window->titleBarColorInactive = style->titleBarColorInactive;
						window->titleFont = style->titleFont;
						window->titleColor = style->titleColor;
						window->titleBarButtonHoverColor = style->titleBarButtonHoverColor;

						window->offsetFromMouse = style->offsetFromMouse;

						window->panelStyle = getAssetRef(AssetType_PanelStyle, style->panelStyle);
					} break;
				}
			}
		}
	}
}

template <typename T>
bool checkStyleMatchesType(AssetRef *reference)
{
	switch (reference->type)
	{
		case AssetType_ButtonStyle: 		return (typeid(T*) == typeid(UI::ButtonStyle*));
		case AssetType_CheckboxStyle: 		return (typeid(T*) == typeid(UI::CheckboxStyle*));
		case AssetType_ConsoleStyle: 		return (typeid(T*) == typeid(UI::ConsoleStyle*));
		case AssetType_DropDownListStyle: 	return (typeid(T*) == typeid(UI::DropDownListStyle*));
		case AssetType_LabelStyle: 	 		return (typeid(T*) == typeid(UI::LabelStyle*));
		case AssetType_PanelStyle:      	return (typeid(T*) == typeid(UI::PanelStyle*));
		case AssetType_ScrollbarStyle:  	return (typeid(T*) == typeid(UI::ScrollbarStyle*));
		case AssetType_SliderStyle:			return (typeid(T*) == typeid(UI::SliderStyle*));
		case AssetType_TextInputStyle:  	return (typeid(T*) == typeid(UI::TextInputStyle*));
		case AssetType_WindowStyle: 		return (typeid(T*) == typeid(UI::WindowStyle*));
		INVALID_DEFAULT_CASE;
	}

	return false;
}

template <typename T>
inline T* findStyle(AssetRef *ref)
{
	ASSERT(checkStyleMatchesType<T>(ref));

	Asset *asset = getAsset(ref);

	return (T*) &asset->_localData;
}

template <> UI::ButtonStyle *findStyle<UI::ButtonStyle>(String styleName)
{
	UI::ButtonStyle *result = null;

	Asset *asset = getAsset(AssetType_ButtonStyle, styleName);
	if (asset != null)
	{
		result = (UI::ButtonStyle *)&asset->_localData;
	}

	return result;
}
template <> UI::CheckboxStyle *findStyle<UI::CheckboxStyle>(String styleName)
{
	UI::CheckboxStyle *result = null;

	Asset *asset = getAsset(AssetType_CheckboxStyle, styleName);
	if (asset != null)
	{
		result = (UI::CheckboxStyle *)&asset->_localData;
	}

	return result;
}
template <> UI::ConsoleStyle *findStyle<UI::ConsoleStyle>(String styleName)
{
	UI::ConsoleStyle *result = null;

	Asset *asset = getAsset(AssetType_ConsoleStyle, styleName);
	if (asset != null)
	{
		result = (UI::ConsoleStyle *)&asset->_localData;
	}

	return result;
}
template <> UI::DropDownListStyle *findStyle<UI::DropDownListStyle>(String styleName)
{
	UI::DropDownListStyle *result = null;

	Asset *asset = getAsset(AssetType_DropDownListStyle, styleName);
	if (asset != null)
	{
		result = (UI::DropDownListStyle *)&asset->_localData;
	}

	return result;
}
template <> UI::LabelStyle *findStyle<UI::LabelStyle>(String styleName)
{
	UI::LabelStyle *result = null;

	Asset *asset = getAsset(AssetType_LabelStyle, styleName);
	if (asset != null)
	{
		result = (UI::LabelStyle *)&asset->_localData;
	}

	return result;
}
template <> UI::PanelStyle *findStyle<UI::PanelStyle>(String styleName)
{
	UI::PanelStyle *result = null;

	Asset *asset = getAsset(AssetType_PanelStyle, styleName);
	if (asset != null)
	{
		result = (UI::PanelStyle *)&asset->_localData;
	}

	return result;
}
template <> UI::ScrollbarStyle *findStyle<UI::ScrollbarStyle>(String styleName)
{
	UI::ScrollbarStyle *result = null;

	Asset *asset = getAsset(AssetType_ScrollbarStyle, styleName);
	if (asset != null)
	{
		result = (UI::ScrollbarStyle *)&asset->_localData;
	}

	return result;
}
template <> UI::SliderStyle *findStyle<UI::SliderStyle>(String styleName)
{
	UI::SliderStyle *result = null;

	Asset *asset = getAsset(AssetType_SliderStyle, styleName);
	if (asset != null)
	{
		result = (UI::SliderStyle *)&asset->_localData;
	}

	return result;
}
template <> UI::TextInputStyle *findStyle<UI::TextInputStyle>(String styleName)
{
	UI::TextInputStyle *result = null;

	Asset *asset = getAsset(AssetType_TextInputStyle, styleName);
	if (asset != null)
	{
		result = (UI::TextInputStyle *)&asset->_localData;
	}

	return result;
}
template <> UI::WindowStyle *findStyle<UI::WindowStyle>(String styleName)
{
	UI::WindowStyle *result = null;

	Asset *asset = getAsset(AssetType_WindowStyle, styleName);
	if (asset != null)
	{
		result = (UI::WindowStyle *)&asset->_localData;
	}

	return result;
}
