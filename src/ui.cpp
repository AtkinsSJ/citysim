// ui.cpp

/**
 * Change the text of the given UiLabel, which regenerates the Texture.
 */
void setUiLabelText(Renderer *renderer, UiLabel *label, char *newText) {
	// TODO: If the text is the same, do nothing!
	freeTexture(&label->texture);
	label->text = newText;
	label->texture = renderText(renderer, label->font, label->text, label->color);
	label->_rect.w = label->texture.w;
	label->_rect.h = label->texture.h;

	switch (label->align & ALIGN_H) {
		case ALIGN_H_CENTER: {
			label->_rect.x = label->origin.x - label->_rect.w/2;
		} break;
		case ALIGN_RIGHT: {
			label->_rect.x = label->origin.x - label->_rect.w;
		} break;
		default: { // Left is default
			label->_rect.x = label->origin.x;
		} break;
	}

	switch (label->align & ALIGN_V) {
		case ALIGN_V_CENTER: {
			label->_rect.y = label->origin.y - label->_rect.h/2;
		} break;
		case ALIGN_BOTTOM: {
			label->_rect.y = label->origin.y - label->_rect.h;
		} break;
		default: { // Top is default
			label->_rect.y = label->origin.y;
		} break;
	}
}

void initUiLabel(UiLabel *label, Renderer *renderer, Coord position, int32 align,
				char *text, TTF_Font *font, Color color) {
	*label = {};

	label->text = text;
	label->font = font;
	label->color = color;
	label->origin = position;
	label->align = align;

	setUiLabelText(renderer, label, text);
}

void freeUiLabel(UiLabel *label) {
	freeTexture(&label->texture);
	*label = {};
}

void initUiIntLabel(UiIntLabel *label, Renderer *renderer, Coord position, int32 align,
				TTF_Font *font, Color color, int32 *watchValue, char *formatString) {
	*label = {};
	label->formatString = formatString;
	label->value = watchValue;
	label->lastValue = *watchValue;

	sprintf(label->buffer, label->formatString, *label->value);
	initUiLabel(&label->label, renderer, position, align, label->buffer, font, color);
}

void freeUiIntLabel(UiIntLabel *label) {
	freeUiLabel(&label->label);
	*label = {};
}

void initUiButton(UiButton *button, Renderer *renderer, Rect rect,
					char *text, TTF_Font* font, Color buttonTextColor,
					Color color, Color hoverColor, Color pressedColor) {
	*button = {};

	button->rect = rect;

	button->backgroundColor = color;
	button->backgroundHoverColor = hoverColor;
	button->backgroundPressedColor = pressedColor;

	// Generate the UiLabel, and centre it
	Coord buttonCenter = {button->rect.x + button->rect.w / 2,
							button->rect.y + button->rect.h / 2};
	initUiLabel(&button->text, renderer, buttonCenter, ALIGN_CENTER, text, font, buttonTextColor);
}

void freeUiButton(UiButton *button) {
	freeUiLabel(&button->text);
	button = {};
}

/**
 * Draws a rectangle relative to the screen.
 */
void drawUiRect(Renderer *renderer, Rect rect, Color color) {
	SDL_SetRenderDrawColor(renderer->sdl_renderer, color.r, color.g, color.b, color.a);
	SDL_SetRenderDrawBlendMode(renderer->sdl_renderer, SDL_BLENDMODE_BLEND);

	SDL_RenderFillRect(renderer->sdl_renderer, &rect.sdl_rect);
}

void drawUiTexture(Renderer *renderer, Texture *texture, Rect rect) {
	SDL_RenderCopy(renderer->sdl_renderer, texture->sdl_texture, null, &rect.sdl_rect);
}

void drawUiLabel(Renderer *renderer, UiLabel *text) {
	drawUiTexture(renderer, &text->texture, text->_rect);
}

void drawUiIntLabel(Renderer *renderer, UiIntLabel *label) {
	if (*label->value != label->lastValue) {
		label->lastValue = *label->value;
		sprintf(label->buffer, label->formatString, *label->value);
		setUiLabelText(renderer, &label->label, label->buffer);
	}
	drawUiLabel(renderer, &label->label);
}

