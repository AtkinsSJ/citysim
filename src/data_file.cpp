#pragma once

LineReader readLines(String filename, Blob data, u32 flags, char commentChar)
{
	LineReader result = {};

	result.filename   = filename;
	result.data       = data;

	result.pos = 0;
	result.lineNumber = 0;
	result.removeComments = (flags & LineReader_RemoveTrailingComments) != 0;
	result.commentChar = commentChar;
	result.skipBlankLines = (flags & LineReader_SkipBlankLines) != 0;

	readNextLineInternal(&result);

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
	String result = reader->nextLine;

	reader->lineNumber = reader->nextLineNumber;
	readNextLineInternal(reader);

	return result;
}

inline bool isDone(LineReader *reader)
{
	return !reader->hasNextLine;
}

void readNextLineInternal(LineReader *reader)
{
	String line = {};

	do
	{
		// Get next line
		++reader->nextLineNumber;
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
				if (line[p] == reader->commentChar)
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
	while (isEmpty(line) && !(reader->pos >= reader->data.size));

	reader->nextLine = line;
	reader->hasNextLine = true;

	if (isEmpty(line))
	{
		if (reader->skipBlankLines)
		{
			reader->hasNextLine = false;
		}
		else if (reader->pos >= reader->data.size)
		{
			reader->hasNextLine = false;
		}
	}
}

Maybe<s64> readInt(LineReader *reader, String command, String arguments)
{
	s64 value;

	if (asInt(nextToken(arguments, &arguments), &value))
	{
		return makeSuccess(value);
	}
	else
	{
		error(reader, "Couldn't parse {0}. Expected 1 int.", {command});
		return makeFailure<s64>();
	}
}

Maybe<bool> readBool(LineReader *reader, String command, String arguments)
{
	bool value;

	if (asBool(nextToken(arguments, &arguments), &value))
	{
		return makeSuccess(value);
	}
	else
	{
		error(reader, "Couldn't parse {0}. Expected 1 boolean (true/false).", {command});
		return makeFailure<bool>();
	}
}

// f32 readPercent(LineReader *reader, String command, String arguments)
// {
// 	f32 value;

// 	s64 intValue;

// 	String token = nextToken(arguments, &arguments);
// 	if (token[token.length-1] != '%')
// 	{
// 		error(reader, "Couldn't parse {0}. Expected 
// 	}
// }

Maybe<V4> readColor(LineReader *reader, String command, String arguments)
{
	s64 r = 0;
	s64 g = 0;
	s64 b = 0;
	s64 a = 255;

	// TODO: Right now this only handles a sequence of 3 or 4 0-255 values for RGB(A).
	// We might want to handle other color definitions eventually which are more friendly, eg 0-1 fractions.

	if (asInt(nextToken(arguments, &arguments), &r)
		&& asInt(nextToken(arguments, &arguments), &g)
		&& asInt(nextToken(arguments, &arguments), &b))
	{
		// Note: This does nothing if it fails, which is fine, because the alpha is optional
		asInt(nextToken(arguments, &arguments), &a);

		return makeSuccess(color255((u8)r, (u8)g, (u8)b, (u8)a));
	}
	else
	{
		error(reader, "Couldn't parse {0}. Expected 3 or 4 integers from 0 to 255, for R G B and optional A.", {command});
		return makeFailure<V4>();
	}
}

Maybe<u32> readAlignment(LineReader *reader, String command, String arguments)
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

			return makeFailure<u32>();
		}

		token = nextToken(arguments, &arguments);
	}

	return makeSuccess(result);
}

Maybe<String> readTextureDefinition(LineReader *reader, String tokens)
{
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

		addTiledSprites(spriteName, textureName, (u32)regionW, (u32)regionH, (u32)regionsAcross, (u32)regionsDown);

		hashString(&spriteName);

		return makeSuccess(spriteName);
	}
	else
	{
		error(reader, "Couldn't parse texture. Expected use: \"texture filename.png width height (tilesAcross=1) (tilesDown=1)\"");
		return makeFailure<String>();
	}
}

Maybe<EffectRadius> readEffectRadius(LineReader *reader, String command, String tokens)
{
	String remainder;
	s64 effectValue;
	s64 effectRadius;

	if (asInt(nextToken(tokens, &remainder), &effectValue))
	{
		// Default to radius=value
		if (!asInt(nextToken(remainder, &remainder), &effectRadius))
		{
			effectRadius = effectValue;
		}

		EffectRadius result = {};

		result.centreValue = truncate32(effectValue);
		result.radius      = truncate32(effectRadius);
		result.outerValue  = 0;

		return makeSuccess(result);
	}
	else
	{
		error(reader, "Couldn't parse effect radius. Expected \"{0} effectAtCentre [radius] [effectAtEdge]\" where effectAtCentre, radius, and effectAtEdge are ints.", {command});

		return makeFailure<EffectRadius>();
	}
}
