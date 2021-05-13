#pragma once

WindowContext::WindowContext(Window *window, UIWindowStyle *windowStyle, UIState *uiState, bool doUpdate, bool doRender, RenderBuffer *renderBuffer)
	: window(window),
	  windowStyle(windowStyle),
	  uiState(uiState),
	  doUpdate(doUpdate),
	  doRender(doRender),
	  renderBuffer(renderBuffer),
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
	       | (doRender ? Panel_DoRender : 0),
	       renderBuffer
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

	newWindow.renderBuffer = getTemporaryRenderBuffer(newWindow.title);

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
				uiState->windowsToMakeActive.add(oldWindowIndex);
				createdWindowAlready = true;
			}
		}
	}

	if (!createdWindowAlready)
	{
		uiState->openWindows.append(newWindow);
		uiState->windowsToMakeActive.add(truncate32(uiState->openWindows.count-1));
	}
}

void closeWindow(WindowProc windowProc)
{
	UIState *uiState = globalAppState.uiState;
	Indexed<Window*> windowToRemove = uiState->openWindows.findFirst([&](Window *window) { return window->windowProc == windowProc; });

	if (windowToRemove.index != -1)
	{
		uiState->windowsToClose.add(windowToRemove.index);
	}
	else if (!uiState->openWindows.isEmpty())
	{
		logInfo("closeWindow() call didn't find any windows that matched the WindowProc."_s);
	}
}

void closeAllWindows()
{
	UIState *uiState = globalAppState.uiState;
	for (s32 windowIndex = 0; windowIndex < uiState->openWindows.count; windowIndex++)
	{
		uiState->windowsToClose.add(windowIndex);
	}
}

