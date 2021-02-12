#pragma once

void UIDrawable::preparePlaceholder(RenderBuffer *buffer)
{
	switch (style->type)
	{
		case Drawable_None:
		{
			return; // Nothing to do!
		} break;

		case Drawable_Color:
		{
			rectPlaceholder = appendDrawRectPlaceholder(buffer, renderer->shaderIds.untextured);
		} break;

		// case Drawable_Image:
		// {

		// } break;

		// case Drawable_Gradient:
		// {
		// 	rectPlaceholder = appendDrawRectPlaceholder(buffer, renderer->shaderIds.untextured);
		// } break;

		// case Drawable_Ninepatch:
		// {

		// } break;

		INVALID_DEFAULT_CASE;
	}
}

void UIDrawable::fillPlaceholder(Rect2I bounds)
{
	switch (style->type)
	{
		case Drawable_None:
		{
			return; // Nothing to do!
		} break;

		case Drawable_Color:
		{
			fillDrawRectPlaceholder(rectPlaceholder, bounds, style->color);
		} break;

		// case Drawable_Image:
		// {

		// } break;

		// case Drawable_Gradient:
		// {
		// 	fillDrawRectPlaceholder(rectPlaceholder, bounds, style->color);
		// } break;

		// case Drawable_Ninepatch:
		// {

		// } break;

		INVALID_DEFAULT_CASE;
	}
}

void UIDrawable::draw(RenderBuffer *buffer, Rect2I bounds)
{
	switch (style->type)
	{
		case Drawable_None:
		{
			return; // Nothing to do!
		} break;

		case Drawable_Color:
		{
			drawSingleRect(buffer, bounds, renderer->shaderIds.untextured, style->color);
		} break;

		// case Drawable_Image:
		// {

		// } break;

		// case Drawable_Gradient:
		// {
		// 	fillDrawRectPlaceholder(rectPlaceholder, bounds, style->color);
		// } break;

		// case Drawable_Ninepatch:
		// {

		// } break;

		INVALID_DEFAULT_CASE;
	}
}
