#pragma once

AppStatus updateAndRenderMainMenu(f32 /*deltaTime*/)
{
	DEBUG_FUNCTION();
	
	AppStatus result = AppStatus_MainMenu;

	V2I position = v2i(inputState->windowWidth / 2, 157);
	s32 maxLabelWidth = inputState->windowWidth - 256;

	UILabelStyle *labelStyle = findStyle<UILabelStyle>("title"_s);
	BitmapFont *font = getFont(&labelStyle->font);

	position.y += (UI::drawText(&renderer->uiBuffer, font, getText("game_title"_s),
			position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;

	position.y += (UI::drawText(&renderer->uiBuffer, font, getText("game_subtitle"_s),
			position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;

	UIButtonStyle *style = findStyle<UIButtonStyle>("default"_s);

	s32 buttonPadding = 8;

	String newGameText  = getText("button_new_game"_s);
	String loadText     = getText("button_load"_s);
	String creditsText  = getText("button_credits"_s);
	String settingsText = getText("button_settings"_s);
	String aboutText    = getText("button_about"_s);
	String exitText     = getText("button_exit"_s);

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

	Rect2I buttonRect = irectXYWH(position.x - (buttonWidth/2), position.y + buttonPadding, buttonWidth, buttonHeight);
	if (UI::putButton(newGameText, buttonRect, style))
	{
		beginNewGame();

		result = AppStatus_Game;
	}
	buttonRect.y += buttonHeight + buttonPadding;
	if (UI::putButton(loadText, buttonRect, style))
	{
		showLoadGameWindow();
	}
	buttonRect.y += buttonHeight + buttonPadding;
	if (UI::putButton(creditsText, buttonRect, style))
	{
		result = AppStatus_Credits;
	}
	buttonRect.y += buttonHeight + buttonPadding;
	if (UI::putButton(settingsText, buttonRect, style))
	{
		showSettingsWindow();
	}
	buttonRect.y += buttonHeight + buttonPadding;
	if (UI::putButton(aboutText, buttonRect, style))
	{
		showAboutWindow();
	}
	buttonRect.y += buttonHeight + buttonPadding;
	if (UI::putButton(exitText, buttonRect, style))
	{
		result = AppStatus_Quit;
	}
	
	return result;
}
