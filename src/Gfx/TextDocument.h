/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Array.h>
#include <Util/Blob.h>
#include <Util/StringView.h>

// TODO: This wants some kind of text formatting and fanciness, but for now it's just lines.
class TextDocument {
public:
    static TextDocument& get(StringView name);

    struct Line {
        StringView text;
    };

    explicit TextDocument(Blob lines_data, Array<Line> lines);
    TextDocument() = default; // FIXME: Temporary until we remove Asset's big union.

    // FIXME: ReadonlySpan
    Array<Line> const& lines() const { return m_lines; }

    void unload();

private:
    Blob m_lines_data;
    Array<Line> m_lines;
};
