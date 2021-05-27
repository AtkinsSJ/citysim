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
			rectPlaceholder = appendDrawRectPlaceholder(buffer, renderer->shaderIds.untextured, false);
		} break;

		case Drawable_Gradient:
		{
			rectPlaceholder = appendDrawRectPlaceholder(buffer, renderer->shaderIds.untextured, false);
		} break;

		case Drawable_Ninepatch:
		{
			ninepatchPlaceholder = appendDrawNinepatchPlaceholder(buffer, getAsset(&style->ninepatch)->ninepatch.texture, renderer->shaderIds.textured);
		} break;

		case Drawable_Sprite:
		{
			rectPlaceholder = appendDrawRectPlaceholder(buffer, renderer->shaderIds.pixelArt, true);
		} break;

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
			fillDrawRectPlaceholder(&rectPlaceholder, bounds, style->color);
		} break;

		case Drawable_Gradient:
		{
			fillDrawRectPlaceholder(&rectPlaceholder, bounds, style->gradient.color00, style->gradient.color01, style->gradient.color10, style->gradient.color11);
		} break;

		case Drawable_Ninepatch:
		{
			fillDrawNinepatchPlaceholder(&ninepatchPlaceholder, bounds, &getAsset(&style->ninepatch)->ninepatch, style->color);
		} break;

		case Drawable_Sprite:
		{
			fillDrawRectPlaceholder(&rectPlaceholder, rect2(bounds), getSprite(&style->sprite), style->color);
		} break;

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

		case Drawable_Gradient:
		{
			drawSingleRect(buffer, bounds, renderer->shaderIds.untextured, style->gradient.color00, style->gradient.color01, style->gradient.color10, style->gradient.color11);
		} break;

		case Drawable_Ninepatch:
		{
			drawNinepatch(buffer, bounds, renderer->shaderIds.textured, &getAsset(&style->ninepatch)->ninepatch, style->color);
		} break;

		case Drawable_Sprite:
		{
			drawSingleSprite(buffer, getSprite(&style->sprite), rect2(bounds), renderer->shaderIds.textured, style->color);
		} break;

		INVALID_DEFAULT_CASE;
	}
}
