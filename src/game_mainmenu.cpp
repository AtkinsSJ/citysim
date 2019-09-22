#pragma once

AppStatus updateAndRenderMainMenu(UIState *uiState)
{
	DEBUG_FUNCTION();
	
	AppStatus result = AppStatus_MainMenu;

	s32 windowWidth  = renderer->uiCamera.size.x;
	UITheme *theme = &assets->theme;

	// Debug text for profiling text rendering
	UILabelStyle *liLabelStyle = findLabelStyle(theme, "small"s);
	uiText(&renderer->uiBuffer, getFont(liLabelStyle->fontName), LOCAL("lorem_ipsum"),
			v2i(0,0), ALIGN_LEFT | ALIGN_TOP,
			// v2(windowWidth * 0.5f,0.0f), ALIGN_H_CENTRE | ALIGN_TOP,
			// v2(windowWidth,0.0f), ALIGN_RIGHT | ALIGN_TOP,
			color255(255, 255, 255, 60), windowWidth);

	V2I position = v2i(windowWidth / 2, 157);
	s32 maxLabelWidth = windowWidth - 256;

	UILabelStyle *labelStyle = findLabelStyle(theme, "title"s);
	BitmapFont *font = getFont(labelStyle->fontName);

	position.y += (uiText(&renderer->uiBuffer, font, LOCAL("game_title"),
			position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;

	position.y += (uiText(&renderer->uiBuffer, font, LOCAL("game_subtitle"),
			position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;

	position.y += (uiText(&renderer->uiBuffer, font, "This\r\nis\r\na\r\nnon-localised\r\ntest\r\nstring.\r\nIt has multiple lines, of\r\ndifferent length\r\nto test\r\nthe alignment on multi-line strings."s,
			position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;

	Rect2I buttonRect = irectXYWH(position.x - (80/2), position.y + 32, 80, 24);
	if (uiButton(uiState, LOCAL("button_play"), buttonRect)) // , SDLK_RETURN
	{
		result = AppStatus_Game;
	}
	buttonRect.y += 32;
	if (uiButton(uiState, LOCAL("button_credits"), buttonRect))
	{
		result = AppStatus_Credits;
	}
	buttonRect.y += 32;
	if (uiButton(uiState, LOCAL("button_settings"), buttonRect))
	{
		result = AppStatus_SettingsMenu;
	}
	buttonRect.y += 32;
	if (uiButton(uiState, LOCAL("button_about"), buttonRect))
	{
		showAboutWindow(uiState);
	}
	buttonRect.y += 32;
	if (uiButton(uiState, LOCAL("button_exit"), buttonRect))
	{
		result = AppStatus_Quit;
	}

	return result;
}
