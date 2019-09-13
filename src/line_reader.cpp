#pragma once

LineReader readLines(String filename, Blob data, u32 flags, char commentChar)
{
	LineReader result = {};

	result.filename = filename;
	result.data = data;

	result.currentLine = {};
	result.lineRemainder = {};
	result.currentLineNumber = 0;
	result.startOfNextLine = 0;

	result.skipBlankLines = (flags & LineReader_SkipBlankLines) != 0;
	result.removeComments = (flags & LineReader_RemoveTrailingComments) != 0;
	result.commentChar = commentChar;

	return result;
}

bool loadNextLine(LineReader *reader)
{
	bool result = true;

	String line = {};

	do
	{
		// Get next line
		++reader->currentLineNumber;
		line.chars = (char *)(reader->data.memory + reader->startOfNextLine);
		line.length = 0;
		while ((reader->startOfNextLine < reader->data.size) && !isNewline(reader->data.memory[reader->startOfNextLine]))
		{
			++reader->startOfNextLine;
			++line.length;
		}

		// Handle Windows' stupid double-character newline.
		if (reader->startOfNextLine < reader->data.size)
		{
			++reader->startOfNextLine;
			if (isNewline(reader->data.memory[reader->startOfNextLine]) && (reader->data.memory[reader->startOfNextLine] != reader->data.memory[reader->startOfNextLine-1]))
			{
				++reader->startOfNextLine;
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
	while (isEmpty(line) && !(reader->startOfNextLine >= reader->data.size));

	reader->currentLine = line;
	reader->lineRemainder = line;

	if (isEmpty(line))
	{
		if (reader->skipBlankLines)
		{
			result = false;
		}
		else if (reader->startOfNextLine >= reader->data.size)
		{
			result = false;
		}
	}

	return result;
}

inline String getLine(LineReader *reader)
{
	return reader->currentLine;
}

inline String getRemainderOfLine(LineReader *reader)
{
	return trim(reader->lineRemainder);
}

void warn(LineReader *reader, char *message, std::initializer_list<String> args)
{
	String text = myprintf(message, args, false);
	logWarn("{0}:{1} - {2}", {reader->filename, formatInt(reader->currentLineNumber), text});
}

void error(LineReader *reader, char *message, std::initializer_list<String> args)
{
	String text = myprintf(message, args, false);
	logError("{0}:{1} - {2}", {reader->filename, formatInt(reader->currentLineNumber), text});
	DEBUG_BREAK();
}

String readToken(LineReader *reader, char splitChar)
{
	String token = nextToken(reader->lineRemainder, &reader->lineRemainder, splitChar);

	return token;
}

Maybe<s64> readInt(LineReader *reader)
{
	String token = readToken(reader);
	Maybe<s64> result = asInt(token);

	if (!result.isValid)
	{
		error(reader, "Couldn't parse '{0}' as an integer.", {token});
	}

	return result;
}

Maybe<f64> readFloat(LineReader *reader)
{
	String token = readToken(reader);
	Maybe<f64> result = makeFailure<f64>();

	if (token[token.length-1] == '%')
	{
		token.length--;

		Maybe<f64> percent = asFloat(token);

		if (!percent.isValid)
		{
			error(reader, "Couldn't parse '{0}%' as a percentage.", {token});
		}
		else
		{
			result = makeSuccess(percent.value * 0.01f);
		}
	}
	else
	{
		Maybe<f64> floatValue = asFloat(token);

		if (!floatValue.isValid)
		{
			error(reader, "Couldn't parse '{0}' as a float.", {token});
		}
		else
		{
			result = makeSuccess(floatValue.value);
		}
	}

	return result;
}

Maybe<bool> readBool(LineReader *reader)
{
	String token = readToken(reader);
	Maybe<bool> result = asBool(token);

	if (!result.isValid)
	{
		error(reader, "Couldn't parse '{0}' as a boolean.", {token});
	}

	return result;
}

Maybe<V4> readColor(LineReader *reader)
{
	// TODO: Right now this only handles a sequence of 3 or 4 0-255 values for RGB(A).
	// We might want to handle other color definitions eventually which are more friendly, eg 0-1 fractions.

	String allArguments = getRemainderOfLine(reader);

	Maybe<s64> r = asInt(readToken(reader));
	Maybe<s64> g = asInt(readToken(reader));
	Maybe<s64> b = asInt(readToken(reader));

	Maybe<V4> result;

	if (r.isValid && g.isValid && b.isValid)
	{
		// NB: We default to fully opaque if no alpha is provided
		// FIXME: We *also* default to fully opaque if the alpha is provided but it can't be read as an int... hmmm.
		Maybe<s64> a = asInt(readToken(reader));

		result = makeSuccess(color255((u8)r.value, (u8)g.value, (u8)b.value, (u8)(a.orDefault(255))));
	}
	else
	{
		error(reader, "Couldn't parse '{0}' as a color. Expected 3 or 4 integers from 0 to 255, for R G B and optional A.", {allArguments});
		result = makeFailure<V4>();
	}

	return result;
}

Maybe<u32> readAlignment(LineReader *reader)
{
	u32 alignment = 0;

	String token = readToken(reader);
	while (!isEmpty(token))
	{
		if (equals(token, "LEFT"))
		{
			if (alignment & ALIGN_H)
			{
				error(reader, "Multiple horizontal alignment keywords given!");
				break;
			}
			alignment |= ALIGN_LEFT;
		}
		else if (equals(token, "H_CENTRE"))
		{
			if (alignment & ALIGN_H)
			{
				error(reader, "Multiple horizontal alignment keywords given!");
				break;
			}
			alignment |= ALIGN_H_CENTRE;
		}
		else if (equals(token, "RIGHT"))
		{
			if (alignment & ALIGN_H)
			{
				error(reader, "Multiple horizontal alignment keywords given!");
				break;
			}
			alignment |= ALIGN_RIGHT;
		}
		else if (equals(token, "TOP"))
		{
			if (alignment & ALIGN_V)
			{
				error(reader, "Multiple vertical alignment keywords given!");
				break;
			}
			alignment |= ALIGN_TOP;
		}
		else if (equals(token, "V_CENTRE"))
		{
			if (alignment & ALIGN_V)
			{
				error(reader, "Multiple vertical alignment keywords given!");
				break;
			}
			alignment |= ALIGN_V_CENTRE;
		}
		else if (equals(token, "BOTTOM"))
		{
			if (alignment & ALIGN_V)
			{
				error(reader, "Multiple vertical alignment keywords given!");
				break;
			}
			alignment |= ALIGN_BOTTOM;
		}
		else
		{
			error(reader, "Unrecognized alignment keyword '{0}'", {token});

			return makeFailure<u32>();
		}

		token = readToken(reader);
	}

	return makeSuccess(alignment);
}

//
// NB: This is the old, @Deprecated way of doing things, which we want to get rid of... but in the interests of
// doing one thing at a time, I've just ported this from the old line-reader code.
// - Sam, 12/09/2019
//
Maybe<String> readTextureDefinition(LineReader *reader)
{
	String spriteName = pushString(&assets->assetArena, readToken(reader));
	String textureName = readToken(reader);

	Maybe<s64> regionW = asInt(readToken(reader));
	Maybe<s64> regionH = asInt(readToken(reader));

	if (regionW.isValid && regionH.isValid)
	{
		Maybe<s64> regionsAcross = asInt(readToken(reader));
		Maybe<s64> regionsDown   = asInt(readToken(reader));

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

Maybe<EffectRadius> readEffectRadius(LineReader *reader)
{
	Maybe<s64> effectValue = asInt(readToken(reader));

	if (effectValue.isValid)
	{
		Maybe<s64> effectRadius = asInt(readToken(reader));
		Maybe<s64> effectOuterValue = asInt(readToken(reader));

		EffectRadius result = {};

		result.centreValue = truncate32(effectValue.value);
		result.radius      = truncate32(effectRadius.orDefault(effectValue.value)); // Default to radius=value
		result.outerValue  = truncate32(effectOuterValue.orDefault(0));

		return makeSuccess(result);
	}
	else
	{
		error(reader, "Couldn't parse effect radius. Expected \"{0} effectAtCentre [radius] [effectAtEdge]\" where effectAtCentre, radius, and effectAtEdge are ints.");

		return makeFailure<EffectRadius>();
	}
}
