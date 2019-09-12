#pragma once

LineReader_Old readLines_old(String filename, Blob data, u32 flags, char commentChar)
{
	LineReader_Old result = {};

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

void warn(LineReader_Old *reader, char *message, std::initializer_list<String> args)
{
	String text = myprintf(message, args, false);
	logWarn("{0}:{1} - {2}", {reader->filename, formatInt(reader->lineNumber), text});
}

void error(LineReader_Old *reader, char *message, std::initializer_list<String> args)
{
	String text = myprintf(message, args, false);
	logError("{0}:{1} - {2}", {reader->filename, formatInt(reader->lineNumber), text});
	DEBUG_BREAK();
}

String nextLine(LineReader_Old *reader)
{
	String result = reader->nextLine;

	reader->lineNumber = reader->nextLineNumber;
	readNextLineInternal(reader);

	return result;
}

inline bool isDone(LineReader_Old *reader)
{
	return !reader->hasNextLine;
}

void readNextLineInternal(LineReader_Old *reader)
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

Maybe<s64> readInt(LineReader_Old *reader, String command, String arguments)
{
	Maybe<s64> result = asInt(nextToken(arguments, &arguments));

	if (!result.isValid)
	{
		error(reader, "Couldn't parse {0}. Expected 1 int.", {command});
	}

	return result;
}

Maybe<bool> readBool(LineReader_Old *reader, String command, String arguments)
{
	Maybe<bool> result = asBool(nextToken(arguments, &arguments));

	if (!result.isValid)
	{
		error(reader, "Couldn't parse {0}. Expected 1 boolean (true/false).", {command});
	}

	return result;
}

// f32 readPercent(LineReader_Old *reader, String command, String arguments)
// {
// 	f32 value;

// 	s64 intValue;

// 	String token = nextToken(arguments, &arguments);
// 	if (token[token.length-1] != '%')
// 	{
// 		error(reader, "Couldn't parse {0}. Expected "
// 	}
// }

Maybe<V4> readColor(LineReader_Old *reader, String command, String arguments)
{
	// TODO: Right now this only handles a sequence of 3 or 4 0-255 values for RGB(A).
	// We might want to handle other color definitions eventually which are more friendly, eg 0-1 fractions.

	Maybe<s64> r = asInt(nextToken(arguments, &arguments));
	Maybe<s64> g = asInt(nextToken(arguments, &arguments));
	Maybe<s64> b = asInt(nextToken(arguments, &arguments));
	Maybe<s64> a = asInt(nextToken(arguments, &arguments));

	if (r.isValid && g.isValid && b.isValid)
	{
		// NB: We default to fully opaque if no alpha is provided

		return makeSuccess(color255((u8)r.value, (u8)g.value, (u8)b.value, (u8)(a.orDefault(255))));
	}
	else
	{
		error(reader, "Couldn't parse {0}. Expected 3 or 4 integers from 0 to 255, for R G B and optional A.", {command});
		return makeFailure<V4>();
	}
}

Maybe<u32> readAlignment(LineReader_Old *reader, String command, String arguments)
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

Maybe<String> readTextureDefinition(LineReader_Old *reader, String tokens)
{
	String spriteName = pushString(&assets->assetArena, nextToken(tokens, &tokens));
	String textureName = nextToken(tokens, &tokens);

	Maybe<s64> regionW = asInt(nextToken(tokens, &tokens));
	Maybe<s64> regionH = asInt(nextToken(tokens, &tokens));

	if (regionW.isValid && regionH.isValid)
	{
		Maybe<s64> regionsAcross = asInt(nextToken(tokens, &tokens));
		Maybe<s64> regionsDown   = asInt(nextToken(tokens, &tokens));

		u32 across = 1;
		u32 down = 1;

		if (regionsAcross.isValid && regionsDown.isValid)
		{
			across = (u32) regionsAcross.value;
			down = (u32) regionsDown.value;
		}

		addTiledSprites(spriteName, textureName, (u32)regionW.value, (u32)regionH.value, across, down);

		hashString(&spriteName);

		return makeSuccess(spriteName);
	}
	else
	{
		error(reader, "Couldn't parse texture. Expected use: \"texture filename.png width height (tilesAcross=1) (tilesDown=1)\"");
		return makeFailure<String>();
	}
}

Maybe<EffectRadius> readEffectRadius(LineReader_Old *reader, String command, String tokens)
{
	String remainder;

	Maybe<s64> effectValue = asInt(nextToken(tokens, &remainder));

	if (effectValue.isValid)
	{
		Maybe<s64> effectRadius = asInt(nextToken(remainder, &remainder));
		Maybe<s64> effectOuterValue = asInt(nextToken(remainder, &remainder));

		EffectRadius result = {};

		result.centreValue = truncate32(effectValue.value);
		result.radius      = truncate32(effectRadius.orDefault(effectValue.value)); // Default to radius=value
		result.outerValue  = truncate32(effectOuterValue.orDefault(0));

		return makeSuccess(result);
	}
	else
	{
		error(reader, "Couldn't parse effect radius. Expected \"{0} effectAtCentre [radius] [effectAtEdge]\" where effectAtCentre, radius, and effectAtEdge are ints.", {command});

		return makeFailure<EffectRadius>();
	}
}
