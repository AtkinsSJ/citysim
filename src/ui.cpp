// ui.cpp

UiButton createButton(Renderer *renderer, Rect rect,
				char *text, TTF_Font* font, Color buttonTextColor,
				Color color, Color hoverColor, Color pressedColor) {
	UiButton button;
	button.rect = rect;
	button.text = text;
	button.textColor = buttonTextColor;
	button.font = font;
	button.backgroundColor = color;
	button.backgroundHoverColor = hoverColor;
	button.backgroundPressedColor = pressedColor;

	// Generate the text texture
	SDL_Surface *textSurface = TTF_RenderUTF8_Blended(button.font, button.text, button.textColor);
	button.textTexture = textureFromSurface(renderer, textSurface);
	SDL_FreeSurface(textSurface);

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
	if (button->mouseOver) {
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