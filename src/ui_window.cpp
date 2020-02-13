#pragma once

void window_beginColumns(WindowContext *context)
{
	context->contentArea = irectXYWH(0,0,0,0);
	context->columnStartOffsetX = 0;
	context->columnIndex = -1;
}

void window_column(WindowContext *context, s32 width, ScrollbarState *scrollbar)
{
	context->columnStartOffsetX = context->columnStartOffsetX + context->contentArea.w + context->columnScrollbarWidth;
	if (context->columnIndex >= 0)
	{
		// NB: If we're here, then there was a previous column
		window_completeColumn(context);

		context->columnStartOffsetX += context->windowStyle->contentPadding;
	}

	context->columnIndex++;
	ColumnInfo *columnInfo = context->columnInfo + context->columnIndex;
	*columnInfo = {};

	columnInfo->width = width;
	if (columnInfo->width <= 0)
	{
		// width 0 means "fill the remainder"
		columnInfo->width = context->totalContentArea.w - (context->contentArea.w + context->columnStartOffsetX) - context->windowStyle->contentPadding;
	}

	context->columnScrollbarWidth = 0;

	context->contentArea = irectXYWH(context->totalContentArea.x + context->columnStartOffsetX, context->totalContentArea.y, columnInfo->width, context->totalContentArea.h);
	context->currentOffset = v2i(0,0);
	context->columnScrollbarState = scrollbar;

	if (scrollbar != null)
	{
		// Scrollbar!
		UIScrollbarStyle *scrollbarStyle = findScrollbarStyle(&assets->theme, context->windowStyle->scrollbarStyleName);

		context->contentArea.w -= scrollbarStyle->width;
		context->columnScrollbarWidth = scrollbarStyle->width;
	}

	if (context->doRender) addBeginScissor(&renderer->uiBuffer, rect2(context->contentArea));
}

void window_columnPercent(WindowContext *context, f32 widthPercent, ScrollbarState *scrollbar)
{
	s32 columnWidth = 0;

	if (widthPercent < 0.001f)
	{
		// width 0 means "fill the remainder"
		columnWidth = 0;
	}
	else
	{
		columnWidth = floor_s32(widthPercent * context->totalContentArea.w);
	}

	window_column(context, columnWidth, scrollbar);
}

Rect2I window_getColumnArea(WindowContext *context)
{
	return context->contentArea;
}

void window_completeColumn(WindowContext *context)
{
	if (context->doUpdate)
	{
		if (context->columnScrollbarState != null)
		{
			context->columnScrollbarState->contentSize = context->currentOffset.y;

			UIScrollbarStyle *scrollbarStyle = findScrollbarStyle(&assets->theme, context->windowStyle->scrollbarStyleName);
			Rect2I scrollbarArea = irectXYWH(context->contentArea.x + context->contentArea.w,
								context->contentArea.y,
								context->columnScrollbarWidth,
								context->contentArea.h);

			updateScrollbar(context->uiState, context->columnScrollbarState, scrollbarArea, scrollbarStyle);
		}
	}

	if (context->doRender)
	{
		addEndScissor(&renderer->uiBuffer);

		if (context->columnScrollbarState != null)
		{
			UIScrollbarStyle *scrollbarStyle = findScrollbarStyle(&assets->theme, context->windowStyle->scrollbarStyleName);
			Rect2I scrollbarArea = irectXYWH(context->contentArea.x + context->contentArea.w,
								context->contentArea.y,
								context->columnScrollbarWidth,
								context->contentArea.h);

			f32 scrollPercent = (f32)context->columnScrollbarState->scrollPosition / (f32)context->columnScrollbarState->contentSize;

			drawScrollbar(&renderer->uiBuffer, scrollPercent, scrollbarArea.pos, scrollbarArea.h, v2i(context->columnScrollbarWidth, context->columnScrollbarWidth), scrollbarStyle->knobColor, scrollbarStyle->backgroundColor, renderer->shaderIds.untextured);
		}
	}
}

void window_endColumns(WindowContext *context)
{
	window_completeColumn(context);

	ASSERT(context->columnIndex < ArrayCount(context->columnInfo));
}

