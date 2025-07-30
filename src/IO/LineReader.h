/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Gfx/Colour.h>
#include <Util/Alignment.h>
#include <Util/Basic.h>
#include <Util/Flags.h>
#include <Util/Memory.h>
#include <Util/String.h>

struct LineReaderPosition {
    String currentLine;
    smm currentLineNumber;
    String lineRemainder;
    smm startOfNextLine;
    bool atEndOfFile;
};

struct LineReader {
    String filename;
    Blob data;

    LineReaderPosition position;

    bool skipBlankLines;
    bool removeComments;
    char commentChar;
};

enum class LineReaderFlags : u8 {
    SkipBlankLines,
    RemoveTrailingComments,
    COUNT,
};
constexpr Flags DefaultLineReaderFlags { LineReaderFlags::SkipBlankLines, LineReaderFlags::RemoveTrailingComments };

LineReader readLines(String filename, Blob data, Flags<LineReaderFlags> flags = DefaultLineReaderFlags, char commentChar = '#');
LineReaderPosition savePosition(LineReader* reader);
void restorePosition(LineReader* reader, LineReaderPosition position);
void restart(LineReader* reader);

// How many lines are in the file?
s32 countLines(Blob data);

bool loadNextLine(LineReader* reader);
String getLine(LineReader* reader);
String getRemainderOfLine(LineReader* reader);

// How many times does the given property appear in the current :Command block?
s32 countPropertyOccurrences(LineReader* reader, String propertyName);

String readToken(LineReader* reader, char splitChar = 0);
String peekToken(LineReader* reader, char splitChar = 0);
s32 countRemainingTokens(LineReader* reader, char splitChar = 0);

Maybe<double> readFloat(LineReader* reader, bool isOptional = false, char splitChar = 0);
Maybe<bool> readBool(LineReader* reader, bool isOptional = false, char splitChar = 0);

Optional<Alignment> readAlignment(LineReader* reader);
Maybe<Colour> readColor(LineReader* reader, bool isOptional = false);
Maybe<Padding> readPadding(LineReader* reader);
Maybe<V2I> readV2I(LineReader* reader);

void warn(LineReader* reader, String message, std::initializer_list<String> args = {});
void error(LineReader* reader, String message, std::initializer_list<String> args = {});

template<typename T>
Maybe<T> readInt(LineReader* reader, bool isOptional = false, char splitChar = 0)
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
