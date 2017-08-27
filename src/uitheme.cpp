#pragma once

struct UIButtonStyle
{
	FontAssetType font;
	V4 textColor;

	V4 backgroundColor;
	V4 hoverColor;
	V4 pressedColor;
};

struct UILabelStyle
{
	FontAssetType font;
	V4 textColor;
};

struct UITextBoxStyle
{
	FontAssetType font;
	V4 textColor;
	V4 backgroundColor;
};

struct UITooltipStyle
{
	FontAssetType font;
	V4 textColorNormal;
	V4 textColorBad;

	V4 backgroundColor;
	real32 borderPadding;
	real32 depth;
};

struct UIMessageStyle
{
	FontAssetType font;
	V4 textColor;

	V4 backgroundColor;
	real32 borderPadding;
	real32 depth;
};

struct UITheme
{
	V4 overlayColor;

	UIButtonStyle buttonStyle;
	UILabelStyle labelStyle;
	UITooltipStyle tooltipStyle;
	UIMessageStyle uiMessageStyle;
	UITextBoxStyle textBoxStyle;
};

struct LineReader
{
	String file;

	int32 pos;
	int32 lineNumber;

	char commentChar;
};

LineReader startFile(String file, char commentChar = '#')
{
	LineReader result = {};
	result.file = file;
	result.pos = 0;
	result.lineNumber = 0;
	result.commentChar = commentChar;

	return result;
}

String nextLine(LineReader *reader)
{
	String line = {};

	do
	{
		// get next line
		++reader->lineNumber;
		line.chars = reader->file.chars + reader->pos;
		line.length = 0;
		while (!isNewline(reader->file.chars[reader->pos]) && (reader->pos < reader->file.length))
		{
			++reader->pos;
			++line.length;
		}
		++reader->pos;
		// fix for windows' stupid double-newline.
		if (isNewline(reader->file.chars[reader->pos]) && (reader->file.chars[reader->pos] != reader->file.chars[reader->pos-1]))
		{
			++reader->pos;
		}

		// trim the comment
		for (int32 p=0; p<line.length; p++)
		{
			if (line.chars[p] == reader->commentChar)
			{
				line.length = p;
				break;
			}
		}

		// trim front whitespace
		line = trimStart(line);
		// trim back whitespace
		line = trimEnd(line);
	}
	while ((line.length <= 0));

	// consoleWriteLine(myprintf("LINE {1}: \"{0}\"", {line, formatInt(reader->lineNumber)}));

	return line;
}

String nextToken(String input, String *remainder)
{
	String firstWord = input;
	firstWord.length = 0;

	while (!isWhitespace(firstWord.chars[firstWord.length], true)
		&& (firstWord.length < input.length))
	{
		++firstWord.length;
	}

	if (remainder)
	{
		remainder->chars = firstWord.chars + firstWord.length;
		remainder->length = input.length - firstWord.length;
		*remainder = trimStart(*remainder);
	}

	return firstWord;
}

V4 readColor255(String input)
{
	int64 r = 0, g = 0, b = 0, a = 255;
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

	return color255((uint8)r, (uint8)g, (uint8)b, (uint8)a);
}

static void invalidPropertyError(LineReader *reader, String property, String section)
{
	consoleWriteLine(myprintf("Error in UITheme file, line {0}: property '{1}' in an invalid section: '{2}'", {formatInt(reader->lineNumber), property, section}), CLS_Error);
}

FontAssetType findFontByName(LineReader *reader, String fontName)
{
	FontAssetType result = FontAssetType_Main;
	if (equals(fontName, "smallFont"))
	{
		result = FontAssetType_Buttons;
	}
	else if (equals(fontName, "normalFont"))
	{
		result = FontAssetType_Main;
	}
	else
	{
		consoleWriteLine(myprintf("Error in UITheme file, line {0}: unrecognized font name '{1}'", {formatInt(reader->lineNumber), fontName}), CLS_Error);
	}
	return result;
}

