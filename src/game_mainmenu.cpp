#pragma once

AppStatus updateAndRenderMainMenu(UIState *uiState)
{
	DEBUG_FUNCTION();
	
	AppStatus result = AppStatus_MainMenu;

	s32 windowWidth = round_s32(renderer->uiCamera.size.x);
	UITheme *theme = &assets->theme;

	// Debug text for profiling text rendering
	UILabelStyle *liLabelStyle = findLabelStyle(theme, "small"_s);
	uiText(&renderer->uiBuffer, getFont(liLabelStyle->fontName), getText("lorem_ipsum"_s),
			v2i(0,0), ALIGN_LEFT | ALIGN_TOP,
			// v2(windowWidth * 0.5f,0.0f), ALIGN_H_CENTRE | ALIGN_TOP,
			// v2(windowWidth,0.0f), ALIGN_RIGHT | ALIGN_TOP,
			color255(255, 255, 255, 60), windowWidth);

	V2I position = v2i(windowWidth / 2, 157);
	s32 maxLabelWidth = windowWidth - 256;

	UILabelStyle *labelStyle = findLabelStyle(theme, "title"_s);
	BitmapFont *font = getFont(labelStyle->fontName);

	position.y += (uiText(&renderer->uiBuffer, font, getText("game_title"_s),
			position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;

	position.y += (uiText(&renderer->uiBuffer, font, getText("game_subtitle"_s),
			position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;

	position.y += (uiText(&renderer->uiBuffer, font, "This\r\nis\r\na\r\nnon-localised\r\ntest\r\nstring.\r\nIt has multiple lines, of\r\ndifferent length\r\nto test\r\nthe alignment on multi-line strings."_s,
			position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;


	UIButtonStyle *style = findButtonStyle(&assets->theme, "default"_s);

	s32 buttonPadding = 8;

	String playText     = getText("button_play"_s);
	String creditsText  = getText("button_credits"_s);
	String settingsText = getText("button_settings"_s);
	String aboutText    = getText("button_about"_s);
	String exitText     = getText("button_exit"_s);

	V2I playSize     = calculateButtonSize(playText,     style);
	V2I creditsSize  = calculateButtonSize(creditsText,  style);
	V2I settingsSize = calculateButtonSize(settingsText, style);
	V2I aboutSize    = calculateButtonSize(aboutText,    style);
	V2I exitSize     = calculateButtonSize(exitText,     style);

	s32 buttonWidth = max(
		playSize.x,
		creditsSize.x,
		settingsSize.x,
		aboutSize.x,
		exitSize.x
	);
	s32 buttonHeight = max(
		playSize.y,
		creditsSize.y,
		settingsSize.y,
		aboutSize.y,
		exitSize.y
	);

	Rect2I buttonRect = irectXYWH(position.x - (buttonWidth/2), position.y + buttonPadding, buttonWidth, buttonHeight);
	if (uiButton(uiState, playText, buttonRect, style)) // , SDLK_RETURN
	{
		result = AppStatus_Game;
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
