/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "about.h"
#include "platform.h"
#include <UI/Window.h>
#include <Util/Vector.h>

static void aboutWindowProc(UI::WindowContext* context, void* /*userData*/)
{
    UI::Panel* ui = &context->windowPanel;

    UI::Panel leftColumn = ui->column(64, ALIGN_LEFT, "plain"_s);
    leftColumn.alignWidgets(ALIGN_H_CENTRE);
    leftColumn.addSprite(getSprite("b_hospital"_s));
    leftColumn.end();

    ui->addLabel(getText("game_title"_s), "title"_s);

    ui->startNewLine(ALIGN_RIGHT);
    ui->addLabel(getText("game_copyright"_s));

    ui->startNewLine(ALIGN_EXPAND_H);

    if (ui->addTextButton(getText("button_website"_s))) {
        openUrlUnsafe("http://samatkins.co.uk");
    }
}

void showAboutWindow()
{
    UI::showWindow(UI::WindowTitle::fromTextAsset("title_about"_s), 300, 200, v2i(0, 0), "default"_s, WindowFlags::Unique | WindowFlags::Modal | WindowFlags::AutomaticHeight, aboutWindowProc);
}
