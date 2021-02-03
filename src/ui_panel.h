#pragma once

// Going to try out using a more OOP style.
// I didn't like putting "window_" at the start of all the Window-UI functions, or
// having to pass it as the parameter all the time.
// Maybe this will feel awkward too, who knows.

struct UIPanel
{
	UIPanel(Rect2I bounds, UIPanelStyle *style);
	UIPanel(Rect2I bounds, String styleName) : UIPanel(bounds, findPanelStyle(&assets->theme, styleName)) {}

	void addText(String text, String styleName = nullString);
	bool addButton(String text, s32 textWidth = -1, ButtonState state = Button_Normal, String styleName = nullString);

	void startNewLine(u32 hAlignment = 0);

	void endPanel();

	Rect2I bounds;
	Rect2I contentArea;
	u32 widgetAlignment;
	s32 currentY;
	s32 currentLeft;
	s32 currentRight;
	s32 largestItemWidth;
	s32 largestItemHeightOnLine;

private:
	void completeWidget(V2I widgetSize);
	void init();
};
