#pragma once

void updateAndRenderMainMenu(AppState *appState, InputState *inputState, Renderer *renderer, AssetManager *assets)
{
	DEBUG_FUNCTION();
	
	AppStatus result = appState->appStatus;

	RenderBuffer *uiBuffer = &renderer->uiBuffer;

	f32 windowWidth = (f32) uiBuffer->camera.size.x;
	f32 windowHeight = (f32) uiBuffer->camera.size.y;
	UITheme *theme = &assets->theme;
	UIState *uiState = &appState->uiState;

	drawRect(uiBuffer, rectXYWH(0, 0, windowWidth, windowHeight), 0, theme->overlayColor);

	V2 position = v2(windowWidth * 0.5f, 157.0f);
	f32 maxLabelWidth = windowWidth - 256;

	//drawGL_TextureAtlasItem(renderer, true, GL_TextureAtlasItem_Menu_Logo, position, v2(499.0f, 154.0f), 0);
	//position.y += 154.0f;

	UILabelStyle *labelStyle = &theme->labelStyle;
	BitmapFont *font = getFont(assets, labelStyle->font);

	position.y += (uiText(uiState, uiBuffer, font, LocalString("City Builder Thing"),
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, labelStyle->textColor, maxLabelWidth)).h;

	position.y += (uiText(uiState, uiBuffer, font, LocalString("Very much a work in progress!"),
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, labelStyle->textColor, maxLabelWidth)).h;

	Rect2 buttonRect = rectXYWH(position.x - (80/2), position.y + 32, 80, 24);
	if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Play"), buttonRect, 1)) // , SDLK_RETURN
	{
		result = AppStatus_Game;
	}
	buttonRect.y += 32;
	if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Credits"), buttonRect, 1))
	{
		result = AppStatus_Credits;
	}
	buttonRect.y += 32;
	if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Settings"), buttonRect, 1))
	{
		result = AppStatus_SettingsMenu;
	}
	buttonRect.y += 32;
	if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Website"), buttonRect, 1))
	{
		openUrlUnsafe("http://samatkins.co.uk");
	}
	buttonRect.y += 32;
	if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Exit"), buttonRect, 1))
	{
		result = AppStatus_Quit;
	}

	appState->appStatus = result;
}
