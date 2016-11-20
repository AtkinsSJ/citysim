// ui.cpp

void initUiState(UIState *uiState)
{
	*uiState = {};
	uiState->actionMode = ActionMode_None;
	uiState->selectedBuildingArchetype = BA_None;

	uiState->tooltip = {};
	uiState->tooltip.offsetFromCursor = v2(16, 20);
}

RealRect uiLabel(GL_Renderer *renderer, BitmapFont *font, char *text, V2 origin, int32 align,
				 real32 depth, V4 color, real32 maxWidth = 0)
{

	TemporaryMemoryArena memory = beginTemporaryMemory(&renderer->renderArena);

	BitmapFontCachedText *textCache = drawTextToCache(&memory, font, text, color, maxWidth);
	V2 topLeft = calculateTextPosition(textCache, origin, align);
	drawCachedText(renderer, textCache, topLeft, depth);
	RealRect bounds = rectXYWH(topLeft.x, topLeft.y, textCache->size.x, textCache->size.y);

	endTemporaryMemory(&memory);

	return bounds;
}

void setTooltip(UIState *uiState, char *text, V4 color)
{
	strncpy(uiState->tooltip.text, text, sizeof(uiState->tooltip.text));
	uiState->tooltip.color = color;
	uiState->tooltip.show = true;
}

void drawTooltip(GL_Renderer *renderer, InputState *inputState, UIState *uiState)
{
	if (uiState->tooltip.show)
	{
		const real32 depth = 100;
		const real32 tooltipPadding = 4;

		V2 mousePos = unproject(&uiState->uiCamera, inputState->mousePosNormalised);

		V2 topLeft = mousePos + uiState->tooltip.offsetFromCursor + v2(tooltipPadding, tooltipPadding);

		RealRect labelRect = uiLabel(renderer, renderer->theme.font, uiState->tooltip.text,
			topLeft, ALIGN_LEFT | ALIGN_TOP, depth + 1, uiState->tooltip.color);

		labelRect = expandRect(labelRect, tooltipPadding);

		drawRect(renderer, true, labelRect, depth, renderer->theme.tooltipBackgroundColor);

		uiState->tooltip.show = false;
	}
}

bool uiButton(GL_Renderer *renderer, InputState *inputState, UIState *uiState, char *text, RealRect bounds, real32 depth,
			bool active=false, SDL_Scancode shortcutKey=SDL_SCANCODE_UNKNOWN, char *tooltip=0)
{
	bool buttonClicked = false;
	V2 mousePos = unproject(&uiState->uiCamera, inputState->mousePosNormalised);

	V4 backColor = renderer->theme.buttonBackgroundColor;

	if (inRect(bounds, mousePos))
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
			setTooltip(uiState, tooltip, renderer->theme.tooltipColorNormal);
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

bool uiMenuButton(GL_Renderer *renderer, InputState *inputState, UIState *uiState, char *text, RealRect bounds,
			real32 depth, UIMenuID menuID, SDL_Scancode shortcutKey=SDL_SCANCODE_UNKNOWN, char *tooltip=0)
{
	bool currentlyOpen = uiState->openMenu == menuID;
	if (uiButton(renderer, inputState, uiState, text, bounds, depth, currentlyOpen, shortcutKey, tooltip))
	{
		if (currentlyOpen)
		{
			uiState->openMenu = UIMenu_None;
			currentlyOpen = false;
		}
		else
		{
			uiState->openMenu = menuID;
			currentlyOpen = true;
		}
	}

	return currentlyOpen;
}

void uiTextInput(GL_Renderer *renderer, InputState *inputState, bool active,
				char *textBuffer, int32 textBufferLength, V2 origin, real32 depth)
{
	if (active)
	{
		int32 textLength = strlen(textBuffer);
		if (inputState->textEntered[0])
		{
			uint32 pos = 0;
			while (inputState->textEntered[pos]
				&& textLength < textBufferLength)
			{
				textBuffer[textLength++] = inputState->textEntered[pos];
				pos++;
			}
		}

		if (keyJustPressed(inputState, SDL_SCANCODE_BACKSPACE)
			&& textLength > 0)
		{
			textBuffer[textLength-1] = 0;
			textLength--;
		}
	}

	const real32 padding = 4;
	RealRect labelRect = uiLabel(renderer, renderer->theme.font, textBuffer, origin + v2(padding, padding),
								 ALIGN_H_CENTRE | ALIGN_TOP, depth + 1, renderer->theme.textboxTextColor);
	labelRect = expandRect(labelRect, padding);
	drawRect(renderer, true, labelRect, depth, renderer->theme.textboxBackgroundColor);
}

void pushUiMessage(UIState *uiState, char *message)
{
	strncpy(uiState->message.text, message, sizeof(uiState->message.text));
	uiState->message.countdown = messageDisplayTime;
}

void drawUiMessage(GL_Renderer *renderer, UIState *uiState)
{
	if (uiState->message.countdown > 0)
	{
		uiState->message.countdown -= SECONDS_PER_FRAME;

		if (uiState->message.countdown > 0)
		{
			const real32 depth = 100;
			const real32 padding = 4;

			real32 t = (real32)uiState->message.countdown / messageDisplayTime;

			V4 backgroundColor = renderer->theme.tooltipBackgroundColor;
			V4 textColor = renderer->theme.tooltipColorNormal;

			if (t < 0.2f)
			{
				// Fade out
				real32 tt = t / 0.2f;
				backgroundColor *= lerp(0, backgroundColor.a, tt);
				textColor *= lerp(0, textColor.a, tt);
			}
			else if (t > 0.8f)
			{
				// Fade in
				real32 tt = (t - 0.8f) / 0.2f;
				backgroundColor.a = lerp(backgroundColor.a, 0, tt);
				textColor.a = lerp(textColor.a, 0, tt);
			}

			V2 origin = v2(uiState->uiCamera.windowWidth * 0.5f, uiState->uiCamera.windowHeight - 8.0f);
			// V2 origin = v2(renderer->worldCamera.windowWidth * 0.5f, renderer->worldCamera.windowHeight - 8.0f);
			RealRect labelRect = uiLabel(renderer, renderer->theme.font, uiState->message.text, origin,
										 ALIGN_H_CENTRE | ALIGN_BOTTOM, depth + 1, textColor);

			labelRect = expandRect(labelRect, padding);

			drawRect(renderer, true, labelRect, depth, backgroundColor);
		}
	}
}