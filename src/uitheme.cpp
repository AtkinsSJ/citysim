#pragma once

Maybe<UIDrawableStyle> readDrawableStyle(LineReader *reader)
{
	String typeName = readToken(reader);
	Maybe<UIDrawableStyle> result = makeFailure<UIDrawableStyle>();

	if (equals(typeName, "none"_s))
	{
		result = makeSuccess(UIDrawableStyle());
	}
	else if (equals(typeName, "color"_s))
	{
		Maybe<V4> color = readColor(reader);
		if (color.isValid)
		{
			result = makeSuccess(UIDrawableStyle(color.value));
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
			result = makeSuccess(UIDrawableStyle(color00.value, color01.value, color10.value, color11.value));
		}
	}
	else if (equals(typeName, "ninepatch"_s))
	{
		String ninepatchName = readToken(reader);

		Maybe<V4> color = readColor(reader, true);
		
		result = makeSuccess(UIDrawableStyle(
			getAssetRef(AssetType_Ninepatch, ninepatchName),
			color.orDefault(makeWhite())
		));
	}
	else if (equals(typeName, "sprite"_s))
	{
		String spriteName = readToken(reader);
		Maybe<s32> spriteIndex = readInt<s32>(reader, true);
		s32 index = spriteIndex.isValid ? spriteIndex.value : 0;

		Maybe<V4> color = readColor(reader, true);

		result = makeSuccess(UIDrawableStyle(getSpriteRef(spriteName, index), color.orDefault(makeWhite())));
	}
	else
	{
		error(reader, "Unrecognised background type '{0}'"_s, {typeName});
	}

	return result;
}

