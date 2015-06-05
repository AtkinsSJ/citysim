// ui.cpp

/**
 * Change the text of the given UiLabel, which regenerates the Texture.
 */
void setText(Renderer *renderer, UiLabel *label, char *newText) {
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

UiLabel createText(Renderer *renderer, Coord position, int32 align, char *text, TTF_Font *font, Color color) {
	UiLabel label = {};
	label.text = text;
	label.font = font;
	label.color = color;
	label.origin = position;
	label.align = align;

	setText(renderer, &label, text);

	return label;
}

void freeText(UiLabel *label) {
	freeTexture(&label->texture);
	label = {};
}

UiButton createButton(Renderer *renderer, Rect rect,
					char *text, TTF_Font* font, Color buttonTextColor,
					Color color, Color hoverColor, Color pressedColor) {

	UiButton button = {};
	button.rect = rect;

	button.backgroundColor = color;
	button.backgroundHoverColor = hoverColor;
	button.backgroundPressedColor = pressedColor;

	// Generate the UiLabel, and centre it
	Coord buttonCenter = {button.rect.x + button.rect.w / 2,
							button.rect.y + button.rect.h / 2};
	button.text = createText(renderer, buttonCenter, ALIGN_CENTER, text, font, buttonTextColor);

	return button;
}

void freeButton(UiButton *button) {
	freeText(&button->text);
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

void drawUiLabelure(Renderer *renderer, Texture *texture, Rect rect) {
	SDL_RenderCopy(renderer->sdl_renderer, texture->sdl_texture, null, &rect.sdl_rect);
}

void drawUiLabel(Renderer *renderer, UiLabel *text) {
	drawUiLabelure(renderer, &text->texture, text->_rect);
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

void addButtonToGroup(UiButton *button, UiButtonGroup *group) {
	SDL_assert(group->buttonCount < ArrayCount(group->buttons));

	group->buttons[group->buttonCount++] = button;
}

/**
 * Get the buttongroup to update its buttons' states, and return whether a button "ate" any click events
 */
bool updateButtonGroup(UiButtonGroup *group, MouseState *mouseState) {

	bool eventEaten = false;

	for (int32 i=0; i<group->buttonCount; i++) {
		UiButton *button = group->buttons[i];
		button->justClicked = false;

		button->mouseOver = inRect(button->rect, {mouseState->x, mouseState->y});
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
				if (group->activeButton) {
					group->activeButton->active = false;
				}
				group->activeButton = button;
				button->active = true;
			}
		} else if (!mouseButtonPressed(mouseState, SDL_BUTTON_LEFT)) {
			button->clickStarted = false;
		}
	}

	if (mouseButtonJustPressed(mouseState, SDL_BUTTON_RIGHT)) {
		// Unselect current thing
		if (group->activeButton) {
			group->activeButton->active = false;
			group->activeButton = null;
		}
	}

	return eventEaten;
}