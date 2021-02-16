#pragma once

struct UIDrawable
{
	UIDrawable() {}
	UIDrawable(UIDrawableStyle *style) : style(style) {}

	void preparePlaceholder(RenderBuffer *buffer);
	void fillPlaceholder(Rect2I bounds);

	void draw(RenderBuffer *buffer, Rect2I bounds);

	UIDrawableStyle *style;

	union
	{
		DrawRectPlaceholder rectPlaceholder;
	};
};
