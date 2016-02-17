// ui.cpp

void setTooltip(GLRenderer *renderer, char *text, V4 color)
{
	strncpy(renderer->tooltip.text, text, sizeof(renderer->tooltip.text));
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
			bool active=false, SDL_Scancode shortcutKey=SDL_SCANCODE_UNKNOWN, char *tooltip=0)
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
	else if (active)
	{
		backColor = renderer->theme.buttonHoverColor;
	}

	drawRect(renderer, true, bounds, depth, backColor);
	uiLabel(renderer, renderer->theme.buttonFont, text, centre(bounds), ALIGN_CENTRE, depth + 1,
			renderer->theme.buttonTextColor);

	// Keyboard shortcut!
	if ((shortcutKey != SDL_SCANCODE_UNKNOWN)
	&& keyJustPressed(inputState, shortcutKey))
	{
		buttonClicked = true;
	}

	return buttonClicked;
}

void pushUiMessage(GLRenderer *renderer, char *message)
{
	strncpy(renderer->message.text, message, sizeof(renderer->message.text));
	renderer->message.countdown = 2000;
}

void drawUiMessage(GLRenderer *renderer)
{
	if (renderer->message.countdown > 0)
	{
		renderer->message.countdown -= MS_PER_FRAME;

		if (renderer->message.countdown > 0)
		{
			TemporaryMemoryArena memory = beginTemporaryMemory(&renderer->renderArena);

			BitmapFontCachedText *textCache = drawTextToCache(&memory, renderer->theme.font,
											renderer->message.text, renderer->theme.tooltipColorNormal);
			V2 topLeft = calculateTextPosition(textCache,
				v2(renderer->worldCamera.windowWidth * 0.5f, renderer->worldCamera.windowHeight - 8.0f),
				ALIGN_H_CENTRE | ALIGN_BOTTOM );
			RealRect bounds = rectXYWH(topLeft.x - 4, topLeft.y - 4,
									   textCache->size.x + 8, textCache->size.y + 8);

			drawRect(renderer, true, bounds, 100, renderer->theme.tooltipBackgroundColor);
			drawCachedText(renderer, textCache, topLeft, 101);

			endTemporaryMemory(&memory);
		}
	}
}