void updateAndRenderWindows(UIState *uiState)
{
	DEBUG_FUNCTION();

	V2I mousePos = v2i(renderer->uiCamera.mousePos);
	Rect2I validWindowArea = irectCentreSize(v2i(renderer->uiCamera.pos), v2i(renderer->uiCamera.size));

	uiState->isAPauseWindowOpen = false;
	s32 tooltipIndex = -1;

	bool isActive = true;
	for (auto it = uiState->openWindows.iterate();
		it.hasNext();
		it.next())
	{
		Window *window = it.get();
		s32 windowIndex = it.getIndex();

		// Skip this Window if we've requested to close it.
		if (uiState->windowsToClose.contains(windowIndex)) continue;

		bool isModal     = (window->flags & WinFlag_Modal) != 0;
		bool hasTitleBar = (window->flags & WinFlag_Headless) == 0;
		bool isTooltip   = (window->flags & WinFlag_Tooltip) != 0;

		if (isTooltip)
		{
			ASSERT(tooltipIndex == -1); // Multiple tooltips???
			tooltipIndex = windowIndex;
		}

		bool shrinkWidth  = (window->flags & WinFlag_ShrinkWidth) != 0;
		bool shrinkHeight = (window->flags & WinFlag_AutomaticHeight) != 0;

		UIWindowStyle *windowStyle = findStyle<UIWindowStyle>(window->styleName);

		s32 barHeight = hasTitleBar ? windowStyle->titleBarHeight : 0;

		// Modal windows get a translucent colour over everything behind them
		if (isModal)
		{
			drawSingleRect(window->renderBuffer, rectPosSize(v2(0,0), renderer->uiCamera.size), renderer->shaderIds.untextured, color255(64, 64, 64, 128)); 
		}

		// If the window is new, make sure it has a valid area by running the WindowProc once
		// (Otherwise, windows may appear in the wrong place or at the wrong size on the frame they are created.)
		if (!window->isInitialised)
		{
			window->isInitialised = true;

			WindowContext context = WindowContext(window, windowStyle, uiState, false, false, null);
			window->windowProc(&context, window->userData);
			context.windowPanel.end(shrinkHeight, shrinkWidth);

			if (shrinkHeight)
			{
				window->area.h = barHeight + context.windowPanel.bounds.h;
			}

			if (shrinkWidth)
			{
				window->area.w = context.windowPanel.bounds.w;
				window->area.x = context.windowPanel.bounds.x;
			}
		}

		// Handle dragging the window
		if (isModal)
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
			
			UI::markMouseInputHandled();
		}
		else if (isTooltip)
		{
			window->area.pos = v2i(renderer->uiCamera.mousePos) + windowStyle->offsetFromMouse;
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

		// Actually run the window proc
		WindowContext context = WindowContext(window, windowStyle, uiState, true, true, window->renderBuffer);
		window->windowProc(&context, window->userData);
		context.windowPanel.end(shrinkHeight, shrinkWidth);

		if (shrinkHeight)
		{
			window->area.h = barHeight + context.windowPanel.bounds.h;
		}

		if (shrinkWidth)
		{
			window->area.w = context.windowPanel.bounds.w;
			window->area.x = context.windowPanel.bounds.x;
		}

		if (context.closeRequested)
		{
			uiState->windowsToClose.add(windowIndex);
		}

		if (!context.closeRequested && ((window->flags & WinFlag_Pause) != 0))
		{
			uiState->isAPauseWindowOpen = true;
		}

		Rect2I wholeWindowArea = window->area;
		Rect2I barArea = irectXYWH(wholeWindowArea.x, wholeWindowArea.y, wholeWindowArea.w, barHeight);
		Rect2I closeButtonRect = irectXYWH(wholeWindowArea.x + wholeWindowArea.w - barHeight, wholeWindowArea.y, barHeight, barHeight);
		Rect2I contentArea = getWindowContentArea(window->area, barHeight, 0);

		if (hasTitleBar)
		{
			bool hoveringOverCloseButton = contains(closeButtonRect, mousePos);

			V4 barColor = (isActive ? windowStyle->titleBarColor : windowStyle->titleBarColorInactive);

			String closeButtonString = "X"_s;
			V4 closeButtonColorHover = windowStyle->titleBarButtonHoverColor;

			BitmapFont *titleFont = getFont(&windowStyle->titleFont);

			drawSingleRect(window->renderBuffer, barArea, renderer->shaderIds.untextured, barColor);
			uiText(window->renderBuffer, titleFont, window->title, barArea.pos + v2i(8, barArea.h / 2), ALIGN_V_CENTRE | ALIGN_LEFT, windowStyle->titleColor);

			if (hoveringOverCloseButton
			 && (!UI::isMouseInputHandled() || windowIndex == 0))
			{
				drawSingleRect(window->renderBuffer, closeButtonRect, renderer->shaderIds.untextured, closeButtonColorHover);
			}
			uiText(window->renderBuffer, titleFont, closeButtonString, v2i(centreOf(closeButtonRect)), ALIGN_CENTRE, windowStyle->titleColor);

			if ((!UI::isMouseInputHandled() || windowIndex == 0)
				 && contains(wholeWindowArea, mousePos)
				 && mouseButtonJustPressed(MouseButton_Left))
			{
				if (hoveringOverCloseButton)
				{
					// If we're inside the X, close it!
					uiState->windowsToClose.add(windowIndex);
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
					uiState->windowsToMakeActive.add(windowIndex);
				}

				// Tooltips don't take mouse input
				if (!isTooltip)
				{
					UI::markMouseInputHandled();
				}
			}
		}

		// Prevent anything behind a modal window from interacting with the mouse
		if (isModal)
		{
			UI::markMouseInputHandled(); 
		}
		// Prevent anything behind this window from interacting with the mouse
		else if (contains(wholeWindowArea, mousePos))
		{
			// Tooltips don't take mouse input
			if (!isTooltip)
			{
				UI::markMouseInputHandled();
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

	// Put the tooltip on top of everything else
	// FIXME: Actually, this won't do that! windowsToMakeActive is not FIFO! But, it's better than nothing
	if (tooltipIndex != -1)
	{
		uiState->windowsToMakeActive.add(tooltipIndex);
	}

	// Close any windows that were requested
	if (!uiState->windowsToClose.isEmpty())
	{
		Array<s32> windowsToClose = uiState->windowsToClose.asSortedArray();

		for (s32 i = windowsToClose.count - 1; i >= 0; i--)
		{
			s32 windowIndex = windowsToClose[i];
			closeWindow(uiState, windowIndex);
		}

		uiState->windowsToClose.clear();
	}

	// Activate any windows
	if (!uiState->windowsToMakeActive.isEmpty())
	{
		Array<s32> windowsToMakeActive = uiState->windowsToMakeActive.asSortedArray();

		// NB: Because the windowsToMakeActive are in ascending index order, and we always move them to a lower position,
		// we are guaranteed to not perturb the positions of any later window indices, so this operation is completely
		// safe, and we can do it really simply. Hooray!
		// - Sam, 07/04/2021
		for (s32 i = 0; i < windowsToMakeActive.count; i++)
		{
			s32 windowIndex = windowsToMakeActive[i];

			// Don't do anything if it's already the active window.
			if (windowIndex == 0) continue;

			uiState->openWindows.moveItemKeepingOrder(windowIndex, 0);
		}

		uiState->windowsToMakeActive.clear();
	}

	// Now, render the windows in the correct order!
	for (auto it = uiState->openWindows.iterateBackwards();
		it.hasNext();
		it.next())
	{
		Window *window = it.get();
		transferRenderBufferData(window->renderBuffer, &renderer->windowBuffer);
	}

	// Remove the tooltip now that it's been shown
	if (tooltipIndex != -1)
	{
		// We moved the tooltip to index 0 when we did the activate-windows loop!
		tooltipIndex = 0; 
		ASSERT((uiState->openWindows.get(tooltipIndex)->flags & WinFlag_Tooltip) != 0);
		closeWindow(uiState, tooltipIndex);
	}
}

inline static
Rect2I getWindowContentArea(Rect2I windowArea, s32 barHeight, s32 margin)
{
	return irectXYWH(windowArea.x + margin,
					windowArea.y + barHeight + margin,
					windowArea.w - (margin * 2),
					windowArea.h - barHeight - (margin * 2));
}

inline static void closeWindow(UIState *uiState, s32 windowIndex)
{
	uiState->windowsToMakeActive.remove(windowIndex);

	Window *window = uiState->openWindows.get(windowIndex);
	
	if (window->onClose != null)
	{
		window->onClose(null, window->userData);
	}

	if (window->renderBuffer != null)
	{
		returnTemporaryRenderBuffer(window->renderBuffer);
		window->renderBuffer = null;
	}
	
	uiState->openWindows.removeIndex(windowIndex, true);
}
