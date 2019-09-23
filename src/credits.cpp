#pragma once

AppStatus updateAndRenderCredits(UIState *uiState)
{
	AppStatus result = AppStatus_Credits;

	s32 windowWidth  = round_s32(renderer->uiCamera.size.x);
	s32 windowHeight = round_s32(renderer->uiCamera.size.y);

	V2I position = v2i(windowWidth / 2, 157);
	s32 maxLabelWidth = windowWidth - 256;

	UILabelStyle *labelStyle = findLabelStyle(&assets->theme, "title"s);
	BitmapFont *font = getFont(labelStyle->fontName);

	Asset *creditsText = getAsset(AssetType_Misc, "credits.txt"s);
	LineReader reader = readLines(creditsText->shortName, creditsText->data, 0);
	while (loadNextLine(&reader))
	{
		String line = getLine(&reader);
		position.y += (uiText(&renderer->uiBuffer, font, line,
			position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;
	}

	UIButtonStyle *style = findButtonStyle(&assets->theme, "default"s);
	s32 uiBorderPadding = 8;
	String backText = LOCAL("button_back");
	V2I backSize = calculateButtonSize(backText, style);
	Rect2I buttonRect = irectXYWH(uiBorderPadding, windowHeight - uiBorderPadding - backSize.y, backSize.x, backSize.y);
	if (uiButton(uiState, LOCAL("button_back"), buttonRect, style, false, SDLK_ESCAPE))
	{
		result = AppStatus_MainMenu;
	}

	return result;
}
