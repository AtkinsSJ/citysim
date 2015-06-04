// ui.cpp

UiText createText(Renderer *renderer, Coord position, char *text, TTF_Font *font, Color color) {
	UiText uiText = {};
	uiText.text = text;
	uiText.font = font;
	uiText.color = color;

	uiText.texture = renderText(renderer, uiText.font, uiText.text, uiText.color);
	uiText.rect = {position, uiText.texture.w, uiText.texture.h};

	return uiText;
}

void freeText(UiText *uiText) {
	freeTexture(&uiText->texture);
	uiText = {};
}

/**
 * Change the text of the given UiText, which regenerates the Texture.
 */
void setText(Renderer *renderer, UiText *uiText, char *newText) {
	freeTexture(&uiText->texture);
	uiText->text = newText;
	uiText->texture = renderText(renderer, uiText->font, uiText->text, uiText->color);
	uiText->rect.w = uiText->texture.w;
	uiText->rect.h = uiText->texture.h;
}

UiButton createButton(Renderer *renderer, Rect rect,
					char *text, TTF_Font* font, Color buttonTextColor,
					Color color, Color hoverColor, Color pressedColor) {

	UiButton button = {};
	button.rect = rect;

	button.backgroundColor = color;
	button.backgroundHoverColor = hoverColor;
	button.backgroundPressedColor = pressedColor;

	// Generate the UiText, and centre it
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

void drawUiTexture(Renderer *renderer, Texture *texture, Rect rect) {
	SDL_RenderCopy(renderer->sdl_renderer, texture->sdl_texture, null, &rect.sdl_rect);
}

void drawUiText(Renderer *renderer, UiText *text) {
	drawUiTexture(renderer, &text->texture, text->rect);
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
	drawUiText(renderer, &button->text);
	// drawUiTexture(renderer, &button->textTexture, button->textRect);
}