// ui.cpp

/**
 * Change the text of the given UiLabel, which regenerates the Texture.
 */
void setUiLabelText(UiLabel *label, char *newText)
{
	// TODO: If the text is the same, do nothing!

	if (label->cache)
	{
		free(label->cache);
	}

	label->text = newText;
	label->cache = drawTextToCache(label->font, label->text, label->color);
}

void initUiLabel(UiLabel *label, V2 position, int32 align,
				char *text, BitmapFont *font, V4 color,
				bool hasBackground=false, V4 backgroundColor=makeWhite(), real32 backgroundPadding=0)
{
	*label = {};

	label->text = text;
	label->font = font;
	label->color = color;
	label->origin = position;
	label->align = align;

	if (hasBackground)
	{
		label->hasBackground = true;
		label->backgroundColor = backgroundColor;
		label->backgroundPadding = backgroundPadding;
	}

	setUiLabelText(label, text);
}

void drawUiLabel(GLRenderer *renderer, UiLabel *label)
{
	V2 topLeft = calculateTextPosition(label->cache, label->origin, label->align);
	if (label->hasBackground)
	{
		RealRect background = rectXYWH(
			topLeft.x - label->backgroundPadding,
			topLeft.y - label->backgroundPadding,
			label->cache->size.x + label->backgroundPadding * 2.0f, 
			label->cache->size.y + label->backgroundPadding * 2.0f
		);
		drawRect(renderer, true, background, 0, label->backgroundColor);
	}
	drawCachedText(renderer, label->cache, topLeft, 0);
}

///////////////////////////////////////////////////////////////////////////////////
//                                UI MESSAGE                                     //
///////////////////////////////////////////////////////////////////////////////////
UiMessage __globalUiMessage;

void initUiMessage(GLRenderer *renderer)
{
	__globalUiMessage = {};
	__globalUiMessage.renderer = renderer;

	__globalUiMessage.background = renderer->theme.overlayColor;

	initUiLabel(&__globalUiMessage.label, {400, 600-8}, ALIGN_H_CENTRE | ALIGN_BOTTOM,
				"", renderer->theme.font, renderer->theme.labelColor,
				true, renderer->theme.overlayColor, 4);
}

void pushUiMessage(char *message)
{

	if (strcmp(message, __globalUiMessage.label.text))
	{
		// Message is differenct
		setUiLabelText(&__globalUiMessage.label, message);
	}

	// Always refresh the countdown
	__globalUiMessage.messageCountdown = 2000;
}

void drawUiMessage(GLRenderer *renderer)
{
	if (__globalUiMessage.messageCountdown > 0)
	{
		__globalUiMessage.messageCountdown -= MS_PER_FRAME;

		if (__globalUiMessage.messageCountdown > 0)
		{
			drawUiLabel(renderer, &__globalUiMessage.label);
		}
	}
}
