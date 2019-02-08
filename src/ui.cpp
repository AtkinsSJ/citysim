// ui.cpp

void setCursor(UIState *uiState, u32 cursorID)
{
	uiState->currentCursor = cursorID;
	SDL_SetCursor(getCursor(uiState->assets, cursorID)->sdlCursor);
}

void setCursorVisible(UIState *uiState, bool visible)
{
	uiState->cursorIsVisible = visible;
	SDL_ShowCursor(visible ? 1 : 0);
}

void initUiState(UIState *uiState, RenderBuffer *uiBuffer, AssetManager *assets, InputState *input)
{
	*uiState = {};

	uiState->uiBuffer = uiBuffer;
	uiState->assets = assets;
	uiState->input = input;

	uiState->closestDepth = uiBuffer->camera.nearClippingPlane;

	initMemoryArena(&uiState->arena, MB(1));

	uiState->tooltip = {};
	uiState->tooltip.text = newString(&uiState->arena, 256);

	uiState->message = {};
	uiState->message.text = newString(&uiState->arena, 256);
	uiState->message.countdown = -1;

	initialiseArray(&uiState->uiRects, 16);

	initChunkedArray(&uiState->openWindows, &uiState->arena, 16);

	setCursorVisible(uiState, false);
}

Rect2 uiText(UIState *uiState, BitmapFont *font, String text, V2 origin, s32 align,
				 f32 depth, V4 color, f32 maxWidth = 0)
{
	DEBUG_FUNCTION();

	Rect2 bounds = rect2(origin, 0, 0);

	BitmapFontCachedText *textCache = drawTextToCache(&globalAppState.globalTempArena, font, text, color, maxWidth);
	if (textCache)
	{
		V2 topLeft = calculateTextPosition(textCache, origin, align);
		drawCachedText(uiState->uiBuffer, textCache, topLeft, depth);
		bounds = rectXYWH(topLeft.x, topLeft.y, textCache->size.x, textCache->size.y);
	}

	return bounds;
}

Rect2 drawTextInput(UIState *uiState, BitmapFont *font, TextInput *textInput, V2 origin, s32 align, f32 depth, V4 color, f32 maxWidth = 0)
{
	DEBUG_FUNCTION();

	Rect2 bounds = rect2(origin, 0, 0);

	BitmapFontCachedText *textCache = drawTextToCache(&globalAppState.globalTempArena, font, makeString(textInput->buffer, textInput->byteLength), color, maxWidth);
	if (textCache)
	{
		V2 topLeft = calculateTextPosition(textCache, origin, align);
		drawCachedText(uiState->uiBuffer, textCache, topLeft, depth);
		bounds = rectXYWH(topLeft.x, topLeft.y, textCache->size.x, textCache->size.y);

		textInput->caretFlashCounter = (f32) fmod(textInput->caretFlashCounter + SECONDS_PER_FRAME, textInput->caretFlashCycleDuration);
		bool showCaret = (textInput->caretFlashCounter < (textInput->caretFlashCycleDuration * 0.5f));

		if (showCaret)
		{
			Rect2 caretRect = rectXYWH(0, 0, 2, font->lineHeight);

			if ((u32) textInput->caretGlyphPos < textCache->charCount)
			{
				RenderItem charCaretIsBefore = textCache->chars[textInput->caretGlyphPos];
				caretRect.x += charCaretIsBefore.rect.x;
				caretRect.y += charCaretIsBefore.rect.y;
			}
			else if (textCache->charCount > 0)
			{
				// we've overrun. Could mean we're just after the last char.
				// So, we grab the last char and then add its width.

				// @FixMe: we really want o move forward by the glyph's xAdvance, but we don't know it once we're here! Sad times.
				RenderItem charCaretIsAfter = textCache->chars[textCache->charCount - 1];
				caretRect.x += charCaretIsAfter.rect.x + (1 + textInput->caretGlyphPos - textCache->charCount) * charCaretIsAfter.rect.w;
				caretRect.y += charCaretIsAfter.rect.y;
			}

			// Trouble is the y. Once we get here, we don't know what the per-character vertical offset is!
			// We don't even know what the character was.
			// So, a hack! We'll round the y to the closest multiple of the line height.

			caretRect.y = floor_f32(caretRect.y / (f32)font->lineHeight) * font->lineHeight;

			caretRect.pos += topLeft;
			caretRect.x -= 1.0f; // Slightly more able to see things with this offset.
			drawRect(uiState->uiBuffer, caretRect, depth + 10, color);
		}
	}

	return bounds;
}

void setTooltip(UIState *uiState, String text, String style)
{
	copyString(text, &uiState->tooltip.text);
	uiState->tooltip.show = true;
	uiState->tooltip.styleName = style;
}