void initUIStyleProperties()
{
	initHashTable(&uiStyleProperties, 0.75f, 256);
	#define PROP(name, _type, inButton, inConsole, inLabel, inPanel, inScrollbar, inTextInput, inWindow) {\
		UIProperty property = {}; \
		property.type = _type; \
		property.offsetInStyleStruct = offsetof(UIStyle, name); \
		property.existsInStyle[UIStyle_Button]    = inButton; \
		property.existsInStyle[UIStyle_Console]   = inConsole; \
		property.existsInStyle[UIStyle_Label]     = inLabel; \
		property.existsInStyle[UIStyle_Panel]     = inPanel; \
		property.existsInStyle[UIStyle_Scrollbar] = inScrollbar; \
		property.existsInStyle[UIStyle_TextInput] = inTextInput; \
		property.existsInStyle[UIStyle_Window]    = inWindow; \
		uiStyleProperties.put(makeString(#name), property); \
	}

	//                                                    btn   cnsl  label  panel  scrll  txtin  windw
	PROP(background,               PropType_Drawable,  true,  true, false,  true,  true,  true, false);
	PROP(buttonStyle,              PropType_Style,      false, false, false,  true, false, false, false);
	PROP(caretFlashCycleDuration,  PropType_Float,      false, false, false, false, false,  true, false);
	PROP(widgetAlignment,          PropType_Alignment,  false, false, false,  true, false, false, false);
	PROP(contentPadding,           PropType_Int,        false, false, false,  true, false, false, false);
	PROP(disabledBackground,       PropType_Drawable,  true, false, false, false, false, false, false);
	PROP(font,                     PropType_Font,        true,  true,  true, false, false,  true, false);
	PROP(hoverBackground,          PropType_Drawable,  true, false, false, false, false, false, false);
	PROP(knob,                     PropType_Drawable, false, false, false, false,  true, false, false);
	PROP(labelStyle,               PropType_Style,      false, false, false,  true, false, false, false);
	PROP(margin,                   PropType_Int,        false, false, false,  true, false, false, false);
	PROP(offsetFromMouse,          PropType_V2I,        false, false, false, false, false, false,  true);
	PROP(padding,                  PropType_Int,         true,  true, false, false, false,  true, false);
	PROP(panelStyle,               PropType_Style,      false, false, false, false, false, false,  true);
	PROP(pressedBackground,        PropType_Drawable,  true, false, false, false, false, false, false);
	PROP(scrollbarStyle,           PropType_Style,      false,  true, false,  true, false, false, false);
	PROP(showCaret,                PropType_Bool,       false, false, false, false, false,  true, false);
	PROP(textAlignment,            PropType_Alignment,   true, false, false, false, false,  true, false);
	PROP(textColor,                PropType_Color,       true, false,  true, false, false,  true, false);
	PROP(textInputStyle,           PropType_Style,      false,  true, false,  true, false, false, false);
	PROP(titleBarButtonHoverColor, PropType_Color,      false, false, false, false, false, false,  true);
	PROP(titleBarColor,            PropType_Color,      false, false, false, false, false, false,  true);
	PROP(titleBarColorInactive,    PropType_Color,      false, false, false, false, false, false,  true);
	PROP(titleBarHeight,           PropType_Int,        false, false, false, false, false, false,  true);
	PROP(titleColor,               PropType_Color,      false, false, false, false, false, false,  true);
	PROP(titleFont,                PropType_Font,       false, false, false, false, false, false,  true);
	PROP(width,                    PropType_Int,        false, false, false, false,  true, false, false);

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
			else if (equals(firstWord, "Console"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target = addStyle(&styles, name, UIStyle_Console);
				target->name = name;
				target->type = UIStyle_Console;
				styleCount[UIStyle_Console]++;
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
				String parentStyleName = readToken(&reader);
				UIStylePack *parentPack = styles.find(parentStyleName);
				if (parentPack == null)
				{
					error(&reader, "Unable to find style named '{0}'"_s, {parentStyleName});
				}
				else
				{
					UIStyle *parent = parentPack->styleByType + target->type;
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
				UIProperty *property = uiStyleProperties.find(firstWord);
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
								String *fontFilename = fontNamesToAssetNames.find(value);
								if (fontFilename != null)
								{
									*fontRef = getAssetRef(AssetType_BitmapFont, *fontFilename);
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

							case PropType_Style: {
								String value = intern(&assets->assetStrings, readToken(&reader));
								// The style-type is already specified in the UIStyle struct, so we only need
								// to overwrite the name of the style we're looking for.
								UIStyleRef *styleRef = ((UIStyleRef*)((u8*)(target) + property->offsetInStyleStruct));
								styleRef->name = value;
							} break;

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

	// First, figure out how much space to allocate
	smm styleArraySize[UIStyleTypeCount] = {};
	styleArraySize[UIStyle_Button] 		= styleCount[UIStyle_Button] 	* sizeof(UIButtonStyle);
	styleArraySize[UIStyle_Console] 	= styleCount[UIStyle_Console] 	* sizeof(UIConsoleStyle);
	styleArraySize[UIStyle_Label] 		= styleCount[UIStyle_Label] 	* sizeof(UILabelStyle);
	styleArraySize[UIStyle_Panel] 		= styleCount[UIStyle_Panel] 	* sizeof(UIPanelStyle);
	styleArraySize[UIStyle_Scrollbar] 	= styleCount[UIStyle_Scrollbar] * sizeof(UIScrollbarStyle);
	styleArraySize[UIStyle_TextInput] 	= styleCount[UIStyle_TextInput] * sizeof(UITextInputStyle);
	styleArraySize[UIStyle_Window] 		= styleCount[UIStyle_Window] 	* sizeof(UIWindowStyle);

	smm totalSize = 0;
	for (s32 i=0; i < UIStyleTypeCount; i++)
	{
		totalSize += styleArraySize[i];
	}

	asset->data = assetsAllocate(assets, totalSize);
	u8* pos = asset->data.memory;

	UITheme *theme = &asset->theme;
	// TODO: Remove this!
	theme->overlayColor = color255(0, 0, 0, 128);

	theme->buttonStyles = makeArray(styleCount[UIStyle_Button], (UIButtonStyle *) pos);
	pos += styleArraySize[UIStyle_Button];
	UIButtonStyle *nextButtonStyle = &theme->buttonStyles[0];

	theme->consoleStyles = makeArray(styleCount[UIStyle_Console], (UIConsoleStyle *) pos);
	pos += styleArraySize[UIStyle_Console];
	UIConsoleStyle *nextConsoleStyle = &theme->consoleStyles[0];

	theme->labelStyles = makeArray(styleCount[UIStyle_Label], (UILabelStyle *) pos);
	pos += styleArraySize[UIStyle_Label];
	UILabelStyle *nextLabelStyle = &theme->labelStyles[0];

	theme->panelStyles = makeArray(styleCount[UIStyle_Panel], (UIPanelStyle *) pos);
	pos += styleArraySize[UIStyle_Panel];
	UIPanelStyle *nextPanelStyle = &theme->panelStyles[0];

	theme->scrollbarStyles = makeArray(styleCount[UIStyle_Scrollbar], (UIScrollbarStyle *) pos);
	pos += styleArraySize[UIStyle_Scrollbar];
	UIScrollbarStyle *nextScrollbarStyle = &theme->scrollbarStyles[0];

	theme->textInputStyles = makeArray(styleCount[UIStyle_TextInput], (UITextInputStyle *) pos);
	pos += styleArraySize[UIStyle_TextInput];
	UITextInputStyle *nextTextInputStyle = &theme->textInputStyles[0];

	theme->windowStyles = makeArray(styleCount[UIStyle_Window], (UIWindowStyle *) pos);
	pos += styleArraySize[UIStyle_Window];
	UIWindowStyle *nextWindowStyle = &theme->windowStyles[0];

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
						UIButtonStyle *button = nextButtonStyle++;
						button->name = style->name;

						button->font = style->font;
						button->textColor = style->textColor;
						button->textAlignment = style->textAlignment;

						button->background = style->background;
						button->hoverBackground = style->hoverBackground;
						button->pressedBackground = style->pressedBackground;
						button->disabledBackground = style->disabledBackground;

						button->padding = style->padding;
					} break;

					case UIStyle_Console: {
						UIConsoleStyle *console = nextConsoleStyle++;
						console->name = style->name;

						console->font = style->font;
						copyMemory(style->outputTextColor, console->outputTextColor, CLS_COUNT);

						console->background = style->background;
						console->padding = style->padding;

						console->scrollbarStyle = style->scrollbarStyle;
						console->textInputStyle = style->textInputStyle;
					} break;

					case UIStyle_Label: {
						UILabelStyle *label = nextLabelStyle++;
						label->name = style->name;

						label->font = style->font;
						label->textColor = style->textColor;
					} break;

					case UIStyle_Panel: {
						UIPanelStyle *panel = nextPanelStyle++;
						panel->name = style->name;

						panel->margin = style->margin;
						panel->contentPadding = style->contentPadding;
						panel->widgetAlignment = style->widgetAlignment;
						panel->background = style->background;

						panel->buttonStyle = style->buttonStyle;
						panel->labelStyle = style->labelStyle;
						panel->scrollbarStyle = style->scrollbarStyle;
						panel->textInputStyle = style->textInputStyle;
					} break;

					case UIStyle_Scrollbar: {
						UIScrollbarStyle *scrollbar = nextScrollbarStyle++;
						scrollbar->name = style->name;

						scrollbar->background = style->background;
						scrollbar->knob = style->knob;
						scrollbar->width = style->width;
					} break;

					case UIStyle_TextInput: {
						UITextInputStyle *textInput = nextTextInputStyle++;
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
						UIWindowStyle *window = nextWindowStyle++;
						window->name = style->name;

						window->titleBarHeight = style->titleBarHeight;
						window->titleBarColor = style->titleBarColor;
						window->titleBarColorInactive = style->titleBarColorInactive;
						window->titleFont = style->titleFont;
						window->titleColor = style->titleColor;
						window->titleBarButtonHoverColor = style->titleBarButtonHoverColor;

						window->offsetFromMouse = style->offsetFromMouse;

						window->panelStyle = style->panelStyle;
					} break;
				}
			}
		}
	}
}

