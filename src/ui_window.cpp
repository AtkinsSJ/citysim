#pragma once

void window_label(WindowContext *context, String text, char *styleName=null)
{
	DEBUG_FUNCTION();

	UILabelStyle *style = null;
	if (styleName)      style = findLabelStyle(&context->uiState->assets->theme, makeString(styleName));
	if (style == null)  style = findLabelStyle(&context->uiState->assets->theme, context->windowStyle->labelStyleName);

	// Add padding between this and the previous element
	if (context->currentOffset.y > 0)
	{
		context->currentOffset.y += context->perItemPadding;
	}

	s32 alignment = context->alignment;
	V2 origin = context->contentArea.pos + context->currentOffset;

	if (alignment & ALIGN_RIGHT)
	{
		origin.x = context->contentArea.pos.x + context->contentArea.w;
	}
	else if (alignment & ALIGN_H_CENTRE)
	{
		origin.x = context->contentArea.pos.x + (context->contentArea.w  / 2.0f);
	}

	BitmapFont *font = getFont(context->uiState->assets, style->fontName);
	if (font)
	{
		f32 maxWidth = context->contentArea.w - context->currentOffset.x;

		V2 size = {};

		if (context->measureOnly)
		{
			size = calculateTextSize(font, text, maxWidth);
		}
		else
		{
			BitmapFontCachedText *textCache = drawTextToCache(context->temporaryMemory, font, text, maxWidth);
			V2 topLeft = calculateTextPosition(textCache, origin, alignment);
			drawCachedText(context->uiState->uiBuffer, textCache, topLeft, context->renderDepth, style->textColor);
			size = textCache->size;
		}

		// For now, we'll always just start a new line.
		// We'll probably want something fancier later.
		context->currentOffset.y += size.y;
	}
}

/*
 * If you pass textWidth, then the button will be sized as though the text was that size. If you leave
 * it blank (or pass -1 manually) then the button will be automatically sized to wrap the contained text.
 * Either way, it always matches the size vertically.
 */
