#pragma once

AppStatus updateAndRenderMainMenu(UIState *uiState, f32 /*deltaTime*/)
{
	DEBUG_FUNCTION();
	
	AppStatus result = AppStatus_MainMenu;

	s32 windowWidth = round_s32(renderer->uiCamera.size.x);
	// s32 windowHeight = round_s32(renderer->uiCamera.size.y);
	UITheme *theme = &assets->theme;

	V2I position = v2i(windowWidth / 2, 157);
	s32 maxLabelWidth = windowWidth - 256;

	UILabelStyle *labelStyle = findLabelStyle(theme, "title"_s);
	BitmapFont *font = getFont(&labelStyle->font);

	position.y += (uiText(&renderer->uiBuffer, font, getText("game_title"_s),
			position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;

	position.y += (uiText(&renderer->uiBuffer, font, getText("game_subtitle"_s),
			position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;

	UIButtonStyle *style = findButtonStyle(&assets->theme, "default"_s);

	s32 buttonPadding = 8;

	String newGameText  = getText("button_new_game"_s);
	String loadText     = getText("button_load"_s);
	String creditsText  = getText("button_credits"_s);
	String settingsText = getText("button_settings"_s);
	String aboutText    = getText("button_about"_s);
	String exitText     = getText("button_exit"_s);

	V2I newGameSize  = calculateButtonSize(newGameText,  style);
	V2I loadSize     = calculateButtonSize(loadText,     style);
	V2I creditsSize  = calculateButtonSize(creditsText,  style);
	V2I settingsSize = calculateButtonSize(settingsText, style);
	V2I aboutSize    = calculateButtonSize(aboutText,    style);
	V2I exitSize     = calculateButtonSize(exitText,     style);

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

	Rect2I buttonRect = irectXYWH(position.x - (buttonWidth/2), position.y + buttonPadding, buttonWidth, buttonHeight);
	if (uiButton(uiState, newGameText, buttonRect, style))
	{
		beginNewGame();

		result = AppStatus_Game;
	}
	buttonRect.y += buttonHeight + buttonPadding;
	if (uiButton(uiState, loadText, buttonRect, style))
	{
		showLoadGameWindow(uiState);
	}
	buttonRect.y += buttonHeight + buttonPadding;
	if (uiButton(uiState, creditsText, buttonRect, style))
	{
		result = AppStatus_Credits;
	}
	buttonRect.y += buttonHeight + buttonPadding;
	if (uiButton(uiState, settingsText, buttonRect, style))
	{
		result = AppStatus_SettingsMenu;
	}
	buttonRect.y += buttonHeight + buttonPadding;
	if (uiButton(uiState, aboutText, buttonRect, style))
	{
		showAboutWindow(uiState);
	}
	buttonRect.y += buttonHeight + buttonPadding;
	if (uiButton(uiState, exitText, buttonRect, style))
	{
		result = AppStatus_Quit;
	}

	// This is a copy of savedGamesWindowProc() in our new system, for testing
	UIPanel testPanel(irectXYWH(32, 4, 780, 580));
	{
		UIPanel bottomBar = testPanel.row(26, ALIGN_BOTTOM, "plain"_s);

		testPanel.addText("This is a test!"_s);
		if (testPanel.addButton("And a button!"_s))
		{
			pushUiMessage(uiState, "You clicked a thing!"_s);
		}

		// Bottom bar
		{
			if (false) //isSaveWindow)
			{

			}
			else
			{
				bottomBar.alignWidgets(ALIGN_LEFT);
				if (bottomBar.addButton(getText("button_back"_s)))
				{
					// context->closeRequested = true;
				}

				bottomBar.alignWidgets(ALIGN_RIGHT);
				if (bottomBar.addButton(getText("button_load"_s), -1))//, selectedSavedGame ? Button_Normal : Button_Disabled))
				{
					// loadGame(context->uiState, selectedSavedGame);
					// context->closeRequested = true;
				}
			}
		}
		bottomBar.endPanel();
	}
	testPanel.endPanel();

	return result;
}
