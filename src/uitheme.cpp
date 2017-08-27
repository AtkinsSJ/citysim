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

	V4 textboxTextColor,
		textboxBackgroundColor;

	UIButtonStyle buttonStyle;
	UILabelStyle labelStyle;
	UITooltipStyle tooltipStyle;
	UIMessageStyle uiMessageStyle;
};

// @Deprecated Old code for hard-coded theme.
void initTheme(UITheme *theme)
{
	theme->buttonStyle.font               = FontAssetType_Buttons;
	theme->buttonStyle.textColor          = color255(   0,   0,   0, 255 );
	theme->buttonStyle.backgroundColor    = color255( 255, 255, 255, 255 );
	theme->buttonStyle.hoverColor 	      = color255( 192, 192, 255, 255 );
	theme->buttonStyle.pressedColor       = color255( 128, 128, 255, 255 );

	theme->labelStyle.font                = FontAssetType_Main;
	theme->labelStyle.textColor           = color255( 255, 255, 255, 255 );

	theme->tooltipStyle.font              = FontAssetType_Main;
	theme->tooltipStyle.textColorNormal   = color255( 255, 255, 255, 255 );
	theme->tooltipStyle.textColorBad      = color255( 255,   0,   0, 255 );
	theme->tooltipStyle.backgroundColor   = color255(   0,   0,   0, 128 );
	theme->tooltipStyle.borderPadding     = 4;
	theme->tooltipStyle.depth             = 100;

	theme->uiMessageStyle.font            = FontAssetType_Main;
	theme->uiMessageStyle.textColor       = color255( 255, 255, 255, 255 );
	theme->uiMessageStyle.backgroundColor = color255(   0,   0,   0, 128 );
	theme->uiMessageStyle.borderPadding   = 4;
	theme->uiMessageStyle.depth           = 100;

	theme->overlayColor           = color255(   0,   0,   0, 128 );

	theme->textboxBackgroundColor = color255( 255, 255, 255, 255 );
	theme->textboxTextColor       = color255(   0,   0,   0, 255 );
	
}

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
	remainder->chars = firstWord.chars + firstWord.length;
	remainder->length = input.length - firstWord.length;
	*remainder = trimStart(*remainder);

	return firstWord;
}

V4 readColor255(String input)
{
	int64 r, g, b, a = 255;
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

	return color255(r,g,b,a);
}

void loadUITheme(UITheme *theme, String file)
{
	initTheme(theme);

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
					consoleWriteLine(myprintf("Read color in UITheme file, line {0}: '{1}' as ({2},{3},{4},{5})", {formatInt(reader.lineNumber), remainder,
						formatFloat(theme->overlayColor.r, 2), formatFloat(theme->overlayColor.g, 2), formatFloat(theme->overlayColor.b, 2), formatFloat(theme->overlayColor.a, 2)}), CLS_Success);
				}
				else
				{
					consoleWriteLine(myprintf("Error in UITheme file, line {0}: property '{1}' in an invalid section", {formatInt(reader.lineNumber), firstWord}), CLS_Error);
				}
			}
			else 
			{
				consoleWriteLine(myprintf("Error in UITheme file, line {0}: unrecognized property '{1}'", {formatInt(reader.lineNumber), firstWord}), CLS_Error);
			}
		}
	}
}
