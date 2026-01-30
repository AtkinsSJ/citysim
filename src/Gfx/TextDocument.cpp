/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "TextDocument.h"
#include <Assets/AssetManager.h>

TextDocument& TextDocument::get(StringView name)
{
    return dynamic_cast<DeprecatedAsset&>(*getAsset(AssetType::TextDocument, name.deprecated_to_string()).loaded_asset).text_document;
}

TextDocument::TextDocument(Blob lines_data, Array<Line> lines)
    : m_lines_data(lines_data)
    , m_lines(move(lines))
{
}

void TextDocument::unload()
{
    asset_manager().assetMemoryAllocated -= m_lines_data.size();
    deallocateRaw(m_lines_data.writable_data());
}
