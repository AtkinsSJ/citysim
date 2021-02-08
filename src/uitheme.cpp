#pragma once

void initUITheme(UITheme *theme)
{
	initHashTable(&theme->fontNamesToAssetNames);

	initHashTable(&theme->buttonStyles);
	initHashTable(&theme->consoleStyles);
	initHashTable(&theme->labelStyles);
	initHashTable(&theme->panelStyles);
	initHashTable(&theme->scrollbarStyles);
	initHashTable(&theme->textInputStyles);
	initHashTable(&theme->windowStyles);

	initHashTable(&theme->styleProperties, 0.75f, 256);
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
		put(&theme->styleProperties, makeString(#name), property); \
	}

	//                                                   btn   cnsl  label  panel  scrll  txtin  windw
	PROP(backgroundColor,          PropType_Color,      true,  true, false,  true,  true,  true,  true);
	PROP(backgroundColorInactive,  PropType_Color,     false, false, false, false, false, false,  true);
	PROP(buttonStyle,              PropType_Style,     false, false, false,  true, false, false,  true);
	PROP(caretFlashCycleDuration,  PropType_Float,     false, false, false, false, false,  true, false);
	PROP(widgetAlignment,          PropType_Alignment, false, false, false,  true, false, false, false);
	PROP(contentPadding,           PropType_Int,       false, false, false,  true, false, false,  true);
	PROP(disabledBackgroundColor,  PropType_Color,      true, false, false, false, false, false, false);
	PROP(font,                     PropType_Font,       true,  true,  true, false, false,  true, false);
	PROP(hoverBackgroundColor,     PropType_Color,      true, false, false, false, false, false, false);
	PROP(knobColor,                PropType_Color,     false, false, false, false,  true, false, false);
	PROP(labelStyle,               PropType_Style,     false, false, false,  true, false, false,  true);
	PROP(margin,                   PropType_Int,       false, false, false,  true, false, false,  true);
	PROP(offsetFromMouse,          PropType_V2I,       false, false, false, false, false, false,  true);
	PROP(padding,                  PropType_Int,        true,  true, false, false, false,  true, false);
	PROP(pressedBackgroundColor,   PropType_Color,      true, false, false, false, false, false, false);
	PROP(scrollbarStyle,           PropType_Style,     false,  true, false,  true, false, false,  true);
	PROP(showCaret,                PropType_Bool,      false, false, false, false, false,  true, false);
	PROP(textAlignment,            PropType_Alignment,  true, false, false, false, false,  true, false);
	PROP(textColor,                PropType_Color,      true, false,  true, false, false,  true, false);
	PROP(textInputStyle,           PropType_Style,     false,  true, false,  true, false, false, false);
	PROP(titleBarButtonHoverColor, PropType_Color,     false, false, false, false, false, false,  true);
	PROP(titleBarColor,            PropType_Color,     false, false, false, false, false, false,  true);
	PROP(titleBarColorInactive,    PropType_Color,     false, false, false, false, false, false,  true);
	PROP(titleBarHeight,           PropType_Int,       false, false, false, false, false, false,  true);
	PROP(titleColor,               PropType_Color,     false, false, false, false, false, false,  true);
	PROP(titleFont,                PropType_Font,      false, false, false, false, false, false,  true);
	PROP(width,                    PropType_Int,       false, false, false, false,  true, false, false);

	#undef PROP

	// TODO: Remove this!
	theme->overlayColor = color255(0, 0, 0, 128);
}

void saveStyleToTheme(UITheme *theme, UIStyle *style)
{
	switch (style->type)
	{
		case UIStyle_Button: {
			UIButtonStyle *button = put(&theme->buttonStyles, style->name, UIButtonStyle());

			button->font = style->font;
			button->textColor = style->textColor;
			button->textAlignment = style->textAlignment;

			button->backgroundColor = style->backgroundColor;
			button->hoverBackgroundColor = style->hoverBackgroundColor;
			button->pressedBackgroundColor = style->pressedBackgroundColor;
			button->disabledBackgroundColor = style->disabledBackgroundColor;

			button->padding = style->padding;
		} break;

		case UIStyle_Console: {
			UIConsoleStyle *console = put(&theme->consoleStyles, style->name, UIConsoleStyle());

			console->font = style->font;
			copyMemory(style->outputTextColor, console->outputTextColor, CLS_COUNT);

			console->backgroundColor = style->backgroundColor;
			console->padding = style->padding;

			console->scrollbarStyle = style->scrollbarStyle;
			console->textInputStyle = style->textInputStyle;
		} break;

		case UIStyle_Label: {
			UILabelStyle *label = put(&theme->labelStyles, style->name, UILabelStyle());

			label->font = style->font;
			label->textColor = style->textColor;
		} break;

		case UIStyle_Panel: {
			UIPanelStyle *panel = put(&theme->panelStyles, style->name, UIPanelStyle());

			panel->margin = style->margin;
			panel->contentPadding = style->contentPadding;
			panel->widgetAlignment = style->widgetAlignment;
			panel->backgroundColor = style->backgroundColor;

			panel->buttonStyle = style->buttonStyle;
			panel->labelStyle = style->labelStyle;
			panel->scrollbarStyle = style->scrollbarStyle;
			panel->textInputStyle = style->textInputStyle;
		} break;

		case UIStyle_Scrollbar: {
			UIScrollbarStyle *scrollbar = put(&theme->scrollbarStyles, style->name, UIScrollbarStyle());

			scrollbar->backgroundColor = style->backgroundColor;
			scrollbar->knobColor = style->knobColor;
			scrollbar->width = style->width;
		} break;

		case UIStyle_TextInput: {
			UITextInputStyle *textInput = put(&theme->textInputStyles, style->name, UITextInputStyle());

			textInput->font = style->font;
			textInput->textColor = style->textColor;
			textInput->textAlignment = style->textAlignment;

			textInput->backgroundColor = style->backgroundColor;
			textInput->padding = style->padding;
	
			textInput->showCaret = style->showCaret;
			textInput->caretFlashCycleDuration = style->caretFlashCycleDuration;
		} break;

		case UIStyle_Window: {
			UIWindowStyle *window = put(&theme->windowStyles, style->name, UIWindowStyle());

			window->titleBarHeight = style->titleBarHeight;
			window->titleBarColor = style->titleBarColor;
			window->titleBarColorInactive = style->titleBarColorInactive;
			window->titleFont = style->titleFont;
			window->titleColor = style->titleColor;
			window->titleBarButtonHoverColor = style->titleBarButtonHoverColor;

			window->backgroundColor = style->backgroundColor;
			window->backgroundColorInactive = style->backgroundColorInactive;

			window->margin = style->margin;
			window->contentPadding = style->contentPadding;

			window->offsetFromMouse = style->offsetFromMouse;

			window->buttonStyle = style->buttonStyle;
			window->labelStyle = style->labelStyle;
			window->scrollbarStyle = style->scrollbarStyle;
		} break;
	}
}

UIStyle *addStyle(HashTable<UIStylePack> *styles, String name, UIStyleType type)
{
	UIStylePack *pack = findOrAdd(styles, name);
	UIStyle *result = pack->styleByType + type;
	*result = UIStyle();

	result->name = name;
	result->type = type;

	return result;
}

void loadUITheme(Blob data, Asset *asset)
{
	LineReader reader = readLines(asset->shortName, data);

	UITheme *theme = &assets->theme;
	clear(&theme->fontNamesToAssetNames);
	clear(&theme->buttonStyles);
	clear(&theme->consoleStyles);
	clear(&theme->labelStyles);
	clear(&theme->panelStyles);
	clear(&theme->scrollbarStyles);
	clear(&theme->textInputStyles);
	clear(&theme->windowStyles);

	HashTable<UIStylePack> styles;
	initHashTable(&styles);

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
					addFont(fontName, fontFilename);
				}
				else
				{
					error(&reader, "Invalid font declaration: '{0}'"_s, {getLine(&reader)});
				}
			}
			else if (equals(firstWord, "General"_s))
			{
				target = null;
				// target->type = UIStyle_General;
			}
			else if (equals(firstWord, "Button"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target = addStyle(&styles, name, UIStyle_Button);
				target->name = name;
				target->type = UIStyle_Button;
			}
			else if (equals(firstWord, "Console"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target = addStyle(&styles, name, UIStyle_Console);
				target->name = name;
				target->type = UIStyle_Console;
			}
			else if (equals(firstWord, "Label"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target = addStyle(&styles, name, UIStyle_Label);
				target->name = name;
				target->type = UIStyle_Label;
			}
			else if (equals(firstWord, "Panel"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target = addStyle(&styles, name, UIStyle_Panel);
				target->name = name;
				target->type = UIStyle_Panel;
			}
			else if (equals(firstWord, "Scrollbar"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target = addStyle(&styles, name, UIStyle_Scrollbar);
				target->name = name;
				target->type = UIStyle_Scrollbar;
			}
			else if (equals(firstWord, "TextInput"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target = addStyle(&styles, name, UIStyle_TextInput);
				target->name = name;
				target->type = UIStyle_TextInput;
			}
			else if (equals(firstWord, "Window"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target = addStyle(&styles, name, UIStyle_Window);
				target->name = name;
				target->type = UIStyle_Window;
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
				UIStylePack *parentPack = find(&styles, parentStyleName);
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
				UIProperty *property = find(&theme->styleProperties, firstWord);
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

							case PropType_Float: {
								Maybe<f64> value = readFloat(&reader);
								if (value.isValid)
								{
									*((f32*)((u8*)(target) + property->offsetInStyleStruct)) = (f32)(value.value);
								}
							} break;

							case PropType_Font: {
								String value = intern(&assets->assetStrings, readToken(&reader));
								FontReference *fontRef = ((FontReference*)((u8*)(target) + property->offsetInStyleStruct));
								*fontRef = {};
								fontRef->fontName = value;
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
								UIStyleReference *styleRef = ((UIStyleReference*)((u8*)(target) + property->offsetInStyleStruct));
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
	for (auto it = iterate(&styles); hasNext(&it); next(&it))
	{
		UIStylePack *stylePack = get(&it);
		for (s32 sectionType = 1; sectionType < UIStyleTypeCount; sectionType++)
		{
			UIStyle *style = stylePack->styleByType + sectionType;
			// For undefined styles, the parent struct will be all nulls, so the type will not match
			if (style->type == sectionType)
			{
				saveStyleToTheme(theme, style);
			}
		}
	}

	freeHashTable(&styles);
}

template <typename T>
bool checkStyleMatchesType(UIStyleReference *reference)
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
inline T* findStyle(UITheme *theme, UIStyleReference *reference)
{
	ASSERT(checkStyleMatchesType<T>(reference));

	// Order must match enum UIStyleType
	static void* (*findStyleByType[UIStyleTypeCount])(UITheme*, String) = {
		null,
		[] (UITheme *theme, String name) { return (void*) findButtonStyle(theme, name); },
		[] (UITheme *theme, String name) { return (void*) findConsoleStyle(theme, name); },
		[] (UITheme *theme, String name) { return (void*) findLabelStyle(theme, name); },
		[] (UITheme *theme, String name) { return (void*) findPanelStyle(theme, name); },
		[] (UITheme *theme, String name) { return (void*) findScrollbarStyle(theme, name); },
		[] (UITheme *theme, String name) { return (void*) findTextInputStyle(theme, name); },
		[] (UITheme *theme, String name) { return (void*) findWindowStyle(theme, name); }
	};

	if (reference->pointer == null)
	{
		reference->pointer = findStyleByType[reference->styleType](theme, reference->name);
	}

	return (T*) reference->pointer;
}
