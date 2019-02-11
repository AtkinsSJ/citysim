#pragma once

void window_label(WindowContext *context, String text, char *styleName=null)
{
	DEBUG_FUNCTION();

	UILabelStyle *style = null;
	if (styleName)      style = findLabelStyle(context->uiState->assets, stringFromChars(styleName));
	if (style == null)  style = context->window->style->labelStyle;

	if (context->currentOffset.y > 0)
	{
		context->currentOffset.y += context->perItemPadding;
	}

	V2 origin = context->contentArea.pos + context->currentOffset;
	BitmapFont *font = getFont(context->uiState->assets, style->fontID);
	if (font)
	{
		f32 maxWidth = context->contentArea.w - context->currentOffset.x;

		BitmapFontCachedText *textCache = drawTextToCache(context->temporaryMemory, font, text, style->textColor, maxWidth);
		drawCachedText(context->uiState->uiBuffer, textCache, origin, context->renderDepth);

		// For now, we'll always just start a new line.
		// We'll probably want something fancier later.
		context->currentOffset.y += textCache->size.y;
	}
}

bool window_button(WindowContext *context, String text)
{
	DEBUG_FUNCTION();
	
	bool buttonClicked = false;
	UIButtonStyle *style = context->window->style->buttonStyle;
	InputState *input = context->uiState->input;

	V4 backColor = style->backgroundColor;
	f32 buttonPadding = style->padding;
	V2 mousePos = context->uiState->uiBuffer->camera.mousePos;

	if (context->currentOffset.y > 0)
	{
		context->currentOffset.y += context->perItemPadding;
	}

	// Let's be a little bit fancy. Figure out the size of the text, then determine the button size based on that!
	V2 origin = context->contentArea.pos + context->currentOffset;
	BitmapFont *font = getFont(context->uiState->assets, style->fontID);
	if (font)
	{
		f32 maxWidth = context->contentArea.w - context->currentOffset.x - (2.0f * buttonPadding);

		BitmapFontCachedText *textCache = drawTextToCache(context->temporaryMemory, font, text, style->textColor, maxWidth);
		drawCachedText(context->uiState->uiBuffer, textCache, origin + v2(buttonPadding, buttonPadding), context->renderDepth + 1.0f);

		Rect2 bounds = rectPosSize(origin, textCache->size + v2(buttonPadding * 2.0f, buttonPadding * 2.0f));

		if (!context->uiState->mouseInputHandled && inRect(bounds, mousePos))
		{
			// Mouse pressed: must have started and currently be inside the bounds to show anything
			// Mouse unpressed: show hover if in bounds
			if (mouseButtonPressed(input, SDL_BUTTON_LEFT))
			{
				if (inRect(bounds, getClickStartPos(input, SDL_BUTTON_LEFT, &context->uiState->uiBuffer->camera)))
				{
					backColor = style->pressedColor;
				}
			}
			else
			{
				if (mouseButtonJustReleased(input, SDL_BUTTON_LEFT)
				 && inRect(bounds, getClickStartPos(input, SDL_BUTTON_LEFT, &context->uiState->uiBuffer->camera)))
				{
					buttonClicked = true;
					context->uiState->mouseInputHandled = true;
				}

				backColor = style->hoverColor;
			}
		}

		drawRect(context->uiState->uiBuffer, bounds, context->renderDepth, backColor);

		// For now, we'll always just start a new line.
		// We'll probably want something fancier later.
		context->currentOffset.y += bounds.size.y;
	}

	return buttonClicked;
}

void setWindowStyle(WindowContext *context, String styleName)
{
	UIWindowStyle *style = findWindowStyle(context->uiState->assets, styleName);
	if (style != null)
	{
		context->window->style = style;
	}
}

static void makeWindowActive(UIState *uiState, s32 windowIndex)
{
	// Don't do anything if it's already the active window.
	if (windowIndex == 0)  return;

	moveItemKeepingOrder(&uiState->openWindows, windowIndex, 0);
}

/**
 * Creates an (in-game) window in the centre of the screen, and puts it in front of all other windows.
 */
