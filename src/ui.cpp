// ui.cpp

void setCursorVisible(UIState *uiState, bool visible)
{
	uiState->cursorIsVisible = visible;
	SDL_ShowCursor(visible ? 1 : 0);
}

void initUiState(UIState *uiState)
{
	*uiState = {};

	initMemoryArena(&uiState->arena, MB(1));

	uiState->actionMode = ActionMode_None;
	uiState->selectedBuildingArchetype = BA_None;

	uiState->tooltip = {};
	uiState->tooltip.offsetFromCursor = v2(16, 20);

	setCursorVisible(uiState, false);
}

void setCursor(UIState *uiState, AssetManager *assets, CursorType cursorID)
{
	uiState->currentCursor = cursorID;
	SDL_SetCursor(getCursor(assets, cursorID)->sdlCursor);
}

RealRect uiText(UIState *uiState, Renderer *renderer, BitmapFont *font, char *text, V2 origin, int32 align,
				 real32 depth, V4 color, real32 maxWidth = 0)
{

	TemporaryMemoryArena memory = beginTemporaryMemory(&uiState->arena);

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

void drawTooltip(UIState *uiState, Renderer *renderer, AssetManager *assets, InputState *inputState)
{
	if (uiState->tooltip.show)
	{
		UITooltipStyle *style = &assets->theme.tooltipStyle;

		V2 mousePos = unproject(&renderer->uiBuffer.camera, inputState->mousePosNormalised);

		V2 topLeft = mousePos + uiState->tooltip.offsetFromCursor + v2(style->borderPadding, style->borderPadding);

		RealRect labelRect = uiText(uiState, renderer, getFont(assets, style->font), uiState->tooltip.text,
			topLeft, ALIGN_LEFT | ALIGN_TOP, style->depth + 1, uiState->tooltip.color);

		labelRect = expandRect(labelRect, style->borderPadding);

		drawRect(&renderer->uiBuffer, labelRect, style->depth, style->backgroundColor);

		uiState->tooltip.show = false;
	}
}

bool uiButton(UIState *uiState, Renderer *renderer, AssetManager *assets, InputState *inputState,
	          char *text, RealRect bounds, real32 depth, bool active=false,
	          SDL_Scancode shortcutKey=SDL_SCANCODE_UNKNOWN, char *tooltip=0)
{
	bool buttonClicked = false;
	V2 mousePos = unproject(&renderer->uiBuffer.camera, inputState->mousePosNormalised);
	UITheme *theme = &assets->theme;
	UIButtonStyle *style = &theme->buttonStyle;
	V4 backColor = style->backgroundColor;

	if (inRect(bounds, mousePos))
	{
		if (mouseButtonPressed(inputState, SDL_BUTTON_LEFT))
		{
			backColor = style->pressedColor;
		}
		else
		{
			backColor = style->hoverColor;
		}

		buttonClicked = mouseButtonJustReleased(inputState, SDL_BUTTON_LEFT);

		if (tooltip)
		{
			setTooltip(uiState, tooltip, theme->tooltipStyle.textColorNormal);
		}
	}
	else if (active)
	{
		backColor = style->hoverColor;
	}

	drawRect(&renderer->uiBuffer, bounds, depth, backColor);
	uiText(uiState, renderer, getFont(assets, style->font), text, centre(bounds), ALIGN_CENTRE, depth + 1,
			style->textColor);

	// Keyboard shortcut!
	if ((shortcutKey != SDL_SCANCODE_UNKNOWN)
	&& keyJustPressed(inputState, shortcutKey))
	{
		buttonClicked = true;
	}

	return buttonClicked;
}

bool uiMenuButton(UIState *uiState, Renderer *renderer, AssetManager *assets, InputState *inputState,
	              char *text, RealRect bounds, real32 depth, UIMenuID menuID,
	              SDL_Scancode shortcutKey=SDL_SCANCODE_UNKNOWN, char *tooltip=0)
{
	bool currentlyOpen = uiState->openMenu == menuID;
	if (uiButton(uiState, renderer, assets, inputState, text, bounds, depth, currentlyOpen, shortcutKey, tooltip))
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

void uiTextInput(UIState *uiState, Renderer *renderer, AssetManager *assets, InputState *inputState,
	             bool active, char *textBuffer, int32 textBufferLength, V2 origin, real32 depth)
{
	UITheme *theme = &assets->theme;

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
	RealRect labelRect = uiText(uiState, renderer, getFont(assets, theme->labelStyle.font), textBuffer, origin + v2(padding, padding),
								 ALIGN_H_CENTRE | ALIGN_TOP, depth + 1, theme->textboxTextColor);
	labelRect = expandRect(labelRect, padding);
	drawRect(&renderer->uiBuffer, labelRect, depth, theme->textboxBackgroundColor);
}

void pushUiMessage(UIState *uiState, char *message)
{
	strncpy(uiState->message.text, message, sizeof(uiState->message.text));
	uiState->message.countdown = messageDisplayTime;
}

void drawUiMessage(UIState *uiState, Renderer *renderer, AssetManager *assets)
{
	if (uiState->message.countdown > 0)
	{
		uiState->message.countdown -= SECONDS_PER_FRAME;

		if (uiState->message.countdown > 0)
		{
			UIMessageStyle *style = &assets->theme.uiMessageStyle;

			real32 t = (real32)uiState->message.countdown / messageDisplayTime;

			V4 backgroundColor = style->backgroundColor;
			V4 textColor = style->textColor;

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

			V2 origin = v2(renderer->uiBuffer.camera.size.x * 0.5f, renderer->uiBuffer.camera.size.y - 8.0f);
			// V2 origin = v2(renderer->worldCamera.windowWidth * 0.5f, renderer->worldCamera.windowHeight - 8.0f);
			RealRect labelRect = uiText(uiState, renderer, getFont(assets, style->font), uiState->message.text, origin,
										 ALIGN_H_CENTRE | ALIGN_BOTTOM, style->depth + 1, textColor);

			labelRect = expandRect(labelRect, style->borderPadding);

			drawRect(&renderer->uiBuffer, labelRect, style->depth, backgroundColor);
		}
	}
}