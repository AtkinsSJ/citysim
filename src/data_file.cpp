#pragma once

LineReader readLines(String filename, Blob data, bool skipBlankLines, bool removeComments, char commentChar)
{
	LineReader result = {};

	result.filename   = filename;
	result.data       = data;

	result.pos = 0;
	result.lineNumber = 0;
	result.removeComments = removeComments;
	result.commentChar = commentChar;
	result.skipBlankLines = skipBlankLines;

	return result;
}

void warn(LineReader *reader, char *message, std::initializer_list<String> args)
{
	String text = myprintf(message, args, false);
	logWarn("{0}:{1} - {2}", {reader->filename, formatInt(reader->lineNumber), text});
}

void error(LineReader *reader, char *message, std::initializer_list<String> args)
{
	String text = myprintf(message, args, false);
	logError("{0}:{1} - {2}", {reader->filename, formatInt(reader->lineNumber), text});
	DEBUG_BREAK();
}

String nextLine(LineReader *reader)
{
	String line = {};

	do
	{
		// Get next line
		++reader->lineNumber;
		line.chars = (char *)(reader->data.memory + reader->pos);
		line.length = 0;
		while ((reader->pos < reader->data.size) && !isNewline(reader->data.memory[reader->pos]))
		{
			++reader->pos;
			++line.length;
		}

		// Handle Windows' stupid double-character newline.
		if (reader->pos < reader->data.size)
		{
			++reader->pos;
			if (isNewline(reader->data.memory[reader->pos]) && (reader->data.memory[reader->pos] != reader->data.memory[reader->pos-1]))
			{
				++reader->pos;
			}
		}

		// Trim the comment
		if (reader->removeComments)
		{
			for (s32 p=0; p<line.length; p++)
			{
				if (line.chars[p] == reader->commentChar)
				{
					line.length = p;
					break;
				}
			}
		}

		// Trim whitespace
		line = trim(line);

		// This seems weird, but basically: The break means all lines get returned if we're not skipping blank ones.
		if (!reader->skipBlankLines) break;
	}
	while (isEmpty(line) && !isDone(reader));

	return line;
}

s64 readInt(LineReader *reader, String command, String arguments)
{
	s64 value;

	if (asInt(nextToken(arguments, &arguments), &value))
	{
		return value;
	}
	else
	{
		error(reader, "Couldn't parse {0}. Expected 1 int.", {command});
		return 0;
	}
}

bool readBool(LineReader *reader, String command, String arguments)
{
	bool value;

	if (asBool(nextToken(arguments, &arguments), &value))
	{
		return value;
	}
	else
	{
		error(reader, "Couldn't parse {0}. Expected 1 boolean (true/false).", {command});
		return false;
	}
}

V4 readColor255(LineReader *reader, String command, String arguments)
{
	s64 r = 0;
	s64 g = 0;
	s64 b = 0;
	s64 a = 255;

	if (asInt(nextToken(arguments, &arguments), &r)
		&& asInt(nextToken(arguments, &arguments), &g)
		&& asInt(nextToken(arguments, &arguments), &b))
	{
		// Note: This does nothing if it fails, which is fine, because the alpha is optional
		asInt(nextToken(arguments, &arguments), &a);

		return color255((u8)r, (u8)g, (u8)b, (u8)a);
	}
	else
	{
		error(reader, "Couldn't parse {0}. Expected 3 or 4 integers from 0 to 255, for R G B and optional A.", {command});
		return {};
	}
}

u32 readAlignment(LineReader *reader, String command, String arguments)
{
	u32 result = 0;

	String token = nextToken(arguments, &arguments);
	while (!isEmpty(token))
	{
		if (equals(token, "LEFT"))
		{
			if (result & ALIGN_H)
			{
				error(reader, "Couldn't parse {0}. Multiple horizontal alignment keywords given!", {command});
				break;
			}
			result |= ALIGN_LEFT;
		}
		else if (equals(token, "H_CENTRE"))
		{
			if (result & ALIGN_H)
			{
				error(reader, "Couldn't parse {0}. Multiple horizontal alignment keywords given!", {command});
				break;
			}
			result |= ALIGN_H_CENTRE;
		}
		else if (equals(token, "RIGHT"))
		{
			if (result & ALIGN_H)
			{
				error(reader, "Couldn't parse {0}. Multiple horizontal alignment keywords given!", {command});
				break;
			}
			result |= ALIGN_RIGHT;
		}
		else if (equals(token, "TOP"))
		{
			if (result & ALIGN_V)
			{
				error(reader, "Couldn't parse {0}. Multiple vertical alignment keywords given!", {command});
				break;
			}
			result |= ALIGN_TOP;
		}
		else if (equals(token, "V_CENTRE"))
		{
			if (result & ALIGN_V)
			{
				error(reader, "Couldn't parse {0}. Multiple vertical alignment keywords given!", {command});
				break;
			}
			result |= ALIGN_V_CENTRE;
		}
		else if (equals(token, "BOTTOM"))
		{
			if (result & ALIGN_V)
			{
				error(reader, "Couldn't parse {0}. Multiple vertical alignment keywords given!", {command});
				break;
			}
			result |= ALIGN_BOTTOM;
		}
		else
		{
			error(reader, "Couldn't parse {0}. Unrecognized alignment keyword '{1}'", {command, token});
			break;
		}

		token = nextToken(arguments, &arguments);
	}

	return result;
}

String readTextureDefinition(LineReader *reader, AssetManager *assets, String tokens)
{
	String result = nullString;

	String spriteName = pushString(&assets->assetArena, nextToken(tokens, &tokens));
	String textureName = nextToken(tokens, &tokens);
	s64 regionW;
	s64 regionH;
	s64 regionsAcross;
	s64 regionsDown;

	if (asInt(nextToken(tokens, &tokens), &regionW)
		&& asInt(nextToken(tokens, &tokens), &regionH))
	{
		if (!(asInt(nextToken(tokens, &tokens), &regionsAcross)
			&& asInt(nextToken(tokens, &tokens), &regionsDown)))
		{
			regionsAcross = 1;
			regionsDown = 1;
		}

		addTiledSprites(assets, spriteName, textureName, (u32)regionW, (u32)regionH, (u32)regionsAcross, (u32)regionsDown);

		hashString(&spriteName);

		result = spriteName;
	}
	else
	{
		error(reader, "Couldn't parse texture. Expected use: \"texture filename.png width height (tilesAcross=1) (tilesDown=1)\"");
	}

	return result;
}
