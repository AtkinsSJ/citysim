/*
 * Copyright (c) 2016-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "credits.h"
#include "line_reader.h"
#include <Assets/AssetManager.h>
#include <UI/UI.h>
#include <Util/Vector.h>

AppStatus updateAndRenderCredits(f32 /*deltaTime*/)
{
    AppStatus result = AppStatus::Credits;

    V2I position = v2i(UI::windowSize.x / 2, 157);
    s32 maxLabelWidth = UI::windowSize.x - 256;

    UI::LabelStyle* labelStyle = getStyle<UI::LabelStyle>("title"_s);

    Asset* creditsText = getAsset(AssetType::Misc, "credits.txt"_s);
    LineReader reader = readLines(creditsText->shortName, creditsText->data, 0);
    while (loadNextLine(&reader)) {
        String line = getLine(&reader);
        V2I labelSize = UI::calculateLabelSize(line, labelStyle, maxLabelWidth);
        Rect2I labelBounds = irectAligned(position, labelSize, labelStyle->textAlignment);
        UI::putLabel(line, labelBounds, labelStyle);
        position.y += labelBounds.h;
    }

    UI::ButtonStyle* style = getStyle<UI::ButtonStyle>("default"_s);
    s32 uiBorderPadding = 8;
    String backText = getText("button_back"_s);
    V2I backSize = UI::calculateButtonSize(backText, style);
    Rect2I buttonRect = irectXYWH(uiBorderPadding, UI::windowSize.y - uiBorderPadding - backSize.y, backSize.x, backSize.y);
    if (UI::putTextButton(getText("button_back"_s), buttonRect, style, ButtonState::Normal)) {
        result = AppStatus::MainMenu;
    }

    return result;
}
