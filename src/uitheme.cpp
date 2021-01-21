#pragma once

void initUITheme(UITheme *theme)
{
	initHashTable(&theme->fontNamesToAssetNames);

	initHashTable(&theme->buttonStyles);
	initHashTable(&theme->consoleStyles);
	initHashTable(&theme->labelStyles);
	initHashTable(&theme->messageStyles);
	initHashTable(&theme->popupMenuStyles);
	initHashTable(&theme->scrollbarStyles);
	initHashTable(&theme->textInputStyles);
	initHashTable(&theme->windowStyles);
}

#define WRONG_SECTION error(&reader, "property '{0}' in an invalid section: '{1}'"_s, {firstWord, target.name})

void loadUITheme(Blob data, Asset *asset)
{
	LineReader reader = readLines(asset->shortName, data);

	UITheme *theme = &assets->theme;
	clear(&theme->fontNamesToAssetNames);
	clear(&theme->buttonStyles);
	clear(&theme->consoleStyles);
	clear(&theme->labelStyles);
	clear(&theme->messageStyles);
	clear(&theme->popupMenuStyles);
	clear(&theme->scrollbarStyles);
	clear(&theme->textInputStyles);
	clear(&theme->windowStyles);

	// Scoped structs and enums are a thing, apparently! WOOHOO!
	enum SectionType {
		Section_None = 0,
		Section_General = 1,
		Section_Button,
		Section_Console,
		Section_Label,
		Section_UIMessage,
		Section_PopupMenu,
		Section_Scrollbar,
		Section_TextInput,
		Section_Window,
	};
	struct
	{
		SectionType type;
		String name;

		union
		{
			UIButtonStyle    *button;
			UIConsoleStyle   *console;
			UILabelStyle     *label;
			UIMessageStyle   *message;
			UIPopupMenuStyle *popupMenu;
			UIScrollbarStyle *scrollbar;
			UITextInputStyle *textInput;
			UIWindowStyle    *window;
		};
	} target = {};

	while (loadNextLine(&reader))
	{
		String firstWord = readToken(&reader);

		if (firstWord.chars[0] == ':')
		{
			// define an item
			++firstWord.chars;
			--firstWord.length;
			target.name = firstWord;

			if (equals(firstWord, "Font"_s))
			{
				target.type = Section_None;
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
				target.type = Section_General;
			}
			else if (equals(firstWord, "Button"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target.type = Section_Button;
				target.button = put(&theme->buttonStyles, name);
			}
			else if (equals(firstWord, "Console"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target.type = Section_Console;
				target.console = put(&theme->consoleStyles, name);
			}
			else if (equals(firstWord, "Label"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target.type = Section_Label;
				target.label = put(&theme->labelStyles, name);
			}
			else if (equals(firstWord, "UIMessage"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target.type = Section_UIMessage;
				target.message = put(&theme->messageStyles, name);
			}
			else if (equals(firstWord, "PopupMenu"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target.type = Section_PopupMenu;
				target.popupMenu = put(&theme->popupMenuStyles, name);
			}
			else if (equals(firstWord, "Scrollbar"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target.type = Section_Scrollbar;
				target.scrollbar = put(&theme->scrollbarStyles, name);
			}
			else if (equals(firstWord, "TextInput"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target.type = Section_TextInput;
				target.textInput = put(&theme->textInputStyles, name);
			}
			else if (equals(firstWord, "Window"_s))
			{
				String name = intern(&assets->assetStrings, readToken(&reader));
				target.type = Section_Window;
				target.window = put(&theme->windowStyles, name);
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
			if (equals(firstWord, "backgroundColor"_s))
			{
				Maybe<V4> backgroundColor = readColor(&reader);
				if (backgroundColor.isValid)
				{
					switch (target.type)
					{
						case Section_Button:    target.button->backgroundColor    = backgroundColor.value; break;
						case Section_Console:   target.console->backgroundColor   = backgroundColor.value; break;
						case Section_UIMessage: target.message->backgroundColor   = backgroundColor.value; break;
						case Section_PopupMenu: target.popupMenu->backgroundColor = backgroundColor.value; break;
						case Section_Scrollbar: target.scrollbar->backgroundColor = backgroundColor.value; break;
						case Section_TextInput: target.textInput->backgroundColor = backgroundColor.value; break;
						case Section_Window:    target.window->backgroundColor    = backgroundColor.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "backgroundColorInactive"_s))
			{
				Maybe<V4> backgroundColorInactive = readColor(&reader);
				if (backgroundColorInactive.isValid)
				{
					switch (target.type)
					{
						case Section_Window:  target.window->backgroundColorInactive = backgroundColorInactive.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "buttonStyle"_s))
			{
				String styleName = intern(&assets->assetStrings, readToken(&reader));
				switch (target.type)
				{
					case Section_PopupMenu:  target.popupMenu->buttonStyleName = styleName; break;
					case Section_Window:     target.window->buttonStyleName    = styleName; break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "caretFlashCycleDuration"_s))
			{
				Maybe<f64> caretFlashCycleDuration = readFloat(&reader);
				if (caretFlashCycleDuration.isValid)
				{
					switch (target.type)
					{
						case Section_TextInput:  target.textInput->caretFlashCycleDuration = (f32) caretFlashCycleDuration.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "contentPadding"_s))
			{
				Maybe<s32> contentPadding = readInt<s32>(&reader);
				if (contentPadding.isValid)
				{
					switch (target.type)
					{
						case Section_PopupMenu:  target.popupMenu->contentPadding = contentPadding.value; break;
						case Section_Window:     target.window->contentPadding    = contentPadding.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "disabledBackgroundColor"_s))
			{
				Maybe<V4> disabledBackgroundColor = readColor(&reader);
				if (disabledBackgroundColor.isValid)
				{
					switch (target.type)
					{
						case Section_Button:  target.button->disabledBackgroundColor = disabledBackgroundColor.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "extends"_s))
			{
				// Clones an existing style
				String parentStyleName = readToken(&reader);

				switch (target.type)
				{
					case Section_Button: {
						UIButtonStyle *parent = findButtonStyle(theme, parentStyleName);
						if (parent != null)
						{
							*(target.button) = *parent;
						}
						else
						{
							error(&reader, "Unable to find Button style named '{0}'"_s, {parentStyleName});
						}
					} break;
					case Section_Console: {
						UIConsoleStyle *parent = findConsoleStyle(theme, parentStyleName);
						if (parent != null)
						{
							*(target.console) = *parent;
						}
						else
						{
							error(&reader, "Unable to find Console style named '{0}'"_s, {parentStyleName});
						}
					} break;
					case Section_Label: {
						UILabelStyle *parent = findLabelStyle(theme, parentStyleName);
						if (parent != null)
						{
							*(target.label) = *parent;
						}
						else
						{
							error(&reader, "Unable to find Label style named '{0}'"_s, {parentStyleName});
						}
					} break;
					case Section_PopupMenu: {
						UIPopupMenuStyle *parent = findPopupMenuStyle(theme, parentStyleName);
						if (parent != null)
						{
							*(target.popupMenu) = *parent;
						}
						else
						{
							error(&reader, "Unable to find PopupMenu style named '{0}'"_s, {parentStyleName});
						}
					} break;
					case Section_Scrollbar: {
						UIScrollbarStyle *parent = findScrollbarStyle(theme, parentStyleName);
						if (parent != null)
						{
							*(target.scrollbar) = *parent;
						}
						else
						{
							error(&reader, "Unable to find Scrollbar style named '{0}'"_s, {parentStyleName});
						}
					} break;
					case Section_TextInput: {
						UITextInputStyle *parent = findTextInputStyle(theme, parentStyleName);
						if (parent != null)
						{
							*(target.textInput) = *parent;
						}
						else
						{
							error(&reader, "Unable to find TextInput style named '{0}'"_s, {parentStyleName});
						}
					} break;
					case Section_UIMessage: {
						UIMessageStyle *parent = findMessageStyle(theme, parentStyleName);
						if (parent != null)
						{
							*(target.message) = *parent;
						}
						else
						{
							error(&reader, "Unable to find UIMessage style named '{0}'"_s, {parentStyleName});
						}
					} break;
					case Section_Window: {
						UIWindowStyle *parent = findWindowStyle(theme, parentStyleName);
						if (parent != null)
						{
							*(target.window) = *parent;
						}
						else
						{
							error(&reader, "Unable to find Window style named '{0}'"_s, {parentStyleName});
						}
					} break;
				}
			}
			else if (equals(firstWord, "font"_s))
			{
				String fontName = intern(&assets->assetStrings, readToken(&reader));
				
				switch (target.type)
				{
					case Section_Button:     target.button->fontName        = fontName; break;
					case Section_Console:    target.console->fontName       = fontName; break;
					case Section_Label:      target.label->fontName         = fontName; break;
					case Section_TextInput:  target.textInput->fontName     = fontName; break;
					case Section_UIMessage:  target.message->fontName       = fontName; break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "hoverBackgroundColor"_s))
			{
				Maybe<V4> hoverBackgroundColor = readColor(&reader);
				if (hoverBackgroundColor.isValid)
				{
					switch (target.type)
					{
						case Section_Button:  target.button->hoverBackgroundColor = hoverBackgroundColor.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "knobColor"_s))
			{
				Maybe<V4> knobColor = readColor(&reader);
				if (knobColor.isValid)
				{
					switch (target.type)
					{
						case Section_Scrollbar:  target.scrollbar->knobColor = knobColor.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "labelStyle"_s))
			{
				String styleName = intern(&assets->assetStrings, readToken(&reader));
				switch (target.type)
				{
					case Section_Window:  target.window->labelStyleName = styleName; break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "margin"_s))
			{
				Maybe<s32> margin = readInt<s32>(&reader);
				if (margin.isValid)
				{
					switch (target.type)
					{
						case Section_PopupMenu:  target.popupMenu->margin = margin.value; break;
						case Section_Window:     target.window->margin    = margin.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "offsetFromMouse"_s))
			{
				Maybe<s32> offsetX = readInt<s32>(&reader);
				Maybe<s32> offsetY = readInt<s32>(&reader);
				if (offsetX.isValid && offsetY.isValid)
				{
					switch (target.type)
					{
						case Section_Window:  target.window->offsetFromMouse = v2i(offsetX.value, offsetY.value); break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "outputTextColor"_s))
			{
				if (target.type == Section_Console)
				{
					String category = readToken(&reader);
					Maybe<V4> color = readColor(&reader);

					if (color.isValid)
					{
						if (equals(category, "default"_s))
						{
							target.console->outputTextColor[CLS_Default] = color.value;
						}
						else if (equals(category, "echo"_s))
						{
							target.console->outputTextColor[CLS_InputEcho] = color.value;
						}
						else if (equals(category, "success"_s))
						{
							target.console->outputTextColor[CLS_Success] = color.value;
						}
						else if (equals(category, "warning"_s))
						{
							target.console->outputTextColor[CLS_Warning] = color.value;
						}
						else if (equals(category, "error"_s))
						{
							target.console->outputTextColor[CLS_Error] = color.value;
						}
						else
						{
							warn(&reader, "Unrecognized output text category '{0}'"_s, {category});
						}
					}
				}
				else
				{
					WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "overlayColor"_s))
			{
				if (target.type == Section_General)
				{
					Maybe<V4> overlayColor = readColor(&reader);
					if (overlayColor.isValid) theme->overlayColor = overlayColor.value;
				}
				else
				{
					WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "padding"_s))
			{
				Maybe<s32> padding = readInt<s32>(&reader);
				if (padding.isValid)
				{
					switch (target.type)
					{
						case Section_Button:     target.button->padding    = padding.value; break;
						case Section_Console:    target.console->padding   = padding.value; break;
						case Section_UIMessage:  target.message->padding   = padding.value; break;
						case Section_TextInput:  target.textInput->padding = padding.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "pressedBackgroundColor"_s))
			{
				Maybe<V4> pressedBackgroundColor = readColor(&reader);
				if (pressedBackgroundColor.isValid)
				{
					switch (target.type)
					{
						case Section_Button:  target.button->pressedBackgroundColor = pressedBackgroundColor.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "scrollbarStyle"_s))
			{
				String styleName = intern(&assets->assetStrings, readToken(&reader));
				switch (target.type)
				{
					case Section_Console:    target.console->scrollbarStyleName   = styleName; break;
					case Section_PopupMenu:  target.popupMenu->scrollbarStyleName = styleName; break;
					case Section_Window:     target.window->scrollbarStyleName    = styleName; break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "showCaret"_s))
			{
				Maybe<bool> showCaret = readBool(&reader);
				if (showCaret.isValid)
				{
					switch (target.type)
					{
						case Section_TextInput:  target.textInput->showCaret  = showCaret.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "textAlignment"_s))
			{
				Maybe<u32> alignment = readAlignment(&reader);
				if (alignment.isValid)
				{
					switch (target.type)
					{
						case Section_Button:     target.button->textAlignment    = alignment.value; break;
						case Section_TextInput:  target.textInput->textAlignment = alignment.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "textColor"_s))
			{
				Maybe<V4> textColor = readColor(&reader);
				if (textColor.isValid)
				{
					switch (target.type)
					{
						case Section_Button:     target.button->textColor       = textColor.value; break;
						case Section_Label:      target.label->textColor        = textColor.value; break;
						case Section_TextInput:  target.textInput->textColor    = textColor.value; break;
						case Section_UIMessage:  target.message->textColor      = textColor.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "textInputStyle"_s))
			{
				String styleName = intern(&assets->assetStrings, readToken(&reader));
				switch (target.type)
				{
					case Section_Console:  target.console->textInputStyleName = styleName; break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "titleBarButtonHoverColor"_s))
			{
				Maybe<V4> titleBarButtonHoverColor = readColor(&reader);
				if (titleBarButtonHoverColor.isValid)
				{
					switch (target.type)
					{
						case Section_Window:  target.window->titleBarButtonHoverColor = titleBarButtonHoverColor.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "titleBarColor"_s))
			{
				Maybe<V4> titleBarColor = readColor(&reader);
				if (titleBarColor.isValid)
				{
					switch (target.type)
					{
						case Section_Window:  target.window->titleBarColor = titleBarColor.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "titleBarColorInactive"_s))
			{
				Maybe<V4> titleBarColorInactive = readColor(&reader);
				if (titleBarColorInactive.isValid)
				{
					switch (target.type)
					{
						case Section_Window:  target.window->titleBarColorInactive = titleBarColorInactive.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "titleBarHeight"_s))
			{
				Maybe<s32> titleBarHeight = readInt<s32>(&reader);
				if (titleBarHeight.isValid)
				{
					switch (target.type)
					{
						case Section_Window:  target.window->titleBarHeight = titleBarHeight.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "titleFont"_s))
			{
				String fontName = intern(&assets->assetStrings, readToken(&reader));

				switch (target.type)
				{
					case Section_Window:  target.window->titleFontName = fontName; break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "titleColor"_s))
			{
				Maybe<V4> titleColor = readColor(&reader);
				if (titleColor.isValid)
				{
					switch (target.type)
					{
						case Section_Window:  target.window->titleColor = titleColor.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "width"_s))
			{
				Maybe<s32> width = readInt<s32>(&reader);
				if (width.isValid)
				{
					switch (target.type)
					{
						case Section_Scrollbar:  target.scrollbar->width = width.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else 
			{
				error(&reader, "Unrecognized property '{0}'"_s, {firstWord});
			}
		}
	}
}
#undef WRONG_SECTION
