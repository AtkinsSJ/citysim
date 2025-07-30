/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "LineReader.h"
#include "../unicode.h"
#include <Sim/Effect.h>
#include <Util/Log.h>

LineReader readLines(String filename, Blob data, Flags<LineReaderFlags> flags, char commentChar)
{
    LineReader result = {};

    result.filename = filename;
    result.data = data;

    result.position = {};

    result.skipBlankLines = flags.has(LineReaderFlags::SkipBlankLines);
    result.removeComments = flags.has(LineReaderFlags::RemoveTrailingComments);
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
    // FIXME: This should be some kind of Span type instead of Blob.

    s32 result = 0;

    smm startOfNextLine = 0;

    // Code originally based on loadNextLine() but with a lot of alterations!
    do {
        ++result;
        while ((startOfNextLine < data.size()) && !isNewline(data.data()[startOfNextLine])) {
            ++startOfNextLine;
        }

        // Handle Windows' stupid double-character newline.
        if (startOfNextLine < data.size()) {
            ++startOfNextLine;
            if (isNewline(data.data()[startOfNextLine]) && (data.data()[startOfNextLine] != data.data()[startOfNextLine - 1])) {
                ++startOfNextLine;
            }
        }
    } while (!(startOfNextLine >= data.size()));

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

        if (_firstWord == propertyName)
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
        line.chars = (char*)(reader->data.data() + reader->position.startOfNextLine);
        line.length = 0;
        while ((reader->position.startOfNextLine < reader->data.size()) && !isNewline(reader->data.data()[reader->position.startOfNextLine])) {
            ++reader->position.startOfNextLine;
            ++line.length;
        }

        // Handle Windows' stupid double-character newline.
        if (reader->position.startOfNextLine < reader->data.size()) {
            ++reader->position.startOfNextLine;
            if (isNewline(reader->data.data()[reader->position.startOfNextLine]) && (reader->data.data()[reader->position.startOfNextLine] != reader->data.data()[reader->position.startOfNextLine - 1])) {
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
    } while (isEmpty(line) && !(reader->position.startOfNextLine >= reader->data.size()));

    reader->position.currentLine = line;
    reader->position.lineRemainder = line;

    if (isEmpty(line)) {
        if (reader->skipBlankLines) {
            result = false;
            reader->position.atEndOfFile = true;
        } else if (reader->position.startOfNextLine >= reader->data.size()) {
            result = false;
            reader->position.atEndOfFile = true;
        }
    }

    return result;
}

String getLine(LineReader* reader)
{
    return reader->position.currentLine;
}

String getRemainderOfLine(LineReader* reader)
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

Maybe<double> readFloat(LineReader* reader, bool isOptional, char splitChar)
{
    String token = readToken(reader, splitChar);
    Maybe<double> result = makeFailure<double>();

    if (!(isOptional && isEmpty(token))) // If it's optional, don't print errors
    {
        if (token[token.length - 1] == '%') {
            token.length--;

            Maybe<double> percent = asFloat(token);

            if (!percent.isValid) {
                error(reader, "Couldn't parse '{0}%' as a percentage."_s, { token });
            } else {
                result = makeSuccess(percent.value * 0.01f);
            }
        } else {
            Maybe<double> floatValue = asFloat(token);

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

Optional<Alignment> readAlignment(LineReader* reader)
{
    Optional<HAlign> h;
    Optional<VAlign> v;

    String token = readToken(reader);
    while (!isEmpty(token)) {
        if (token == "LEFT"_s) {
            if (h.has_value()) {
                error(reader, "Multiple horizontal alignment keywords given!"_s);
                break;
            }
            h = HAlign::Left;
        } else if (token == "H_CENTRE"_s) {
            if (h.has_value()) {
                error(reader, "Multiple horizontal alignment keywords given!"_s);
                break;
            }
            h = HAlign::Centre;
        } else if (token == "RIGHT"_s) {
            if (h.has_value()) {
                error(reader, "Multiple horizontal alignment keywords given!"_s);
                break;
            }
            h = HAlign::Right;
        } else if (token == "EXPAND_H"_s) {
            if (h.has_value()) {
                error(reader, "Multiple horizontal alignment keywords given!"_s);
                break;
            }
            h = HAlign::Fill;
        } else if (token == "TOP"_s) {
            if (v.has_value()) {
                error(reader, "Multiple vertical alignment keywords given!"_s);
                break;
            }
            v = VAlign::Top;
        } else if (token == "V_CENTRE"_s) {
            if (v.has_value()) {
                error(reader, "Multiple vertical alignment keywords given!"_s);
                break;
            }
            v = VAlign::Centre;
        } else if (token == "BOTTOM"_s) {
            if (v.has_value()) {
                error(reader, "Multiple vertical alignment keywords given!"_s);
                break;
            }
            v = VAlign::Bottom;
        } else if (token == "EXPAND_V"_s) {
            if (v.has_value()) {
                error(reader, "Multiple vertical alignment keywords given!"_s);
                break;
            }
            v = VAlign::Fill;
        } else {
            error(reader, "Unrecognized alignment keyword '{0}'"_s, { token });

            return {};
        }

        token = readToken(reader);
    }

    return Alignment { h.release_value(), v.release_value() };
}

Maybe<Colour> readColor(LineReader* reader, bool isOptional)
{
    // TODO: Right now this only handles a sequence of 3 or 4 0-255 values for RGB(A).
    // We might want to handle other color definitions eventually which are more friendly, eg 0-1 fractions.

    String allArguments = getRemainderOfLine(reader);

    Maybe<Colour> result = makeFailure<Colour>();

    if (!(isOptional && isEmpty(allArguments))) {
        Maybe<u8> r = readInt<u8>(reader);
        Maybe<u8> g = readInt<u8>(reader);
        Maybe<u8> b = readInt<u8>(reader);

        if (r.isValid && g.isValid && b.isValid) {
            // NB: We default to fully opaque if no alpha is provided
            Maybe<u8> a = readInt<u8>(reader, true);

            result = makeSuccess(Colour::from_rgb_255(r.value, g.value, b.value, a.orDefault(255)));
        } else {
            error(reader, "Couldn't parse '{0}' as a color. Expected 3 or 4 integers from 0 to 255, for R G B and optional A."_s, { allArguments });
            result = makeFailure<Colour>();
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
