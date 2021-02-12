#pragma once

struct UIBackground
{
	UIBackground() {}
	UIBackground(UIBackgroundStyle *style) : style(style) {}

	void preparePlaceholder(RenderBuffer *buffer);
	void fillPlaceholder(Rect2I bounds);

	void draw(RenderBuffer *buffer, Rect2I bounds);

	UIBackgroundStyle *style;

	union
	{
		RenderItem_DrawSingleRect *rectPlaceholder;
	};
};