bool window_button(WindowContext *context, String text, s32 textWidth=-1)
{
	DEBUG_FUNCTION();
	
	bool buttonClicked = false;
	UIButtonStyle *style = findButtonStyle(&context->uiState->assets->theme, context->windowStyle->buttonStyleName);
	InputState *input = context->uiState->input;

	u32 textAlignment = style->textAlignment;
	V4 backColor = style->backgroundColor;
	f32 buttonPadding = style->padding;
	V2 mousePos = context->uiState->uiBuffer->camera.mousePos;

	// Add padding between this and the previous element
	if (context->currentOffset.y > 0)
	{
		context->currentOffset.y += context->perItemPadding;
	}

	s32 alignment = context->alignment;
	V2 origin = context->contentArea.pos + context->currentOffset;

	if (alignment & ALIGN_RIGHT)
	{
		origin.x = context->contentArea.pos.x + context->contentArea.w;
	}
	else if (alignment & ALIGN_H_CENTRE)
	{
		origin.x = context->contentArea.pos.x + (context->contentArea.w  / 2.0f);
	}

	BitmapFont *font = getFont(context->uiState->assets, style->fontName);
	if (font)
	{
		f32 maxWidth;
		if (textWidth == -1)
		{
			maxWidth = context->contentArea.w - context->currentOffset.x - (2.0f * buttonPadding);
		}
		else
		{
			maxWidth = (f32) textWidth;
		}

		V2 buttonSize = {};

		if (context->measureOnly)
		{
			V2 textSize = calculateTextSize(font, text, maxWidth);
			Rect2 bounds;
			if (textWidth == -1)
			{
	 			bounds = rectAligned(origin, textSize + v2(buttonPadding * 2.0f, buttonPadding * 2.0f), alignment);
			}
			else
			{
				bounds = rectAligned(origin, v2((f32)textWidth, textSize.y) + v2(buttonPadding * 2.0f, buttonPadding * 2.0f), alignment);
			}
			buttonSize = bounds.size;
		}
		else
		{
			BitmapFontCachedText *textCache = drawTextToCache(context->temporaryMemory, font, text, maxWidth);

			Rect2 bounds;
			if (textWidth == -1)
			{
	 			bounds = rectAligned(origin, textCache->size + v2(buttonPadding * 2.0f, buttonPadding * 2.0f), alignment);
			}
			else
			{
				bounds = rectAligned(origin, v2((f32)textWidth, textCache->size.y) + v2(buttonPadding * 2.0f, buttonPadding * 2.0f), alignment);
			}
			buttonSize = bounds.size;

			V2 textOrigin = originWithinRectangle(bounds, textAlignment, buttonPadding);
			V2 textTopLeft = calculateTextPosition(textCache, textOrigin, textAlignment);
			drawCachedText(context->uiState->uiBuffer, textCache, textTopLeft, context->renderDepth + 1.0f, style->textColor);

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
		}

		// For now, we'll always just start a new line.
		// We'll probably want something fancier later.
		context->currentOffset.y += buttonSize.y;
	}

	return buttonClicked;
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
void showWindow(UIState *uiState, String title, s32 width, s32 height, String styleName, u32 flags, WindowProc windowProc, void *userData)
{
	if (windowProc == null)
	{
		logError("showWindow() called with a null WindowProc. That doesn't make sense? Title: {0}", {title});
		return;
	}

	Window newWindow = {};
	newWindow.title = title;
	newWindow.flags = flags;
	newWindow.styleName = styleName;

	V2 windowOrigin = uiState->uiBuffer->camera.pos;
	newWindow.area = irectCentreWH(v2i(windowOrigin), width, height);
	
	newWindow.windowProc = windowProc;
	newWindow.userData = userData;

	bool createdWindowAlready = false;

	if (uiState->openWindows.count > 0)
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
		makeWindowActive(uiState, truncate32(uiState->openWindows.count-1));
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
		UIWindowStyle *windowStyle = findWindowStyle(&uiState->assets->theme, window->styleName);

		V4 backColor = (isActive ? windowStyle->backgroundColor : windowStyle->backgroundColorInactive);

		f32 barHeight = windowStyle->titleBarHeight;
		V4 barColor = (isActive ? windowStyle->titleBarColor : windowStyle->titleBarColorInactive);
		V4 titleColor = windowStyle->titleColor;

		f32 contentPadding = windowStyle->contentPadding;

		String closeButtonString = makeString("X");
		V4 closeButtonColorHover = windowStyle->titleBarButtonHoverColor;

		BitmapFont *titleFont = getFont(uiState->assets, windowStyle->titleFontName);

		WindowContext context = {};
		context.uiState = uiState;
		context.temporaryMemory = &globalAppState.globalTempArena;
		context.window = window;
		context.windowStyle = windowStyle;
		context.contentArea = getWindowContentArea(window->area, barHeight, contentPadding);
		context.currentOffset = v2(0,0);
		context.alignment = ALIGN_TOP | ALIGN_LEFT;
		context.renderDepth = depth + 1.0f;
		context.perItemPadding = 4.0f;

		// Run the WindowProc once first so we can measure its size
		if (window->flags & WinFlag_AutomaticHeight)
		{
			context.measureOnly = true;
			window->windowProc(&context, window->userData);
			window->area.h = round_s32(barHeight + context.currentOffset.y + (contentPadding * 2.0f));
		}

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

		// Run the window proc FOR REALZ
		context.measureOnly = false;
		context.alignment = ALIGN_TOP | ALIGN_LEFT;
		context.currentOffset = v2(0,0);
		context.contentArea = getWindowContentArea(window->area, barHeight, contentPadding);
		window->windowProc(&context, window->userData);
		if (context.closeRequested)
		{
			closeWindow = windowIndex;
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