#pragma once

void initUITheme(UITheme *theme)
{
	initHashTable(&theme->fontNamesToAssetNames);
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

#define WRONG_SECTION error(&reader, "property '{0}' in an invalid section: '{1}'", {firstWord, target.name})

void loadUITheme(AssetManager *assets, Blob data, Asset *asset)
{
	LineReader reader = readLines(asset->shortName, data, true, true, '#');

	UITheme *theme = &assets->theme;
	clear(&theme->fontNamesToAssetNames);

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

	while (!isDone(&reader))
	{
		String line = nextLine(&reader);
		if (line.length == 0) break;

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
					addFont(assets, pushString(&assets->assetArena, fontName), pushString(&assets->assetArena, fontFilename));
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
					WRONG_SECTION;
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
					default:  WRONG_SECTION;
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
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "hoverColor"))
			{
				switch (target.type)
				{
					case Section_Button:  target.button->hoverColor = readColor255(&reader, firstWord, remainder); break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "pressedColor"))
			{
				switch (target.type)
				{
					case Section_Button:  target.button->pressedColor = readColor255(&reader, firstWord, remainder); break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "padding"))
			{
				switch (target.type)
				{
					case Section_Button:     target.button->padding    = (f32) readInt(&reader, firstWord, remainder); break;
					case Section_Tooltip:    target.tooltip->padding   = (f32) readInt(&reader, firstWord, remainder); break;
					case Section_UIMessage:  target.message->padding   = (f32) readInt(&reader, firstWord, remainder); break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "font"))
			{
				String fontName = pushString(&assets->assetArena, nextToken(remainder, &remainder));
				
				switch (target.type)
				{
					case Section_Button:     target.button->fontName        = fontName; break;
					case Section_Label:      target.label->fontName         = fontName; break;
					case Section_Tooltip:    target.tooltip->fontName       = fontName; break;
					case Section_TextBox:    target.textBox->fontName       = fontName; break;
					case Section_UIMessage:  target.message->fontName       = fontName; break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "titleFont"))
			{
				String fontName = pushString(&assets->assetArena, nextToken(remainder, &remainder));

				switch (target.type)
				{
					case Section_Window:  target.window->titleFontName = fontName; break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "titleColor"))
			{
				switch (target.type)
				{
					case Section_Window:  target.window->titleColor = readColor255(&reader, firstWord, remainder); break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "titleBarHeight"))
			{
				switch (target.type)
				{
					case Section_Window:  target.window->titleBarHeight = (f32) readInt(&reader, firstWord, remainder); break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "titleBarColor"))
			{
				switch (target.type)
				{
					case Section_Window:  target.window->titleBarColor = readColor255(&reader, firstWord, remainder); break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "titleBarColorInactive"))
			{
				switch (target.type)
				{
					case Section_Window:  target.window->titleBarColorInactive = readColor255(&reader, firstWord, remainder); break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "titleBarButtonHoverColor"))
			{
				switch (target.type)
				{
					case Section_Window:  target.window->titleBarButtonHoverColor = readColor255(&reader, firstWord, remainder); break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "backgroundColorInactive"))
			{
				switch (target.type)
				{
					case Section_Window:  target.window->backgroundColorInactive = readColor255(&reader, firstWord, remainder); break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "contentPadding"))
			{
				switch (target.type)
				{
					case Section_Window:  target.window->contentPadding = (f32) readInt(&reader, firstWord, remainder); break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "contentLabelStyle"))
			{
				String styleName = nextToken(remainder, &remainder);
				switch (target.type)
				{
					case Section_Window:  target.window->labelStyle = findLabelStyle(assets, styleName); break;
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "contentButtonStyle"))
			{
				String styleName = nextToken(remainder, &remainder);
				switch (target.type)
				{
					case Section_Window:  target.window->buttonStyle = findButtonStyle(assets, styleName); break;
					default:  WRONG_SECTION;
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
					default:  WRONG_SECTION;
				}
			}
			else if (equals(firstWord, "textAlignment"))
			{
				switch (target.type)
				{
					case Section_Button:  target.button->textAlignment = readAlignment(&reader, firstWord, remainder); break;
					default:  WRONG_SECTION;
				}
			}
			else 
			{
				error(&reader, "Unrecognized property '{0}'", {firstWord});
			}
		}
	}
}
#undef WRONG_SECTION
