#pragma once

void initUITheme(UITheme *theme)
{
	initHashTable(&theme->fontNamesToAssetNames);

	initHashTable(&theme->buttonStyles);
	initHashTable(&theme->consoleStyles);
	initHashTable(&theme->labelStyles);
	initHashTable(&theme->messageStyles);
	initHashTable(&theme->textBoxStyles);
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
	clear(&theme->textBoxStyles);
	clear(&theme->windowStyles);

	// Scoped structs and enums are a thing, apparently! WOOHOO!
	enum SectionType {
		Section_None = 0,
		Section_General = 1,
		Section_Button,
		Section_Console,
		Section_Label,
		Section_UIMessage,
		Section_TextBox,
		Section_Window,
	};
	struct
	{
		SectionType type;
		String name;

		union
		{
			UIButtonStyle  *button;
			UIConsoleStyle *console;
			UILabelStyle   *label;
			UIMessageStyle *message;
			UITextBoxStyle *textBox;
			UIWindowStyle  *window;
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
					addFont(pushString(&assets->assetArena, fontName), pushString(&assets->assetArena, fontFilename));
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
				String name = pushString(&assets->assetArena, readToken(&reader));
				target.type = Section_Button;
				target.button = put(&theme->buttonStyles, name);
			}
			else if (equals(firstWord, "Console"_s))
			{
				String name = pushString(&assets->assetArena, readToken(&reader));
				target.type = Section_Console;
				target.console = put(&theme->consoleStyles, name);
			}
			else if (equals(firstWord, "Label"_s))
			{
				String name = pushString(&assets->assetArena, readToken(&reader));
				target.type = Section_Label;
				target.label = put(&theme->labelStyles, name);
			}
			else if (equals(firstWord, "UIMessage"_s))
			{
				String name = pushString(&assets->assetArena, readToken(&reader));
				target.type = Section_UIMessage;
				target.message = put(&theme->messageStyles, name);
			}
			else if (equals(firstWord, "TextBox"_s))
			{
				String name = pushString(&assets->assetArena, readToken(&reader));
				target.type = Section_TextBox;
				target.textBox = put(&theme->textBoxStyles, name);
			}
			else if (equals(firstWord, "Window"_s))
			{
				String name = pushString(&assets->assetArena, readToken(&reader));
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
			// properties of the item
			if (equals(firstWord, "backgroundColor"_s))
			{
				Maybe<V4> backgroundColor = readColor(&reader);
				if (backgroundColor.isValid)
				{
					switch (target.type)
					{
						case Section_Button:    target.button->backgroundColor    = backgroundColor.value; break;
						case Section_Console:   target.console->backgroundColor   = backgroundColor.value; break;
						case Section_TextBox:   target.textBox->backgroundColor   = backgroundColor.value; break;
						case Section_UIMessage: target.message->backgroundColor   = backgroundColor.value; break;
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
			else if (equals(firstWord, "contentButtonStyle"_s))
			{
				String styleName = pushString(&assets->assetArena, readToken(&reader));
				switch (target.type)
				{
					case Section_Window:  target.window->buttonStyleName = styleName; break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "contentLabelStyle"_s))
			{
				String styleName = pushString(&assets->assetArena, readToken(&reader));
				switch (target.type)
				{
					case Section_Window:  target.window->labelStyleName = styleName; break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "contentPadding"_s))
			{
				Maybe<s64> contentPadding = readInt(&reader);
				if (contentPadding.isValid)
				{
					switch (target.type)
					{
						case Section_Window:  target.window->contentPadding = (s32) contentPadding.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "font"_s))
			{
				String fontName = pushString(&assets->assetArena, readToken(&reader));
				
				switch (target.type)
				{
					case Section_Button:     target.button->fontName        = fontName; break;
					case Section_Console:    target.console->fontName       = fontName; break;
					case Section_Label:      target.label->fontName         = fontName; break;
					case Section_TextBox:    target.textBox->fontName       = fontName; break;
					case Section_UIMessage:  target.message->fontName       = fontName; break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "hoverColor"_s))
			{
				Maybe<V4> hoverColor = readColor(&reader);
				if (hoverColor.isValid)
				{
					switch (target.type)
					{
						case Section_Button:  target.button->hoverColor = hoverColor.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "inputBackgroundColor"_s))
			{
				Maybe<V4> inputBackgroundColor = readColor(&reader);
				if (inputBackgroundColor.isValid)
				{
					switch (target.type)
					{
						case Section_Console:  target.console->inputBackgroundColor = inputBackgroundColor.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "offsetFromMouse"_s))
			{
				Maybe<s64> offsetX = readInt(&reader);
				Maybe<s64> offsetY = readInt(&reader);
				if (offsetX.isValid && offsetY.isValid)
				{
					switch (target.type)
					{
						case Section_Window:  target.window->offsetFromMouse = v2i(truncate32(offsetX.value), truncate32(offsetY.value)); break;
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
				Maybe<s64> padding = readInt(&reader);
				if (padding.isValid)
				{
					switch (target.type)
					{
						case Section_Button:     target.button->padding    = (s32) padding.value; break;
						case Section_Console:    target.console->padding   = (s32) padding.value; break;
						case Section_UIMessage:  target.message->padding   = (s32) padding.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "pressedColor"_s))
			{
				Maybe<V4> pressedColor = readColor(&reader);
				if (pressedColor.isValid)
				{
					switch (target.type)
					{
						case Section_Button:  target.button->pressedColor = pressedColor.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "scrollBarKnobColor"_s))
			{
				Maybe<V4> scrollBarKnobColor = readColor(&reader);
				if (scrollBarKnobColor.isValid)
				{
					switch (target.type)
					{
						case Section_Console:  target.console->scrollBarKnobColor = scrollBarKnobColor.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "scrollBarWidth"_s))
			{
				Maybe<s64> scrollBarWidth = readInt(&reader);
				if (scrollBarWidth.isValid)
				{
					switch (target.type)
					{
						case Section_Console:    target.console->scrollBarWidth  = (s32) scrollBarWidth.value; break;
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
						case Section_Button:  target.button->textAlignment = alignment.value; break;
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
						case Section_Console:    target.console->inputTextColor = textColor.value; break;
						case Section_Label:      target.label->textColor        = textColor.value; break;
						case Section_TextBox:    target.textBox->textColor      = textColor.value; break;
						case Section_UIMessage:  target.message->textColor      = textColor.value; break;
						default:  WRONG_SECTION;
					}
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
				Maybe<s64> titleBarHeight = readInt(&reader);
				if (titleBarHeight.isValid)
				{
					switch (target.type)
					{
						case Section_Window:  target.window->titleBarHeight = (s32) titleBarHeight.value; break;
						default:  WRONG_SECTION;
					}
				}
			}
			else if (equals(firstWord, "titleFont"_s))
			{
				String fontName = pushString(&assets->assetArena, readToken(&reader));

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
			else 
			{
				error(&reader, "Unrecognized property '{0}'"_s, {firstWord});
			}
		}
	}
}
#undef WRONG_SECTION
