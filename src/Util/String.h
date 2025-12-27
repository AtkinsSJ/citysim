/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Enum.h>
#include <Util/Forward.h>
#include <Util/Optional.h>
#include <Util/StringBase.h>
#include <Util/StringView.h>
#include <initializer_list>
#include <typeinfo>

//
// NB: Strings are UTF-8. The length is measured in bytes, not in glyphs.
// Probably one day we should make this consistent with TextInput using both, but eh.
//

enum class WithHash : u8 {
    No,
    Yes,
};

// FIXME: Distinguishing between owning and non-owning Strings would be good. (AKA, introduce StringView.)
struct String {
    using Hash = u32;

    // FIXME: initializer_list isn't the best option
    static String join(std::initializer_list<String> strings, Optional<String> between = {});
    static String repeat(char c, u32 length);

    static Optional<String> from_blob(Blob const&, WithHash = WithHash::No);
    static String from_null_terminated(char const*);

    String() = default;
    String(char const* chars, size_t length, WithHash = WithHash::No);
    String(char* chars, size_t length, WithHash = WithHash::No);

    StringView view() const;
    operator StringView() const { return view(); }

    char operator[](s32 index) const;
    char* raw_pointer_to_characters() const { return m_chars; }

    Hash hash() const;

    size_t length() const { return m_length; }
    void deprecated_set_length(size_t new_length) { m_length = new_length; }

    bool is_empty() const;

    bool is_null_terminated() const;
    bool is_valid() const;

    Optional<u32> find(char needle, SearchFrom = SearchFrom::Start, Optional<u32> start_index = {}) const;
    bool contains(char) const;
    bool starts_with(String const& prefix) const;
    bool starts_with(char prefix) const;
    bool ends_with(String const& suffix) const;
    bool ends_with(char suffix) const;

    String trimmed(TrimSide = TrimSide::Both) const;

    // NB: You can pass null for leftResult or rightResult to ignore that part.
    bool split_in_two(char divider, String* left_result, String* right_result);

    Optional<s64> to_int() const;
    Optional<double> to_float() const;
    Optional<bool> to_bool() const;

    bool operator==(String const&) const;

private:
    Hash compute_hash() const;

    size_t m_length;
    char* m_chars;
    mutable Hash m_hash {};
};

String const nullString = {};

inline String operator""_s(char const* chars, size_t length)
{
    return { chars, length };
}

inline String operator""_h(char const* chars, size_t length)
{
    return { chars, length, WithHash::Yes };
}

void copyString(char const* src, s32 srcLength, String* dest);
void copyString(String src, String* dest);
void copyString(StringView src, String* dest);

String pushString(MemoryArena* arena, s32 length);
String pushString(MemoryArena* arena, char const* src);
String pushString(MemoryArena* arena, String src);
String pushString(MemoryArena* arena, StringView src);

String formatInt(u64 value, u8 base = 10, s32 zeroPadWidth = 0);
inline String formatInt(u32 value, u8 base = 10, s32 zeroPadWidth = 0) { return formatInt((u64)value, base, zeroPadWidth); }
inline String formatInt(u16 value, u8 base = 10, s32 zeroPadWidth = 0) { return formatInt((u64)value, base, zeroPadWidth); }
inline String formatInt(u8 value, u8 base = 10, s32 zeroPadWidth = 0) { return formatInt((u64)value, base, zeroPadWidth); }

String formatInt(s64 value, u8 base = 10, s32 zeroPadWidth = 0);
inline String formatInt(s32 value, u8 base = 10, s32 zeroPadWidth = 0) { return formatInt((s64)value, base, zeroPadWidth); }
inline String formatInt(s16 value, u8 base = 10, s32 zeroPadWidth = 0) { return formatInt((s64)value, base, zeroPadWidth); }
inline String formatInt(s8 value, u8 base = 10, s32 zeroPadWidth = 0) { return formatInt((s64)value, base, zeroPadWidth); }

template<Enum T>
constexpr String formatInt(T e)
{
    return formatInt(to_underlying(e));
}

String formatFloat(double value, s32 decimalPlaces);
inline String formatFloat(float value, s32 decimalPlaces) { return formatFloat((double)value, decimalPlaces); }

String formatString(String value, s32 length = -1, bool alignLeft = true, char paddingChar = ' ');
inline String formatString(char const* value, s32 length = -1, bool alignLeft = true, char paddingChar = ' ')
{
    return formatString(String::from_null_terminated(value), length, alignLeft, paddingChar);
}

String formatBool(bool value);

String myprintf(String format, std::initializer_list<StringView> args, bool zeroTerminate = false);

template<typename T>
String typeNameOf()
{
    return String::from_null_terminated(typeid(T).name());
}
