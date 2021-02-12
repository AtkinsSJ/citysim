#pragma once

void UIBackground::preparePlaceholder(RenderBuffer *buffer)
{
	switch (style->type)
	{
		case Background_None:
		{
			return; // Nothing to do!
		} break;

		case Background_Color:
		{
			rectPlaceholder = appendDrawRectPlaceholder(buffer, renderer->shaderIds.untextured);
		} break;

		// case Background_Image:
		// {

		// } break;

		// case Background_Gradient:
		// {
		// 	rectPlaceholder = appendDrawRectPlaceholder(buffer, renderer->shaderIds.untextured);
		// } break;

		// case Background_Ninepatch:
		// {

		// } break;

		INVALID_DEFAULT_CASE;
	}
}

void UIBackground::fillPlaceholder(Rect2I bounds)
{
	switch (style->type)
	{
		case Background_None:
		{
			return; // Nothing to do!
		} break;

		case Background_Color:
		{
			fillDrawRectPlaceholder(rectPlaceholder, bounds, style->color);
		} break;

		// case Background_Image:
		// {

		// } break;

		// case Background_Gradient:
		// {
		// 	fillDrawRectPlaceholder(rectPlaceholder, bounds, style->color);
		// } break;

		// case Background_Ninepatch:
		// {

		// } break;

		INVALID_DEFAULT_CASE;
	}
}
