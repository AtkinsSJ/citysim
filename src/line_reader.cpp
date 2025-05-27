#pragma once

LineReader readLines(String filename, Blob data, u32 flags, char commentChar)
{
    LineReader result = {};

    result.filename = filename;
    result.data = data;

    result.position = {};

    result.skipBlankLines = (flags & LineReader_SkipBlankLines) != 0;
    result.removeComments = (flags & LineReader_RemoveTrailingComments) != 0;
    result.commentChar = commentChar;

    return result;
}

LineReaderPosition savePosition(LineReader* reader)
{
    return reader->position;
}

void restorePosition(LineReader* reader, LineReaderPosition position)
{
    reader->position = position;
}

void restart(LineReader* reader)
{
    reader->position = {};
}

s32 countLines(Blob data)
{
    s32 result = 0;

    smm startOfNextLine = 0;

    // Code originally based on loadNextLine() but with a lot of alterations!
    do {
        ++result;
        while ((startOfNextLine < data.size) && !isNewline(data.memory[startOfNextLine])) {
            ++startOfNextLine;
        }

        // Handle Windows' stupid double-character newline.
        if (startOfNextLine < data.size) {
            ++startOfNextLine;
            if (isNewline(data.memory[startOfNextLine]) && (data.memory[startOfNextLine] != data.memory[startOfNextLine - 1])) {
                ++startOfNextLine;
            }
        }
    } while (!(startOfNextLine >= data.size));

    return result;
}

s32 countPropertyOccurrences(LineReader* reader, String propertyName)
{
    s32 result = 0;

    LineReaderPosition savedPosition = savePosition(reader);
    while (loadNextLine(reader)) {
        String _firstWord = readToken(reader);
        if (_firstWord[0] == ':')
            break; // We've reached the next :Command

        if (equals(_firstWord, propertyName))
            result++;
    }
    restorePosition(reader, savedPosition);

    return result;
}

bool loadNextLine(LineReader* reader)
{
    bool result = true;

    String line = {};

    do {
        // Get next line
        ++reader->position.currentLineNumber;
        line.chars = (char*)(reader->data.memory + reader->position.startOfNextLine);
        line.length = 0;
        while ((reader->position.startOfNextLine < reader->data.size) && !isNewline(reader->data.memory[reader->position.startOfNextLine])) {
            ++reader->position.startOfNextLine;
            ++line.length;
        }

        // Handle Windows' stupid double-character newline.
        if (reader->position.startOfNextLine < reader->data.size) {
            ++reader->position.startOfNextLine;
            if (isNewline(reader->data.memory[reader->position.startOfNextLine]) && (reader->data.memory[reader->position.startOfNextLine] != reader->data.memory[reader->position.startOfNextLine - 1])) {
                ++reader->position.startOfNextLine;
            }
        }

        // Trim the comment
        if (reader->removeComments) {
            for (s32 p = 0; p < line.length; p++) {
                if (line[p] == reader->commentChar) {
                    line.length = p;
                    break;
                }
            }
        }

        // Trim whitespace
        line = trim(line);

        // This seems weird, but basically: The break means all lines get returned if we're not skipping blank ones.
        if (!reader->skipBlankLines)
            break;
    } while (isEmpty(line) && !(reader->position.startOfNextLine >= reader->data.size));

    reader->position.currentLine = line;
    reader->position.lineRemainder = line;

    if (isEmpty(line)) {
        if (reader->skipBlankLines) {
            result = false;
            reader->position.atEndOfFile = true;
        } else if (reader->position.startOfNextLine >= reader->data.size) {
            result = false;
            reader->position.atEndOfFile = true;
        }
    }

    return result;
}

inline String getLine(LineReader* reader)
{
    return reader->position.currentLine;
}

inline String getRemainderOfLine(LineReader* reader)
{
    return trim(reader->position.lineRemainder);
}

void warn(LineReader* reader, String message, std::initializer_list<String> args)
{
    String text = myprintf(message, args, false);
    String lineNumber = reader->position.atEndOfFile ? "EOF"_s : formatInt(reader->position.currentLineNumber);
    logWarn("{0}:{1} - {2}"_s, { reader->filename, lineNumber, text });
}

void error(LineReader* reader, String message, std::initializer_list<String> args)
{
    String text = myprintf(message, args, false);
    String lineNumber = reader->position.atEndOfFile ? "EOF"_s : formatInt(reader->position.currentLineNumber);
    logError("{0}:{1} - {2}"_s, { reader->filename, lineNumber, text });
}

String readToken(LineReader* reader, char splitChar)
{
    String token = nextToken(reader->position.lineRemainder, &reader->position.lineRemainder, splitChar);

    return token;
}

