/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "LineReader.h"
#include <Util/Log.h>
#include <Util/Optional.h>
#include <Util/Rectangle.h>
#include <Util/Unicode.h>

LineReader::LineReader(String filename, Blob data, Flags<LineReaderFlags> flags, char commentChar)
    : m_filename(filename)
    , m_data(data)
    , m_skip_blank_lines(flags.has(LineReaderFlags::SkipBlankLines))
    , m_remove_comments(flags.has(LineReaderFlags::RemoveTrailingComments))
    , m_comment_char(commentChar)
{
}

LineReader::State LineReader::save_state() const
{
    return m_state;
}

void LineReader::restore_state(State const& position)
{
    m_state = position;
}

void LineReader::restart()
{
    m_state = {};
}

u32 LineReader::count_lines(Blob const& data)
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

u32 LineReader::count_occurrences_of_property_in_current_command(String const& property_name) const
{
    u32 result = 0;

    // Conceptually this method is const, because the LineReader is left in the same state it was in before.
    // However, we do need to modify its state temporarily. So, we have some const_cast nastiness.
    auto& mutable_this = const_cast<LineReader&>(*this);

    auto saved_position = save_state();
    while (mutable_this.load_next_line()) {
        String first_word = mutable_this.next_token();
        if (first_word[0] == ':')
            break; // We've reached the next :Command

        if (first_word == property_name)
            result++;
    }
    mutable_this.restore_state(saved_position);

    return result;
}

bool LineReader::load_next_line()
{
    bool result = true;

    String line = {};

    do {
        // Get next line
        ++m_state.current_line_number;
        line.chars = (char*)(m_data.data() + m_state.start_of_next_line);
        line.length = 0;
        while ((m_state.start_of_next_line < m_data.size()) && !isNewline(m_data.data()[m_state.start_of_next_line])) {
            ++m_state.start_of_next_line;
            ++line.length;
        }

        // Handle Windows' stupid double-character newline.
        if (m_state.start_of_next_line < m_data.size()) {
            ++m_state.start_of_next_line;
            if (isNewline(m_data.data()[m_state.start_of_next_line]) && (m_data.data()[m_state.start_of_next_line] != m_data.data()[m_state.start_of_next_line - 1])) {
                ++m_state.start_of_next_line;
            }
        }

        // Trim the comment
        if (m_remove_comments) {
            for (s32 p = 0; p < line.length; p++) {
                if (line[p] == m_comment_char) {
                    line.length = p;
                    break;
                }
            }
        }

        // Trim whitespace
        line = trim(line);

        // This seems weird, but basically: The break means all lines get returned if we're not skipping blank ones.
        if (!m_skip_blank_lines)
            break;
    } while (isEmpty(line) && !(m_state.start_of_next_line >= m_data.size()));

    m_state.current_line = line;
    m_state.line_remainder = line;

    if (isEmpty(line)) {
        if (m_skip_blank_lines) {
            result = false;
            m_state.at_end_of_file = true;
        } else if (m_state.start_of_next_line >= m_data.size()) {
            result = false;
            m_state.at_end_of_file = true;
        }
    }

    return result;
}

String LineReader::current_line() const
{
    return m_state.current_line;
}

String LineReader::remainder_of_current_line() const
{
    return trim(m_state.line_remainder);
}

void LineReader::warn(String message, std::initializer_list<String> args) const
{
    String text = myprintf(message, args, false);
    String lineNumber = m_state.at_end_of_file ? "EOF"_s : formatInt(m_state.current_line_number);
    logWarn("{0}:{1} - {2}"_s, { m_filename, lineNumber, text });
}

void LineReader::error(String message, std::initializer_list<String> args) const
{
    String text = myprintf(message, args, false);
    String lineNumber = m_state.at_end_of_file ? "EOF"_s : formatInt(m_state.current_line_number);
    logError("{0}:{1} - {2}"_s, { m_filename, lineNumber, text });
}

