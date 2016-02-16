// immediate-ui.cpp

void uiLabel(GLRenderer *renderer, BitmapFont *font, char *text, V2 origin, int32 align, real32 depth, V4 color)
{
	// TODO: Implement alignment!
	V2 topLeft = origin; //calculateTextPosition(label->cache, label->origin, label->align);
	
	drawText(renderer, font, topLeft, text, depth, color);
}

bool uiButton(GLRenderer *renderer, InputState *inputState, char *text,
				RealRect bounds, real32 depth)
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
	}

	drawRect(renderer, true, bounds, depth, backColor);
	uiLabel(renderer, renderer->theme.buttonFont, text, centre(bounds), ALIGN_CENTRE, depth + 1,
			renderer->theme.buttonTextColor);

	return buttonClicked;
}