V2I window_getCurrentLayoutPosition(WindowContext *context)
{
	V2I result = context->contentArea.pos + context->currentOffset;

	// Adjust if we're in a scrolling column area
	ColumnInfo *columnInfo = context->columnInfo + context->columnIndex;
	if (context->columnScrollbarState != null)
	{
		// if (*columnInfo->scrollYPercent > 0.5f)
		// {
		// 	int foo = 9;
		// }
		result.y = result.y - context->columnScrollbarState->scrollPosition;
	}

	return result;
}

void window_label(WindowContext *context, String text, String styleName)
{
	DEBUG_FUNCTION();

	UILabelStyle *style = null;
	if (!isEmpty(styleName))  style = findLabelStyle(&assets->theme, styleName);
	if (style == null)        style = findLabelStyle(&assets->theme, context->windowStyle->labelStyleName);

	// Add padding between this and the previous element
	if (context->currentOffset.y > 0)
	{
		context->currentOffset.y += context->perItemPadding;
	}

	u32 alignment = context->alignment;
	V2I origin = window_getCurrentLayoutPosition(context);

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

bool window_button(WindowContext *context, String text, s32 textWidth, bool isActive)
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

	s32 buttonAlignment = context->alignment;
	V2I buttonOrigin = window_getCurrentLayoutPosition(context);

	if (buttonAlignment & ALIGN_RIGHT)
	{
		buttonOrigin.x = context->contentArea.pos.x + context->contentArea.w;
	}
	else if (buttonAlignment & ALIGN_H_CENTRE)
	{
		buttonOrigin.x = context->contentArea.pos.x + (context->contentArea.w  / 2);
	}

	BitmapFont *font = getFont(style->fontName);
	if (font)
	{
		s32 buttonWidth;
		if (textWidth == -1)
		{
			buttonWidth = calculateButtonSize(text, style, context->contentArea.w - context->currentOffset.x).x;
		}
		else
		{
			buttonWidth = textWidth + (style->padding * 2);
		}

		V2I textSize = calculateTextSize(font, text, buttonWidth - (style->padding * 2));
		Rect2I buttonBounds = irectAligned(buttonOrigin, v2i(buttonWidth, textSize.y + (buttonPadding * 2)), buttonAlignment);

		if (context->doRender)
		{
			V4 backColor = style->backgroundColor;
			RenderItem_DrawSingleRect *background = appendDrawRectPlaceholder(&renderer->uiBuffer, renderer->shaderIds.untextured);

			V2I textOrigin = alignWithinRectangle(buttonBounds, textAlignment, buttonPadding);
			V2I textTopLeft = calculateTextPosition(textOrigin, textSize, textAlignment);
			Rect2I textBounds = irectPosSize(textTopLeft, textSize);
			drawText(&renderer->uiBuffer, font, text, textBounds, textAlignment, style->textColor, renderer->shaderIds.text);

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
			else if (isActive)
			{
				backColor = style->hoverColor;
			}

			fillDrawRectPlaceholder(background, buttonBounds, backColor);
		}

		if (context->doUpdate)
		{
			if (justClickedOnUI(context->uiState, buttonBounds))
			{
				buttonClicked = true;
			}
		}

		// For now, we'll always just start a new line.
		// We'll probably want something fancier later.
		context->currentOffset.y += buttonBounds.h;
		context->largestItemWidth = max(buttonBounds.w, context->largestItemWidth);
	}

	return buttonClicked;
}

