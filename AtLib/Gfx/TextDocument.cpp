/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "TextDocument.h"
#include <Assets/AssetManager.h>
#include <IO/LineReader.h>

ErrorOr<NonnullOwnPtr<TextDocument>> TextDocument::load(AssetMetadata& metadata, Blob file_data)
{
    auto& assets = asset_manager();
    auto source_data = assets.allocate_blob(file_data);

    LineReader reader { metadata.shortName, source_data, {} };
    auto lines = assets.allocate_array<Line>(reader.line_count());
    while (reader.load_next_line()) {
        lines.append({ .text = reader.current_line() });
    }
    return { adopt_own(*new TextDocument { move(source_data), move(lines) }) };
}

TextDocument::TextDocument(Blob source_data, Array<Line> lines)
    : m_source_data(source_data)
    , m_lines(move(lines))
{
}

TextDocument::~TextDocument() = default;

void TextDocument::unload(AssetMetadata&)
{
    auto& assets = asset_manager();
    assets.deallocate(m_source_data);
    assets.deallocate(m_lines);
}
