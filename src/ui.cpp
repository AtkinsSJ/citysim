// ui.cpp

/**
 * Change the text of the given UiLabel, which regenerates the Texture.
 */
void setUiLabelText(GLRenderer *renderer, UiLabel *label, char *newText)
{
#if 0
	// TODO: If the text is the same, do nothing!
	freeTexture(&label->texture);
	label->text = newText;
	label->texture = renderText(renderer, label->font, label->text, label->color);
	label->_rect.w = label->texture.w;
	label->_rect.h = label->texture.h;
	updateUiLabelPosition(label);
#else

	if (label->cache)
	{
		free(label->cache);
	}

	label->text = newText;
	label->cache = drawTextToCache(renderer, label->font, label->text, &label->color);
#endif
}

void initUiLabel(UiLabel *label, GLRenderer *renderer, V2 position, int32 align,
				char *text, BitmapFont *font, Color color)
{
	*label = {};

	label->text = text;
	label->font = font;
	label->color = color;
	label->origin = position;
	label->align = align;

	setUiLabelText(renderer, label, text);
}

void initUiIntLabel(UiIntLabel *label, GLRenderer *renderer, V2 position, int32 align,
				BitmapFont *font, Color color, int32 *watchValue, char *formatString)
{
	*label = {};
	label->formatString = formatString;
	label->value = watchValue;
	label->lastValue = *watchValue;

	sprintf(label->buffer, label->formatString, *label->value);
	initUiLabel(&label->label, renderer, position, align, label->buffer, font, color);
}

void initUiButton(UiButton *button, GLRenderer *renderer, RealRect rect, char *text,
					SDL_Scancode shortcutKey=SDL_SCANCODE_UNKNOWN, char *tooltip=0)
{
	*button = {};

	button->rect = rect;

	button->backgroundColor = renderer->theme.buttonBackgroundColor;
	button->backgroundHoverColor = renderer->theme.buttonHoverColor;
	button->backgroundPressedColor = renderer->theme.buttonPressedColor;

	button->shortcutKey = shortcutKey;
	button->tooltip = tooltip;

	// Generate the UiLabel, and centre it
	V2 buttonCenter = v2(button->rect.x + button->rect.w / 2.0f,
						button->rect.y + button->rect.h / 2.0f);
	initUiLabel(&button->text, renderer, buttonCenter, ALIGN_CENTER, text,
				renderer->theme.buttonFont, renderer->theme.buttonTextColor);
}

void drawUiLabel(GLRenderer *renderer, UiLabel *label)
{
	drawCachedText(renderer, label->cache, label->origin, label->align);
	// drawText(renderer, label->font, label->origin, label->text, &label->color);
}

void drawUiIntLabel(GLRenderer *renderer, UiIntLabel *label)
{
	if (*label->value != label->lastValue) {
		label->lastValue = *label->value;
		sprintf(label->buffer, label->formatString, *label->value);
		setUiLabelText(renderer, &label->label, label->buffer);
	}
	drawUiLabel(renderer, &label->label);
}

bool updateUiButton(GLRenderer *renderer, Tooltip *tooltip, UiButton *button, MouseState *mouseState,
					KeyboardState *keyboardState)
{
	
	bool eventEaten = false;

	button->justClicked = false;

	button->mouseOver = inRect(button->rect, v2(mouseState->pos));
	if (button->mouseOver)
	{
		eventEaten = true;

		if (button->tooltip)
		{
			// Display the tooltip!
			showTooltip(tooltip, renderer, button->tooltip);
		}
	}

	if (button->shortcutKey
		&& keyJustPressed(keyboardState, button->shortcutKey) )
	{

		// We triggered the shortcut!
		button->justClicked = true;

	}
	else if (mouseButtonJustPressed(mouseState, SDL_BUTTON_LEFT))
	{
		// See if a button is click-started
		button->clickStarted = button->mouseOver;
	}
	else if (mouseButtonJustReleased(mouseState, SDL_BUTTON_LEFT))
	{
		// Did we trigger a button?
		if (button->clickStarted && button->mouseOver)
		{
			button->justClicked = true;
		}
	}
	else if (!mouseButtonPressed(mouseState, SDL_BUTTON_LEFT))
	{
		button->clickStarted = false;
	}

	return eventEaten;
}

void drawUiButton(GLRenderer *renderer, UiButton *button)
{
	if (button->active)
	{
		drawRect(renderer, true, button->rect, &button->backgroundPressedColor);
	}
	else if (button->mouseOver)
	{
		if (button->clickStarted)
		{
			drawRect(renderer, true, button->rect, &button->backgroundPressedColor);
		}
		else
		{
			drawRect(renderer, true, button->rect, &button->backgroundHoverColor);
		}
	}
	else
	{
		drawRect(renderer, true, button->rect, &button->backgroundColor);
	}
	drawUiLabel(renderer, &button->text);
}