bool window_textInput(WindowContext *context, TextInput *textInput, String styleName)
{
	DEBUG_FUNCTION();

	bool result = false;
	if (context->doUpdate)
	{
		result = updateTextInput(textInput);
	}

	UITextInputStyle *style = null;
	if (!isEmpty(styleName))  style = findTextInputStyle(&assets->theme, styleName);
	if (style == null)        style = findTextInputStyle(&assets->theme, "default"_s);

	// Add padding between this and the previous element
	if (context->currentOffset.y > 0)
	{
		context->currentOffset.y += context->perItemPadding;
	}

	u32 alignment = context->alignment;
	V2I origin = window_getCurrentLayoutPosition(context);

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
		s32 width = context->contentArea.w - context->currentOffset.x;
		V2I textInputSize = calculateTextInputSize(textInput, style, width);
		V2I topLeft = calculateTextPosition(origin, textInputSize, alignment);
		Rect2I bounds = irectPosSize(topLeft, textInputSize);

		if (context->doRender)
		{
			drawTextInput(&renderer->uiBuffer, textInput, style, bounds);
		}

		if (context->doUpdate)
		{
			// Capture the input focus if we just clicked on this TextInput
			if (justClickedOnUI(context->uiState, bounds))
			{
				captureInput(textInput);
				textInput->caretFlashCounter = 0;
			}
		}

		// For now, we'll always just start a new line.
		// We'll probably want something fancier later.
		context->currentOffset.y += textInputSize.y;
		
		context->largestItemWidth = max(textInputSize.x, context->largestItemWidth);
	}

	return result;
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
void showWindow(UIState *uiState, String title, s32 width, s32 height, V2I position, String styleName, u32 flags, WindowProc windowProc, void *userData, WindowProc onClose)
{
	if (windowProc == null)
	{
		logError("showWindow() called with a null WindowProc. That doesn't make sense? Title: {0}"_s, {title});
		return;
	}

	Window newWindow = {};
	newWindow.title = title;
	newWindow.flags = flags;
	newWindow.styleName = styleName;

	newWindow.area = irectPosSize(position, v2i(width, height));
	
	newWindow.windowProc = windowProc;
	newWindow.userData = userData;
	newWindow.onClose = onClose;

	bool createdWindowAlready = false;

	if (uiState->openWindows.count > 0)
	{
		// If the window wants to be unique, then we search for an existing one with the same WindowProc
		if (flags & WinFlag_Unique)
		{
			Window *toReplace = null;

			s32 oldWindowIndex = 0;
			for (auto it = iterate(&uiState->openWindows);
				hasNext(&it);
				next(&it))
			{
				Window *oldWindow = get(&it);
				if (oldWindow->windowProc == windowProc)
				{
					toReplace = oldWindow;
					oldWindowIndex = (s32) getIndex(&it);
					break;
				}
			}

			if (toReplace)
			{
				//
				// I don't think we actually want to keep the old window position, at least we don't in the
				// one case that actually uses this! (inspectTileWindowProc)
				// But leaving it commented in case I change my mind later.
				// - Sam, 08/08/2019
				//
				// Update 24/09/2019: I'm finding it really annoying having the window move, so un-commenting
				// this again!
				//

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
	context->doUpdate = true;
	context->doRender = false;

	context->totalContentArea = getWindowContentArea(context->window->area, (context->window->flags & WinFlag_Headless) ? 0 : context->windowStyle->titleBarHeight, context->windowStyle->contentPadding);
	context->contentArea = context->totalContentArea;
	context->currentOffset = v2i(0,0);
	context->largestItemWidth = 0;
	context->alignment = ALIGN_TOP | ALIGN_LEFT;
	context->perItemPadding = 4; // TODO: Make this part of the style!
}

void prepareForRender(WindowContext *context)
{
	context->doRender = true;
	context->doUpdate = false;
	
	context->totalContentArea = getWindowContentArea(context->window->area, (context->window->flags & WinFlag_Headless) ? 0 : context->windowStyle->titleBarHeight, context->windowStyle->contentPadding);
	context->contentArea = context->totalContentArea;
	context->currentOffset = v2i(0,0);
	context->largestItemWidth = 0;
	context->alignment = ALIGN_TOP | ALIGN_LEFT;
	context->perItemPadding = 4; // TODO: Make this part of the style!
}

void updateWindow(UIState *uiState, Window *window, WindowContext *context, bool isActive)
{
	V2I mousePos = v2i(renderer->uiCamera.mousePos);
	Rect2I validWindowArea = irectCentreSize(v2i(renderer->uiCamera.pos), v2i(renderer->uiCamera.size));

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
		hasNext(&it);
		next(&it))
	{
		Window *window = get(&it);
		s32 windowIndex = (s32) getIndex(&it);

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
		Window *window = get(&uiState->openWindows, closeWindow);
		if (window->onClose != null)
		{
			// TODO: We probably don't need to pass a context at all to the onClose() callback,
			// but I don't have a clear idea right now of what things we might want to do in
			// onClose() so I'm leaving it as a WindowProc for now, because I don't want to
			// litter the code with a bunch of different callback types. IDK.
			// - Sam, 16/01/2020
			UIWindowStyle *windowStyle = findWindowStyle(&assets->theme, window->styleName);
			WindowContext context = makeWindowContext(window, windowStyle, uiState);
			window->onClose(null, window->userData);
		}

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
		hasNext(&it);
		next(&it))
	{
		Window *window = get(&it);
		s32 windowIndex = getIndex(&it);

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

			String closeButtonString = "X"_s;
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
