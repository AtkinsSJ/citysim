/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Asset.h>
#include <Util/Array.h>
#include <Util/Blob.h>
#include <Util/StringView.h>

// TODO: This wants some kind of text formatting and fanciness, but for now it's just lines.
class TextDocument final : public Asset {
    ASSET_SUBCLASS_METHODS(TextDocument);

public:
    struct Line {
        StringView text;
    };

    TextDocument() = default;
    static ErrorOr<NonnullOwnPtr<TextDocument>> load(AssetMetadata&, Blob file_data);
    virtual ~TextDocument() override;

    // FIXME: ReadonlySpan
    Array<Line> const& lines() const { return m_lines; }

    virtual void unload(AssetMetadata&) override;

private:
    TextDocument(Blob source_data, Blob lines_data, Array<Line> lines);

    Blob m_source_data;
    Blob m_lines_data;
    Array<Line> m_lines;
};