void drawUiButton(Renderer *renderer, UiButton *button) {
	if (button->active) {
		drawUiRect(renderer, button->rect, button->backgroundPressedColor);
	} else if (button->mouseOver) {
		if (button->clickStarted) {
			drawUiRect(renderer, button->rect, button->backgroundPressedColor);
		} else {
			drawUiRect(renderer, button->rect, button->backgroundHoverColor);
		}
	} else {
		drawUiRect(renderer, button->rect, button->backgroundColor);
	}
	drawUiLabel(renderer, &button->text);
}

UiButton *addButtonToGroup(UiButtonGroup *group) {
	ASSERT(group->buttonCount < ArrayCount(group->buttons), "UiButtonGroup is full!");
	return &group->buttons[group->buttonCount++];
}

void setActiveButton(UiButtonGroup *group, UiButton *button) {
	if (group->activeButton) {
		group->activeButton->active = false;
	}
	group->activeButton = button;
	if (button) {
		button->active = true;
	}
}

/**
 * Get the buttongroup to update its buttons' states, and return whether a button "ate" any click events
 */
bool updateUiButtonGroup(UiButtonGroup *group, MouseState *mouseState) {

	bool eventEaten = false;

	for (int32 i=0; i<group->buttonCount; i++) {
		UiButton *button = group->buttons + i;
		button->justClicked = false;

		button->mouseOver = inRect(button->rect, coord(mouseState->x, mouseState->y));
		if (button->mouseOver) {
			eventEaten = true;
		}

		if (mouseButtonJustPressed(mouseState, SDL_BUTTON_LEFT)) {
			// See if a button is click-started
			button->clickStarted = button->mouseOver;
		} else if (mouseButtonJustReleased(mouseState, SDL_BUTTON_LEFT)) {
			// Did we trigger a button?
			if (button->clickStarted && button->mouseOver) {
				button->justClicked = true;

				// Active button
				setActiveButton(group, button);
			}
		} else if (!mouseButtonPressed(mouseState, SDL_BUTTON_LEFT)) {
			button->clickStarted = false;
		}
	}

	return eventEaten;
}

void drawUiButtonGroup(Renderer *renderer, UiButtonGroup *group) {
	for (int32 i=0; i<group->buttonCount; i++) {
		drawUiButton(renderer, group->buttons + i);
	}
}

void freeUiButtonGroup(UiButtonGroup *group) {
	for (int32 i=0; i<group->buttonCount; i++) {
		freeUiButton(group->buttons + i);
	}
}

///////////////////////////////////////////////////////////////////////////////////
//                                UI MESSAGE                                     //
///////////////////////////////////////////////////////////////////////////////////
UiMessage __globalUiMessage;

void initUiMessage(Renderer *renderer) {
	__globalUiMessage = {};
	__globalUiMessage.renderer = renderer;

	__globalUiMessage.background = {0,0,0,128};

	initUiLabel(&__globalUiMessage.label, renderer, {400, 600-8}, ALIGN_H_CENTER | ALIGN_BOTTOM, "", renderer->fontLarge, {255, 255, 255, 255});
}

void pushUiMessage(char *message) {

	if (strcmp(message, __globalUiMessage.label.text)) {
		// Message is differenct
		setUiLabelText(__globalUiMessage.renderer, &__globalUiMessage.label, message);

		__globalUiMessage.rect.x = __globalUiMessage.label._rect.x - 4;
		__globalUiMessage.rect.y = __globalUiMessage.label._rect.y - 4;
		__globalUiMessage.rect.w = __globalUiMessage.label._rect.w + 8;
		__globalUiMessage.rect.h = __globalUiMessage.label._rect.h + 8;
	}

	// Always refresh the countdown
	__globalUiMessage.messageCountdown = 2000;
}

void drawUiMessage(Renderer *renderer) {
	if (__globalUiMessage.messageCountdown > 0) {
		__globalUiMessage.messageCountdown -= MS_PER_FRAME;

		if (__globalUiMessage.messageCountdown > 0) {

			drawUiRect(renderer, __globalUiMessage.rect, __globalUiMessage.background);

			drawUiLabel(renderer, &__globalUiMessage.label);
		}
	}
}

void freeUiMessage() {
	freeUiLabel(&__globalUiMessage.label);
}
