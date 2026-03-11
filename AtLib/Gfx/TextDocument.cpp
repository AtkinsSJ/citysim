/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "TextDocument.h"
#include <Assets/AssetManager.h>

ErrorOr<NonnullOwnPtr<TextDocument>> TextDocument::load(AssetMetadata& metadata, Blob file_data)
{
    // FIXME: Combine things into one allocation?
    auto source_data = Assets::assets_allocate(file_data.size());
    memcpy(source_data.writable_data(), file_data.data(), file_data.size());

    LineReader reader { metadata.shortName, source_data, {} };
    auto line_count = reader.line_count();
    auto lines_data = Assets::assets_allocate(line_count * sizeof(TextDocument::Line));
    auto lines_array = makeArray(line_count, reinterpret_cast<TextDocument::Line*>(lines_data.writable_data()));
    while (reader.load_next_line()) {
        auto line = reader.current_line();
        lines_array.append({ .text = line });
    }
    return { adopt_own(*new TextDocument { move(source_data), move(lines_data), move(lines_array) }) };
}

TextDocument::TextDocument(Blob source_data, Blob lines_data, Array<Line> lines)
    : m_source_data(source_data)
    , m_lines_data(lines_data)
    , m_lines(move(lines))
{
}

TextDocument::~TextDocument() = default;

void TextDocument::unload(AssetMetadata&)
{
    Assets::assets_deallocate(m_source_data);
    Assets::assets_deallocate(m_lines_data);
}
