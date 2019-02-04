#pragma once

void window_text(WindowContext *context, String text, V4 color)
{
	DEBUG_FUNCTION();

	V2 origin = context->contentArea.pos + context->currentOffset;
	BitmapFont *font = getFont(context->assets, FontAssetType_Buttons);
	f32 maxWidth = context->contentArea.w - context->currentOffset.x;

	BitmapFontCachedText *textCache = drawTextToCache(context->temporaryMemory, font, text, color, maxWidth);
	V2 topLeft = calculateTextPosition(textCache, origin, ALIGN_TOP | ALIGN_LEFT);
	drawCachedText(context->uiState->uiBuffer, textCache, topLeft, context->renderDepth);

	// For now, we'll always just start a new line.
	// We'll probably want something fancier later.
	context->currentOffset.y += textCache->size.y;
}

bool window_button()
{
	DEBUG_FUNCTION();
	return false;
}

/**
 * Creates an (in-game) window in the centre of the screen, and puts it in front of all other windows.
 * If you pass -1 to the height, then the height will be determined automatically by the window's content.
 */
void showWindow(UIState *uiState, String title, s32 width, s32 height, WindowProc windowProc, void *userData)
{
	Window newWindow = {};
	s32 offset = uiState->openWindows.itemCount * 20;
	newWindow.title = title;

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

void updateAndRenderWindows(UIState *uiState, AssetManager *assets, InputState *inputState)
{
	DEBUG_FUNCTION();

	V2 mousePos = uiState->uiBuffer->camera.mousePos;
	bool mouseInputHandled = false;
	Window *newActiveWindow = null;
	s32 closeWindow = -1;

	V4 backColor       = color255(96, 96, 96, 128);
	V4 backColorActive = color255(96, 96, 96, 255);

	f32 barHeight = 32.0f;
	V4 barColor       = color255(96, 128, 96, 128);
	V4 barColorActive = color255(96, 196, 96, 255);
	V4 titleColor     = color255(255, 255, 255, 255);

	f32 contentPadding = 4.0f;

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

		if (window->windowProc != null)
		{
			WindowContext context = {};
			context.uiState = uiState;
			context.assets = assets;
			context.temporaryMemory = &globalAppState.globalTempArena;

			context.contentArea = getWindowContentArea(window->area, barHeight, contentPadding);
			context.currentOffset = v2(0,0);
			context.renderDepth = depth + 1.0f;

			window->windowProc(&context, window, window->userData);

			if (window->hasAutomaticHeight)
			{
				window->area.h = barHeight + context.currentOffset.y + (contentPadding * 2.0f);
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

		drawRect(uiState->uiBuffer, contentArea, depth, isActive ? backColorActive : backColor);
		drawRect(uiState->uiBuffer, barArea, depth, isActive ? barColorActive : barColor);
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