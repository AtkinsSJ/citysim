#pragma once

void window_label(WindowContext *context, String text, char *styleName)
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

		V2 size = calculateTextSize(font, text, maxWidth);

		if (!context->measureOnly)
		{
			V2 topLeft = calculateTextPosition(origin, size, alignment);
			drawText(context->uiState->uiBuffer, font, text, topLeft, maxWidth, context->renderDepth, style->textColor, context->uiState->textShaderID);
		}

		// For now, we'll always just start a new line.
		// We'll probably want something fancier later.
		context->currentOffset.y += size.y;
		
		context->largestItemWidth = max(size.x, context->largestItemWidth);
	}
}

bool window_button(WindowContext *context, String text, s32 textWidth)
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

		V2 textSize = calculateTextSize(font, text, maxWidth);
		Rect2 buttonBounds;
		if (textWidth == -1)
		{
 			buttonBounds = rectAligned(origin, textSize + v2(buttonPadding * 2.0f, buttonPadding * 2.0f), alignment);
		}
		else
		{
			buttonBounds = rectAligned(origin, v2((f32)textWidth, textSize.y) + v2(buttonPadding * 2.0f, buttonPadding * 2.0f), alignment);
		}

		if (!context->measureOnly)
		{
			V2 textOrigin = alignWithinRectangle(buttonBounds, textAlignment, buttonPadding);
			V2 textTopLeft = calculateTextPosition(textOrigin, textSize, textAlignment);

			drawText(context->uiState->uiBuffer, font, text, textTopLeft, maxWidth, context->renderDepth + 1.0f, style->textColor, context->uiState->textShaderID);

			if (!context->uiState->mouseInputHandled && contains(buttonBounds, mousePos))
			{
				// Mouse pressed: must have started and currently be inside the bounds to show anything
				// Mouse unpressed: show hover if in bounds
				if (mouseButtonPressed(input, MouseButton_Left))
				{
					if (contains(buttonBounds, getClickStartPos(input, MouseButton_Left, &context->uiState->uiBuffer->camera)))
					{
						backColor = style->pressedColor;
					}
				}
				else
				{
					if (mouseButtonJustReleased(input, MouseButton_Left)
					 && contains(buttonBounds, getClickStartPos(input, MouseButton_Left, &context->uiState->uiBuffer->camera)))
					{
						buttonClicked = true;
						context->uiState->mouseInputHandled = true;
					}

					backColor = style->hoverColor;
				}
			}

			drawRect(context->uiState->uiBuffer, buttonBounds, context->renderDepth, context->uiState->untexturedShaderID,backColor);
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
	newWindow.area = irectCentreSize(v2i(windowOrigin), v2i(width, height));
	
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
	Rect2I validWindowArea = irectCentreSize(v2i(uiState->uiBuffer->camera.pos), v2i(uiState->uiBuffer->camera.size));

	s32 windowIndex = 0;
	bool isActive = true;
	for (auto it = iterate(&uiState->openWindows);
		!it.isDone;
		next(&it), windowIndex++)
	{
		Window *window = get(it);

		f32 depth = 2000.0f - (20.0f * windowIndex);
		bool isModal     = isActive && (window->flags & WinFlag_Modal) != 0;
		bool hasTitleBar = (window->flags & WinFlag_Headless) == 0;
		bool isTooltip   = (window->flags & WinFlag_Tooltip) != 0;

		UIWindowStyle *windowStyle = findWindowStyle(&uiState->assets->theme, window->styleName);

		V4 backColor = (isActive ? windowStyle->backgroundColor : windowStyle->backgroundColorInactive);

		f32 barHeight = hasTitleBar ? windowStyle->titleBarHeight : 0;

		f32 contentPadding = windowStyle->contentPadding;

		WindowContext context = {};
		context.uiState = uiState;
		context.temporaryMemory = &globalAppState.globalTempArena;
		context.window = window;
		context.windowStyle = windowStyle;
		context.contentArea = getWindowContentArea(window->area, barHeight, contentPadding);
		context.currentOffset = v2(0,0);
		context.largestItemWidth = 0;
		context.alignment = ALIGN_TOP | ALIGN_LEFT;
		context.renderDepth = depth + 1.0f;
		context.perItemPadding = 4.0f;

		// Run the WindowProc once first so we can measure its size
		if (window->flags & (WinFlag_AutomaticHeight | WinFlag_ShrinkWidth))
		{
			context.measureOnly = true;
			window->windowProc(&context, window->userData);

			if (window->flags & WinFlag_AutomaticHeight)
			{
				window->area.h = round_s32(barHeight + context.currentOffset.y + (contentPadding * 2.0f));
			}

			if (window->flags & WinFlag_ShrinkWidth)
			{
				window->area.w = round_s32(context.largestItemWidth + (contentPadding * 2.0f));
			}
		}

		// Handle dragging/position first, BEFORE we use the window rect anywhere
		if (isModal)
		{
			// Modal windows can't be moved, they just auto-centre
			window->area = centreWithin(validWindowArea, window->area);
		}
		else if (isActive && uiState->isDraggingWindow)
		{
			if (mouseButtonJustReleased(inputState, MouseButton_Left))
			{
				uiState->isDraggingWindow = false;
			}
			else
			{
				window->area.pos = v2i(uiState->windowDragWindowStartPos + (mousePos - getClickStartPos(inputState, MouseButton_Left, &uiState->uiBuffer->camera)));
			}
			
			uiState->mouseInputHandled = true;
		}
		else if (isTooltip)
		{
			window->area.pos = v2i(uiState->uiBuffer->camera.mousePos) + windowStyle->offsetFromMouse;
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

		// Run the window proc FOR REALZ
		context.measureOnly = false;
		context.alignment = ALIGN_TOP | ALIGN_LEFT;
		context.currentOffset = v2(0,0);
		context.largestItemWidth = 0;
		context.contentArea = getWindowContentArea(window->area, barHeight, contentPadding);
		window->windowProc(&context, window->userData);
		if (context.closeRequested || isTooltip)
		{
			closeWindow = windowIndex;
		}

		Rect2 wholeWindowArea = rect2(window->area);
		Rect2 barArea = rectXYWH(wholeWindowArea.x, wholeWindowArea.y, wholeWindowArea.w, barHeight);
		Rect2 closeButtonRect = rectXYWH(wholeWindowArea.x + wholeWindowArea.w - barHeight, wholeWindowArea.y, barHeight, barHeight);
		Rect2 contentArea = getWindowContentArea(window->area, barHeight, 0);

		bool hoveringOverCloseButton = contains(closeButtonRect, mousePos);

		if (!uiState->mouseInputHandled
			 && contains(wholeWindowArea, mousePos)
			 && mouseButtonJustPressed(inputState, MouseButton_Left))
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
					uiState->windowDragWindowStartPos = v2(window->area.pos);
				}

				// Make this the active window! 
				newActiveWindow = windowIndex;
			}

			// Tooltips don't take mouse input
			if (!isTooltip)
			{
				uiState->mouseInputHandled = true;
			}
		}

		drawRect(uiState->uiBuffer, contentArea, depth, uiState->untexturedShaderID, backColor);

		if (hasTitleBar)
		{
			V4 barColor = (isActive ? windowStyle->titleBarColor : windowStyle->titleBarColorInactive);
			V4 titleColor = windowStyle->titleColor;

			String closeButtonString = makeString("X");
			V4 closeButtonColorHover = windowStyle->titleBarButtonHoverColor;

			BitmapFont *titleFont = getFont(uiState->assets, windowStyle->titleFontName);

			drawRect(uiState->uiBuffer, barArea, depth, uiState->untexturedShaderID, barColor);
			uiText(uiState, titleFont, window->title, barArea.pos + v2(8.0f, barArea.h * 0.5f), ALIGN_V_CENTRE | ALIGN_LEFT, depth + 1.0f, titleColor);

			if (hoveringOverCloseButton && !uiState->mouseInputHandled)  drawRect(uiState->uiBuffer, closeButtonRect, depth + 1.0f, uiState->untexturedShaderID, closeButtonColorHover);
			uiText(uiState, titleFont, closeButtonString, centreOf(closeButtonRect), ALIGN_CENTRE, depth + 2.0f, titleColor);
		}

		if (isModal)
		{
			uiState->mouseInputHandled = true;
			drawRect(uiState->uiBuffer, rectPosSize(v2(0,0), uiState->uiBuffer->camera.size), depth - 1.0f, uiState->untexturedShaderID, color255(64, 64, 64, 128)); 
		}

		if (contains(wholeWindowArea, mousePos))
		{
			// Tooltips don't take mouse input
			if (!isTooltip)
			{
				uiState->mouseInputHandled = true;
			}
		}

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
