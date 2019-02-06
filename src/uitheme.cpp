#pragma once

V4 readColor255(String input)
{
	s64 r = 0, g = 0, b = 0, a = 255;
	String token, rest;
	bool succeeded;

	token = nextToken(input, &rest);
	succeeded = asInt(token, &r);

	if (succeeded)
	{
		token = nextToken(rest, &rest);
		succeeded = asInt(token, &g);
	}

	if (succeeded)
	{
		token = nextToken(rest, &rest);
		succeeded = asInt(token, &b);
	}

	if (succeeded)
	{
		token = nextToken(rest, &rest);
	}

	if (token.length)
	{
		succeeded = asInt(token, &a);
	}

	return color255((u8)r, (u8)g, (u8)b, (u8)a);
}

s32 findFontByName(LineReader *reader, AssetManager *assets, String fontName)
{
	s32 result = -1;
	
	s32 i = 0;
	for (auto it = iterate(&assets->fonts); !it.isDone; next(&it), i++)
	{
		auto font = get(it);
		if (equals(font->name, fontName))
		{
			result = i;
			break;
		}
	}

	if (result == -1)
	{
		error(reader, "Unrecognized font name '{0}'", {fontName});
	}

	return result;
}

UILabelStyle *findLabelStyle(AssetManager *assets, String name)
{
	UILabelStyle *result = null;

	for (auto it = iterate(&assets->labelStyles); !it.isDone; next(&it))
	{
		auto style = get(it);
		if (equals(style->name, name))
		{
			result = style;
			break;
		}
	}

	return result;
}

