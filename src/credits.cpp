#pragma once

void updateAndRenderCredits(AppState *appState, InputState *inputState, Renderer *renderer, AssetManager *assets)
{
	AppStatus result = appState->appStatus;

	RenderBuffer *uiBuffer = &renderer->uiBuffer;
	real32 windowWidth = (real32) uiBuffer->camera.size.x;
	real32 windowHeight = (real32) uiBuffer->camera.size.y;
	UITheme *theme = &assets->theme;
	UIState *uiState = &appState->uiState;

	V2 position = v2(windowWidth * 0.5f, 157.0f);
	real32 maxLabelWidth = windowWidth - 256;

	UILabelStyle *labelStyle = &theme->labelStyle;
	BitmapFont *font = getFont(assets, labelStyle->font);

	position.y += (uiText(uiState, uiBuffer, font, LocalString("Under London"),
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, labelStyle->textColor, maxLabelWidth)).h;

	position.y += (uiText(uiState, uiBuffer, font, LocalString("Everything © Samuel Atkins 2017"),
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, labelStyle->textColor, maxLabelWidth)).h;
	position.y += (uiText(uiState, uiBuffer, font, LocalString("... except for the things that are not."),
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, labelStyle->textColor, maxLabelWidth)).h;

	real32 uiBorderPadding = 4;
	RealRect buttonRect = rectXYWH(uiBorderPadding, windowHeight - uiBorderPadding - 24, 80, 24);
	if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Back"), buttonRect, 1, false, SDLK_ESCAPE))
	{
		result = AppStatus_MainMenu;
	}

	appState->appStatus = result;
}
