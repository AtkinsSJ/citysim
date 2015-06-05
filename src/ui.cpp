// ui.cpp

UiLabel createText(Renderer *renderer, Coord position, char *text, TTF_Font *font, Color color) {
	UiLabel label = {};
	label.text = text;
	label.font = font;
	label.color = color;

	label.texture = renderText(renderer, label.font, label.text, label.color);
	label.rect = {position, label.texture.w, label.texture.h};

	return label;
}

void freeText(UiLabel *label) {
	freeTexture(&label->texture);
	label = {};
}

/**
 * Change the text of the given UiLabel, which regenerates the Texture.
 */
void setText(Renderer *renderer, UiLabel *label, char *newText) {
	// TODO: If the text is the same, do nothing!
	freeTexture(&label->texture);
	label->text = newText;
	label->texture = renderText(renderer, label->font, label->text, label->color);
	label->rect.w = label->texture.w;
	label->rect.h = label->texture.h;
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
	button.text = createText(renderer, {0,0}, text, font, buttonTextColor);
	button.text.rect.x = button.rect.x + (button.rect.w - button.text.rect.w) / 2;
	button.text.rect.y = button.rect.y + (button.rect.h - button.text.rect.h) / 2;

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
	drawUiLabelure(renderer, &text->texture, text->rect);
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
	// drawUiLabelure(renderer, &button->textTexture, button->textRect);
}