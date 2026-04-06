/*
 * Copyright (c) 2016-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "MainMenu.h"
#include <Debug/Debug.h>
#include <Gfx/Renderer.h>
#include <Menus/About.h>
#include <Menus/Credits.h>
#include <Menus/SavedGames.h>
#include <Settings/Settings.h>
#include <Sim/Game.h>
#include <UI/Panel.h>

OwnedRef<MainMenuScene> MainMenuScene::create()
{
    return adopt_own(*new MainMenuScene);
}

void MainMenuScene::update_and_render(float delta_time)
{
    DEBUG_FUNCTION();

    auto& renderer = the_renderer();
    auto& app = App::the();

    auto window_size = renderer.window_size();
    UI::Panel panel = UI::Panel({ 0, window_size.y / 4, window_size.x, window_size.y }, "mainMenu"_s);

    panel.addLabel(getText("game_title"_s));
    panel.addLabel(getText("game_subtitle"_s));

    String newGameText = getText("button_new_game"_s);
    String loadText = getText("button_load"_s);
    String creditsText = getText("button_credits"_s);
    String settingsText = getText("button_settings"_s);
    String aboutText = getText("button_about"_s);
    String exitText = getText("button_exit"_s);

    // TODO: Some way of telling the panel to make these buttons all the same size.
    // Maybe we just create a child panel set to HAlign::Fill its widgets?
    /*
    UI::ButtonStyle *style = getStyle<UI::ButtonStyle>(panel.style->buttonStyle);

    V2I newGameSize  = UI::calculateButtonSize(newGameText,  style);
    V2I loadSize     = UI::calculateButtonSize(loadText,     style);
    V2I creditsSize  = UI::calculateButtonSize(creditsText,  style);
    V2I settingsSize = UI::calculateButtonSize(settingsText, style);
    V2I aboutSize    = UI::calculateButtonSize(aboutText,    style);
    V2I exitSize     = UI::calculateButtonSize(exitText,     style);

    s32 buttonWidth = max(
            newGameSize.x,
            loadSize.x,
            creditsSize.x,
            settingsSize.x,
            aboutSize.x,
            exitSize.x
    );
    s32 buttonHeight = max(
            newGameSize.y,
            loadSize.y,
            creditsSize.y,
            settingsSize.y,
            aboutSize.y,
            exitSize.y
    );
    */
    if (panel.addTextButton(newGameText)) {
        // FIXME: Make seed configurable, or at least not fixed!
        app.switch_to_scene(GameScene::create_new(12345));
    }
    if (panel.addTextButton(loadText)) {
        showLoadGameWindow();
    }
    if (panel.addTextButton(creditsText)) {
        app.switch_to_scene(CreditsScene::create());
    }
    if (panel.addTextButton(settingsText)) {
        showSettingsWindow();
    }
    if (panel.addTextButton(aboutText)) {
        showAboutWindow();
    }
    if (panel.addTextButton(exitText)) {
        input_state().receivedQuitSignal = true;
    }

    panel.end(true);
}
