#pragma once

void window_label(WindowContext *context, String text, V4 color={})
{
	DEBUG_FUNCTION();

	UILabelStyle *style = context->window->style->labelStyle;

	// Use the theme-defined color if none is provided. We determine this by checking for ~0 alpha, which
	// isn't *perfect* but should work well enough.
	if (color.a < 0.01f)
	{
		color = style->textColor;
	}

	V2 origin = context->contentArea.pos + context->currentOffset;
	BitmapFont *font = getFont(context->uiState->assets, style->fontID);
	if (font)
	{
		f32 maxWidth = context->contentArea.w - context->currentOffset.x;

		BitmapFontCachedText *textCache = drawTextToCache(context->temporaryMemory, font, text, color, maxWidth);
		drawCachedText(context->uiState->uiBuffer, textCache, origin, context->renderDepth);

		// For now, we'll always just start a new line.
		// We'll probably want something fancier later.
		context->currentOffset.y += textCache->size.y + context->perItemPadding;
	}
}

bool window_button(WindowContext *context, String text)
{
	DEBUG_FUNCTION();
	
	bool buttonClicked = false;
	UIButtonStyle *style = context->window->style->buttonStyle;

	V4 backColor = style->backgroundColor;
	f32 buttonPadding = style->padding;
	V2 mousePos = context->uiState->uiBuffer->camera.mousePos;

	// Let's be a little bit fancy. Figure out the size of the text, then determine the button size based on that!
	V2 origin = context->contentArea.pos + context->currentOffset;
	BitmapFont *font = getFont(context->uiState->assets, style->fontID);
	if (font)
	{
		f32 maxWidth = context->contentArea.w - context->currentOffset.x - (2.0f * buttonPadding);

		BitmapFontCachedText *textCache = drawTextToCache(context->temporaryMemory, font, text, style->textColor, maxWidth);
		drawCachedText(context->uiState->uiBuffer, textCache, origin + v2(buttonPadding, buttonPadding), context->renderDepth + 1.0f);

		Rect2 bounds = rectPosSize(origin, textCache->size + v2(buttonPadding * 2.0f, buttonPadding * 2.0f));

		if (inRect(bounds, mousePos))
		{
			if (mouseButtonJustPressed(context->uiState->input, SDL_BUTTON_LEFT))
			{
				buttonClicked = true;
				backColor = style->pressedColor;
			}
			else if (mouseButtonPressed(context->uiState->input, SDL_BUTTON_LEFT))
			{
				backColor = style->pressedColor;
			}
			else
			{
				backColor = style->hoverColor;
			}
		}

		drawRect(context->uiState->uiBuffer, bounds, context->renderDepth, backColor);

		// For now, we'll always just start a new line.
		// We'll probably want something fancier later.
		context->currentOffset.y += bounds.size.y + context->perItemPadding;
	}

	return buttonClicked;
}

/**
 * Creates an (in-game) window in the centre of the screen, and puts it in front of all other windows.
 * If you pass -1 to the height, then the height will be determined automatically by the window's content.
 */
void showWindow(UIState *uiState, String title, s32 width, s32 height, String style, WindowProc windowProc, void *userData)
{
	Window newWindow = {};
	newWindow.title = title;
	newWindow.style = findWindowStyle(uiState->assets, style);

	V2 windowOrigin = uiState->uiBuffer->camera.pos;
	newWindow.hasAutomaticHeight = (height == -1);
	if (newWindow.hasAutomaticHeight)
	{
		windowOrigin.y *= 0.5f;
	}
	newWindow.area = irectCentreWH(v2i(windowOrigin), width, height);
	
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

inline static
Rect2 getWindowContentArea(Rect2I windowArea, f32 barHeight, f32 contentPadding)
{
	return rectXYWH(windowArea.x + contentPadding,
					windowArea.y + barHeight + contentPadding,
					windowArea.w - (contentPadding * 2.0f),
					windowArea.h - barHeight - (contentPadding * 2.0f));
}

void updateAndRenderWindows(UIState *uiState)
{
	DEBUG_FUNCTION();

	InputState *inputState = uiState->input;
	V2 mousePos = uiState->uiBuffer->camera.mousePos;
	bool mouseInputHandled = false;
	Window *newActiveWindow = null;
	s32 closeWindow = -1;

	s32 windowIndex = 0;
	for (auto it = iterate(&uiState->openWindows);
		!it.isDone;
		next(&it), windowIndex++)
	{
		Window *window = get(it);
		f32 depth = 2000.0f - (20.0f * windowIndex);
		bool isActive = (windowIndex == 0);

		V4 backColor = (isActive ? window->style->backgroundColor : window->style->backgroundColorInactive);

		f32 barHeight = window->style->titleBarHeight;
		V4 barColor = (isActive ? window->style->titleBarColor : window->style->titleBarColorInactive);
		V4 titleColor = window->style->titleColor;

		f32 contentPadding = window->style->contentPadding;

		String closeButtonString = stringFromChars("X");
		V4 closeButtonColorHover = window->style->titleBarButtonHoverColor;

		BitmapFont *titleFont = getFont(uiState->assets, window->style->titleFontID);


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

		if (window->windowProc != null)
		{
			WindowContext context = {};
			context.uiState = uiState;
			context.temporaryMemory = &globalAppState.globalTempArena;
			context.window = window;

			context.contentArea = getWindowContentArea(window->area, barHeight, contentPadding);
			context.currentOffset = v2(0,0);
			context.renderDepth = depth + 1.0f;
			context.perItemPadding = 4.0f;

			window->windowProc(&context, window, window->userData);

			if (window->hasAutomaticHeight)
			{
				window->area.h = round_s32(barHeight + context.currentOffset.y + (contentPadding * 2.0f));
			}
		}

		Rect2 wholeWindowArea = rect2(window->area);
		Rect2 barArea = rectXYWH(wholeWindowArea.x, wholeWindowArea.y, wholeWindowArea.w, barHeight);
		Rect2 closeButtonRect = rectXYWH(wholeWindowArea.x + wholeWindowArea.w - barHeight, wholeWindowArea.y, barHeight, barHeight);
		Rect2 contentArea = getWindowContentArea(window->area, barHeight, 0);

		bool hoveringOverCloseButton = inRect(closeButtonRect, mousePos);

		if (!mouseInputHandled
			 && inRect(wholeWindowArea, mousePos)
			 && mouseButtonJustPressed(inputState, SDL_BUTTON_LEFT))
		{
			if (hoveringOverCloseButton)
			{
				// If we're inside the X, close it!
				closeWindow = windowIndex;
			}
			else
			{
				if (inRect(barArea, mousePos))
				{
					// If we're inside the title bar, start dragging!
					uiState->isDraggingWindow = true;
				}

				// Make this the active window! 
				newActiveWindow = window;
			}

			mouseInputHandled = true;
		}

		drawRect(uiState->uiBuffer, contentArea, depth, backColor);
		drawRect(uiState->uiBuffer, barArea, depth, barColor);
		uiText(uiState, titleFont, window->title, barArea.pos + v2(8.0f, barArea.h * 0.5f), ALIGN_V_CENTRE | ALIGN_LEFT, depth + 1.0f, titleColor);

		if (hoveringOverCloseButton)  drawRect(uiState->uiBuffer, closeButtonRect, depth + 1.0f, closeButtonColorHover);
		uiText(uiState, titleFont, closeButtonString, centre(closeButtonRect), ALIGN_CENTRE, depth + 2.0f, titleColor);
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