String LineReader::next_token(Optional<char> split_char)
{
    return nextToken(m_state.line_remainder, &m_state.line_remainder, split_char.value_or(0));
}

String LineReader::peek_token(Optional<char> split_char)
{
    return nextToken(m_state.line_remainder, nullptr, split_char.value_or(0));
}

s32 LineReader::count_remaining_tokens_in_current_line(Optional<char> split_char) const
{
    return countTokens(m_state.line_remainder, split_char.value_or(0));
}

Optional<bool> LineReader::read_bool(IsRequired is_required, Optional<char> split_char)
{
    String token = next_token(split_char);

    if (isEmpty(token)) {
        if (is_required == IsRequired::Yes)
            error("Expected a boolean value."_s);
        return {};
    }

    if (auto maybe_bool = asBool(token); maybe_bool.isValid)
        return maybe_bool.value;

    error("Couldn't parse '{0}' as a boolean."_s, { token });
    return {};
}

Optional<double> LineReader::read_double(IsRequired is_required, Optional<char> split_char)
{
    String token = next_token(split_char);

    if (isEmpty(token)) {
        if (is_required == IsRequired::Yes)
            error("Expected a floating-point or percentage value."_s);
        return {};
    }

    if (token[token.length - 1] == '%') {
        token.length--;

        if (auto percent = asFloat(token); percent.isValid)
            return percent.value * 0.01;

        error("Couldn't parse '{0}%' as a percentage."_s, { token });
        return {};
    }

    if (auto float_value = asFloat(token); float_value.isValid)
        return float_value.value;

    error("Couldn't parse '{0}' as a float."_s, { token });
    return {};
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

    s32 valueCount = reader->count_remaining_tokens_in_current_line();
    switch (valueCount) {
    case 1: {
        if (auto value = reader->read_int<s32>(); value.has_value()) {
            Padding padding = {};
            padding.top = value.value();
            padding.right = value.value();
            padding.bottom = value.value();
            padding.left = value.value();

            result = makeSuccess(padding);
        }
    } break;

    case 2: {
        auto vValue = reader->read_int<s32>();
        auto hValue = reader->read_int<s32>();
        if (vValue.has_value() && hValue.has_value()) {
            Padding padding = {};
            padding.top = vValue.value();
            padding.right = hValue.value();
            padding.bottom = vValue.value();
            padding.left = hValue.value();

            result = makeSuccess(padding);
        }
    } break;

    case 3: {
        auto topValue = reader->read_int<s32>();
        auto hValue = reader->read_int<s32>();
        auto bottomValue = reader->read_int<s32>();
        if (topValue.has_value() && hValue.has_value() && bottomValue.has_value()) {
            Padding padding = {};
            padding.top = topValue.value();
            padding.right = hValue.value();
            padding.bottom = bottomValue.value();
            padding.left = hValue.value();

            result = makeSuccess(padding);
        }
    } break;

    case 4: {
        auto topValue = reader->read_int<s32>();
        auto rightValue = reader->read_int<s32>();
        auto bottomValue = reader->read_int<s32>();
        auto leftValue = reader->read_int<s32>();
        if (all_have_values(topValue, rightValue, bottomValue, leftValue)) {
            Padding padding = {};
            padding.top = topValue.value();
            padding.right = rightValue.value();
            padding.bottom = bottomValue.value();
            padding.left = leftValue.value();

            result = makeSuccess(padding);
        }
    } break;
    }

    return result;
}

Maybe<V2I> readV2I(LineReader* reader)
{
    Maybe<V2I> result = makeFailure<V2I>();

    String token = reader->peek_token();

    auto x = reader->read_int<s32>(LineReader::IsRequired::Yes, 'x');
    auto y = reader->read_int<s32>();

    if (x.has_value() && y.has_value()) {
        result = makeSuccess<V2I>(v2i(x.release_value(), y.release_value()));
    } else {
        reader->error("Couldn't parse '{0}' as a V2I, expected 2 integers with an 'x' between, eg '8x12'"_s, { token });
    }

    return result;
}
