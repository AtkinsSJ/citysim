#pragma once

void window_label(WindowContext *context, String text, char *styleName)
{
	DEBUG_FUNCTION();

	UILabelStyle *style = null;
	if (styleName)      style = findLabelStyle(&assets->theme, makeString(styleName));
	if (style == null)  style = findLabelStyle(&assets->theme, context->windowStyle->labelStyleName);

	// Add padding between this and the previous element
	if (context->currentOffset.y > 0)
	{
		context->currentOffset.y += context->perItemPadding;
	}

	u32 alignment = context->alignment;
	V2I origin = context->contentArea.pos + context->currentOffset;

	if (alignment & ALIGN_RIGHT)
	{
		origin.x = context->contentArea.pos.x + context->contentArea.w;
	}
	else if (alignment & ALIGN_H_CENTRE)
	{
		origin.x = context->contentArea.pos.x + (context->contentArea.w  / 2);
	}

	BitmapFont *font = getFont(style->fontName);
	if (font)
	{
		s32 maxWidth = context->contentArea.w - context->currentOffset.x;

		V2I textSize = calculateTextSize(font, text, maxWidth);
		V2I topLeft = calculateTextPosition(origin, textSize, alignment);
		Rect2I bounds = irectPosSize(topLeft, textSize);

		if (context->doRender)
		{
			drawText(&renderer->uiBuffer, font, text, bounds, alignment, style->textColor, renderer->shaderIds.text);
		}

		// For now, we'll always just start a new line.
		// We'll probably want something fancier later.
		context->currentOffset.y += textSize.y;
		
		context->largestItemWidth = max(textSize.x, context->largestItemWidth);
	}
}

