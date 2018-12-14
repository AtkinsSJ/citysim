// ui.cpp

void setCursor(UIState *uiState, AssetManager *assets, u32 cursorID)
{
	uiState->currentCursor = cursorID;
	SDL_SetCursor(getCursor(assets, cursorID)->sdlCursor);
}

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

	uiState->tooltip = {};
	uiState->tooltip.offsetFromCursor = v2(16, 20);
	uiState->tooltip.text = newString(&uiState->arena, 256);

	uiState->message = {};
	uiState->message.text = newString(&uiState->arena, 256);
	uiState->message.countdown = -1;

	initialiseArray(&uiState->uiRects, 16);

	setCursorVisible(uiState, false);
}

Rect2 uiText(UIState *uiState, RenderBuffer *uiBuffer, BitmapFont *font, String text, V2 origin, s32 align,
				 f32 depth, V4 color, f32 maxWidth = 0)
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

Rect2 drawTextInput(UIState *uiState, RenderBuffer *uiBuffer, BitmapFont *font, TextInput *textInput, V2 origin, s32 align, f32 depth, V4 color, f32 maxWidth = 0)
{
	DEBUG_FUNCTION();

	TemporaryMemory memory = beginTemporaryMemory(&uiState->arena);

	BitmapFontCachedText *textCache = drawTextToCache(&memory, font, makeString(textInput->buffer, textInput->byteLength), color, maxWidth);
	V2 topLeft = calculateTextPosition(textCache, origin, align);
	drawCachedText(uiBuffer, textCache, topLeft, depth);
	Rect2 bounds = rectXYWH(topLeft.x, topLeft.y, textCache->size.x, textCache->size.y);

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

		caretRect.y = (f32) floor(caretRect.y / (f32)font->lineHeight) * font->lineHeight;

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
	          String text, Rect2 bounds, f32 depth, bool active=false,
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
	              String text, Rect2 bounds, f32 depth, UIMenuID menuID,
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

void pushUiMessage(UIState *uiState, String message)
{
	copyString(message, &uiState->message.text);
	uiState->message.countdown = uiMessageDisplayTime;
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

			V2 origin = v2(uiBuffer->camera.size.x * 0.5f, uiBuffer->camera.size.y - 8.0f);
			Rect2 labelRect = uiText(uiState, uiBuffer, getFont(assets, style->font), uiState->message.text, origin,
										 ALIGN_H_CENTRE | ALIGN_BOTTOM, style->depth + 1, textColor);

			labelRect = expand(labelRect, style->borderPadding);

			drawRect(uiBuffer, labelRect, style->depth, backgroundColor);
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

void startDragging(UIState *uiState, V2I mouseTilePos)
{
	uiState->isDragging = true;
	uiState->mouseDragStartPos = uiState->mouseDragEndPos = mouseTilePos;
}

void updateDragging(UIState *uiState, V2I mouseTilePos)
{
	if (uiState->isDragging)
	{
		uiState->mouseDragEndPos = mouseTilePos;
	}
}

Rect2I getDragRect(UIState *uiState)
{
	Rect2I dragRect = irectXYWH(0, 0, 0, 0);
	if (uiState->isDragging)
	{
		dragRect = irectCovering(uiState->mouseDragStartPos, uiState->mouseDragEndPos);
	}

	return dragRect;
}

Rect2I getDragLine(UIState *uiState)
{
	// Axis-aligned straight line, in one dimension.
	// So, if you drag a diagonal line, it picks which direction has greater length and uses that.

	Rect2I result = irectXYWH(0, 0, 0, 0);
	if (uiState->isDragging)
	{
		// determine orientation
		s32 xDiff = abs(uiState->mouseDragStartPos.x - uiState->mouseDragEndPos.x);
		s32 yDiff = abs(uiState->mouseDragStartPos.y - uiState->mouseDragEndPos.y);
		if (xDiff > yDiff)
		{
			// X
			result.w = xDiff + 1;
			result.h = 1;

			result.x = MIN(uiState->mouseDragStartPos.x, uiState->mouseDragEndPos.x);
			result.y = uiState->mouseDragStartPos.y;
		}
		else
		{
			// Y
			result.w = 1;
			result.h = yDiff + 1;

			result.x = uiState->mouseDragStartPos.x;
			result.y = MIN(uiState->mouseDragStartPos.y, uiState->mouseDragEndPos.y);
		} 
	}

	return result;
}

void cancelDragging(UIState *uiState)
{
	uiState->isDragging = false;
}