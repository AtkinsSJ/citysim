/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "LineReader.h"
#include <Util/Log.h>
#include <Util/Optional.h>
#include <Util/Unicode.h>

LineReader::LineReader(String filename, Blob data, ::Flags<Flags> flags, char commentChar)
    : m_filename(filename)
    , m_data(data)
    , m_skip_blank_lines(flags.has(Flags::SkipBlankLines))
    , m_remove_comments(flags.has(Flags::RemoveTrailingComments))
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
        line = line.trimmed();

        // This seems weird, but basically: The break means all lines get returned if we're not skipping blank ones.
        if (!m_skip_blank_lines)
            break;
    } while (line.is_empty() && !(m_state.start_of_next_line >= m_data.size()));

    m_state.current_line = line;
    m_state.line_remainder = line;

    if (line.is_empty()) {
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
    return m_state.line_remainder.trimmed();
}

void LineReader::warn(String message, std::initializer_list<StringView> args) const
{
    String text = myprintf(message, args, false);
    String lineNumber = m_state.at_end_of_file ? "EOF"_s : formatInt(m_state.current_line_number);
    logWarn("{0}:{1} - {2}"_s, { m_filename, lineNumber, text });
}

void LineReader::error(String message, std::initializer_list<StringView> args) const
{
    String text = myprintf(message, args, false);
    String lineNumber = m_state.at_end_of_file ? "EOF"_s : formatInt(m_state.current_line_number);
    logError("{0}:{1} - {2}"_s, { m_filename, lineNumber, text });
}

String LineReader::next_token(Optional<char> split_char)
{
    return m_state.line_remainder.next_token(&m_state.line_remainder, split_char);
}

String LineReader::peek_token(Optional<char> split_char)
{
    return m_state.line_remainder.next_token(nullptr, split_char);
}

s32 LineReader::count_remaining_tokens_in_current_line(Optional<char> split_char) const
{
    return m_state.line_remainder.count_tokens(split_char);
}

Optional<bool> LineReader::read_bool(IsRequired is_required, Optional<char> split_char)
{
    String token = next_token(split_char);

    if (token.is_empty()) {
        if (is_required == IsRequired::Yes)
            error("Expected a boolean value."_s);
        return {};
    }

    if (auto maybe_bool = token.to_bool(); maybe_bool.has_value())
        return maybe_bool.value();

    error("Couldn't parse '{0}' as a boolean."_s, { token });
    return {};
}

Optional<double> LineReader::read_double(IsRequired is_required, Optional<char> split_char)
{
    String token = next_token(split_char);

    if (token.is_empty()) {
        if (is_required == IsRequired::Yes)
            error("Expected a floating-point or percentage value."_s);
        return {};
    }

    if (token[token.length - 1] == '%') {
        token.length--;

        if (auto percent = token.to_float(); percent.has_value())
            return percent.value() * 0.01;

        error("Couldn't parse '{0}%' as a percentage."_s, { token });
        return {};
    }

    if (auto float_value = token.to_float(); float_value.has_value())
        return float_value.value();

    error("Couldn't parse '{0}' as a float."_s, { token });
    return {};
}
