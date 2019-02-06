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

void loadUITheme(AssetManager *assets, File file)
{
	LineReader reader = startFile(file, true, true, '#');
	// Scoped enums are a thing, apparently! WOOHOO!
	enum TargetType {
		Section_None = 0,
		Section_General = 1,
		Section_Button,
		Section_Label,
		Section_Tooltip,
		Section_UIMessage,
		Section_TextBox,
		Section_Window,
	};
	TargetType currentTarget = Section_None;
	String sectionName = {};

	UITheme *theme = &assets->theme;

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
			sectionName = firstWord;

			if (equals(firstWord, "Font"))
			{
				currentTarget = Section_None;
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
			else if (equals(firstWord, "General"))    currentTarget = Section_General;
			else if (equals(firstWord, "Button"))     currentTarget = Section_Button;
			else if (equals(firstWord, "Label"))      currentTarget = Section_Label;
			else if (equals(firstWord, "Tooltip"))    currentTarget = Section_Tooltip;
			else if (equals(firstWord, "UIMessage"))  currentTarget = Section_UIMessage;
			else if (equals(firstWord, "TextBox"))    currentTarget = Section_TextBox;
			else if (equals(firstWord, "Window"))     currentTarget = Section_Window;
			else                                      error(&reader, "Unrecognized command: '{0}'", {firstWord});
		}
		else
		{
			// properties of the item
			if (equals(firstWord, "overlayColor"))
			{
				if (currentTarget == Section_General)
				{
					theme->overlayColor = readColor255(remainder);
				}
				else
				{
					error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "textColor"))
			{
				switch (currentTarget)
				{
					case Section_Button:     theme->buttonStyle.textColor    = readColor255(remainder); break;
					case Section_Label:      theme->labelStyle.textColor     = readColor255(remainder); break;
					case Section_TextBox:    theme->textBoxStyle.textColor   = readColor255(remainder); break;
					case Section_UIMessage:  theme->uiMessageStyle.textColor = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "backgroundColor"))
			{
				switch (currentTarget)
				{
					case Section_Button:    theme->buttonStyle.backgroundColor    = readColor255(remainder); break;
					case Section_TextBox:   theme->textBoxStyle.backgroundColor   = readColor255(remainder); break;
					case Section_Tooltip:   theme->tooltipStyle.backgroundColor   = readColor255(remainder); break;
					case Section_UIMessage: theme->uiMessageStyle.backgroundColor = readColor255(remainder); break;
					case Section_Window:    theme->windowStyle.backgroundColor    = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "hoverColor"))
			{
				switch (currentTarget)
				{
					case Section_Button:  theme->buttonStyle.hoverColor = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "pressedColor"))
			{
				switch (currentTarget)
				{
					case Section_Button:  theme->buttonStyle.pressedColor = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "textColorNormal"))
			{
				switch (currentTarget)
				{
					case Section_Tooltip:  theme->tooltipStyle.textColorNormal = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "textColorBad"))
			{
				switch (currentTarget)
				{
					case Section_Tooltip:  theme->tooltipStyle.textColorBad = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "depth"))
			{
				s64 intValue;
				if (!asInt(remainder, &intValue)) error(&reader, "Could not parse {0} as an integer.", {remainder});

				switch (currentTarget)
				{
					case Section_Tooltip:    theme->tooltipStyle.depth   = (f32) intValue; break;
					case Section_UIMessage:  theme->uiMessageStyle.depth = (f32) intValue; break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "padding"))
			{
				s64 intValue;
				if (!asInt(remainder, &intValue)) error(&reader, "Could not parse {0} as an integer.", {remainder});

				switch (currentTarget)
				{
					case Section_Button:  theme->buttonStyle.padding = (f32) intValue; break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "borderPadding"))
			{
				s64 intValue;
				if (!asInt(remainder, &intValue)) error(&reader, "Could not parse {0} as an integer.", {remainder});

				switch (currentTarget)
				{
					case Section_Tooltip:    theme->tooltipStyle.borderPadding   = (f32) intValue; break;
					case Section_UIMessage:  theme->uiMessageStyle.borderPadding = (f32) intValue; break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "font"))
			{
				String fontName = nextToken(remainder, null);
				s32 fontID = findFontByName(&reader, assets, fontName);

				switch (currentTarget)
				{
					case Section_Button:     theme->buttonStyle.fontID        = fontID; break;
					case Section_Label:      theme->labelStyle.fontID         = fontID; break;
					case Section_Tooltip:    theme->tooltipStyle.fontID       = fontID; break;
					case Section_TextBox:    theme->textBoxStyle.fontID       = fontID; break;
					case Section_UIMessage:  theme->uiMessageStyle.fontID     = fontID; break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "titleFont"))
			{
				String fontName = nextToken(remainder, null);
				s32 fontID = findFontByName(&reader, assets, fontName);

				switch (currentTarget)
				{
					case Section_Window:  theme->windowStyle.titleFontID = fontID; break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "titleColor"))
			{
				switch (currentTarget)
				{
					case Section_Window:  theme->windowStyle.titleColor = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "titleBarHeight"))
			{
				s64 intValue;
				if (!asInt(remainder, &intValue)) error(&reader, "Could not parse {0} as an integer.", {remainder});

				switch (currentTarget)
				{
					case Section_Window:  theme->windowStyle.titleBarHeight = (f32) intValue; break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "titleBarColor"))
			{
				switch (currentTarget)
				{
					case Section_Window:  theme->windowStyle.titleBarColor = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "titleBarColorInactive"))
			{
				switch (currentTarget)
				{
					case Section_Window:  theme->windowStyle.titleBarColorInactive = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "titleBarButtonHoverColor"))
			{
				switch (currentTarget)
				{
					case Section_Window:  theme->windowStyle.titleBarButtonHoverColor = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "backgroundColorInactive"))
			{
				switch (currentTarget)
				{
					case Section_Window:  theme->windowStyle.backgroundColorInactive = readColor255(remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "contentPadding"))
			{
				s64 intValue;
				if (!asInt(remainder, &intValue)) error(&reader, "Could not parse {0} as an integer.", {remainder});

				switch (currentTarget)
				{
					case Section_Window:  theme->windowStyle.contentPadding = (f32) intValue; break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "contentTextStyle"))
			{
				// TODO: Replace this with a by-name lookup!
				switch (currentTarget)
				{
					case Section_Window:  theme->windowStyle.labelStyle = &theme->labelStyle; break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else if (equals(firstWord, "contentButtonStyle"))
			{
				// TODO: Replace this with a by-name lookup!
				switch (currentTarget)
				{
					case Section_Window:  theme->windowStyle.buttonStyle = &theme->buttonStyle; break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, sectionName});
				}
			}
			else 
			{
				error(&reader, "Unrecognized property '{0}'", {firstWord});
			}
		}
	}
}
