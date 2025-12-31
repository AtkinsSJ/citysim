/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "String.h"
#include <Util/Maths.h>
#include <Util/Memory.h>
#include <Util/MemoryArena.h>
#include <Util/StringBuilder.h>
#include <stdio.h> // For snprintf

String::String(char* chars, size_t length, WithHash with_hash)
    : StringBase(chars, length)
{
    if (with_hash == WithHash::Yes)
        m_hash = compute_hash();
}

String::String(char const* chars, size_t length, WithHash with_hash)
    : StringBase(chars, length)
{
    if (with_hash == WithHash::Yes)
        m_hash = compute_hash();
}

Optional<String> String::from_blob(Blob const& blob, WithHash with_hash)
{
    String result { (char*)blob.data(), (u32)truncate32(blob.size()) };
    if (!result.is_valid())
        return {};
    if (with_hash == WithHash::Yes)
        result.m_hash = result.compute_hash();
    return result;
}

String String::from_null_terminated(char const* chars)
{
    return { chars, strlen(chars) };
}

void copyString(char const* src, s32 srcLength, String* dest)
{
    s32 copyLength = min(srcLength, dest->length());
    copyMemory(src, dest->deprecated_editable_characters(), copyLength);
    dest->deprecated_set_length(copyLength);
}

void copyString(String src, String* dest)
{
    copyString(src.raw_pointer_to_characters(), src.length(), dest);
}

void copyString(StringView src, String* dest)
{
    copyString(src.raw_pointer_to_characters(), src.length(), dest);
}

String pushString(MemoryArena* arena, s32 length)
{
    return String { arena->allocate_multiple<char>(length), static_cast<size_t>(length) };
}

String pushString(MemoryArena* arena, char const* src)
{
    s32 len = truncate32(strlen(src));

    String s = pushString(arena, len);
    copyString(src, len, &s);
    return s;
}

String pushString(MemoryArena* arena, String src)
{
    String s = pushString(arena, src.length());
    copyString(src, &s);
    return s;
}

String pushString(MemoryArena* arena, StringView src)
{
    String s = pushString(arena, src.length());
    copyString(src.raw_pointer_to_characters(), src.length(), &s);
    return s;
}

bool String::operator==(String const& other) const
{
    if (m_hash && other.m_hash && m_hash != other.m_hash)
        return false;
    return StringBase::operator==(other);
}

bool String::is_valid() const
{
    // NB: The final char (length-1) is allowed to be null, so we only check until the one before that
    for (s32 index = 0; index < length() - 1; index++) {
        if (char_at(index) == 0)
            return false;
    }

    return true;
}

String::Hash String::compute_hash() const
{
    // FNV-1a hash
    // http://www.isthe.com/chongo/tech/comp/fnv/
    Hash result = 2166136261;
    for (s32 i = 0; i < length(); i++) {
        result ^= char_at(i);
        result *= 16777619;
    }

    return max(result, 1); // Force the hash to be at least 1, so we can use 0 to mean "no hash"
}

String::Hash String::hash() const
{
    if (!m_hash)
        m_hash = compute_hash();
    return m_hash;
}

String String::trimmed(TrimSide trim_side) const
{
    return with_whitespace_trimmed(trim_side).deprecated_to_string();
}

bool String::is_null_terminated() const
{
    return ends_with(0);
}

// NB: You can pass null for leftResult or rightResult to ignore that part.
bool String::split_in_two(char divider, String* left_result, String* right_result)
{
    if (auto divider_position = find(divider); divider_position.has_value()) {
        if (left_result)
            *left_result = view().substring(0, divider_position.value()).deprecated_to_string();

        if (right_result)
            *right_result = view().substring(divider_position.value() + 1).deprecated_to_string();

        return true;
    }
    return false;
}

String String::join(std::initializer_list<String> strings, Optional<String> between)
{
    String result = nullString;

    if (strings.size() > 0) {
        // Count up the resulting length
        size_t resultLength = between.has_value() ? truncate32(between.value().length() * (strings.size() - 1)) : 0;
        for (auto it = strings.begin(); it != strings.end(); it++) {
            resultLength += it->length();
        }

        StringBuilder stb { resultLength };

        stb.append(*strings.begin());

        for (auto it = (strings.begin() + 1); it != strings.end(); it++) {
            if (between.has_value())
                stb.append(between.value());
            stb.append(*it);
        }

        result = stb.deprecated_to_string();
    }

    return result;
}

/**
 * A printf() that takes a string like "Hello {0}!" where each {n} is replaced by the arg at that index.
 * Extra { and } characters may be stripped. We try to print out any invalid {n} indices but there is not a ton
 * of error-reporting. Try to pass valid input!
 * You pass the arguments in an initializer-list, so like: myprintf("Hello {0}!", {name});
 * All arguments must already be Strings. Use the various formatXXX() functions to convert things to Strings.
 */
