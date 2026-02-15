/*
 * Copyright (c) 2016-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Credits.h"
#include <Assets/AssetManager.h>
#include <Gfx/TextDocument.h>
#include <UI/UI.h>
#include <UI/UITheme.h>
#include <Util/Vector.h>

AppStatus updateAndRenderCredits(float /*deltaTime*/)
{
    AppStatus result = AppStatus::Credits;

    V2I position = v2i(UI::windowSize.x / 2, 157);
    s32 maxLabelWidth = UI::windowSize.x - 256;

    auto& label_style = UI::LabelStyle::get("title"_s);

    auto& credits_text = TextDocument::get("credits.txt"_s);
    for (auto& line : credits_text.lines()) {
        V2I labelSize = UI::calculateLabelSize(line.text, &label_style, maxLabelWidth);
        Rect2I labelBounds = Rect2I::create_aligned(position, labelSize, label_style.textAlignment);
        UI::putLabel(line.text, labelBounds, &label_style);
        position.y += labelBounds.height();
    }

    auto& button_style = UI::ButtonStyle::get("default"_s);
    s32 uiBorderPadding = 8;
    String backText = getText("button_back"_s);
    V2I backSize = UI::calculateButtonSize(backText, &button_style);
    Rect2I buttonRect { uiBorderPadding, UI::windowSize.y - uiBorderPadding - backSize.y, backSize.x, backSize.y };
    if (UI::putTextButton(getText("button_back"_s), buttonRect, &button_style, ButtonState::Normal)) {
        result = AppStatus::MainMenu;
    }

    return result;
}
