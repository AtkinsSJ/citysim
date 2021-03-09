#pragma once

WindowContext::WindowContext(Window *window, UIWindowStyle *windowStyle, UIState *uiState, bool doUpdate, bool doRender)
	: window(window),
	  windowStyle(windowStyle),
	  uiState(uiState),
	  doUpdate(doUpdate),
	  doRender(doRender),
	  windowPanel(
	  	  irectXYWH(
	  	  		window->area.x,
	  	  		window->area.y + ((window->flags & WinFlag_Headless) ? 0 : windowStyle->titleBarHeight),
	  	  		window->area.w,
	  	  		(window->flags & WinFlag_AutomaticHeight) ? 10000 : 
	  	  			(window->area.h - ((window->flags & WinFlag_Headless) ? 0 : windowStyle->titleBarHeight))
	  	  ),
	  	  findStyle<UIPanelStyle>(&windowStyle->panelStyle), 
	      Panel_LayoutTopToBottom
	       | ((window->flags & WinFlag_Tooltip) ? 0 : Panel_BlocksMouse)
	       | (doUpdate ? Panel_DoUpdate : 0)
	       | (doRender ? Panel_DoRender : 0)
	  )
{}

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
			for (auto it = uiState->openWindows.iterate();
				it.hasNext();
				it.next())
			{
				Window *oldWindow = it.get();
				if (oldWindow->windowProc == windowProc)
				{
					toReplace = oldWindow;
					oldWindowIndex = (s32) it.getIndex();
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
		uiState->openWindows.append(newWindow);
		makeWindowActive(uiState, truncate32(uiState->openWindows.count-1));
	}
}

void updateWindow(UIState *uiState, Window *window, WindowContext *context, bool isActive)
{
	V2I mousePos = v2i(renderer->uiCamera.mousePos);
	Rect2I validWindowArea = irectCentreSize(v2i(renderer->uiCamera.pos), v2i(renderer->uiCamera.size));

	window->windowProc(context, window->userData);

	bool shrinkWidth  = (window->flags & WinFlag_ShrinkWidth) != 0;
	bool shrinkHeight = (window->flags & WinFlag_AutomaticHeight) != 0;

	context->windowPanel.end(shrinkHeight, shrinkWidth);

	if (shrinkHeight)
	{
		s32 barHeight = (window->flags & WinFlag_Headless) ? 0 : context->windowStyle->titleBarHeight;
		window->area.h = barHeight + context->windowPanel.bounds.h;
	}

	if (shrinkWidth)
	{
		window->area.w = context->windowPanel.bounds.w;
		window->area.x = context->windowPanel.bounds.x;
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

	uiState->isAPauseWindowOpen = false;

	bool isActive = true;
	for (auto it = uiState->openWindows.iterate();
		it.hasNext();
		it.next())
	{
		Window *window = it.get();
		s32 windowIndex = (s32) it.getIndex();

		bool isModal     = (window->flags & WinFlag_Modal) != 0;
		bool hasTitleBar = (window->flags & WinFlag_Headless) == 0;
		bool isTooltip   = (window->flags & WinFlag_Tooltip) != 0;

		UIWindowStyle *windowStyle = findStyle<UIWindowStyle>(window->styleName);

		s32 barHeight = hasTitleBar ? windowStyle->titleBarHeight : 0;

		WindowContext context = WindowContext(window, windowStyle, uiState, true, false);

		// Run the WindowProc once first so we can measure its size
		updateWindow(uiState, window, &context, isActive);

		if (context.closeRequested || isTooltip)
		{
			closeWindow = windowIndex;
		}

		if (!context.closeRequested && ((window->flags & WinFlag_Pause) != 0))
		{
			uiState->isAPauseWindowOpen = true;
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

			// Tooltips don't take mouse input
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
			// Tooltips don't take mouse input
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
		Window *window = uiState->openWindows.get(closeWindow);
		if (window->onClose != null)
		{
			window->onClose(null, window->userData);
		}

		uiState->isDraggingWindow = false;
		uiState->openWindows.removeIndex(closeWindow, true);
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
	for (auto it = uiState->openWindows.iterateBackwards();
		it.hasNext();
		it.next())
	{
		Window *window = it.get();
		s32 windowIndex = it.getIndex();

		bool isActive = window->wasActiveLastUpdate;
		bool isModal     = (window->flags & WinFlag_Modal) != 0;
		bool hasTitleBar = (window->flags & WinFlag_Headless) == 0;

		bool shrinkWidth  = (window->flags & WinFlag_ShrinkWidth) != 0;
		bool shrinkHeight = (window->flags & WinFlag_AutomaticHeight) != 0;

		if (isModal)
		{
			drawSingleRect(&renderer->uiBuffer, rectPosSize(v2(0,0), renderer->uiCamera.size), renderer->shaderIds.untextured, color255(64, 64, 64, 128)); 
		}

		UIWindowStyle *windowStyle = findStyle<UIWindowStyle>(window->styleName);

		if (!window->isInitialised)
		{
			WindowContext updateContext = WindowContext(window, windowStyle, uiState, true, false);
			updateWindow(uiState, window, &updateContext, isActive);
			updateContext.windowPanel.end(shrinkHeight, shrinkWidth);
			window->isInitialised = true;
		}

		WindowContext context = WindowContext(window, windowStyle, uiState, false, true);
		window->windowProc(&context, window->userData);
		context.windowPanel.end(shrinkHeight, shrinkWidth);

		Rect2I wholeWindowArea = window->area;
		s32 barHeight = hasTitleBar ? windowStyle->titleBarHeight : 0;
		Rect2I barArea = irectXYWH(wholeWindowArea.x, wholeWindowArea.y, wholeWindowArea.w, barHeight);
		Rect2I closeButtonRect = irectXYWH(wholeWindowArea.x + wholeWindowArea.w - barHeight, wholeWindowArea.y, barHeight, barHeight);
		Rect2I contentArea = getWindowContentArea(window->area, barHeight, 0);

		bool hoveringOverCloseButton = contains(closeButtonRect, mousePos);

		if (hasTitleBar)
		{
			V4 barColor = (isActive ? windowStyle->titleBarColor : windowStyle->titleBarColorInactive);
			V4 titleColor = windowStyle->titleColor;

			String closeButtonString = "X"_s;
			V4 closeButtonColorHover = windowStyle->titleBarButtonHoverColor;

			BitmapFont *titleFont = getFont(&windowStyle->titleFont);

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

static void makeWindowActive(UIState *uiState, s32 windowIndex)
{
	// Don't do anything if it's already the active window.
	if (windowIndex == 0)  return;

	uiState->openWindows.moveItemKeepingOrder(windowIndex, 0);
}

inline static
Rect2I getWindowContentArea(Rect2I windowArea, s32 barHeight, s32 margin)
{
	return irectXYWH(windowArea.x + margin,
					windowArea.y + barHeight + margin,
					windowArea.w - (margin * 2),
					windowArea.h - barHeight - (margin * 2));
}