template <typename T>
bool checkStyleMatchesType(UIStyleRef *reference)
{
	switch (reference->styleType)
	{
		case UIStyle_Button: 	 return (typeid(T*) == typeid(UIButtonStyle*));
		case UIStyle_Console: 	 return (typeid(T*) == typeid(UIConsoleStyle*));
		case UIStyle_Label: 	 return (typeid(T*) == typeid(UILabelStyle*));
		case UIStyle_Panel:      return (typeid(T*) == typeid(UIPanelStyle*));
		case UIStyle_Scrollbar:  return (typeid(T*) == typeid(UIScrollbarStyle*));
		case UIStyle_TextInput:  return (typeid(T*) == typeid(UITextInputStyle*));
		case UIStyle_Window: 	 return (typeid(T*) == typeid(UIWindowStyle*));
		INVALID_DEFAULT_CASE;
	}

	return false;
}

template <typename T>
inline T* findStyle(UIStyleRef *ref)
{
	ASSERT(checkStyleMatchesType<T>(ref));

	// Order must match enum UIStyleType
	static void* (*findStyleByType[UIStyleTypeCount])(String) = {
		null,
		[] (String name) { return (void*) findStyle<UIButtonStyle>(name); },
		[] (String name) { return (void*) findStyle<UIConsoleStyle>(name); },
		[] (String name) { return (void*) findStyle<UILabelStyle>(name); },
		[] (String name) { return (void*) findStyle<UIPanelStyle>(name); },
		[] (String name) { return (void*) findStyle<UIScrollbarStyle>(name); },
		[] (String name) { return (void*) findStyle<UITextInputStyle>(name); },
		[] (String name) { return (void*) findStyle<UIWindowStyle>(name); }
	};

	if (SDL_TICKS_PASSED(assets->lastAssetReloadTicks, ref->pointerRetrievedTicks))
	{
		ref->pointer = findStyleByType[ref->styleType](ref->name);
		ref->pointerRetrievedTicks = SDL_GetTicks();
	}

	return (T*) ref->pointer;
}

template <> UIButtonStyle *findStyle<UIButtonStyle>(String styleName)
{
	return findInArrayByName(&assets->theme->buttonStyles, styleName);
}
template <> UIConsoleStyle *findStyle<UIConsoleStyle>(String styleName)
{
	return findInArrayByName(&assets->theme->consoleStyles, styleName);
}
template <> UILabelStyle *findStyle<UILabelStyle>(String styleName)
{
	return findInArrayByName(&assets->theme->labelStyles, styleName);
}
template <> UIPanelStyle *findStyle<UIPanelStyle>(String styleName)
{
	return findInArrayByName(&assets->theme->panelStyles, styleName);
}
template <> UIScrollbarStyle *findStyle<UIScrollbarStyle>(String styleName)
{
	return findInArrayByName(&assets->theme->scrollbarStyles, styleName);
}
template <> UITextInputStyle *findStyle<UITextInputStyle>(String styleName)
{
	return findInArrayByName(&assets->theme->textInputStyles, styleName);
}
template <> UIWindowStyle *findStyle<UIWindowStyle>(String styleName)
{
	return findInArrayByName(&assets->theme->windowStyles, styleName);
}
