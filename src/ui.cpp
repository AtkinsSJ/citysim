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
	uiState->tooltip.text = newString(&uiState->arena, 256);

	setCursorVisible(uiState, false);
}

Rect2 uiText(UIState *uiState, RenderBuffer *uiBuffer, BitmapFont *font, String text, V2 origin, int32 align,
				 real32 depth, V4 color, real32 maxWidth = 0)
{
	DEBUG_FUNCTION();

	TemporaryMemory memory = beginTemporaryMemory(&uiState->arena);

	BitmapFontCachedText *textCache = drawTextToCache(&memory, font, text, color, maxWidth);
	V2 topLeft = calculateTextPosition(textCache, origin, align);
	drawCachedText(uiBuffer, textCache, topLeft, depth);
	Rect2 bounds = rectXYWH(topLeft.x, topLeft.y, textCache->size.x, textCache->size.y);

	endTemporaryMemory(&memory);

	return bounds;
}

Rect2 drawTextInput(UIState *uiState, RenderBuffer *uiBuffer, BitmapFont *font, TextInput *textInput, V2 origin, int32 align, real32 depth, V4 color, real32 maxWidth = 0)
{
	DEBUG_FUNCTION();

	TemporaryMemory memory = beginTemporaryMemory(&uiState->arena);

	BitmapFontCachedText *textCache = drawTextToCache(&memory, font, makeString(textInput->buffer, textInput->byteLength), color, maxWidth);
	V2 topLeft = calculateTextPosition(textCache, origin, align);
	drawCachedText(uiBuffer, textCache, topLeft, depth);
	Rect2 bounds = rectXYWH(topLeft.x, topLeft.y, textCache->size.x, textCache->size.y);

	textInput->caretFlashCounter = fmod(textInput->caretFlashCounter + SECONDS_PER_FRAME, textInput->caretFlashCycleDuration);
	bool showCaret = (textInput->caretFlashCounter < (textInput->caretFlashCycleDuration * 0.5f));

	if (showCaret)
	{
		Rect2 caretRect = rectXYWH(0, 0, 2, font->lineHeight);

		if ((uint32) textInput->caretGlyphPos < textCache->charCount)
		{
			RenderItem charCaretIsBefore = textCache->chars[textInput->caretGlyphPos];
			caretRect.x += charCaretIsBefore.rect.x;
			caretRect.y += charCaretIsBefore.rect.y;
		}
		else if (textCache->charCount > 0)
		{
			// we've overrun. Could mean we're just after the last char.
			// So, we grab the last char and then add its width.
			RenderItem charCaretIsAfter = textCache->chars[textCache->charCount - 1];
			caretRect.x += charCaretIsAfter.rect.x + (1 + textInput->caretGlyphPos - textCache->charCount) * charCaretIsAfter.rect.w;
			caretRect.y += charCaretIsAfter.rect.y;
		}

		// Trouble is the y. Once we get here, we don't know what the per-character vertical offset is!
		// We don't even know what the character was.
		// So, a hack! We'll round the y to the closest multiple of the line height.

		caretRect.y = floor(caretRect.y / (real32)font->lineHeight) * font->lineHeight;

		caretRect.pos += topLeft;
		caretRect.x -= 1.0f; // Slightly more able to see things with this offset.
		drawRect(uiBuffer, caretRect, depth + 10, color);
	}

	endTemporaryMemory(&memory);

	return bounds;
}

void setTooltip(UIState *uiState, String text, V4 color)
{
	copyString(text, &uiState->tooltip.text);
	uiState->tooltip.color = color;
	uiState->tooltip.show = true;
}

void drawTooltip(UIState *uiState, RenderBuffer *uiBuffer, AssetManager *assets)
{
	if (uiState->tooltip.show)
	{
		UITooltipStyle *style = &assets->theme.tooltipStyle;

		V2 mousePos = uiBuffer->camera.mousePos;

		V2 topLeft = mousePos + uiState->tooltip.offsetFromCursor + v2(style->borderPadding, style->borderPadding);

		Rect2 labelRect = uiText(uiState, uiBuffer, getFont(assets, style->font), uiState->tooltip.text,
			topLeft, ALIGN_LEFT | ALIGN_TOP, style->depth + 1, uiState->tooltip.color);

		labelRect = expand(labelRect, style->borderPadding);

		drawRect(uiBuffer, labelRect, style->depth, style->backgroundColor);

		uiState->tooltip.show = false;
	}
}

