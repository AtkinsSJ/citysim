#pragma once

void updateAndRenderMainMenu(AppState *appState, InputState *inputState, Renderer *renderer, AssetManager *assets)
{
	DEBUG_FUNCTION();
	
	AppStatus result = appState->appStatus;

	real32 windowWidth = (real32) renderer->uiBuffer.camera.size.x;
	real32 windowHeight = (real32) renderer->uiBuffer.camera.size.y;
	UITheme *theme = &assets->theme;
	UIState *uiState = &appState->uiState;

	drawRect(&renderer->uiBuffer, rectXYWH(0, 0, windowWidth, windowHeight), 0, theme->overlayColor);

	V2 position = v2(windowWidth * 0.5f, 157.0f);
	real32 maxLabelWidth = windowWidth - 256;

	//drawGL_TextureAtlasItem(renderer, true, GL_TextureAtlasItem_Menu_Logo, position, v2(499.0f, 154.0f), 0);
	//position.y += 154.0f;

	UILabelStyle *labelStyle = &theme->labelStyle;
	BitmapFont *font = getFont(assets, labelStyle->font);

	position.y += (uiText(uiState, renderer, font, LocalString("Under London"),
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, labelStyle->textColor, maxLabelWidth)).h;

	position.y += (uiText(uiState, renderer, font, LocalString("Very much a work in progress!"),
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, labelStyle->textColor, maxLabelWidth)).h;

	RealRect buttonRect = rectXYWH(position.x - (80/2), position.y + 32, 80, 24);
	if (uiButton(uiState, renderer, assets, inputState, LocalString("Play"), buttonRect, 1)) // , SDLK_RETURN
	{
		result = AppStatus_Game;
	}
	buttonRect.y += 32;
	if (uiButton(uiState, renderer, assets, inputState, LocalString("Credits"), buttonRect, 1))
	{
		result = AppStatus_Credits;
	}
	buttonRect.y += 32;
	if (uiButton(uiState, renderer, assets, inputState, LocalString("Settings"), buttonRect, 1))
	{
		result = AppStatus_SettingsMenu;
	}
	buttonRect.y += 32;
	if (uiButton(uiState, renderer, assets, inputState, LocalString("Website"), buttonRect, 1))
	{
		openUrlUnsafe("http://samatkins.co.uk");
	}
	buttonRect.y += 32;
	if (uiButton(uiState, renderer, assets, inputState, LocalString("Exit"), buttonRect, 1))
	{
		result = AppStatus_Quit;
	}

	appState->appStatus = result;
}
