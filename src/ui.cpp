// ui.cpp

UiButton createButton(Renderer *renderer, Rect rect,
				char *text, TTF_Font* font, Color buttonTextColor,
				Color color, Color hoverColor, Color pressedColor) {
	UiButton button = {};
	button.rect = rect;
	button.text = text;
	button.textColor = buttonTextColor;
	button.font = font;
	button.backgroundColor = color;
	button.backgroundHoverColor = hoverColor;
	button.backgroundPressedColor = pressedColor;

	// Generate the text texture
	button.textTexture = renderText(renderer, button.font, button.text, button.textColor);

	// Centre the text on the button
	button.textRect.w = button.textTexture.w;
	button.textRect.h = button.textTexture.h;
	button.textRect.x = button.rect.x + (button.rect.w - button.textRect.w) / 2;
	button.textRect.y = button.rect.y + (button.rect.h - button.textRect.h) / 2;

	return button;
}

void freeButton(UiButton *button) {
	freeTexture(&button->textTexture);
	button = {};
}

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
	drawUiTexture(renderer, &button->textTexture, button->textRect);
}

void drawUiText(Renderer *renderer, UiText *text) {
	drawUiTexture(renderer, &text->texture, text->rect);
}