bool uiButton(UIState *uiState, RenderBuffer *uiBuffer, AssetManager *assets, InputState *inputState,
	          String text, Rect2 bounds, real32 depth, bool active=false,
	          SDL_Keycode shortcutKey=SDLK_UNKNOWN, String tooltip=nullString)
{
	DEBUG_FUNCTION();
	
	bool buttonClicked = false;
	V2 mousePos = uiBuffer->camera.mousePos;
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

		if (tooltip.length)
		{
			setTooltip(uiState, tooltip, theme->tooltipStyle.textColorNormal);
		}
	}
	else if (active)
	{
		backColor = style->hoverColor;
	}

	drawRect(uiBuffer, bounds, depth, backColor);
	uiText(uiState, uiBuffer, getFont(assets, style->font), text, centre(bounds), ALIGN_CENTRE, depth + 1,
			style->textColor);

	// Keyboard shortcut!
	if ((shortcutKey != SDLK_UNKNOWN)
	&& keyJustPressed(inputState, shortcutKey))
	{
		buttonClicked = true;
	}

	return buttonClicked;
}

bool uiMenuButton(UIState *uiState, RenderBuffer *uiBuffer, AssetManager *assets, InputState *inputState,
	              String text, Rect2 bounds, real32 depth, UIMenuID menuID,
	              SDL_Keycode shortcutKey=SDLK_UNKNOWN, String tooltip=nullString)
{
	DEBUG_FUNCTION();
	
	bool currentlyOpen = uiState->openMenu == menuID;
	if (uiButton(uiState, uiBuffer, assets, inputState, text, bounds, depth, currentlyOpen, shortcutKey, tooltip))
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

// void uiTextInput(UIState *uiState, RenderBuffer *uiBuffer, AssetManager *assets, InputState *inputState,
// 	             bool active, char *textBuffer, int32 textBufferLength, V2 origin, real32 depth)
// {
// 	DEBUG_FUNCTION();
	
// 	UITheme *theme = &assets->theme;

// 	if (active)
// 	{
// 		int32 textLength = strlen(textBuffer);
// 		if (inputState->textEntered[0])
// 		{
// 			uint32 pos = 0;
// 			while (inputState->textEntered[pos]
// 				&& textLength < textBufferLength)
// 			{
// 				textBuffer[textLength++] = inputState->textEntered[pos];
// 				pos++;
// 			}
// 		}

// 		if (keyJustPressed(inputState, SDLK_BACKSPACE)
// 			&& textLength > 0)
// 		{
// 			textBuffer[textLength-1] = 0;
// 			textLength--;
// 		}
// 	}

// 	const real32 padding = 4;
// 	Rect2 labelRect = uiText(uiState, uiBuffer, getFont(assets, theme->labelStyle.font), textBuffer, origin + v2(padding, padding),
// 								 ALIGN_H_CENTRE | ALIGN_TOP, depth + 1, theme->textboxTextColor);
// 	labelRect = expandRect2I(labelRect, padding);
// 	drawRect2I(uiBuffer, labelRect, depth, theme->textboxBackgroundColor);
// }

void pushUiMessage(UIState *uiState, String message)
{
	copyString(message, &uiState->message.text);
	uiState->message.countdown = messageDisplayTime;
}

void drawUiMessage(UIState *uiState, RenderBuffer *uiBuffer, AssetManager *assets)
{
	DEBUG_FUNCTION();
	
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

			V2 origin = v2(uiBuffer->camera.size.x * 0.5f, uiBuffer->camera.size.y - 8.0f);
			Rect2 labelRect = uiText(uiState, uiBuffer, getFont(assets, style->font), uiState->message.text, origin,
										 ALIGN_H_CENTRE | ALIGN_BOTTOM, style->depth + 1, textColor);

			labelRect = expand(labelRect, style->borderPadding);

			drawRect(uiBuffer, labelRect, style->depth, backgroundColor);
		}
	}
}

void drawScrollBar(RenderBuffer *uiBuffer, V2 topLeft, real32 height, real32 scrollPercent, V2 knobSize, real32 depth, V4 knobColor)
{
	knobSize.y = MIN(knobSize.y, height); // force knob to fit
	real32 knobTravelableH = height - knobSize.y;
	real32 scrollY = scrollPercent * knobTravelableH;
	Rect2 knobRect = rectXYWH(topLeft.x, topLeft.y + scrollY, knobSize.x, knobSize.y);
	drawRect(uiBuffer, knobRect, depth, knobColor);
}