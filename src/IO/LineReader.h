/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Flags.h>
#include <Util/Memory.h>
#include <Util/Optional.h>
#include <Util/String.h>
#include <Util/TokenReader.h>

class LineReader {
public:
    struct State {
        String current_line;
        smm current_line_number;
        TokenReader current_line_reader { current_line };
        smm start_of_next_line;
        bool at_end_of_file;
    };

    enum class IsRequired : u8 {
        No,
        Yes,
    };

    enum class Flags : u8 {
        SkipBlankLines,
        RemoveTrailingComments,
        COUNT,
    };
    static constexpr ::Flags DefaultFlags { Flags::SkipBlankLines, Flags::RemoveTrailingComments };

    LineReader(String filename, Blob data, ::Flags<Flags> flags = DefaultFlags, char commentChar = '#');

    State save_state() const;
    void restore_state(State const&);
    void restart();

    // FIXME: I don't like this API. Use a "has_next() / get_next()" style instead.
    //        Or, for_each_line() with a callback?
    //        Or, an iterator of some sort?
    bool load_next_line();
    String current_line() const;
    String remainder_of_current_line() const;

    Optional<StringView> next_token(Optional<char> split_char = {});
    Optional<StringView> peek_token(Optional<char> split_char = {});

    s32 count_remaining_tokens_in_current_line(Optional<char> split_char = {}) const;
    u32 count_occurrences_of_property_in_current_command(String const& property_name) const;

    Optional<bool> read_bool(IsRequired = IsRequired::Yes, Optional<char> split_char = {});

    Optional<double> read_double(IsRequired = IsRequired::Yes, Optional<char> split_char = {});
    Optional<float> read_float(IsRequired is_required = IsRequired::Yes, Optional<char> split_char = {})
    {
        if (auto maybe_double = read_double(is_required, split_char); maybe_double.has_value())
            return static_cast<float>(maybe_double.release_value());
        return {};
    }

    template<typename T>
    Optional<T> read_int(IsRequired is_required = IsRequired::Yes, Optional<char> split_char = {})
    {
        auto maybe_token = next_token(split_char);

        if (!maybe_token.has_value()) {
            if (is_required == IsRequired::Yes)
                error("Expected an integer value."_s);
            return {};
        }
        auto& token = maybe_token.value();

        if (auto maybe_s64 = token.to_int(); maybe_s64.has_value()) {
            if (canCastIntTo<T>(maybe_s64.value()))
                return static_cast<T>(maybe_s64.value());

            error("Value {0} cannot fit in a {1}."_s, { token, typeNameOf<T>() });
            return {};
        }

        error("Couldn't parse '{0}' as an integer."_s, { token });
        return {};
    }

    void warn(String message, std::initializer_list<StringView> args = {}) const;
    void error(String message, std::initializer_list<StringView> args = {}) const;

    static u32 count_lines(Blob const& data);

private:
    String m_filename;
    Blob m_data;

    State m_state {};

    bool m_skip_blank_lines;
    bool m_remove_comments;
    char m_comment_char;
};