void showWindow(UIState *uiState, String title, s32 width, s32 height, String style, u32 flags, WindowProc windowProc, void *userData)
{
	Window newWindow = {};
	newWindow.title = title;
	newWindow.flags = flags;
	newWindow.style = findWindowStyle(uiState->assets, style);

	V2 windowOrigin = uiState->uiBuffer->camera.pos;
	newWindow.area = irectCentreWH(v2i(windowOrigin), width, height);
	
	newWindow.windowProc = windowProc;
	newWindow.userData = userData;

	bool createdWindowAlready = false;

	if (uiState->openWindows.itemCount > 0)
	{
		// If the window wants to be unique, then we search for an existing one with the same WindowProc
		if (flags & WinFlag_Unique)
		{
			Window *toReplace = null;

			s32 oldWindowIndex = 0;
			for (auto it = iterate(&uiState->openWindows);
				!it.isDone;
				next(&it), oldWindowIndex++)
			{
				Window *oldWindow = get(it);
				if (oldWindow->windowProc == windowProc)
				{
					toReplace = oldWindow;
					break;
				}
			}

			if (toReplace)
			{
				// Make it keep the current window's position
				newWindow.area = toReplace->area;

				*toReplace = newWindow;
				makeWindowActive(uiState, oldWindowIndex);
				createdWindowAlready = true;
			}
		}
	}

	if (!createdWindowAlready)
	{
		append(&uiState->openWindows, newWindow);
		makeWindowActive(uiState, uiState->openWindows.itemCount-1);
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
	s32 newActiveWindow = -1;
	s32 closeWindow = -1;
	Rect2I validWindowArea = irectCentreDim(uiState->uiBuffer->camera.pos, uiState->uiBuffer->camera.size);

	s32 windowIndex = 0;
	for (auto it = iterate(&uiState->openWindows);
		!it.isDone;
		next(&it), windowIndex++)
	{
		Window *window = get(it);
		f32 depth = 2000.0f - (20.0f * windowIndex);
		bool isActive = (windowIndex == 0);
		bool isModal = isActive && (window->flags & WinFlag_Modal) != 0;

		V4 backColor = (isActive ? window->style->backgroundColor : window->style->backgroundColorInactive);

		f32 barHeight = window->style->titleBarHeight;
		V4 barColor = (isActive ? window->style->titleBarColor : window->style->titleBarColorInactive);
		V4 titleColor = window->style->titleColor;

		f32 contentPadding = window->style->contentPadding;

		String closeButtonString = stringFromChars("X");
		V4 closeButtonColorHover = window->style->titleBarButtonHoverColor;

		BitmapFont *titleFont = getFont(uiState->assets, window->style->titleFontID);

		// Handle dragging/position first, BEFORE we use the window rect anywhere
		if (isModal)
		{
			// Modal windows can't be moved, they just auto-centre
			window->area = centreRectangle(window->area, validWindowArea);
		}
		else if (isActive && uiState->isDraggingWindow)
		{
			if (mouseButtonJustReleased(inputState, SDL_BUTTON_LEFT))
			{
				uiState->isDraggingWindow = false;
			}
			else
			{
				window->area.pos = v2i(uiState->windowDragWindowStartPos + (mousePos - getClickStartPos(inputState, SDL_BUTTON_LEFT, &uiState->uiBuffer->camera)));
			}
			
			uiState->mouseInputHandled = true;
		}

		// Keep window on screen
		window->area = confineRectangle(window->area, validWindowArea);

		// Window proc
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

			window->windowProc(&context, window->userData);

			if (window->flags & WinFlag_AutomaticHeight)
			{
				window->area.h = round_s32(barHeight + context.currentOffset.y + (contentPadding * 2.0f));
			}

			if (context.closeRequested)
			{
				closeWindow = windowIndex;
			}
		}

		Rect2 wholeWindowArea = rect2(window->area);
		Rect2 barArea = rectXYWH(wholeWindowArea.x, wholeWindowArea.y, wholeWindowArea.w, barHeight);
		Rect2 closeButtonRect = rectXYWH(wholeWindowArea.x + wholeWindowArea.w - barHeight, wholeWindowArea.y, barHeight, barHeight);
		Rect2 contentArea = getWindowContentArea(window->area, barHeight, 0);

		bool hoveringOverCloseButton = inRect(closeButtonRect, mousePos);

		if (!uiState->mouseInputHandled
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
				if (!isModal && inRect(barArea, mousePos))
				{
					// If we're inside the title bar, start dragging!
					uiState->isDraggingWindow = true;
					uiState->windowDragWindowStartPos = v2(window->area.pos);
				}

				// Make this the active window! 
				newActiveWindow = windowIndex;
			}

			uiState->mouseInputHandled = true;
		}

		drawRect(uiState->uiBuffer, contentArea, depth, backColor);
		drawRect(uiState->uiBuffer, barArea, depth, barColor);
		uiText(uiState, titleFont, window->title, barArea.pos + v2(8.0f, barArea.h * 0.5f), ALIGN_V_CENTRE | ALIGN_LEFT, depth + 1.0f, titleColor);

		if (hoveringOverCloseButton && !uiState->mouseInputHandled)  drawRect(uiState->uiBuffer, closeButtonRect, depth + 1.0f, closeButtonColorHover);
		uiText(uiState, titleFont, closeButtonString, centre(closeButtonRect), ALIGN_CENTRE, depth + 2.0f, titleColor);

		if (isModal)
		{
			uiState->mouseInputHandled = true;
			drawRect(uiState->uiBuffer, rectPosSize(v2(0,0), uiState->uiBuffer->camera.size), depth - 1.0f, color255(64, 64, 64, 128)); 
		}

		if (inRect(wholeWindowArea, mousePos))
		{
			uiState->mouseInputHandled = true;
		}
	}

	if (closeWindow != -1)
	{
		uiState->isDraggingWindow = false;
		removeIndex(&uiState->openWindows, closeWindow, true);
	}
	/*
	 * NB: This is an imaginary else-if, because if we try to set a new active window, AND close one,
	 * then things break. However, we never intentionally do that! I'm leaving this code "dangerous",
	 * because that way, we crash when we try to do both at once, instead of hiding the bug.
	 * - Sam, 3/2/2019
	 */
	if (newActiveWindow != -1)
	{
		makeWindowActive(uiState, newActiveWindow);
	}
}