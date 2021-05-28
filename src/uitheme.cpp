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
		Maybe<s32> spriteIndex = readInt<s32>(reader, true);
		s32 index = spriteIndex.isValid ? spriteIndex.value : 0;

		Maybe<V4> color = readColor(reader, true);

		UIDrawableStyle drawable = {};
		drawable.type = Drawable_Sprite;
		drawable.color = color.orDefault(makeWhite());
		drawable.sprite = getSpriteRef(spriteName, index);

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

void initUIStyleProperties()
{
	initHashTable(&uiStyleProperties, 0.75f, 256);
	#define PROP(name, _type, inButton, inCheckbox, inConsole, inDDL, inLabel, inPanel, inScrollbar, inTextInput, inWindow) {\
		UIProperty property = {}; \
		property.type = _type; \
		property.offsetInStyleStruct = offsetof(UIStyle, name); \
		property.existsInStyle[UIStyle_Button]    = inButton; \
		property.existsInStyle[UIStyle_Checkbox]  = inCheckbox; \
		property.existsInStyle[UIStyle_Console]   = inConsole; \
		property.existsInStyle[UIStyle_DropDownList]   = inDDL; \
		property.existsInStyle[UIStyle_Label]     = inLabel; \
		property.existsInStyle[UIStyle_Panel]     = inPanel; \
		property.existsInStyle[UIStyle_Scrollbar] = inScrollbar; \
		property.existsInStyle[UIStyle_TextInput] = inTextInput; \
		property.existsInStyle[UIStyle_Window]    = inWindow; \
		uiStyleProperties.put(makeString(#name), property); \
	}

	//                                                    btn  chkbx   cnsl    DDL  label  panel  scrll  txtin  windw
	PROP(background,				PropType_Drawable,	 true,  true,  true, false, false,  true,  true,  true, false);
	PROP(buttonStyle,				PropType_Style,		false, false, false,  true, false,  true, false, false, false);
	PROP(caretFlashCycleDuration,	PropType_Float,		false, false, false, false, false, false, false,  true, false);
	PROP(checkImage,				PropType_Drawable,	false,  true, false, false, false, false, false, false, false);
	PROP(checkboxStyle,				PropType_Style,		false, false, false, false, false,  true, false, false, false);
	PROP(contentSize,				PropType_V2I,		false,  true, false, false, false, false, false, false, false);
	PROP(contentPadding,			PropType_Int,		 true, false, false, false, false,  true, false, false, false);
	PROP(disabledBackground,		PropType_Drawable,	 true,  true, false, false, false, false, false, false, false);
	PROP(disabledCheckImage,		PropType_Drawable,	false,  true, false, false, false, false, false, false, false);
	PROP(dropDownListStyle,			PropType_Style,		false,  true, false, false, false,  true, false, false, false);
	PROP(endIcon,					PropType_Drawable,	 true, false, false, false, false, false, false, false, false);
	PROP(endIconAlignment,			PropType_Alignment,	 true, false, false, false, false, false, false, false, false);
	PROP(font,						PropType_Font,		 true, false,  true, false,  true, false, false,  true, false);
	PROP(hoverBackground,			PropType_Drawable,	 true,  true, false, false, false, false, false, false, false);
	PROP(hoverCheckImage,			PropType_Drawable,	false,  true, false, false, false, false, false, false, false);
	PROP(labelStyle,				PropType_Style,		false, false, false, false, false,  true, false, false, false);
	PROP(margin,					PropType_Int,		false, false, false, false, false,  true, false, false, false);
	PROP(offsetFromMouse,			PropType_V2I,		false, false, false, false, false, false, false, false,  true);
	PROP(padding,					PropType_Int,		 true,  true,  true, false, false, false, false,  true, false);
	PROP(panelStyle,				PropType_Style,		false, false, false,  true, false, false, false, false,  true);
	PROP(pressedBackground,			PropType_Drawable,	 true,  true, false, false, false, false, false, false, false);
	PROP(pressedCheckImage,			PropType_Drawable,	false,  true, false, false, false, false, false, false, false);
	PROP(scrollbarStyle,			PropType_Style,		false, false,  true, false, false,  true, false, false, false);
	PROP(showCaret,					PropType_Bool,		false, false, false, false, false, false, false,  true, false);
	PROP(startIcon,					PropType_Drawable,	 true, false, false, false, false, false, false, false, false);
	PROP(startIconAlignment,		PropType_Alignment,	 true, false, false, false, false, false, false, false, false);
	PROP(textAlignment,				PropType_Alignment,	 true, false, false, false, false, false, false,  true, false);
	PROP(textColor,					PropType_Color,		 true, false, false, false,  true, false, false,  true, false);
	PROP(textInputStyle,			PropType_Style,		false, false,  true, false, false,  true, false, false, false);
	PROP(thumb,						PropType_Drawable,	false, false, false, false, false, false,  true, false, false);
	PROP(titleBarButtonHoverColor,	PropType_Color,		false, false, false, false, false, false, false, false,  true);
	PROP(titleBarColor,				PropType_Color,		false, false, false, false, false, false, false, false,  true);
	PROP(titleBarColorInactive,		PropType_Color,		false, false, false, false, false, false, false, false,  true);
	PROP(titleBarHeight,			PropType_Int,		false, false, false, false, false, false, false, false,  true);
	PROP(titleColor,				PropType_Color,		false, false, false, false, false, false, false, false,  true);
	PROP(titleFont,					PropType_Font,		false, false, false, false, false, false, false, false,  true);
	PROP(widgetAlignment,			PropType_Alignment,	false, false, false, false, false,  true, false, false, false);
	PROP(width,						PropType_Int,		false, false, false, false, false, false,  true, false, false);

	// PROP(track,						PropType_Drawable,	false, false, false, false, false, false, false, false, false);
	// PROP(thumbSize,					PropType_V2I,		false, false, false, false, false, false, false, false, false);
	// PROP(width,				PropType_Int,        false, false, false, false, false, false, false, false, false);

	#undef PROP
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
			else if (equals(firstWord, "Button"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target = addStyle(&styles, name, UIStyle_Button);
				target->name = name;
				target->type = UIStyle_Button;
				styleCount[UIStyle_Button]++;
			}
			else if (equals(firstWord, "Checkbox"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target = addStyle(&styles, name, UIStyle_Checkbox);
				target->name = name;
				target->type = UIStyle_Checkbox;
				styleCount[UIStyle_Checkbox]++;
			}
			else if (equals(firstWord, "Console"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target = addStyle(&styles, name, UIStyle_Console);
				target->name = name;
				target->type = UIStyle_Console;
				styleCount[UIStyle_Console]++;
			}
			else if (equals(firstWord, "DropDownList"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target = addStyle(&styles, name, UIStyle_DropDownList);
				target->name = name;
				target->type = UIStyle_DropDownList;
				styleCount[UIStyle_DropDownList]++;
			}
			else if (equals(firstWord, "Label"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target = addStyle(&styles, name, UIStyle_Label);
				target->name = name;
				target->type = UIStyle_Label;
				styleCount[UIStyle_Label]++;
			}
			else if (equals(firstWord, "Panel"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target = addStyle(&styles, name, UIStyle_Panel);
				target->name = name;
				target->type = UIStyle_Panel;
				styleCount[UIStyle_Panel]++;
			}
			else if (equals(firstWord, "Scrollbar"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target = addStyle(&styles, name, UIStyle_Scrollbar);
				target->name = name;
				target->type = UIStyle_Scrollbar;
				styleCount[UIStyle_Scrollbar]++;
			}
			else if (equals(firstWord, "TextInput"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target = addStyle(&styles, name, UIStyle_TextInput);
				target->name = name;
				target->type = UIStyle_TextInput;
				styleCount[UIStyle_TextInput]++;
			}
			else if (equals(firstWord, "Window"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target = addStyle(&styles, name, UIStyle_Window);
				target->name = name;
				target->type = UIStyle_Window;
				styleCount[UIStyle_Window]++;
			}
			else
			{
				error(&reader, "Unrecognized command: '{0}'"_s, {firstWord});
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
			else if (equals(firstWord, "outputTextColor"_s))
			{
				String category = readToken(&reader);
				Maybe<V4> color = readColor(&reader);

				if (color.isValid)
				{
					if (equals(category, "default"_s))
					{
						target->outputTextColor[CLS_Default] = color.value;
					}
					else if (equals(category, "echo"_s))
					{
						target->outputTextColor[CLS_InputEcho] = color.value;
					}
					else if (equals(category, "success"_s))
					{
						target->outputTextColor[CLS_Success] = color.value;
					}
					else if (equals(category, "warning"_s))
					{
						target->outputTextColor[CLS_Warning] = color.value;
					}
					else if (equals(category, "error"_s))
					{
						target->outputTextColor[CLS_Error] = color.value;
					}
					else
					{
						warn(&reader, "Unrecognized output text category '{0}'"_s, {category});
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
						button->hoverBackground = style->hoverBackground;
						button->pressedBackground = style->pressedBackground;
						button->disabledBackground = style->disabledBackground;

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
						checkbox->hoverBackground = style->hoverBackground;
						checkbox->pressedBackground = style->pressedBackground;
						checkbox->disabledBackground = style->disabledBackground;

						checkbox->contentSize = style->contentSize;
						checkbox->checkImage = style->checkImage;
						checkbox->hoverCheckImage = style->hoverCheckImage;
						checkbox->pressedCheckImage = style->pressedCheckImage;
						checkbox->disabledCheckImage = style->disabledCheckImage;

						// ASSERT(checkbox->checkImage.hasFixedSize()
						//  && checkbox->hoverCheckImage.hasFixedSize()
						//  && checkbox->pressedCheckImage.hasFixedSize()
						//  && checkbox->disabledCheckImage.hasFixedSize());
					} break;

					case UIStyle_Console: {
						Asset *childAsset = addAsset(AssetType_ConsoleStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UIConsoleStyle *console = &childAsset->consoleStyle;
						console->name = style->name;

						console->font = style->font;
						copyMemory(style->outputTextColor, console->outputTextColor, CLS_COUNT);

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
						panel->dropDownListStyle	= getAssetRef(AssetType_DropDownListStyle, style->dropDownListStyle);
						panel->labelStyle 			= getAssetRef(AssetType_LabelStyle, style->labelStyle);
						panel->scrollbarStyle 		= getAssetRef(AssetType_ScrollbarStyle, style->scrollbarStyle);
						panel->textInputStyle 		= getAssetRef(AssetType_TextInputStyle, style->textInputStyle);
					} break;

					case UIStyle_Scrollbar: {
						Asset *childAsset = addAsset(AssetType_ScrollbarStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UIScrollbarStyle *scrollbar = &childAsset->scrollbarStyle;
						scrollbar->name = style->name;

						scrollbar->background = style->background;
						scrollbar->thumb = style->thumb;
						scrollbar->width = style->width;
					} break;

					case UIStyle_Slider: {
						Asset *childAsset = addAsset(AssetType_SliderStyle, style->name, 0);
						addChildAsset(asset, childAsset);

						UISliderStyle *slider = &childAsset->sliderStyle;
						slider->name = style->name;

						slider->track = style->track;
						slider->thumb = style->thumb;
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
