#pragma once

AppStatus updateAndRenderCredits(UIState *uiState)
{
	AppStatus result = AppStatus_Credits;

	f32 windowWidth = (f32) renderer->uiCamera.size.x;
	f32 windowHeight = (f32) renderer->uiCamera.size.y;

	V2 position = v2(windowWidth * 0.5f, 157.0f);
	f32 maxLabelWidth = windowWidth - 256;

	UILabelStyle *labelStyle = findLabelStyle(&assets->theme, makeString("title"));
	BitmapFont *font = getFont(labelStyle->fontName);

	Asset *creditsText = getAsset(AssetType_Misc, makeString("credits.txt"));
	LineReader_Old reader = readLines_old(creditsText->shortName, creditsText->data, 0);
	while (!isDone(&reader))
	{
		String line = nextLine(&reader);
		position.y += (uiText(&renderer->uiBuffer, font, line,
			position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;
	}

	f32 uiBorderPadding = 4;
	Rect2 buttonRect = rectXYWH(uiBorderPadding, windowHeight - uiBorderPadding - 24, 80, 24);
	if (uiButton(uiState, LOCAL("button_back"), buttonRect, false, SDLK_ESCAPE))
	{
		result = AppStatus_MainMenu;
	}

	return result;
}
