// immediate-ui.cpp

void setTooltip(GLRenderer *renderer, char *text, V4 color)
{
	strcpy(renderer->tooltip.text, text);
	renderer->tooltip.color = color;
	renderer->tooltip.show = true;
}

void drawTooltip(GLRenderer *renderer, InputState *inputState)
{
	if (renderer->tooltip.show)
	{
		TemporaryMemoryArena memory = beginTemporaryMemory(&renderer->renderArena);

		BitmapFontCachedText *textCache =
			drawTextToCache(&memory, renderer->theme.font, renderer->tooltip.text, renderer->tooltip.color);
		V2 topLeft = v2(inputState->mousePos) + renderer->tooltip.offsetFromCursor;
		RealRect bounds = rectXYWH(topLeft.x, topLeft.y, textCache->size.x + 8, textCache->size.y + 8);

		drawRect(renderer, true, bounds, 100, renderer->theme.tooltipBackgroundColor);
		drawCachedText(renderer, textCache, topLeft + v2(4,4), 101);

		endTemporaryMemory(&memory);

		renderer->tooltip.show = false;
	}
}

void uiLabel(GLRenderer *renderer, BitmapFont *font, char *text, V2 origin, int32 align, real32 depth, V4 color)
{
	TemporaryMemoryArena memory = beginTemporaryMemory(&renderer->renderArena);

	BitmapFontCachedText *textCache = drawTextToCache(&memory, font, text, color);
	V2 topLeft = calculateTextPosition(textCache, origin, align);
	drawCachedText(renderer, textCache, topLeft, depth);

	endTemporaryMemory(&memory);
}

bool uiButton(GLRenderer *renderer, InputState *inputState, char *text, RealRect bounds, real32 depth,
			SDL_Scancode shortcutKey=SDL_SCANCODE_UNKNOWN, char *tooltip=0)
{
	bool buttonClicked = false;

	V4 backColor = renderer->theme.buttonBackgroundColor;

	if (inRect(bounds, v2(inputState->mousePos)))
	{
		if (mouseButtonPressed(inputState, SDL_BUTTON_LEFT))
		{
			backColor = renderer->theme.buttonPressedColor;
		}
		else
		{
			backColor = renderer->theme.buttonHoverColor;
		}

		buttonClicked = mouseButtonJustReleased(inputState, SDL_BUTTON_LEFT);

		// tooltip!
		if (tooltip)
		{
			setTooltip(renderer, tooltip, renderer->theme.tooltipColorNormal);
		}
	}

	drawRect(renderer, true, bounds, depth, backColor);
	uiLabel(renderer, renderer->theme.buttonFont, text, centre(bounds), ALIGN_CENTRE, depth + 1,
			renderer->theme.buttonTextColor);

	return buttonClicked;
}