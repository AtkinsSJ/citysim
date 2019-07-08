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

	drawRect(uiBuffer, rectXYWH(0, 0, windowWidth, windowHeight), 0, uiState->untexturedShaderID, theme->overlayColor);

	V2 position = v2(windowWidth * 0.5f, 157.0f);
	f32 maxLabelWidth = windowWidth - 256;

	UILabelStyle *labelStyle = findLabelStyle(theme, makeString("title"));
	BitmapFont *font = getFont(assets, labelStyle->fontName);

	position.y += (uiText(uiState, font, LOCAL("game_title"),
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, labelStyle->textColor, maxLabelWidth)).h;

	position.y += (uiText(uiState, font, LOCAL("game_subtitle"),
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, labelStyle->textColor, maxLabelWidth)).h;

	position.y += (uiText(uiState, font, makeString("This\nis\na\nnon-localised\ntest\nstring.\nIt has multiple lines, of\ndifferent length\nto test\nthe alignment on multi-line strings."),
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, labelStyle->textColor, maxLabelWidth)).h;

	Rect2 buttonRect = rectXYWH(position.x - (80/2), position.y + 32, 80, 24);
	if (uiButton(uiState, LOCAL("button_play"), buttonRect, 1)) // , SDLK_RETURN
	{
		result = AppStatus_Game;
	}
	buttonRect.y += 32;
	if (uiButton(uiState, LOCAL("button_credits"), buttonRect, 1))
	{
		result = AppStatus_Credits;
	}
	buttonRect.y += 32;
	if (uiButton(uiState, LOCAL("button_settings"), buttonRect, 1))
	{
		result = AppStatus_SettingsMenu;
	}
	buttonRect.y += 32;
	if (uiButton(uiState, LOCAL("button_about"), buttonRect, 1))
	{
		showAboutWindow(uiState);
	}
	buttonRect.y += 32;
	if (uiButton(uiState, LOCAL("button_exit"), buttonRect, 1))
	{
		result = AppStatus_Quit;
	}

	UILabelStyle *liLabelStyle = findLabelStyle(theme, makeString("small"));
	uiText(uiState, getFont(assets, liLabelStyle->fontName), LOCAL("lorem_ipsum"),
			v2(0.0f,0.0f), ALIGN_LEFT | ALIGN_TOP,
			// v2(windowWidth * 0.5f,0.0f), ALIGN_H_CENTRE | ALIGN_TOP,
			// v2(windowWidth,0.0f), ALIGN_RIGHT | ALIGN_TOP,
			1, color255(255, 255, 255, 60), windowWidth);

	appState->appStatus = result;
}