String peekToken(LineReader* reader, char splitChar)
{
    String token = nextToken(reader->position.lineRemainder, nullptr, splitChar);

    return token;
}

s32 countRemainingTokens(LineReader* reader, char splitChar)
{
    return countTokens(reader->position.lineRemainder, splitChar);
}

template<typename T>
Maybe<T> readInt(LineReader* reader, bool isOptional, char splitChar)
{
    String token = readToken(reader, splitChar);
    Maybe<T> result = makeFailure<T>();

    if (!(isOptional && isEmpty(token))) // If it's optional, don't print errors
    {
        Maybe<s64> s64Result = asInt(token);

        if (!s64Result.isValid) {
            error(reader, "Couldn't parse '{0}' as an integer."_s, { token });
        } else {
            if (canCastIntTo<T>(s64Result.value)) {
                result = makeSuccess<T>((T)s64Result.value);
            } else {
                error(reader, "Value {0} cannot fit in a {1}."_s, { token, typeNameOf<T>() });
            }
        }
    }

    return result;
}

Maybe<f64> readFloat(LineReader* reader, bool isOptional, char splitChar)
{
    String token = readToken(reader, splitChar);
    Maybe<f64> result = makeFailure<f64>();

    if (!(isOptional && isEmpty(token))) // If it's optional, don't print errors
    {
        if (token[token.length - 1] == '%') {
            token.length--;

            Maybe<f64> percent = asFloat(token);

            if (!percent.isValid) {
                error(reader, "Couldn't parse '{0}%' as a percentage."_s, { token });
            } else {
                result = makeSuccess(percent.value * 0.01f);
            }
        } else {
            Maybe<f64> floatValue = asFloat(token);

            if (!floatValue.isValid) {
                error(reader, "Couldn't parse '{0}' as a float."_s, { token });
            } else {
                result = makeSuccess(floatValue.value);
            }
        }
    }

    return result;
}

Maybe<bool> readBool(LineReader* reader, bool isOptional, char splitChar)
{
    String token = readToken(reader, splitChar);

    Maybe<bool> result = makeFailure<bool>();

    if (!(isOptional && isEmpty(token))) // If it's optional, don't print errors
    {
        result = asBool(token);

        if (!result.isValid) {
            error(reader, "Couldn't parse '{0}' as a boolean."_s, { token });
        }
    }

    return result;
}

Maybe<u32> readAlignment(LineReader* reader)
{
    u32 alignment = 0;

    String token = readToken(reader);
    while (!isEmpty(token)) {
        if (equals(token, "LEFT"_s)) {
            if (alignment & ALIGN_H) {
                error(reader, "Multiple horizontal alignment keywords given!"_s);
                break;
            }
            alignment |= ALIGN_LEFT;
        } else if (equals(token, "H_CENTRE"_s)) {
            if (alignment & ALIGN_H) {
                error(reader, "Multiple horizontal alignment keywords given!"_s);
                break;
            }
            alignment |= ALIGN_H_CENTRE;
        } else if (equals(token, "RIGHT"_s)) {
            if (alignment & ALIGN_H) {
                error(reader, "Multiple horizontal alignment keywords given!"_s);
                break;
            }
            alignment |= ALIGN_RIGHT;
        } else if (equals(token, "EXPAND_H"_s)) {
            if (alignment & ALIGN_H) {
                error(reader, "Multiple horizontal alignment keywords given!"_s);
                break;
            }
            alignment |= ALIGN_EXPAND_H;
        } else if (equals(token, "TOP"_s)) {
            if (alignment & ALIGN_V) {
                error(reader, "Multiple vertical alignment keywords given!"_s);
                break;
            }
            alignment |= ALIGN_TOP;
        } else if (equals(token, "V_CENTRE"_s)) {
            if (alignment & ALIGN_V) {
                error(reader, "Multiple vertical alignment keywords given!"_s);
                break;
            }
            alignment |= ALIGN_V_CENTRE;
        } else if (equals(token, "BOTTOM"_s)) {
            if (alignment & ALIGN_V) {
                error(reader, "Multiple vertical alignment keywords given!"_s);
                break;
            }
            alignment |= ALIGN_BOTTOM;
        } else if (equals(token, "EXPAND_V"_s)) {
            if (alignment & ALIGN_V) {
                error(reader, "Multiple vertical alignment keywords given!"_s);
                break;
            }
            alignment |= ALIGN_EXPAND_V;
        } else {
            error(reader, "Unrecognized alignment keyword '{0}'"_s, { token });

            return makeFailure<u32>();
        }

        token = readToken(reader);
    }

    return makeSuccess(alignment);
}

