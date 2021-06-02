#pragma once

Maybe<UIDrawableStyle> readDrawableStyle(LineReader *reader)
{
	String typeName = readToken(reader);
	Maybe<UIDrawableStyle> result = makeFailure<UIDrawableStyle>();

	if (equals(typeName, "none"_s))
	{
		UIDrawableStyle drawable = {};
		drawable.type = Drawable_None;

		result = makeSuccess(drawable);
	}
	else if (equals(typeName, "color"_s))
	{
		Maybe<V4> color = readColor(reader);
		if (color.isValid)
		{
			UIDrawableStyle drawable = {};
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
			UIDrawableStyle drawable = {};
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
		
		UIDrawableStyle drawable = {};
		drawable.type = Drawable_Ninepatch;
		drawable.color = color.orDefault(makeWhite());
		drawable.ninepatch = getAssetRef(AssetType_Ninepatch, ninepatchName);

		result = makeSuccess(drawable);
	}
	else if (equals(typeName, "sprite"_s))
	{
		String spriteName = readToken(reader);

		Maybe<V4> color = readColor(reader, true);

		UIDrawableStyle drawable = {};
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

inline bool UIDrawableStyle::hasFixedSize()
{
	return (type == Drawable_None || type == Drawable_Sprite);
}

V2I UIDrawableStyle::getSize()
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

void initUIStyleConstants()
{
	initHashTable(&uiStyleProperties, 0.75f, 256);
	#define PROP(name, _type) {\
		UIProperty property = {}; \
		property.type = _type; \
		property.offsetInStyleStruct = offsetof(UIStyle, name); \
		uiStyleProperties.put(makeString(#name), property); \
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

	assignStyleProperties(UIStyle_Button, {
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
	assignStyleProperties(UIStyle_Checkbox, {
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
	assignStyleProperties(UIStyle_Console, {
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
	assignStyleProperties(UIStyle_DropDownList, {
		"buttonStyle"_s,
		"panelStyle"_s,
	});
	assignStyleProperties(UIStyle_Label, {
		"font"_s,
		"textColor"_s,
	});
	assignStyleProperties(UIStyle_Panel, {
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
	assignStyleProperties(UIStyle_Scrollbar, {
		"background"_s,
		"thumb"_s,
		"thumbDisabled"_s,
		"thumbHover"_s,
		"thumbPressed"_s,
		"width"_s,
	});
	assignStyleProperties(UIStyle_Slider, {
		"track"_s,
		"trackThickness"_s,
		"thumb"_s,
		"thumbDisabled"_s,
		"thumbHover"_s,
		"thumbPressed"_s,
		"thumbSize"_s,
	});
	assignStyleProperties(UIStyle_TextInput, {
		"background"_s,
		"caretFlashCycleDuration"_s,
		"font"_s,
		"padding"_s,
		"showCaret"_s,
		"textAlignment"_s,
		"textColor"_s,
	});
	assignStyleProperties(UIStyle_Window, {
		"offsetFromMouse"_s,
		"panelStyle"_s,
		"titleBarButtonHoverColor"_s,
		"titleBarColor"_s,
		"titleBarColorInactive"_s,
		"titleBarHeight"_s,
		"titleColor"_s,
		"titleFont"_s,
	});

	initHashTable(&uiStyleTypesByName, 0.75f, 256);
	uiStyleTypesByName.put("Button"_s, UIStyle_Button);
	uiStyleTypesByName.put("Checkbox"_s, UIStyle_Checkbox);
	uiStyleTypesByName.put("Console"_s, UIStyle_Console);
	uiStyleTypesByName.put("DropDownList"_s, UIStyle_DropDownList);
	uiStyleTypesByName.put("Label"_s, UIStyle_Label);
	uiStyleTypesByName.put("Panel"_s, UIStyle_Panel);
	uiStyleTypesByName.put("Scrollbar"_s, UIStyle_Scrollbar);
	uiStyleTypesByName.put("Slider"_s, UIStyle_Slider);
	uiStyleTypesByName.put("TextInput"_s, UIStyle_TextInput);
	uiStyleTypesByName.put("Window"_s, UIStyle_Window);
}

void assignStyleProperties(UIStyleType type, std::initializer_list<String> properties)
{
	for (String *propName = (String*)properties.begin(); propName < properties.end(); propName++)
	{
		UIProperty *property = uiStyleProperties.find(*propName).orDefault(null);
		property->existsInStyle[type] = true;
	}
}

struct UIStylePack
{
	UIStyle styleByType[UIStyleTypeCount];
};
UIStyle *addStyle(HashTable<UIStylePack> *styles, String name, UIStyleType type)
{
	UIStylePack *pack = styles->findOrAdd(name);
	UIStyle *result = pack->styleByType + type;
	*result = UIStyle();

	result->name = name;
	result->type = type;

	return result;
}

void loadUITheme(Blob data, Asset *asset)
{
	LineReader reader = readLines(asset->shortName, data);

	HashTable<UIStylePack> styles;
	initHashTable(&styles);

	HashTable<String> fontNamesToAssetNames;
	initHashTable(&fontNamesToAssetNames);

	defer {
		freeHashTable(&styles);
		freeHashTable(&fontNamesToAssetNames);
	};

	s32 styleCount[UIStyleTypeCount] = {};

	String currentSection = nullString;
	UIStyle *target = null;

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
				Maybe<UIStyleType> foundStyleType = uiStyleTypesByName.findValue(firstWord);
				if (foundStyleType.isValid)
				{
					UIStyleType styleType = foundStyleType.value;

					String name = intern(&assets->assetStrings, readToken(&reader));
					target = addStyle(&styles, name, styleType);
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
				Maybe<UIStylePack *> parentPack = styles.find(parentStyle);
				if (!parentPack.isValid)
				{
					error(&reader, "Unable to find style named '{0}'"_s, {parentStyle});
				}
				else
				{
					UIStyle *parent = parentPack.value->styleByType + target->type;
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
				UIProperty *property = uiStyleProperties.find(firstWord).orDefault(null);
				if (property)
				{
					if (property->existsInStyle[target->type])
					{
						switch (property->type)
						{
							case PropType_Alignment: {
								Maybe<u32> value = readAlignment(&reader);
								if (value.isValid)
								{
									*((u32*)((u8*)(target) + property->offsetInStyleStruct)) = value.value;
								}
							} break;

							case PropType_Bool: {
								Maybe<bool> value = readBool(&reader);
								if (value.isValid)
								{
									*((bool*)((u8*)(target) + property->offsetInStyleStruct)) = value.value;
								}
							} break;

							case PropType_Color: {
								Maybe<V4> value = readColor(&reader);
								if (value.isValid)
								{
									*((V4*)((u8*)(target) + property->offsetInStyleStruct)) = value.value;
								}
							} break;

							case PropType_Drawable: {
								Maybe<UIDrawableStyle> value = readDrawableStyle(&reader);
								if (value.isValid)
								{
									*((UIDrawableStyle*)((u8*)(target) + property->offsetInStyleStruct)) = value.value;
								}
							} break;

							case PropType_Float: {
								Maybe<f64> value = readFloat(&reader);
								if (value.isValid)
								{
									*((f32*)((u8*)(target) + property->offsetInStyleStruct)) = (f32)(value.value);
								}
							} break;

							case PropType_Font: {
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

							case PropType_Int: {
								Maybe<s32> value = readInt<s32>(&reader);
								if (value.isValid)
								{
									*((s32*)((u8*)(target) + property->offsetInStyleStruct)) = value.value;
								}
							} break;

							case PropType_Style: // NB: Style names are just Strings now
							case PropType_String: {
								String value = intern(&assets->assetStrings, readToken(&reader));
								// Strings are read directly, so we don't need an if(valid) check
								*((String*)((u8*)(target) + property->offsetInStyleStruct)) = value;
							} break;

							case PropType_V2I: {
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
	for (s32 i=0; i < UIStyleTypeCount; i++)
	{
		totalStyleCount += styleCount[i];
	}
	allocateChildren(asset, totalStyleCount);

	for (auto it = styles.iterate(); it.hasNext(); it.next())
	{
		UIStylePack *stylePack = it.get();
		for (s32 sectionType = 1; sectionType < UIStyleTypeCount; sectionType++)
		{
			UIStyle *style = stylePack->styleByType + sectionType;
			// For undefined styles, the parent struct will be all nulls, so the type will not match
			if (style->type == sectionType)
			{
				switch (style->type)
				{
					case UIStyle_Button: {
						Asset *childAsset = addAsset(AssetType_ButtonStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UIButtonStyle *button = &childAsset->buttonStyle;
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

					case UIStyle_Checkbox: {
						Asset *childAsset = addAsset(AssetType_CheckboxStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UICheckboxStyle *checkbox = &childAsset->checkboxStyle;
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

					case UIStyle_Console: {
						Asset *childAsset = addAsset(AssetType_ConsoleStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UIConsoleStyle *console = &childAsset->consoleStyle;
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

					case UIStyle_DropDownList: {
						Asset *childAsset = addAsset(AssetType_DropDownListStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UIDropDownListStyle *ddl = &childAsset->dropDownListStyle;
						ddl->name = style->name;

						ddl->buttonStyle = getAssetRef(AssetType_ButtonStyle, style->buttonStyle);
						ddl->panelStyle = getAssetRef(AssetType_PanelStyle, style->panelStyle);
					} break;

					case UIStyle_Label: {
						Asset *childAsset = addAsset(AssetType_LabelStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UILabelStyle *label = &childAsset->labelStyle;
						label->name = style->name;

						label->font = style->font;
						label->textColor = style->textColor;
					} break;

					case UIStyle_Panel: {
						Asset *childAsset = addAsset(AssetType_PanelStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UIPanelStyle *panel = &childAsset->panelStyle;
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

					case UIStyle_Scrollbar: {
						Asset *childAsset = addAsset(AssetType_ScrollbarStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UIScrollbarStyle *scrollbar = &childAsset->scrollbarStyle;
						scrollbar->name = style->name;

						scrollbar->background = style->background;
						scrollbar->thumb = style->thumb;
						scrollbar->thumbDisabled = style->thumbDisabled;
						scrollbar->thumbHover = style->thumbHover;
						scrollbar->thumbPressed = style->thumbPressed;
						scrollbar->width = style->width;
					} break;

					case UIStyle_Slider: {
						Asset *childAsset = addAsset(AssetType_SliderStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UISliderStyle *slider = &childAsset->sliderStyle;
						slider->name = style->name;

						slider->track = style->track;
						slider->trackThickness = style->trackThickness;
						slider->thumb = style->thumb;
						slider->thumbDisabled = style->thumbDisabled;
						slider->thumbHover = style->thumbHover;
						slider->thumbPressed = style->thumbPressed;
						slider->thumbSize = style->thumbSize;
					} break;

					case UIStyle_TextInput: {
						Asset *childAsset = addAsset(AssetType_TextInputStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UITextInputStyle *textInput = &childAsset->textInputStyle;
						textInput->name = style->name;

						textInput->font = style->font;
						textInput->textColor = style->textColor;
						textInput->textAlignment = style->textAlignment;

						textInput->background = style->background;
						textInput->padding = style->padding;
				
						textInput->showCaret = style->showCaret;
						textInput->caretFlashCycleDuration = style->caretFlashCycleDuration;
					} break;

					case UIStyle_Window: {
						Asset *childAsset = addAsset(AssetType_WindowStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UIWindowStyle *window = &childAsset->windowStyle;
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
		case AssetType_ButtonStyle: 		return (typeid(T*) == typeid(UIButtonStyle*));
		case AssetType_CheckboxStyle: 		return (typeid(T*) == typeid(UICheckboxStyle*));
		case AssetType_ConsoleStyle: 		return (typeid(T*) == typeid(UIConsoleStyle*));
		case AssetType_DropDownListStyle: 	return (typeid(T*) == typeid(UIDropDownListStyle*));
		case AssetType_LabelStyle: 	 		return (typeid(T*) == typeid(UILabelStyle*));
		case AssetType_PanelStyle:      	return (typeid(T*) == typeid(UIPanelStyle*));
		case AssetType_ScrollbarStyle:  	return (typeid(T*) == typeid(UIScrollbarStyle*));
		case AssetType_SliderStyle:			return (typeid(T*) == typeid(UISliderStyle*));
		case AssetType_TextInputStyle:  	return (typeid(T*) == typeid(UITextInputStyle*));
		case AssetType_WindowStyle: 		return (typeid(T*) == typeid(UIWindowStyle*));
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

template <> UIButtonStyle *findStyle<UIButtonStyle>(String styleName)
{
	UIButtonStyle *result = null;

	Asset *asset = getAsset(AssetType_ButtonStyle, styleName);
	if (asset != null)
	{
		result = (UIButtonStyle *)&asset->_localData;
	}

	return result;
}
template <> UICheckboxStyle *findStyle<UICheckboxStyle>(String styleName)
{
	UICheckboxStyle *result = null;

	Asset *asset = getAsset(AssetType_CheckboxStyle, styleName);
	if (asset != null)
	{
		result = (UICheckboxStyle *)&asset->_localData;
	}

	return result;
}
template <> UIConsoleStyle *findStyle<UIConsoleStyle>(String styleName)
{
	UIConsoleStyle *result = null;

	Asset *asset = getAsset(AssetType_ConsoleStyle, styleName);
	if (asset != null)
	{
		result = (UIConsoleStyle *)&asset->_localData;
	}

	return result;
}
template <> UIDropDownListStyle *findStyle<UIDropDownListStyle>(String styleName)
{
	UIDropDownListStyle *result = null;

	Asset *asset = getAsset(AssetType_DropDownListStyle, styleName);
	if (asset != null)
	{
		result = (UIDropDownListStyle *)&asset->_localData;
	}

	return result;
}
template <> UILabelStyle *findStyle<UILabelStyle>(String styleName)
{
	UILabelStyle *result = null;

	Asset *asset = getAsset(AssetType_LabelStyle, styleName);
	if (asset != null)
	{
		result = (UILabelStyle *)&asset->_localData;
	}

	return result;
}
template <> UIPanelStyle *findStyle<UIPanelStyle>(String styleName)
{
	UIPanelStyle *result = null;

	Asset *asset = getAsset(AssetType_PanelStyle, styleName);
	if (asset != null)
	{
		result = (UIPanelStyle *)&asset->_localData;
	}

	return result;
}
template <> UIScrollbarStyle *findStyle<UIScrollbarStyle>(String styleName)
{
	UIScrollbarStyle *result = null;

	Asset *asset = getAsset(AssetType_ScrollbarStyle, styleName);
	if (asset != null)
	{
		result = (UIScrollbarStyle *)&asset->_localData;
	}

	return result;
}
template <> UISliderStyle *findStyle<UISliderStyle>(String styleName)
{
	UISliderStyle *result = null;

	Asset *asset = getAsset(AssetType_SliderStyle, styleName);
	if (asset != null)
	{
		result = (UISliderStyle *)&asset->_localData;
	}

	return result;
}
template <> UITextInputStyle *findStyle<UITextInputStyle>(String styleName)
{
	UITextInputStyle *result = null;

	Asset *asset = getAsset(AssetType_TextInputStyle, styleName);
	if (asset != null)
	{
		result = (UITextInputStyle *)&asset->_localData;
	}

	return result;
}
template <> UIWindowStyle *findStyle<UIWindowStyle>(String styleName)
{
	UIWindowStyle *result = null;

	Asset *asset = getAsset(AssetType_WindowStyle, styleName);
	if (asset != null)
	{
		result = (UIWindowStyle *)&asset->_localData;
	}

	return result;
}
