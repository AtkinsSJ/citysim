#pragma once

namespace UI
{
	struct DrawableStyle;
	
	struct Drawable
	{
		Drawable() {}
		Drawable(DrawableStyle *style) : style(style) {}

		void preparePlaceholder(RenderBuffer *buffer);
		void fillPlaceholder(Rect2I bounds);

		void draw(RenderBuffer *buffer, Rect2I bounds);

		DrawableStyle *style;

		union
		{
			DrawRectPlaceholder      rectPlaceholder;
			DrawNinepatchPlaceholder ninepatchPlaceholder;
		};
	};
}
