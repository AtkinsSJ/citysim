#pragma once

AppStatus updateAndRenderMainMenu(UIState *uiState)
{
	DEBUG_FUNCTION();
	
	AppStatus result = AppStatus_MainMenu;

	s32 windowWidth = round_s32(renderer->uiCamera.size.x);
	UITheme *theme = &assets->theme;

	V2I position = v2i(windowWidth / 2, 157);
	s32 maxLabelWidth = windowWidth - 256;

	UILabelStyle *labelStyle = findLabelStyle(theme, "title"_s);
	BitmapFont *font = getFont(labelStyle->fontName);

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
		result = AppStatus_Game;
	}
	buttonRect.y += buttonHeight + buttonPadding;
	if (uiButton(uiState, loadText, buttonRect, style))
	{
		City *city = &globalAppState.gameState->city;
		FileHandle saveFile = openFile("whatever.sav\0"_s, FileAccess_Read);
		bool loadSucceeded = loadSaveFile(&saveFile, city);
		closeFile(&saveFile);

		if (loadSucceeded)
		{
			pushUiMessage(uiState, myprintf(getText("msg_load_success"_s), {saveFile.path}));
		}
		else
		{
			pushUiMessage(uiState, myprintf(getText("msg_load_failure"_s), {saveFile.path}));
		}
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

	return result;
}
