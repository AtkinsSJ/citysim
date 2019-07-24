#pragma once

void updateAndRenderMainMenu(AppState *appState, Renderer *renderer, AssetManager *assets)
{
	DEBUG_FUNCTION();
	
	AppStatus result = appState->appStatus;

	RenderBuffer *uiBuffer = &renderer->uiBuffer;

	f32 windowWidth  = (f32) uiBuffer->camera.size.x;
	f32 windowHeight = (f32) uiBuffer->camera.size.y;
	UITheme *theme = &assets->theme;
	UIState *uiState = &appState->uiState;

	drawSingleRect(uiBuffer, rectXYWH(0, 0, windowWidth, windowHeight), renderer->shaderIds.untextured, theme->overlayColor);

	V2 position = v2(windowWidth * 0.5f, 157.0f);
	f32 maxLabelWidth = windowWidth - 256;

	UILabelStyle *labelStyle = findLabelStyle(theme, makeString("title"));
	BitmapFont *font = getFont(assets, labelStyle->fontName);

	position.y += (uiText(renderer, uiState->uiBuffer, font, LOCAL("game_title"),
			position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;

	position.y += (uiText(renderer, uiState->uiBuffer, font, LOCAL("game_subtitle"),
			position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;

	position.y += (uiText(renderer, uiState->uiBuffer, font, makeString("This\r\nis\r\na\r\nnon-localised\r\ntest\r\nstring.\r\nIt has multiple lines, of\r\ndifferent length\r\nto test\r\nthe alignment on multi-line strings."),
			position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;

	Rect2 buttonRect = rectXYWH(position.x - (80/2), position.y + 32, 80, 24);
	if (uiButton(uiState, renderer, LOCAL("button_play"), buttonRect)) // , SDLK_RETURN
	{
		result = AppStatus_Game;
	}
	buttonRect.y += 32;
	if (uiButton(uiState, renderer, LOCAL("button_credits"), buttonRect))
	{
		result = AppStatus_Credits;
	}
	buttonRect.y += 32;
	if (uiButton(uiState, renderer, LOCAL("button_settings"), buttonRect))
	{
		result = AppStatus_SettingsMenu;
	}
	buttonRect.y += 32;
	if (uiButton(uiState, renderer, LOCAL("button_about"), buttonRect))
	{
		showAboutWindow(uiState);
	}
	buttonRect.y += 32;
	if (uiButton(uiState, renderer, LOCAL("button_exit"), buttonRect))
	{
		result = AppStatus_Quit;
	}

	UILabelStyle *liLabelStyle = findLabelStyle(theme, makeString("small"));
	uiText(renderer, uiState->uiBuffer, getFont(assets, liLabelStyle->fontName), LOCAL("lorem_ipsum"),
			v2(0.0f,0.0f), ALIGN_LEFT | ALIGN_TOP,
			// v2(windowWidth * 0.5f,0.0f), ALIGN_H_CENTRE | ALIGN_TOP,
			// v2(windowWidth,0.0f), ALIGN_RIGHT | ALIGN_TOP,
			color255(255, 255, 255, 60), windowWidth);

	appState->appStatus = result;
}
