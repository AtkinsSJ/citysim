#pragma once

static s32 findFontByName(LineReader *reader, AssetManager *assets, String fontName)
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

template<typename Style>
Style *findStyle(ChunkedArray<Style> *stylesArray, String name)
{
	Style *result = null;

	for (auto it = iterate(stylesArray); !it.isDone; next(&it))
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

inline
UIButtonStyle *findButtonStyle(AssetManager *assets, String name)
{
	return findStyle(&assets->buttonStyles, name);
}

inline
UILabelStyle *findLabelStyle(AssetManager *assets, String name)
{
	return findStyle(&assets->labelStyles, name);
}

inline
UITooltipStyle *findTooltipStyle(AssetManager *assets, String name)
{
	return findStyle(&assets->tooltipStyles, name);
}

inline
UIMessageStyle *findMessageStyle(AssetManager *assets, String name)
{
	return findStyle(&assets->messageStyles, name);
}

inline
UITextBoxStyle *findTextBoxStyle(AssetManager *assets, String name)
{
	return findStyle(&assets->textBoxStyles, name);
}

inline
UIWindowStyle *findWindowStyle(AssetManager *assets, String name)
{
	return findStyle(&assets->windowStyles, name);
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
				String name = nextToken(remainder, &remainder);
				target.type = Section_Button;
				target.button = appendBlank(&assets->buttonStyles);
				target.button->name = pushString(&assets->assetArena, name);
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
				String name = nextToken(remainder, &remainder);
				target.type = Section_Tooltip;
				target.tooltip = appendBlank(&assets->tooltipStyles);
				target.tooltip->name = pushString(&assets->assetArena, name);
			}
			else if (equals(firstWord, "UIMessage"))
			{
				String name = nextToken(remainder, &remainder);
				target.type = Section_UIMessage;
				target.message = appendBlank(&assets->messageStyles);
				target.message->name = pushString(&assets->assetArena, name);
			}
			else if (equals(firstWord, "TextBox"))
			{
				String name = nextToken(remainder, &remainder);
				target.type = Section_TextBox;
				target.textBox = appendBlank(&assets->textBoxStyles);
				target.textBox->name = pushString(&assets->assetArena, name);
			}
			else if (equals(firstWord, "Window"))
			{
				String name = nextToken(remainder, &remainder);
				target.type = Section_Window;
				target.window = appendBlank(&assets->windowStyles);
				target.window->name = pushString(&assets->assetArena, name);
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
					theme->overlayColor = readColor255(&reader, firstWord, remainder);
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
					case Section_Button:     target.button->textColor    = readColor255(&reader, firstWord, remainder); break;
					case Section_Label:      target.label->textColor     = readColor255(&reader, firstWord, remainder); break;
					case Section_TextBox:    target.textBox->textColor   = readColor255(&reader, firstWord, remainder); break;
					case Section_Tooltip:    target.tooltip->textColor   = readColor255(&reader, firstWord, remainder); break;
					case Section_UIMessage:  target.message->textColor   = readColor255(&reader, firstWord, remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "backgroundColor"))
			{
				switch (target.type)
				{
					case Section_Button:    target.button->backgroundColor    = readColor255(&reader, firstWord, remainder); break;
					case Section_TextBox:   target.textBox->backgroundColor   = readColor255(&reader, firstWord, remainder); break;
					case Section_Tooltip:   target.tooltip->backgroundColor   = readColor255(&reader, firstWord, remainder); break;
					case Section_UIMessage: target.message->backgroundColor   = readColor255(&reader, firstWord, remainder); break;
					case Section_Window:    target.window->backgroundColor    = readColor255(&reader, firstWord, remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "hoverColor"))
			{
				switch (target.type)
				{
					case Section_Button:  target.button->hoverColor = readColor255(&reader, firstWord, remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "pressedColor"))
			{
				switch (target.type)
				{
					case Section_Button:  target.button->pressedColor = readColor255(&reader, firstWord, remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "padding"))
			{
				switch (target.type)
				{
					case Section_Button:     target.button->padding    = (f32) readInt(&reader, firstWord, remainder); break;
					case Section_Tooltip:    target.tooltip->padding   = (f32) readInt(&reader, firstWord, remainder); break;
					case Section_UIMessage:  target.message->padding   = (f32) readInt(&reader, firstWord, remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "font"))
			{
				s32 fontID = findFontByName(&reader, assets, nextToken(remainder, &remainder));

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
				s32 fontID = findFontByName(&reader, assets, nextToken(remainder, &remainder));

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
					case Section_Window:  target.window->titleColor = readColor255(&reader, firstWord, remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "titleBarHeight"))
			{
				switch (target.type)
				{
					case Section_Window:  target.window->titleBarHeight = (f32) readInt(&reader, firstWord, remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "titleBarColor"))
			{
				switch (target.type)
				{
					case Section_Window:  target.window->titleBarColor = readColor255(&reader, firstWord, remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "titleBarColorInactive"))
			{
				switch (target.type)
				{
					case Section_Window:  target.window->titleBarColorInactive = readColor255(&reader, firstWord, remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "titleBarButtonHoverColor"))
			{
				switch (target.type)
				{
					case Section_Window:  target.window->titleBarButtonHoverColor = readColor255(&reader, firstWord, remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "backgroundColorInactive"))
			{
				switch (target.type)
				{
					case Section_Window:  target.window->backgroundColorInactive = readColor255(&reader, firstWord, remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "contentPadding"))
			{
				switch (target.type)
				{
					case Section_Window:  target.window->contentPadding = (f32) readInt(&reader, firstWord, remainder); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "contentLabelStyle"))
			{
				String styleName = nextToken(remainder, &remainder);
				switch (target.type)
				{
					case Section_Window:  target.window->labelStyle = findLabelStyle(assets, styleName); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "contentButtonStyle"))
			{
				String styleName = nextToken(remainder, &remainder);
				switch (target.type)
				{
					case Section_Window:  target.window->buttonStyle = findButtonStyle(assets, styleName); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "offset"))
			{
				s64 offsetX, offsetY;
				if (!asInt(nextToken(remainder, &remainder), &offsetX)) error(&reader, "Could not parse {0} as an integer.", {remainder});
				if (!asInt(nextToken(remainder, &remainder), &offsetY)) error(&reader, "Could not parse {0} as an integer.", {remainder});

				switch (target.type)
				{
					case Section_Tooltip:  target.tooltip->offsetFromCursor = v2((s32)offsetX, (s32)offsetY); break;
					default:  error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name});
				}
			}
			else if (equals(firstWord, "textAlignment"))
			{
				switch (target.type)
				{
					case Section_Button:  target.button->textAlignment = readAlignment(&reader, firstWord, remainder); break;
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
