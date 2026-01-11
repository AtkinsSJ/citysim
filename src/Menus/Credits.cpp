/*
 * Copyright (c) 2016-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Credits.h"
#include <Assets/AssetManager.h>
#include <IO/LineReader.h>
#include <UI/UI.h>
#include <Util/Vector.h>

AppStatus updateAndRenderCredits(float /*deltaTime*/)
{
    AppStatus result = AppStatus::Credits;

    V2I position = v2i(UI::windowSize.x / 2, 157);
    s32 maxLabelWidth = UI::windowSize.x - 256;

    UI::LabelStyle* labelStyle = getStyle<UI::LabelStyle>("title"_s);

    auto& creditsText = getAsset(AssetType::TextDocument, "credits.txt"_s);
    LineReader reader { creditsText.shortName, creditsText.data, {} };
    while (reader.load_next_line()) {
        auto line = reader.current_line();
        V2I labelSize = UI::calculateLabelSize(line, labelStyle, maxLabelWidth);
        Rect2I labelBounds = Rect2I::create_aligned(position, labelSize, labelStyle->textAlignment);
        UI::putLabel(line, labelBounds, labelStyle);
        position.y += labelBounds.height();
    }

    UI::ButtonStyle* style = getStyle<UI::ButtonStyle>("default"_s);
    s32 uiBorderPadding = 8;
    String backText = getText("button_back"_s);
    V2I backSize = UI::calculateButtonSize(backText, style);
    Rect2I buttonRect { uiBorderPadding, UI::windowSize.y - uiBorderPadding - backSize.y, backSize.x, backSize.y };
    if (UI::putTextButton(getText("button_back"_s), buttonRect, style, ButtonState::Normal)) {
        result = AppStatus::MainMenu;
    }

    return result;
}
