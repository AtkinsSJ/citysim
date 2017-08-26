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
		while (isWhitespace(line.chars[0], false))
		{
			++line.chars;
			--line.length;
		}

		// trim back whitespace
		while (isWhitespace(line.chars[line.length-1], false))
		{
			--line.length;
		}
	}
	while ((line.length <= 0));

	consoleWriteLine(myprintf("LINE {1}: \"{0}\"", {line, formatInt(reader->lineNumber)}));

	return line;
}

void loadUITheme(UITheme *target, String file)
{
	initTheme(target);

	LineReader reader = startFile(file, '#');

	while (reader.pos < reader.file.length)
	{
		String line = nextLine(&reader);

		// TODO: actually parse the lines!
	}
}
