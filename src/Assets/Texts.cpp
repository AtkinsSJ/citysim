/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Texts.h"
#include <Assets/AssetManager.h>
#include <IO/LineReader.h>
#include <Util/StringBuilder.h>

namespace Assets {

ErrorOr<NonnullOwnPtr<Texts>> Texts::load(AssetMetadata& metadata, Blob file_data, bool is_fallback_locale)
{
    HashTable<String>& texts_table = is_fallback_locale ? asset_manager().defaultTexts : asset_manager().texts;
    LineReader reader { metadata.shortName, file_data };

    // NB: We store the strings inside the asset data, so it's one block of memory instead of many small ones.
    // However, since we allocate before we parse the file, we need to make sure that the output texts are
    // never longer than the input texts, or we could run out of space!
    // Right now, the only processing we do is replacing \n with a newline character, and similar, so the
    // output can only ever be smaller or the same size as the input.
    // - Sam, 01/10/2019

    // We also put an array of keys into the same allocation.
    // We use the number of lines in the file as a heuristic - we know it'll be slightly more than
    // the number of texts in the file, because they're 1 per line, and we don't have many blanks.

    auto line_count = reader.line_count();
    auto key_array_size = sizeof(String) * line_count;
    auto data = assets_allocate(file_data.size() + key_array_size);
    auto keys = makeArray(line_count, reinterpret_cast<String*>(data.writable_data()));

    auto text_data = data.sub_blob(key_array_size);
    StringBuilder string_data_builder { text_data };
    char* write_position = reinterpret_cast<char*>(text_data.writable_data());

    while (reader.load_next_line()) {
        auto input_key = reader.next_token();
        if (!input_key.has_value())
            continue;
        auto input_text = reader.remainder_of_current_line();

        // Store the key
        string_data_builder.append(input_key.value());
        String key { write_position, input_key.value().length() };
        write_position += key.length();

        // Store the text
        auto* text_start = write_position;
        auto text_length = 0u;

        for (s32 charIndex = 0; charIndex < input_text.length(); charIndex++) {
            char c = input_text[charIndex];
            if (c == '\\') {
                if (charIndex + 1 < input_text.length() && input_text[charIndex + 1] == 'n') {
                    string_data_builder.append('\n');
                    text_length++;
                    charIndex++;
                    continue;
                }
            }

            string_data_builder.append(c);
            text_length++;
        }

        write_position += text_length;

        // Check that we don't already have a text with that name.
        // If we do, one will overwrite the other, and that could be unpredictable if they're
        // in different files. Things will still work, but it would be confusing! And unintended.
        auto existing_text = texts_table.find_value(key);
        if (existing_text.has_value() && existing_text != key) {
            reader.warn("Text asset with ID '{0}' already exists in the texts table! Existing value: \"{1}\""_s, { key, existing_text.value() });
        }

        keys.append(key);

        texts_table.put(key, String { text_start, text_length });
    }

    return adopt_own(*new Texts(move(data), move(keys), is_fallback_locale));
}

Texts::Texts(Blob data, Array<String> keys, bool is_fallback_locale)
    : m_data(move(data))
    , m_keys(move(keys))
    , m_is_fallback_locale(is_fallback_locale)
{
}

void Texts::unload(AssetMetadata&)
{
    // Remove all of our texts from the table
    HashTable<String>* textsTable = (m_is_fallback_locale ? &asset_manager().defaultTexts : &asset_manager().texts);
    for (auto const& key : m_keys)
        textsTable->remove(key);
    assets_deallocate(m_data);
    m_keys = makeEmptyArray<String>();
}

}