UiButton *addButtonToGroup(UiButtonGroup *group)
{
	ASSERT(group->buttonCount < ArrayCount(group->buttons), "UiButtonGroup is full!");
	return &group->buttons[group->buttonCount++];
}

void setActiveButton(UiButtonGroup *group, UiButton *button)
{
	if (group->activeButton)
	{
		group->activeButton->active = false;
	}
	group->activeButton = button;
	if (button)
	{
		button->active = true;
	}
}

/**
 * Get the buttongroup to update its buttons' states, and return whether a button "ate" any click events
 */
bool updateUiButtonGroup(GLRenderer *renderer, Tooltip *tooltip, UiButtonGroup *group,
	MouseState *mouseState, KeyboardState *keyboardState)
{

	bool eventEaten = false;

	for (int32 i=0; i<group->buttonCount; i++)
	{
		UiButton *button = group->buttons + i;
		button->justClicked = false;

		button->mouseOver = inRect(button->rect, v2(mouseState->pos));
		if (button->mouseOver)
		{
			eventEaten = true;

			if (button->tooltip)
			{
				// Display the tooltip!
				showTooltip(tooltip, renderer, button->tooltip);
			}
		}

		if (button->shortcutKey
			&& keyJustPressed(keyboardState, button->shortcutKey) )
		{

			// We triggered the shortcut!
			button->justClicked = true;
			setActiveButton(group, button);

		}
		else if (mouseButtonJustPressed(mouseState, SDL_BUTTON_LEFT))
		{
			// See if a button is click-started
			button->clickStarted = button->mouseOver;
		}
		else if (mouseButtonJustReleased(mouseState, SDL_BUTTON_LEFT))
		{
			// Did we trigger a button?
			if (button->clickStarted && button->mouseOver)
			{
				button->justClicked = true;

				// Active button
				setActiveButton(group, button);
			}
		}
		else if (!mouseButtonPressed(mouseState, SDL_BUTTON_LEFT))
		{
			button->clickStarted = false;
		}
	}

	return eventEaten;
}

void drawUiButtonGroup(GLRenderer *renderer, UiButtonGroup *group)
{
	for (int32 i=0; i<group->buttonCount; i++)
	{
		drawUiButton(renderer, group->buttons + i);
	}
}

///////////////////////////////////////////////////////////////////////////////////
//                                UI MESSAGE                                     //
///////////////////////////////////////////////////////////////////////////////////
UiMessage __globalUiMessage;

void initUiMessage(GLRenderer *renderer)
{
	__globalUiMessage = {};
	__globalUiMessage.renderer = renderer;

	__globalUiMessage.background = {0,0,0,128};

	initUiLabel(&__globalUiMessage.label, renderer, {400, 600-8}, ALIGN_H_CENTER | ALIGN_BOTTOM,
				"", renderer->theme.font, {255, 255, 255, 255});
}

void pushUiMessage(char *message)
{

	if (strcmp(message, __globalUiMessage.label.text))
	{
		// Message is differenct
		setUiLabelText(__globalUiMessage.renderer, &__globalUiMessage.label, message);
#if 0
		__globalUiMessage.rect.x = __globalUiMessage.label._rect.x - 4;
		__globalUiMessage.rect.y = __globalUiMessage.label._rect.y - 4;
		__globalUiMessage.rect.w = __globalUiMessage.label._rect.w + 8;
		__globalUiMessage.rect.h = __globalUiMessage.label._rect.h + 8;
#endif
	}

	// Always refresh the countdown
	__globalUiMessage.messageCountdown = 2000;
}

void drawUiMessage(GLRenderer *renderer)
{
#if 0
	if (__globalUiMessage.messageCountdown > 0)
	{
		__globalUiMessage.messageCountdown -= MS_PER_FRAME;

		if (__globalUiMessage.messageCountdown > 0)
		{
			V2 *textSize = &__globalUiMessage.label.cache->size;
			RealRect backgroundRect = rectXYWH();
			drawRect(renderer, true, backgroundRect, &__globalUiMessage.background);

			drawUiLabel(renderer, &__globalUiMessage.label);
		}
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////////
//                                 TOOLTIPS                                      //
///////////////////////////////////////////////////////////////////////////////////

void showTooltip(Tooltip *tooltip, GLRenderer *renderer, char *text)
{
	if (strcmp(tooltip->buffer, text))
	{
		strcpy(tooltip->buffer, text);
		tooltip->label.color = {255,255,255,255};
		setUiLabelText(renderer, &tooltip->label, tooltip->buffer);
	}
	tooltip->show = true;
}
