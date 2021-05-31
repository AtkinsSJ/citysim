#pragma once

AppStatus updateAndRenderCredits(f32 /*deltaTime*/)
{
	AppStatus result = AppStatus_Credits;

	V2I position = v2i(UI::windowSize.x / 2, 157);
	s32 maxLabelWidth = UI::windowSize.x - 256;

	UILabelStyle *labelStyle = findStyle<UILabelStyle>("title"_s);
	BitmapFont *font = getFont(&labelStyle->font);

	Asset *creditsText = getAsset(AssetType_Misc, "credits.txt"_s);
	LineReader reader = readLines(creditsText->shortName, creditsText->data, 0);
	while (loadNextLine(&reader))
	{
		String line = getLine(&reader);
		position.y += (UI::drawText(&renderer->uiBuffer, font, line,
			position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;
	}

	UIButtonStyle *style = findStyle<UIButtonStyle>("default"_s);
	s32 uiBorderPadding = 8;
	String backText = getText("button_back"_s);
	V2I backSize = UI::calculateButtonSize(backText, style);
	Rect2I buttonRect = irectXYWH(uiBorderPadding, UI::windowSize.y - uiBorderPadding - backSize.y, backSize.x, backSize.y);
	if (UI::putTextButton(getText("button_back"_s), buttonRect, style, Button_Normal))
	{
		result = AppStatus_MainMenu;
	}

	return result;
}