void loadUITheme(UITheme *theme, String file)
{
	LineReader reader = startFile(file, '#');
	// Scoped enums are a thing, apparently! WOOHOO!
	enum TargetType {
		Section_None = 0,
		Section_General = 1,
		Section_Button,
		Section_Label,
		Section_Tooltip,
		Section_UIMessage,
		Section_TextBox,
	};
	TargetType currentTarget = Section_None;
	String sectionName = {};

	while (reader.pos < reader.file.length)
	{
		String line = nextLine(&reader);

		String firstWord, remainder;
		firstWord = nextToken(line, &remainder);

		// consoleWriteLine(myprintf("First word: '{0}', remainder: '{1}'", {firstWord, remainder}));

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

				if (fontName.length && fontFilename.length)
				{
					// TODO: Decide how this works. I'm not sure the font declaration even wants to work this way!
				}
				else
				{
					consoleWriteLine(myprintf("Invalid font declaration, line {0}: '{1}'", {formatInt(reader.lineNumber), line}), CLS_Error);
				}
			}
			else if (equals(firstWord, "General"))
			{
				currentTarget = Section_General;
			}
			else if (equals(firstWord, "Button"))
			{
				currentTarget = Section_Button;
			}
			else if (equals(firstWord, "Label"))
			{
				currentTarget = Section_Label;
			}
			else if (equals(firstWord, "Tooltip"))
			{
				currentTarget = Section_Tooltip;
			}
			else if (equals(firstWord, "UIMessage"))
			{
				currentTarget = Section_UIMessage;
			}
			else if (equals(firstWord, "TextBox"))
			{
				currentTarget = Section_TextBox;
			}
			else
			{
				consoleWriteLine(myprintf("Unrecognized command in UITheme file, line {0}: '{1}'", {formatInt(reader.lineNumber), firstWord}), CLS_Error);
			}
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
					invalidPropertyError(&reader, firstWord, sectionName);
				}
			}
			else if (equals(firstWord, "textColor"))
			{
				switch (currentTarget)
				{
					case Section_Button: {
						theme->buttonStyle.textColor = readColor255(remainder);
					} break;
					case Section_Label: {
						theme->labelStyle.textColor = readColor255(remainder);
					} break;
					case Section_TextBox: {
						theme->textBoxStyle.textColor = readColor255(remainder);
					} break;
					case Section_UIMessage: {
						theme->uiMessageStyle.textColor = readColor255(remainder);
					} break;
					default:
						invalidPropertyError(&reader, firstWord, sectionName);
						break;
				}
			}
			else if (equals(firstWord, "backgroundColor"))
			{
				switch (currentTarget)
				{
					case Section_Button: {
						theme->buttonStyle.backgroundColor = readColor255(remainder);
					} break;
					case Section_TextBox: {
						theme->textBoxStyle.backgroundColor = readColor255(remainder);
					} break;
					case Section_Tooltip: {
						theme->tooltipStyle.backgroundColor = readColor255(remainder);
					} break;
					case Section_UIMessage: {
						theme->uiMessageStyle.backgroundColor = readColor255(remainder);
					} break;
					default:
						invalidPropertyError(&reader, firstWord, sectionName);
						break;
				}
			}
			else if (equals(firstWord, "hoverColor"))
			{
				switch (currentTarget)
				{
					case Section_Button: {
						theme->buttonStyle.hoverColor = readColor255(remainder);
					} break;
					default:
						invalidPropertyError(&reader, firstWord, sectionName);
						break;
				}
			}
			else if (equals(firstWord, "pressedColor"))
			{
				switch (currentTarget)
				{
					case Section_Button: {
						theme->buttonStyle.pressedColor = readColor255(remainder);
					} break;
					default:
						invalidPropertyError(&reader, firstWord, sectionName);
						break;
				}
			}
			else if (equals(firstWord, "textColorNormal"))
			{
				switch (currentTarget)
				{
					case Section_Tooltip: {
						theme->tooltipStyle.textColorNormal = readColor255(remainder);
					} break;
					default:
						invalidPropertyError(&reader, firstWord, sectionName);
						break;
				}
			}
			else if (equals(firstWord, "textColorBad"))
			{
				switch (currentTarget)
				{
					case Section_Tooltip: {
						theme->tooltipStyle.textColorBad = readColor255(remainder);
					} break;
					default:
						invalidPropertyError(&reader, firstWord, sectionName);
						break;
				}
			}
			else if (equals(firstWord, "depth"))
			{
				switch (currentTarget)
				{
					case Section_Tooltip: {
						int64 intValue;
						if (asInt(remainder, &intValue))
						{
							theme->tooltipStyle.depth = (real32) intValue;
						}
					} break;
					case Section_UIMessage: {
						int64 intValue;
						if (asInt(remainder, &intValue))
						{
							theme->uiMessageStyle.depth = (real32) intValue;
						}
					} break;
					default:
						invalidPropertyError(&reader, firstWord, sectionName);
						break;
				}
			}
			else if (equals(firstWord, "borderPadding"))
			{
				switch (currentTarget)
				{
					case Section_Tooltip: {
						int64 intValue;
						if (asInt(remainder, &intValue))
						{
							theme->tooltipStyle.borderPadding = (real32)intValue;
						}
					} break;
					case Section_UIMessage: {
						int64 intValue;
						if (asInt(remainder, &intValue))
						{
							theme->uiMessageStyle.borderPadding = (real32)intValue;
						}
					} break;
					default:
						invalidPropertyError(&reader, firstWord, sectionName);
						break;
				}
			}
			else if (equals(firstWord, "font"))
			{
				switch (currentTarget)
				{
					case Section_Button: {
						String fontName = nextToken(remainder, null);
						theme->buttonStyle.font = findFontByName(&reader, fontName);
					} break;
					case Section_Label: {
						String fontName = nextToken(remainder, null);
						theme->labelStyle.font = findFontByName(&reader, fontName);
					} break;
					case Section_Tooltip: {
						String fontName = nextToken(remainder, null);
						theme->tooltipStyle.font = findFontByName(&reader, fontName);
					} break;
					case Section_TextBox: {
						String fontName = nextToken(remainder, null);
						theme->textBoxStyle.font = findFontByName(&reader, fontName);
					} break;
					case Section_UIMessage: {
						String fontName = nextToken(remainder, null);
						theme->uiMessageStyle.font = findFontByName(&reader, fontName);
					} break;
					default:
						invalidPropertyError(&reader, firstWord, sectionName);
						break;
				}
			}
			else 
			{
				consoleWriteLine(myprintf("Error in UITheme file, line {0}: unrecognized property '{1}'", {formatInt(reader.lineNumber), firstWord}), CLS_Error);
			}
		}
	}
}