String myprintf(StringView format, std::initializer_list<StringView> args, bool zeroTerminate)
{
    // Null bytes are a pain.
    // If we have a terminating one, just trim it off for now (and optionally re-attach it
    // based on the zeroTerminate param.
    // Any nulls in-line will just be printed to the result, following the "garbage in, garbage out"
    // principle!
    if (format.ends_with(0))
        format = format.substring(0, format.length() - 1);

    StringBuilder stb { format.length() * 4 };

    s32 positionalIndex = 0;

    for (s32 i = 0; i < format.length(); i++) {
        if (format[i] == '{') {
            i++; // Skip the {

            s32 startOfNumber = i;

            // Run until the next character is a } or we're done
            while (((i + 1) < format.length())
                && (format[i] != '}')) {
                i++;
            }

            s32 indexLength = i - startOfNumber;

            // Use the current position, if no index was provided
            s32 index = positionalIndex;

            if (indexLength > 0) {
                // Positional
                auto index_string = format.substring(startOfNumber, indexLength);
                if (auto parsed_index = index_string.to_int(); parsed_index.has_value())
                    index = (s32)parsed_index.value();
            }

            if ((index >= 0) && (index < args.size())) {
                auto arg = args.begin()[index];

                if (arg.ends_with("\0"_sv))
                    arg = arg.substring(0, arg.length() - 1);

                stb.append(arg);
            } else {
                // If the index is invalid, show some kind of error. For now, we'll just insert the {n} as given.
                stb.append(format.substring(startOfNumber - 1, indexLength + 2));
            }

            positionalIndex++;
        } else {
            auto startIndex = i;

            // Run until the next character is a { or we're done
            while (((i + 1) < format.length())
                && (format[i + 1] != '{')) {
                i++;
            }

            stb.append(format.substring(startIndex, i + 1 - startIndex));
        }
    }

    if (zeroTerminate && (stb.is_empty() || stb.char_at(stb.length() - 1) != 0)) {
        stb.append('\0');
    }

    String result = stb.deprecated_to_string();

    return result;
}

char const* const intBaseChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
String formatInt(u64 value, u8 base, s32 zeroPadWidth)
{
    ASSERT((base > 1) && (base <= 36)); // formatInt() only handles base 2 to base 36.
    s32 arraySize = max(64, zeroPadWidth);
    char* temp = temp_arena().allocate_multiple<char>(arraySize); // Worst case is base 1, which is 64 characters!
    auto count = 0u;

    u64 v = value;

    do {
        // We start at the end and work backwards, so that we don't have to reverse the string afterwards!
        temp[arraySize - 1 - count++] = intBaseChars[v % base];
        v = v / base;
    } while (v != 0);

    while (count < zeroPadWidth) {
        temp[arraySize - 1 - count++] = '0';
    }

    return String { temp + (arraySize - count), count };
}

String formatInt(s64 value, u8 base, s32 zeroPadWidth)
{
    ASSERT((base > 1) && (base <= 36)); // formatInt() only handles base 2 to base 36.
    s32 arraySize = max(65, zeroPadWidth + 1);
    char* temp = temp_arena().allocate_multiple<char>(arraySize); // Worst case is base 1, which is 64 characters! Plus 1 for sign
    bool isNegative = (value < 0);
    auto count = 0u;

    // One complication here: If we're passed s64_MIN, then -value is 1 larger than can be held in an s64!
    // So, rather than flipping it and treating it like a positive number with an '-' appended,
    // we have to make each digit positive as we get it.

    // s64 v = isNegative ? -value : value;
    s64 v = value;

    do {
        temp[arraySize - 1 - count++] = intBaseChars[((isNegative ? -1 : 1) * (v % base))];
        v = v / base;
    } while (v != 0);

    while (count < zeroPadWidth) {
        temp[arraySize - 1 - count++] = '0';
    }

    if (isNegative) {
        temp[arraySize - 1 - count++] = '-';
    }

    return String { temp + (arraySize - count), count };
}

// TODO: formatFloat() is a total trainwreck, we should really do this a lot better!
// Whether we use our own float-to-string routine or not, we definitely don't want to
// continue calling myprintf() to produce a string for snprintf() and then pass that
// on to myprintf() again! Like, that's super dumb.
String formatFloat(double value, s32 decimalPlaces)
{
    String formatString = myprintf("%.{0}f"_s, { formatInt(decimalPlaces) }, true);

    size_t length = 100; // TODO: is 100 enough?
    char* buffer = temp_arena().allocate_multiple<char>(length);
    size_t written = snprintf(buffer, length, formatString.raw_pointer_to_characters(), value);

    return { buffer, min(written, length) };
}

String formatString(String value, s32 length, bool alignLeft, char paddingChar)
{
    if ((value.length() == length) || (length == -1))
        return value;

    if (length < value.length())
        return { value.raw_pointer_to_characters(), (size_t)length };

    String result = pushString(&temp_arena(), length);

    if (alignLeft) {
        for (s32 i = 0; i < value.length(); i++) {
            result.deprecated_editable_characters()[i] = value[i];
        }
        for (s32 i = value.length(); i < length; i++) {
            result.deprecated_editable_characters()[i] = paddingChar;
        }
    } else // alignRight
    {
        s32 startPos = length - value.length();
        for (s32 i = 0; i < value.length(); i++) {
            result.deprecated_editable_characters()[i + startPos] = value[i];
        }
        for (s32 i = 0; i < startPos; i++) {
            result.deprecated_editable_characters()[i] = paddingChar;
        }
    }

    return result;
}

String formatBool(bool value)
{
    if (value)
        return "true"_s;
    else
        return "false"_s;
}

String String::repeat(char c, u32 length)
{
    String result = pushString(&temp_arena(), length);

    for (s32 i = 0; i < length; i++) {
        result.deprecated_editable_characters()[i] = c;
    }

    return result;
}
