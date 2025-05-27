#pragma once

AppStatus updateAndRenderMainMenu(f32 /*deltaTime*/)
{
    DEBUG_FUNCTION();

    AppStatus result = AppStatus_MainMenu;

    UI::Panel panel = UI::Panel(irectXYWH(0, renderer->windowHeight / 4, renderer->windowWidth, renderer->windowHeight), "mainMenu"_s);

    panel.addLabel(getText("game_title"_s));
    panel.addLabel(getText("game_subtitle"_s));

    String newGameText = getText("button_new_game"_s);
    String loadText = getText("button_load"_s);
    String creditsText = getText("button_credits"_s);
    String settingsText = getText("button_settings"_s);
    String aboutText = getText("button_about"_s);
    String exitText = getText("button_exit"_s);

    // TODO: Some way of telling the panel to make these buttons all the same size.
    // Maybe we just create a child panel set to ALIGN_EXPAND_H its widgets?
    /*
    UI::ButtonStyle *style = getStyle<UI::ButtonStyle>(&panel.style->buttonStyle);

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
        beginNewGame();

        result = AppStatus_Game;
    }
    if (panel.addTextButton(loadText)) {
        showLoadGameWindow();
    }
    if (panel.addTextButton(creditsText)) {
        result = AppStatus_Credits;
    }
    if (panel.addTextButton(settingsText)) {
        showSettingsWindow();
    }
    if (panel.addTextButton(aboutText)) {
        showAboutWindow();
    }
    if (panel.addTextButton(exitText)) {
        result = AppStatus_Quit;
    }

    panel.end(true);

    return result;
}