bool window_button(WindowContext *context, String text, s32 textWidth)
{
	DEBUG_FUNCTION();
	
	bool buttonClicked = false;
	UIButtonStyle *style = findButtonStyle(&assets->theme, context->windowStyle->buttonStyleName);

	u32 textAlignment = style->textAlignment;
	s32 buttonPadding = style->padding;
	V2I mousePos = v2i(renderer->uiCamera.mousePos);

	// Add padding between this and the previous element
	if (context->currentOffset.y > 0)
	{
		context->currentOffset.y += context->perItemPadding;
	}

	s32 alignment = context->alignment;
	V2I origin = context->contentArea.pos + context->currentOffset;

	if (alignment & ALIGN_RIGHT)
	{
		origin.x = context->contentArea.pos.x + context->contentArea.w;
	}
	else if (alignment & ALIGN_H_CENTRE)
	{
		origin.x = context->contentArea.pos.x + (context->contentArea.w  / 2);
	}

	BitmapFont *font = getFont(style->fontName);
	if (font)
	{
		s32 maxWidth;
		if (textWidth == -1)
		{
			maxWidth = context->contentArea.w - context->currentOffset.x - (2 * buttonPadding);
		}
		else
		{
			maxWidth = textWidth;
		}

		// @Cleanup: This if() seems unnecessary, we could just put textWidth into textSize and use that?
		// It's weird and inconsistent, we treat textWidth as the width sometimes and maxWidth others, and
		// they could be different values because of calculateTextSize??? Actually I don't even know.
		V2I textSize = calculateTextSize(font, text, maxWidth);
		Rect2I buttonBounds;
		if (textWidth == -1)
		{
 			buttonBounds = irectAligned(origin, textSize + v2i(buttonPadding * 2, buttonPadding * 2), alignment);
		}
		else
		{
			buttonBounds = irectAligned(origin, v2i(textWidth, textSize.y) + v2i(buttonPadding * 2, buttonPadding * 2), alignment);
		}

		V2I textOrigin = alignWithinRectangle(buttonBounds, textAlignment, buttonPadding);
		V2I textTopLeft = calculateTextPosition(textOrigin, textSize, textAlignment);
		Rect2I bounds = irectPosSize(textTopLeft, textSize);

		if (context->doRender)
		{
			V4 backColor = style->backgroundColor;
			RenderItem_DrawSingleRect *background = appendDrawRectPlaceholder(&renderer->uiBuffer, renderer->shaderIds.untextured);

			drawText(&renderer->uiBuffer, font, text, bounds, textAlignment, style->textColor, renderer->shaderIds.text);

			if (context->window->wasActiveLastUpdate && contains(buttonBounds, mousePos))
			{
				// Mouse pressed: must have started and currently be inside the bounds to show anything
				// Mouse unpressed: show hover if in bounds
				if (mouseButtonPressed(MouseButton_Left))
				{
					if (contains(buttonBounds, getClickStartPos(MouseButton_Left, &renderer->uiCamera)))
					{
						backColor = style->pressedColor;
					}
				}
				else
				{
					backColor = style->hoverColor;
				}
			}

			fillDrawRectPlaceholder(background, buttonBounds, backColor);
		}

		if (!context->uiState->mouseInputHandled && contains(buttonBounds, mousePos))
		{
			if (mouseButtonJustReleased(MouseButton_Left)
			 && contains(buttonBounds, getClickStartPos(MouseButton_Left, &renderer->uiCamera)))
			{
				buttonClicked = true;
				context->uiState->mouseInputHandled = true;
			}
		}

		// For now, we'll always just start a new line.
		// We'll probably want something fancier later.
		context->currentOffset.y += buttonBounds.h;
		context->largestItemWidth = max(buttonBounds.w, context->largestItemWidth);
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
void showWindow(UIState *uiState, String title, s32 width, s32 height, V2I position, String styleName, u32 flags, WindowProc windowProc, void *userData)
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

	newWindow.area = irectPosSize(position, v2i(width, height));
	
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
				next(&it))
			{
				Window *oldWindow = get(it);
				if (oldWindow->windowProc == windowProc)
				{
					toReplace = oldWindow;
					oldWindowIndex = (s32) getIndex(it);
					break;
				}
			}

			if (toReplace)
			{
				// I don't think we actually want to keep the old window position, at least we don't in the
				// one case that actually uses this! (inspectTileWindowProc)
				// But leaving it commented in case I change my mind later.
				// - Sam, 08/08/2019

				// Make it keep the current window's position
				// newWindow.area = toReplace->area;

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
Rect2I getWindowContentArea(Rect2I windowArea, s32 barHeight, s32 contentPadding)
{
	return irectXYWH(windowArea.x + contentPadding,
					windowArea.y + barHeight + contentPadding,
					windowArea.w - (contentPadding * 2),
					windowArea.h - barHeight - (contentPadding * 2));
}

WindowContext makeWindowContext(Window *window, UIWindowStyle *windowStyle, UIState *uiState)
{
	WindowContext context = {};
	context.uiState = uiState;
	context.window = window;
	context.windowStyle = windowStyle;
	context.contentArea = getWindowContentArea(window->area, (window->flags & WinFlag_Headless) ? 0 : windowStyle->titleBarHeight, windowStyle->contentPadding);
	context.currentOffset = v2i(0,0);
	context.largestItemWidth = 0;
	context.alignment = ALIGN_TOP | ALIGN_LEFT;
	context.perItemPadding = 4; // TODO: Make this part of the style!

	return context;
}

void prepareForUpdate(WindowContext *context)
{
	context->doRender = false;

	context->contentArea = getWindowContentArea(context->window->area, (context->window->flags & WinFlag_Headless) ? 0 : context->windowStyle->titleBarHeight, context->windowStyle->contentPadding);
	context->currentOffset = v2i(0,0);
	context->largestItemWidth = 0;
	context->alignment = ALIGN_TOP | ALIGN_LEFT;
	context->perItemPadding = 4; // TODO: Make this part of the style!
}

void prepareForRender(WindowContext *context)
{
	context->doRender = true;
	
	context->contentArea = getWindowContentArea(context->window->area, (context->window->flags & WinFlag_Headless) ? 0 : context->windowStyle->titleBarHeight, context->windowStyle->contentPadding);
	context->currentOffset = v2i(0,0);
	context->largestItemWidth = 0;
	context->alignment = ALIGN_TOP | ALIGN_LEFT;
	context->perItemPadding = 4; // TODO: Make this part of the style!
}

void updateWindow(UIState *uiState, Window *window, WindowContext *context, bool isActive)
{
	V2I mousePos = v2i(renderer->uiCamera.mousePos);
	Rect2I validWindowArea = irectCentreSize(v2i(renderer->uiCamera.pos), v2i(renderer->uiCamera.size));

	if (window->flags & (WinFlag_AutomaticHeight | WinFlag_ShrinkWidth))
	{
		prepareForUpdate(context);
		window->windowProc(context, window->userData);

		if (window->flags & WinFlag_AutomaticHeight)
		{
			s32 barHeight = (window->flags & WinFlag_Headless) ? 0 : context->windowStyle->titleBarHeight;
			window->area.h = barHeight + context->currentOffset.y + (context->windowStyle->contentPadding * 2);
		}

		if (window->flags & WinFlag_ShrinkWidth)
		{
			window->area.w = context->largestItemWidth + (context->windowStyle->contentPadding * 2);
		}
	}

	// Handle dragging/position first, BEFORE we use the window rect anywhere
	if (window->flags & WinFlag_Modal)
	{
		// Modal windows can't be moved, they just auto-centre
		window->area = centreWithin(validWindowArea, window->area);
	}
	else if (isActive && uiState->isDraggingWindow)
	{
		if (mouseButtonJustReleased(MouseButton_Left))
		{
			uiState->isDraggingWindow = false;
		}
		else
		{
			V2I clickStartPos = v2i(getClickStartPos(MouseButton_Left, &renderer->uiCamera));
			window->area.x = uiState->windowDragWindowStartPos.x + mousePos.x - clickStartPos.x;
			window->area.y = uiState->windowDragWindowStartPos.y + mousePos.y - clickStartPos.y;
		}
		
		uiState->mouseInputHandled = true;
	}
	else if (window->flags & WinFlag_Tooltip)
	{
		window->area.pos = v2i(renderer->uiCamera.mousePos) + context->windowStyle->offsetFromMouse;
	}

	// Keep window on screen
	{
		// X
		if (window->area.w > validWindowArea.w)
		{
			// If it's too big, centre it.
			window->area.x = validWindowArea.x - ((window->area.w - validWindowArea.w) / 2);
		}
		else if (window->area.x < validWindowArea.x)
		{
			window->area.x = validWindowArea.x;
		}
		else if ((window->area.x + window->area.w) > (validWindowArea.x + validWindowArea.w))
		{
			window->area.x = validWindowArea.x + validWindowArea.w - window->area.w;
		}

		// Y
		if (window->area.h > validWindowArea.h)
		{
			// If it's too big, centre it.
			window->area.y = validWindowArea.y - ((window->area.h - validWindowArea.h) / 2);
		}
		else if (window->area.y < validWindowArea.y)
		{
			window->area.y = validWindowArea.y;
		}
		else if ((window->area.y + window->area.h) > (validWindowArea.y + validWindowArea.h))
		{
			window->area.y = validWindowArea.y + validWindowArea.h - window->area.h;
		}
	}
}

void updateWindows(UIState *uiState)
{
	DEBUG_FUNCTION();

	V2I mousePos = v2i(renderer->uiCamera.mousePos);
	s32 newActiveWindow = -1;
	s32 closeWindow = -1;
	Rect2I validWindowArea = irectCentreSize(v2i(renderer->uiCamera.pos), v2i(renderer->uiCamera.size));

	bool isActive = true;
	for (auto it = iterate(&uiState->openWindows);
		!it.isDone;
		next(&it))
	{
		Window *window = get(it);
		s32 windowIndex = (s32) getIndex(it);

		bool isModal     = (window->flags & WinFlag_Modal) != 0;
		bool hasTitleBar = (window->flags & WinFlag_Headless) == 0;
		bool isTooltip   = (window->flags & WinFlag_Tooltip) != 0;

		UIWindowStyle *windowStyle = findWindowStyle(&assets->theme, window->styleName);

		s32 barHeight = hasTitleBar ? windowStyle->titleBarHeight : 0;

		WindowContext context = makeWindowContext(window, windowStyle, uiState);

		// Run the WindowProc once first so we can measure its size
		updateWindow(uiState, window, &context, isActive);

		if (context.closeRequested || isTooltip)
		{
			closeWindow = windowIndex;
		}

		Rect2I wholeWindowArea = window->area;
		Rect2I barArea = irectXYWH(wholeWindowArea.x, wholeWindowArea.y, wholeWindowArea.w, barHeight);
		Rect2I closeButtonRect = irectXYWH(wholeWindowArea.x + wholeWindowArea.w - barHeight, wholeWindowArea.y, barHeight, barHeight);
		Rect2I contentArea = getWindowContentArea(window->area, barHeight, 0);

		bool hoveringOverCloseButton = contains(closeButtonRect, mousePos);

		if ((!uiState->mouseInputHandled || windowIndex == 0)
			 && contains(wholeWindowArea, mousePos)
			 && mouseButtonJustPressed(MouseButton_Left))
		{
			if (hoveringOverCloseButton)
			{
				// If we're inside the X, close it!
				closeWindow = windowIndex;
			}
			else
			{
				if (!isModal && contains(barArea, mousePos))
				{
					// If we're inside the title bar, start dragging!
					uiState->isDraggingWindow = true;
					uiState->windowDragWindowStartPos = window->area.pos;
				}

				// Make this the active window! 
				newActiveWindow = windowIndex;
			}

			// Tooltips don't take mouse inputState
			if (!isTooltip)
			{
				uiState->mouseInputHandled = true;
			}
		}

		if (isModal)
		{
			uiState->mouseInputHandled = true; 
		}

		if (contains(wholeWindowArea, mousePos))
		{
			// Tooltips don't take mouse inputState
			if (!isTooltip)
			{
				uiState->mouseInputHandled = true;
			}
		}

		window->wasActiveLastUpdate = isActive;

		//
		// NB: This is a little confusing, so some explanation:
		// Tooltips are windows, theoretically the front-most window, because they're shown fresh each frame.
		// We take the front-most window as the active one. Problem is, we don't want the "real" front-most
		// window to appear inactive just because a tooltip is visible. It feels weird and glitchy.
		// So, instead of "isActive = (i == 0)", weset it to true before the loop, and then set it to false
		// the first time we finish a window that wasn't a tooltip, which will be the front-most non-tooltip
		// window!
		// Actually, a similar thing will apply to UI messages once we port those, so I'll have to break it
		// into a separate WindowFlag.
		//
		// - Sam, 02/06/2019
		//
		if (!isTooltip)
		{
			isActive = false;
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

void renderWindows(UIState *uiState)
{
	V2I mousePos = v2i(renderer->uiCamera.mousePos);
	for (auto it = iterateBackwards(&uiState->openWindows);
		!it.isDone;
		next(&it))
	{
		Window *window = get(it);
		s32 windowIndex = getIndex(it);

		bool isActive = window->wasActiveLastUpdate;
		bool isModal     = (window->flags & WinFlag_Modal) != 0;
		bool hasTitleBar = (window->flags & WinFlag_Headless) == 0;

		if (isModal)
		{
			drawSingleRect(&renderer->uiBuffer, rectPosSize(v2(0,0), renderer->uiCamera.size), renderer->shaderIds.untextured, color255(64, 64, 64, 128)); 
		}

		UIWindowStyle *windowStyle = findWindowStyle(&assets->theme, window->styleName);
		WindowContext context = makeWindowContext(window, windowStyle, uiState);

		if (!window->isInitialised)
		{
			updateWindow(uiState, window, &context, isActive);
			window->isInitialised = true;
		}

		RenderItem_DrawSingleRect *contentBackground = appendDrawRectPlaceholder(&renderer->uiBuffer, renderer->shaderIds.untextured);
		prepareForRender(&context);
		window->windowProc(&context, window->userData);

		Rect2I wholeWindowArea = window->area;
		s32 barHeight = hasTitleBar ? windowStyle->titleBarHeight : 0;
		Rect2I barArea = irectXYWH(wholeWindowArea.x, wholeWindowArea.y, wholeWindowArea.w, barHeight);
		Rect2I closeButtonRect = irectXYWH(wholeWindowArea.x + wholeWindowArea.w - barHeight, wholeWindowArea.y, barHeight, barHeight);
		Rect2I contentArea = getWindowContentArea(window->area, barHeight, 0);

		bool hoveringOverCloseButton = contains(closeButtonRect, mousePos);

		V4 backColor = (isActive ? windowStyle->backgroundColor : windowStyle->backgroundColorInactive);
		fillDrawRectPlaceholder(contentBackground, contentArea, backColor);

		if (hasTitleBar)
		{
			V4 barColor = (isActive ? windowStyle->titleBarColor : windowStyle->titleBarColorInactive);
			V4 titleColor = windowStyle->titleColor;

			String closeButtonString = "X"s;
			V4 closeButtonColorHover = windowStyle->titleBarButtonHoverColor;

			BitmapFont *titleFont = getFont(windowStyle->titleFontName);

			drawSingleRect(&renderer->uiBuffer, barArea, renderer->shaderIds.untextured, barColor);
			uiText(&renderer->uiBuffer, titleFont, window->title, barArea.pos + v2i(8, barArea.h / 2), ALIGN_V_CENTRE | ALIGN_LEFT, titleColor);

			if (hoveringOverCloseButton
			 && (!uiState->mouseInputHandled || windowIndex == 0))
			{
				drawSingleRect(&renderer->uiBuffer, closeButtonRect, renderer->shaderIds.untextured, closeButtonColorHover);
			}
			uiText(&renderer->uiBuffer, titleFont, closeButtonString, v2i(centreOf(closeButtonRect)), ALIGN_CENTRE, titleColor);
		}
	}
}
