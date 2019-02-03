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

	initChunkedArray(&uiState->openWindows, &uiState->arena, 16);

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

void showWindow(UIState *uiState, String title, WindowProc windowProc, void *userData)
{
	Window newWindow = {};
	s32 offset = uiState->openWindows.itemCount * 20;
	newWindow.title = title;
	newWindow.area = irectXYWH(100 + offset, 100 + offset, 200, 200);
	newWindow.windowProc = windowProc;
	newWindow.userData = userData;

	if (uiState->openWindows.itemCount > 0)
	{
		// We do some fiddling to make the new window the first one:
		// Append a copy of the currently-active window, then overwrite the original with the new window
		Window *oldActiveWindow = get(&uiState->openWindows, 0);
		append(&uiState->openWindows, *oldActiveWindow);
		*oldActiveWindow = newWindow;
	}
	else
	{
		append(&uiState->openWindows, newWindow);
	}
}

void updateAndRenderWindows(UIState *uiState, RenderBuffer *uiBuffer, AssetManager *assets, InputState *inputState)
{
	DEBUG_FUNCTION();

	V2 mousePos = uiBuffer->camera.mousePos;
	bool mouseInputHandled = false;
	Window *newActiveWindow = null;
	s32 closeWindow = -1;

	V4 backColor       = color255(96, 96, 96, 128);
	V4 backColorActive = color255(96, 96, 96, 255);

	f32 barHeight = 32.0f;
	V4 barColor       = color255(96, 128, 96, 128);
	V4 barColorActive = color255(96, 196, 96, 255);
	V4 titleColor     = color255(255, 255, 255, 255);

	String closeButtonString = stringFromChars("X");
	V4 closeButtonColorHover = color255(255, 64, 64, 255);

	BitmapFont *titleFont = getFont(assets, FontAssetType_Main);

	s32 windowIndex = 0;
	for (auto it = iterate(&uiState->openWindows);
		!it.isDone;
		next(&it), windowIndex++)
	{
		Window *window = get(it);
		f32 depth = 2000.0f - (20.0f * windowIndex);

		bool isActive = (windowIndex == 0);

		// Handle dragging first, BEFORE we use the window rect anywhere
		if (isActive && uiState->isDraggingWindow)
		{
			if (mouseButtonJustReleased(inputState, SDL_BUTTON_LEFT))
			{
				uiState->isDraggingWindow = false;
			}
			else
			{
				window->area.pos += inputState->mouseDeltaRaw;
			}
			
			mouseInputHandled = true;
		}

		Rect2 windowAreaF = rect2(window->area);
		Rect2 barAreaF = rectXYWH(window->area.x, window->area.y, window->area.w, barHeight);
		Rect2 closeButtonRect = rectXYWH(window->area.x + window->area.w - barHeight, window->area.y, barHeight, barHeight);

		bool hoveringOverCloseButton = inRect(closeButtonRect, mousePos);

		if (!mouseInputHandled
			 && inRect(windowAreaF, mousePos)
			 && mouseButtonJustPressed(inputState, SDL_BUTTON_LEFT))
		{
			if (hoveringOverCloseButton)
			{
				// If we're inside the X, close it!
				closeWindow = windowIndex;
			}
			else
			{
				if (inRect(barAreaF, mousePos))
				{
					// If we're inside the title bar, start dragging!
					uiState->isDraggingWindow = true;
				}

				// Make this the active window! 
				newActiveWindow = window;
			}

			mouseInputHandled = true;
		}

		if (window->windowProc != null)
		{
			window->windowProc(window->userData);
		}

		drawRect(uiBuffer, windowAreaF, depth, isActive ? backColorActive : backColor);
		drawRect(uiBuffer, barAreaF, depth + 1.0f, isActive ? barColorActive : barColor);
		uiText(uiState, uiBuffer, titleFont, window->title, barAreaF.pos + v2(8.0f, barAreaF.h * 0.5f), ALIGN_V_CENTRE | ALIGN_LEFT, depth + 2.0f, titleColor);

		if (hoveringOverCloseButton)  drawRect(uiBuffer, closeButtonRect, depth + 2.0f, closeButtonColorHover);
		uiText(uiState, uiBuffer, titleFont, closeButtonString, centre(closeButtonRect), ALIGN_CENTRE, depth + 3.0f, titleColor);
	}

	if (closeWindow != -1)
	{
		uiState->isDraggingWindow = false;
		removeIndex(&uiState->openWindows, closeWindow);
	}
	/*
	 * NB: This is an imaginary else-if, because if we try to set a new active window, AND close one,
	 * then things break. However, we never intentionally do that! I'm leaving this code "dangerous",
	 * because that way, we crash when we try to do both at once, instead of hiding the bug.
	 * - Sam, 3/2/2019
	 */
	if (newActiveWindow != null)
	{
		// For now, just swap it with window #0. This is hacky but it'll work
		Window *oldActiveWindow = get(&uiState->openWindows, 0);
		Window temp = *oldActiveWindow;
		*oldActiveWindow = *newActiveWindow;
		*newActiveWindow = temp;
	}
}