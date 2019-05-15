#pragma once

void updateAndRenderCredits(AppState *appState, Renderer *renderer, AssetManager *assets)
{
	AppStatus result = appState->appStatus;

	RenderBuffer *uiBuffer = &renderer->uiBuffer;
	f32 windowWidth = (f32) uiBuffer->camera.size.x;
	f32 windowHeight = (f32) uiBuffer->camera.size.y;
	UIState *uiState = &appState->uiState;
	uiState->mouseInputHandled = false;

	V2 position = v2(windowWidth * 0.5f, 157.0f);
	f32 maxLabelWidth = windowWidth - 256;

	UILabelStyle *labelStyle = findLabelStyle(&assets->theme, makeString("title"));
	BitmapFont *font = getFont(assets, labelStyle->fontName);

	Asset *creditsText = getAsset(assets, AssetType_Misc, makeString("credits.txt"));
	LineReader reader = readLines(creditsText->shortName, creditsText->data, false, false);
	while (!isDone(&reader))
	{
		String line = nextLine(&reader);
		position.y += (uiText(uiState, font, line,
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, labelStyle->textColor, maxLabelWidth)).h;
	}

	f32 uiBorderPadding = 4;
	Rect2 buttonRect = rectXYWH(uiBorderPadding, windowHeight - uiBorderPadding - 24, 80, 24);
	if (uiButton(uiState, LocalString("Back"), buttonRect, 1, false, SDLK_ESCAPE))
	{
		result = AppStatus_MainMenu;
	}

	appState->appStatus = result;
}