Maybe<V4> readColor(LineReader* reader, bool isOptional)
{
    // TODO: Right now this only handles a sequence of 3 or 4 0-255 values for RGB(A).
    // We might want to handle other color definitions eventually which are more friendly, eg 0-1 fractions.

    String allArguments = getRemainderOfLine(reader);

    Maybe<V4> result = makeFailure<V4>();

    if (!(isOptional && isEmpty(allArguments))) {
        Maybe<u8> r = readInt<u8>(reader);
        Maybe<u8> g = readInt<u8>(reader);
        Maybe<u8> b = readInt<u8>(reader);

        if (r.isValid && g.isValid && b.isValid) {
            // NB: We default to fully opaque if no alpha is provided
            Maybe<u8> a = readInt<u8>(reader, true);

            result = makeSuccess(color255(r.value, g.value, b.value, a.orDefault(255)));
        } else {
            error(reader, "Couldn't parse '{0}' as a color. Expected 3 or 4 integers from 0 to 255, for R G B and optional A."_s, { allArguments });
            result = makeFailure<V4>();
        }
    }

    return result;
}

Maybe<EffectRadius> readEffectRadius(LineReader* reader)
{
    Maybe<s32> radius = readInt<s32>(reader);

    if (radius.isValid) {
        Maybe<s32> centreValue = readInt<s32>(reader, true);
        Maybe<s32> outerValue = readInt<s32>(reader, true);

        EffectRadius result = {};

        result.radius = radius.value;
        result.centreValue = centreValue.orDefault(radius.value); // Default to value=radius
        result.outerValue = outerValue.orDefault(0);

        return makeSuccess(result);
    } else {
        error(reader, "Couldn't parse effect radius. Expected \"{0} radius [effectAtCentre] [effectAtEdge]\" where radius, effectAtCentre, and effectAtEdge are ints."_s);

        return makeFailure<EffectRadius>();
    }
}

Maybe<Padding> readPadding(LineReader* reader)
{
    // Padding definitions may be 1, 2, 3 or 4 values, as CSS does it:
    //   All
    //   Vertical Horizontal
    //   Top Horizontal Bottom
    //   Top Right Bottom Left
    // So, clockwise from the top, with sensible fallbacks

    Maybe<Padding> result = makeFailure<Padding>();

    s32 valueCount = countRemainingTokens(reader);
    switch (valueCount) {
    case 1: {
        Maybe<s32> value = readInt<s32>(reader);
        if (value.isValid) {
            Padding padding = {};
            padding.top = value.value;
            padding.right = value.value;
            padding.bottom = value.value;
            padding.left = value.value;

            result = makeSuccess(padding);
        }
    } break;

    case 2: {
        Maybe<s32> vValue = readInt<s32>(reader);
        Maybe<s32> hValue = readInt<s32>(reader);
        if (vValue.isValid && hValue.isValid) {
            Padding padding = {};
            padding.top = vValue.value;
            padding.right = hValue.value;
            padding.bottom = vValue.value;
            padding.left = hValue.value;

            result = makeSuccess(padding);
        }
    } break;

    case 3: {
        Maybe<s32> topValue = readInt<s32>(reader);
        Maybe<s32> hValue = readInt<s32>(reader);
        Maybe<s32> bottomValue = readInt<s32>(reader);
        if (topValue.isValid && hValue.isValid && bottomValue.isValid) {
            Padding padding = {};
            padding.top = topValue.value;
            padding.right = hValue.value;
            padding.bottom = bottomValue.value;
            padding.left = hValue.value;

            result = makeSuccess(padding);
        }
    } break;

    case 4: {
        Maybe<s32> topValue = readInt<s32>(reader);
        Maybe<s32> rightValue = readInt<s32>(reader);
        Maybe<s32> bottomValue = readInt<s32>(reader);
        Maybe<s32> leftValue = readInt<s32>(reader);
        if (topValue.isValid && rightValue.isValid && bottomValue.isValid && leftValue.isValid) {
            Padding padding = {};
            padding.top = topValue.value;
            padding.right = rightValue.value;
            padding.bottom = bottomValue.value;
            padding.left = leftValue.value;

            result = makeSuccess(padding);
        }
    } break;
    }

    return result;
}

Maybe<V2I> readV2I(LineReader* reader)
{
    Maybe<V2I> result = makeFailure<V2I>();

    String token = peekToken(reader);

    Maybe<s32> x = readInt<s32>(reader, false, 'x');
    Maybe<s32> y = readInt<s32>(reader);

    if (x.isValid && y.isValid) {
        result = makeSuccess<V2I>(v2i(x.value, y.value));
    } else {
        error(reader, "Couldn't parse '{0}' as a V2I, expected 2 integers with an 'x' between, eg '8x12'"_s, { token });
    }

    return result;
}