void loadUITheme(AssetManager *assets, File file)
{
	LineReader reader = startFile(file, true, true, '#');

	UITheme *theme = &assets->theme;

	// Scoped structs and enums are a thing, apparently! WOOHOO!
	enum SectionType {
		Section_None = 0,
		Section_General = 1,
		Section_Button,
		Section_Label,
		Section_Tooltip,
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
			UIButtonStyle *button;
			UILabelStyle *label;
			UITooltipStyle *tooltip;
			UIMessageStyle *message;
			UITextBoxStyle *textBox;
			UIWindowStyle *window;
		};
	} target = {};

	while (reader.pos < reader.file.length)
	{
		String line = nextLine(&reader);

		String firstWord, remainder;
		firstWord = nextToken(line, &remainder);

		if (firstWord.chars[0] == ':')
		{
			// define an item
			++firstWord.chars;
			--firstWord.length;
			target.name = firstWord;

			if (equals(firstWord, "Font"))
			{
				target.type = Section_None;
				String fontName, fontFilename;
				fontName = nextToken(remainder, &fontFilename);
				fontFilename = trimStart(fontFilename);

				if (fontName.length && fontFilename.length)
				{
					addBMFont(assets, fontName, fontFilename);
				}
				else
				{
					error(&reader, "Invalid font declaration: '{0}'", {line});
				}
			}
			else if (equals(firstWord, "General"))
			{
				target.type = Section_General;
			}
			else if (equals(firstWord, "Button"))
			{
				target.type = Section_Button;
				target.button = &theme->buttonStyle;
			}
			else if (equals(firstWord, "Label"))
			{
				String name = nextToken(remainder, &remainder);
				target.type = Section_Label;
				target.label = appendBlank(&assets->labelStyles);
				target.label->name = pushString(&assets->assetArena, name);
			}
			else if (equals(firstWord, "Tooltip"))
			{
				target.type = Section_Tooltip;
				target.tooltip = &theme->tooltipStyle;
			}
			else if (equals(firstWord, "UIMessage"))
			{
				target.type = Section_UIMessage;
				target.message = &theme->uiMessageStyle;
			}
			else if (equals(firstWord, "TextBox"))
			{
				target.type = Section_TextBox;
				target.textBox = &theme->textBoxStyle;
			}
			else if (equals(firstWord, "Window"))
			{
				target.type = Section_Window;
				target.window = &theme->windowStyle;
			}
			else
			{
				error(&reader, "Unrecognized command: '{0}'", {firstWord});
			}
		}
		else
		{
			// properties of the item
			if (equals(firstWord, "overlayColor"))
			{
				if (target.type == Section_General)
				{
					theme->overlayColor = readColor255(remainder);
				}
				else
				{
					error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "textColor"))
			{
				switch (target.type)
				{
					case Section_Button:     target.button->textColor    = readColor255(remainder); break;
					case Section_Label:      target.label->textColor     = readColor255(remainder); break;
					case Section_TextBox:    target.textBox->textColor   = readColor255(remainder); break;
					case Section_UIMessage:  target.message->textColor   = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "backgroundColor"))
			{
				switch (target.type)
				{
					case Section_Button:    target.button->backgroundColor    = readColor255(remainder); break;
					case Section_TextBox:   target.textBox->backgroundColor   = readColor255(remainder); break;
					case Section_Tooltip:   target.tooltip->backgroundColor   = readColor255(remainder); break;
					case Section_UIMessage: target.message->backgroundColor   = readColor255(remainder); break;
					case Section_Window:    target.window->backgroundColor    = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "hoverColor"))
			{
				switch (target.type)
				{
					case Section_Button:  target.button->hoverColor = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "pressedColor"))
			{
				switch (target.type)
				{
					case Section_Button:  target.button->pressedColor = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "textColorNormal"))
			{
				switch (target.type)
				{
					case Section_Tooltip:  target.tooltip->textColorNormal = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "textColorBad"))
			{
				switch (target.type)
				{
					case Section_Tooltip:  target.tooltip->textColorBad = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "depth"))
			{
				s64 intValue;
				if (!asInt(remainder, &intValue)) error(&reader, "Could not parse {0} as an integer.", {remainder});

				switch (target.type)
				{
					case Section_Tooltip:    target.tooltip->depth   = (f32) intValue; break;
					case Section_UIMessage:  target.message->depth   = (f32) intValue; break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "padding"))
			{
				s64 intValue;
				if (!asInt(remainder, &intValue)) error(&reader, "Could not parse {0} as an integer.", {remainder});

				switch (target.type)
				{
					case Section_Button:  target.button->padding = (f32) intValue; break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "borderPadding"))
			{
				s64 intValue;
				if (!asInt(remainder, &intValue)) error(&reader, "Could not parse {0} as an integer.", {remainder});

				switch (target.type)
				{
					case Section_Tooltip:    target.tooltip->borderPadding   = (f32) intValue; break;
					case Section_UIMessage:  target.message->borderPadding   = (f32) intValue; break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "font"))
			{
				String fontName = nextToken(remainder, null);
				s32 fontID = findFontByName(&reader, assets, fontName);

				switch (target.type)
				{
					case Section_Button:     target.button->fontID        = fontID; break;
					case Section_Label:      target.label->fontID         = fontID; break;
					case Section_Tooltip:    target.tooltip->fontID       = fontID; break;
					case Section_TextBox:    target.textBox->fontID       = fontID; break;
					case Section_UIMessage:  target.message->fontID       = fontID; break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "titleFont"))
			{
				String fontName = nextToken(remainder, null);
				s32 fontID = findFontByName(&reader, assets, fontName);

				switch (target.type)
				{
					case Section_Window:  target.window->titleFontID = fontID; break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "titleColor"))
			{
				switch (target.type)
				{
					case Section_Window:  target.window->titleColor = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "titleBarHeight"))
			{
				s64 intValue;
				if (!asInt(remainder, &intValue)) error(&reader, "Could not parse {0} as an integer.", {remainder});

				switch (target.type)
				{
					case Section_Window:  target.window->titleBarHeight = (f32) intValue; break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "titleBarColor"))
			{
				switch (target.type)
				{
					case Section_Window:  target.window->titleBarColor = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "titleBarColorInactive"))
			{
				switch (target.type)
				{
					case Section_Window:  target.window->titleBarColorInactive = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "titleBarButtonHoverColor"))
			{
				switch (target.type)
				{
					case Section_Window:  target.window->titleBarButtonHoverColor = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "backgroundColorInactive"))
			{
				switch (target.type)
				{
					case Section_Window:  target.window->backgroundColorInactive = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "contentPadding"))
			{
				s64 intValue;
				if (!asInt(remainder, &intValue)) error(&reader, "Could not parse {0} as an integer.", {remainder});

				switch (target.type)
				{
					case Section_Window:  target.window->contentPadding = (f32) intValue; break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "contentLabelStyle"))
			{
				// TODO: Replace this with a by-name lookup!
				String styleName = nextToken(remainder, &remainder);
				switch (target.type)
				{
					case Section_Window:  target.window->labelStyle = findLabelStyle(assets, styleName); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "contentButtonStyle"))
			{
				// TODO: Replace this with a by-name lookup!
				switch (target.type)
				{
					case Section_Window:  target.window->buttonStyle = &theme->buttonStyle; break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else 
			{
				error(&reader, "Unrecognized property '{0}'", {firstWord});
			}
		}
	}
}