void drawTooltip(UIState *uiState)
{
	if (uiState->tooltip.show)
	{
		UITooltipStyle *style = findTooltipStyle(uiState->assets, uiState->tooltip.styleName);
		f32 depth = uiState->closestDepth - 100.0f;

		V2 mousePos = uiState->uiBuffer->camera.mousePos;

		V2 topLeft = mousePos + style->offsetFromCursor + v2(style->padding, style->padding);

		Rect2 labelRect = uiText(uiState, getFont(uiState->assets, style->fontID), uiState->tooltip.text,
			topLeft, ALIGN_LEFT | ALIGN_TOP, depth + 1.0f, style->textColor);

		labelRect = expand(labelRect, style->padding);

		drawRect(uiState->uiBuffer, labelRect, depth, style->backgroundColor);

		uiState->tooltip.show = false;
	}
}

bool uiButton(UIState *uiState,
	          String text, Rect2 bounds, f32 depth, bool active=false,
	          SDL_Keycode shortcutKey=SDLK_UNKNOWN, String tooltip=nullString)
{
	DEBUG_FUNCTION();
	
	bool buttonClicked = false;
	V2 mousePos = uiState->uiBuffer->camera.mousePos;
	InputState *input = uiState->input;
	UIButtonStyle *style = findButtonStyle(uiState->assets, stringFromChars("general"));
	V4 backColor = style->backgroundColor;

	if (inRect(bounds, mousePos))
	{
		if (mouseButtonPressed(input, SDL_BUTTON_LEFT))
		{
			backColor = style->pressedColor;
		}
		else
		{
			backColor = style->hoverColor;
		}

		buttonClicked = mouseButtonJustReleased(input, SDL_BUTTON_LEFT);

		if (tooltip.length)
		{
			setTooltip(uiState, tooltip, stringFromChars("general"));
		}
	}
	else if (active)
	{
		backColor = style->hoverColor;
	}

	drawRect(uiState->uiBuffer, bounds, depth, backColor);
	uiText(uiState, getFont(uiState->assets, style->fontID), text, centre(bounds), ALIGN_CENTRE, depth + 1,
			style->textColor);

	// Keyboard shortcut!
	if ((shortcutKey != SDLK_UNKNOWN)
	&& keyJustPressed(input, shortcutKey))
	{
		buttonClicked = true;
	}

	return buttonClicked;
}

bool uiMenuButton(UIState *uiState,
	              String text, Rect2 bounds, f32 depth, s32 menuID,
	              SDL_Keycode shortcutKey=SDLK_UNKNOWN, String tooltip=nullString)
{
	DEBUG_FUNCTION();
	
	bool currentlyOpen = uiState->openMenu == menuID;
	if (uiButton(uiState, text, bounds, depth, currentlyOpen, shortcutKey, tooltip))
	{
		if (currentlyOpen)
		{
			uiState->openMenu = 0;
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

void pushUiMessage(UIState *uiState, String message)
{
	copyString(message, &uiState->message.text);
	uiState->message.countdown = uiMessageDisplayTime;
}

void drawUiMessage(UIState *uiState)
{
	DEBUG_FUNCTION();
	
	if (uiState->message.countdown > 0)
	{
		uiState->message.countdown -= SECONDS_PER_FRAME;

		if (uiState->message.countdown > 0)
		{
			UIMessageStyle *style = findMessageStyle(uiState->assets, stringFromChars("general"));

			f32 t = (f32)uiState->message.countdown / uiMessageDisplayTime;

			V4 backgroundColor = style->backgroundColor;
			V4 textColor = style->textColor;

			if (t < 0.2f)
			{
				// Fade out
				f32 tt = t / 0.2f;
				backgroundColor *= lerp(0, backgroundColor.a, tt);
				textColor *= lerp(0, textColor.a, tt);
			}
			else if (t > 0.8f)
			{
				// Fade in
				f32 tt = (t - 0.8f) / 0.2f;
				backgroundColor *= lerp(backgroundColor.a, 0, tt);
				textColor *= lerp(textColor.a, 0, tt);
			}

			f32 depth = uiState->closestDepth - 100.0f;

			V2 origin = v2(uiState->uiBuffer->camera.size.x * 0.5f, uiState->uiBuffer->camera.size.y - 8.0f);
			Rect2 labelRect = uiText(uiState, getFont(uiState->assets, style->fontID), uiState->message.text, origin,
										 ALIGN_H_CENTRE | ALIGN_BOTTOM, depth + 1.0f, textColor);

			labelRect = expand(labelRect, style->padding);

			drawRect(uiState->uiBuffer, labelRect, depth, backgroundColor);
		}
	}
}

void drawScrollBar(RenderBuffer *uiBuffer, V2 topLeft, f32 height, f32 scrollPercent, V2 knobSize, f32 depth, V4 knobColor)
{
	knobSize.y = MIN(knobSize.y, height); // force knob to fit
	f32 knobTravelableH = height - knobSize.y;
	f32 scrollY = scrollPercent * knobTravelableH;
	Rect2 knobRect = rectXYWH(topLeft.x, topLeft.y + scrollY, knobSize.x, knobSize.y);
	drawRect(uiBuffer, knobRect, depth, knobColor);
}

inline void uiCloseMenus(UIState *uiState)
{
	uiState->openMenu = 0;
}
