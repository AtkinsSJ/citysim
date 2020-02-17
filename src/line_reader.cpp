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

void restart(LineReader *reader)
{
	reader->currentLine = {};
	reader->lineRemainder = {};
	reader->currentLineNumber = 0;
	reader->startOfNextLine = 0;
}

s32 countLines(Blob data)
{
	s32 result = 0;

	smm startOfNextLine = 0;

	// @Copypasta from loadNextLine() but with a lot of alterations!
	do
	{
		++result;
		while ((startOfNextLine < data.size) && !isNewline(data.memory[startOfNextLine]))
		{
			++startOfNextLine;
		}

		// Handle Windows' stupid double-character newline.
		if (startOfNextLine < data.size)
		{
			++startOfNextLine;
			if (isNewline(data.memory[startOfNextLine]) && (data.memory[startOfNextLine] != data.memory[startOfNextLine-1]))
			{
				++startOfNextLine;
			}
		}
	}
	while (!(startOfNextLine >= data.size));

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

void warn(LineReader *reader, String message, std::initializer_list<String> args)
{
	String text = myprintf(message, args, false);
	logWarn("{0}:{1} - {2}"_s, {reader->filename, formatInt(reader->currentLineNumber), text});
}

void error(LineReader *reader, String message, std::initializer_list<String> args)
{
	String text = myprintf(message, args, false);
	logError("{0}:{1} - {2}"_s, {reader->filename, formatInt(reader->currentLineNumber), text});
	DEBUG_BREAK();
}

String readToken(LineReader *reader, char splitChar)
{
	String token = nextToken(reader->lineRemainder, &reader->lineRemainder, splitChar);

	return token;
}

String peekToken(LineReader *reader, char splitChar)
{
	String token = nextToken(reader->lineRemainder, null, splitChar);

	return token;
}

template<typename T>
Maybe<T> readInt(LineReader *reader, bool isOptional, char splitChar)
{
	String token = readToken(reader, splitChar);
	Maybe<T> result = makeFailure<T>();

	if (! (isOptional && isEmpty(token))) // If it's optional, don't print errors
	{
		Maybe<s64> s64Result = asInt(token);

		if (!s64Result.isValid)
		{
			error(reader, "Couldn't parse '{0}' as an integer."_s, {token});
		}
		else
		{
			if (canCastIntTo<T>(s64Result.value))
			{
				result = makeSuccess<T>((T) s64Result.value);
			}
			else
			{
				error(reader, "Value {0} cannot fit in a {1}."_s, {token, typeNameOf<T>()});
			}
		}
	}

	return result;
}

Maybe<f64> readFloat(LineReader *reader, bool isOptional, char splitChar)
{
	String token = readToken(reader, splitChar);
	Maybe<f64> result = makeFailure<f64>();

	if (! (isOptional && isEmpty(token))) // If it's optional, don't print errors
	{
		if (token[token.length-1] == '%')
		{
			token.length--;

			Maybe<f64> percent = asFloat(token);

			if (!percent.isValid)
			{
				error(reader, "Couldn't parse '{0}%' as a percentage."_s, {token});
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
				error(reader, "Couldn't parse '{0}' as a float."_s, {token});
			}
			else
			{
				result = makeSuccess(floatValue.value);
			}
		}
	}

	return result;
}

Maybe<bool> readBool(LineReader *reader, bool isOptional, char splitChar)
{
	String token = readToken(reader, splitChar);

	Maybe<bool> result = makeFailure<bool>();

	if (! (isOptional && isEmpty(token))) // If it's optional, don't print errors
	{
		result = asBool(token);

		if (!result.isValid)
		{
			error(reader, "Couldn't parse '{0}' as a boolean."_s, {token});
		}
	}

	return result;
}

Maybe<V4> readColor(LineReader *reader)
{
	// TODO: Right now this only handles a sequence of 3 or 4 0-255 values for RGB(A).
	// We might want to handle other color definitions eventually which are more friendly, eg 0-1 fractions.

	String allArguments = getRemainderOfLine(reader);

	Maybe<u8> r = readInt<u8>(reader);
	Maybe<u8> g = readInt<u8>(reader);
	Maybe<u8> b = readInt<u8>(reader);

	Maybe<V4> result;

	if (r.isValid && g.isValid && b.isValid)
	{
		// NB: We default to fully opaque if no alpha is provided
		// FIXME: We *also* default to fully opaque if the alpha is provided but it can't be read as an int... hmmm.
		Maybe<s64> a = asInt(readToken(reader));

		result = makeSuccess(color255(r.value, g.value, b.value, (u8)a.orDefault(255)));
	}
	else
	{
		error(reader, "Couldn't parse '{0}' as a color. Expected 3 or 4 integers from 0 to 255, for R G B and optional A."_s, {allArguments});
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
		if (equals(token, "LEFT"_s))
		{
			if (alignment & ALIGN_H)
			{
				error(reader, "Multiple horizontal alignment keywords given!"_s);
				break;
			}
			alignment |= ALIGN_LEFT;
		}
		else if (equals(token, "H_CENTRE"_s))
		{
			if (alignment & ALIGN_H)
			{
				error(reader, "Multiple horizontal alignment keywords given!"_s);
				break;
			}
			alignment |= ALIGN_H_CENTRE;
		}
		else if (equals(token, "RIGHT"_s))
		{
			if (alignment & ALIGN_H)
			{
				error(reader, "Multiple horizontal alignment keywords given!"_s);
				break;
			}
			alignment |= ALIGN_RIGHT;
		}
		else if (equals(token, "EXPAND_H"_s))
		{
			if (alignment & ALIGN_H)
			{
				error(reader, "Multiple horizontal alignment keywords given!"_s);
				break;
			}
			alignment |= ALIGN_EXPAND_H;
		}
		else if (equals(token, "TOP"_s))
		{
			if (alignment & ALIGN_V)
			{
				error(reader, "Multiple vertical alignment keywords given!"_s);
				break;
			}
			alignment |= ALIGN_TOP;
		}
		else if (equals(token, "V_CENTRE"_s))
		{
			if (alignment & ALIGN_V)
			{
				error(reader, "Multiple vertical alignment keywords given!"_s);
				break;
			}
			alignment |= ALIGN_V_CENTRE;
		}
		else if (equals(token, "BOTTOM"_s))
		{
			if (alignment & ALIGN_V)
			{
				error(reader, "Multiple vertical alignment keywords given!"_s);
				break;
			}
			alignment |= ALIGN_BOTTOM;
		}
		else if (equals(token, "EXPAND_V"_s))
		{
			if (alignment & ALIGN_V)
			{
				error(reader, "Multiple vertical alignment keywords given!"_s);
				break;
			}
			alignment |= ALIGN_EXPAND_V;
		}
		else
		{
			error(reader, "Unrecognized alignment keyword '{0}'"_s, {token});

			return makeFailure<u32>();
		}

		token = readToken(reader);
	}

	return makeSuccess(alignment);
}

Maybe<V2I> readV2I(LineReader *reader)
{
	Maybe<V2I> result = makeFailure<V2I>();

	String token = peekToken(reader);

	Maybe<s32> x = readInt<s32>(reader, false, 'x');
	Maybe<s32> y = readInt<s32>(reader);

	if (x.isValid && y.isValid)
	{
		result = makeSuccess<V2I>(v2i(x.value, y.value));
	}
	else
	{
		error(reader, "Couldn't parse '{0}' as a V2I, expected 2 integers with an 'x' between, eg '8x12'"_s, {token});
	}

	return result;
}

Maybe<EffectRadius> readEffectRadius(LineReader *reader)
{
	Maybe<s32> radius = readInt<s32>(reader);

	if (radius.isValid)
	{
		Maybe<s32> centreValue = readInt<s32>(reader, true);
		Maybe<s32> outerValue  = readInt<s32>(reader, true);

		EffectRadius result = {};

		result.radius      = radius.value;
		result.centreValue = centreValue.orDefault(radius.value); // Default to value=radius
		result.outerValue  = outerValue.orDefault(0);

		return makeSuccess(result);
	}
	else
	{
		error(reader, "Couldn't parse effect radius. Expected \"{0} radius [effectAtCentre] [effectAtEdge]\" where radius, effectAtCentre, and effectAtEdge are ints."_s);

		return makeFailure<EffectRadius>();
	}
}
