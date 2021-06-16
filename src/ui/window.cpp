#pragma once

namespace UI
{
	WindowContext::WindowContext(Window *window, WindowStyle *windowStyle, bool hideWidgets, RenderBuffer *renderBuffer)
		: window(window),
		  windowStyle(windowStyle),
		  hideWidgets(hideWidgets),
		  renderBuffer(renderBuffer),
		  windowPanel(
		  	  irectXYWH(
		  	  		window->area.x,
		  	  		window->area.y + ((window->flags & WindowFlags::Headless) ? 0 : windowStyle->titleBarHeight),
		  	  		window->area.w,
		  	  		(window->flags & WindowFlags::AutomaticHeight) ? 10000 : 
		  	  			(window->area.h - ((window->flags & WindowFlags::Headless) ? 0 : windowStyle->titleBarHeight))
		  	  ),
		  	  getStyle<PanelStyle>(&windowStyle->panelStyle), 
		      ((window->flags & WindowFlags::Tooltip) ? PanelFlags::AllowClickThrough : 0)
		       | (hideWidgets ? PanelFlags::HideWidgets : 0),
		       renderBuffer
		  )
	{}

	WindowTitle WindowTitle::none()
	{
		WindowTitle result = {};
		result.type = Type::None;

		return result;
	}

	WindowTitle WindowTitle::fromTextAsset(String assetID)
	{
		WindowTitle result = {};
		result.type = Type::TextAsset;
		result.assetID = assetID;

		return result;
	}

	WindowTitle WindowTitle::fromLambda(String (*lambda)())
	{
		WindowTitle result = {};
		result.type = Type::Calculated;
		result.calculateTitle = lambda;

		return result;
	}

	String WindowTitle::getString()
	{
		String result = nullString;

		switch (type)
		{
			case Type::None:       result = nullString;       break;;
			case Type::TextAsset:  result = getText(assetID); break;
			case Type::Calculated: result = calculateTitle(); break;
			INVALID_DEFAULT_CASE;
		}

		return result;
	}

	/**
	 * Creates an (in-game) window in the centre of the screen, and puts it in front of all other windows.
	 */
	void showWindow(UI::WindowTitle title, s32 width, s32 height, V2I position, String styleName, u32 flags, WindowProc windowProc, void *userData, WindowProc onClose)
	{
		if (windowProc == null)
		{
			logError("showWindow() called with a null WindowProc. That doesn't make sense? Title: {0}"_s, {title.getString()});
			return;
		}

		Window newWindow = {};
		newWindow.id = uiState.nextWindowID++;
		newWindow.title = title;
		newWindow.flags = flags;
		newWindow.styleName = styleName;

		newWindow.area = irectPosSize(position, v2i(width, height));
		
		newWindow.windowProc = windowProc;
		newWindow.userData = userData;
		newWindow.onClose = onClose;

		newWindow.renderBuffer = getTemporaryRenderBuffer(newWindow.title.getString());

		bool createdWindowAlready = false;

		if (uiState.openWindows.count > 0)
		{
			// If the window wants to be unique, then we search for an existing one with the same WindowProc
			if (flags & WindowFlags::Unique)
			{
				Window *toReplace = null;

				s32 oldWindowIndex = 0;
				for (auto it = uiState.openWindows.iterate();
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
					uiState.windowsToMakeActive.add(oldWindowIndex);
					createdWindowAlready = true;
				}
			}
		}

		if (!createdWindowAlready)
		{
			uiState.openWindows.append(newWindow);
			uiState.windowsToMakeActive.add(truncate32(uiState.openWindows.count-1));
		}
	}

	bool hasPauseWindowOpen()
	{
		return uiState.isAPauseWindowOpen;
	}

	bool isWindowOpen(WindowProc windowProc)
	{
		Indexed<Window*> windowToRemove = uiState.openWindows.findFirst([&](Window *window) { return window->windowProc == windowProc; });

		return windowToRemove.index != -1;
	}

	void closeWindow(WindowProc windowProc)
	{
		Indexed<Window*> windowToRemove = uiState.openWindows.findFirst([&](Window *window) { return window->windowProc == windowProc; });

		if (windowToRemove.index != -1)
		{
			uiState.windowsToClose.add(windowToRemove.index);
		}
		else if (!uiState.openWindows.isEmpty())
		{
			logInfo("closeWindow() call didn't find any windows that matched the WindowProc."_s);
		}
	}

	void closeAllWindows()
	{
		for (s32 windowIndex = 0; windowIndex < uiState.openWindows.count; windowIndex++)
		{
			uiState.windowsToClose.add(windowIndex);
		}
	}

	void updateAndRenderWindows()
	{
		DEBUG_FUNCTION_T(DCDT_UI);

		// This is weird, the UI camera should always be positioned with 0,0 being the bottom-left I thought?
		Rect2I validWindowArea = irectCentreSize(v2i(renderer->uiCamera.pos), v2i(renderer->uiCamera.size));

		uiState.isAPauseWindowOpen = false;
		s32 tooltipIndex = -1;

		bool isActive = true;
		for (auto it = uiState.openWindows.iterate();
			it.hasNext();
			it.next())
		{
			Window *window = it.get();
			s32 windowIndex = it.getIndex();

			// Skip this Window if we've requested to close it.
			if (uiState.windowsToClose.contains(windowIndex)) continue;

			bool isModal     = (window->flags & WindowFlags::Modal) != 0;
			bool hasTitleBar = (window->flags & WindowFlags::Headless) == 0;
			bool isTooltip   = (window->flags & WindowFlags::Tooltip) != 0;

			if (isTooltip)
			{
				ASSERT(tooltipIndex == -1); // Multiple tooltips???
				tooltipIndex = windowIndex;
			}

			bool shrinkWidth  = (window->flags & WindowFlags::ShrinkWidth) != 0;
			bool shrinkHeight = (window->flags & WindowFlags::AutomaticHeight) != 0;

			WindowStyle *windowStyle = getStyle<WindowStyle>(window->styleName);

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

				WindowContext context = WindowContext(window, windowStyle, true, null);
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
				window->area = centreWithin(validWindowArea, window->area.size);
			}
			else if (isDragging((void*)window->id))
			{
				window->area.pos = getDraggingObjectPos();
			}
			else if (isTooltip)
			{
				window->area.pos = mousePos + windowStyle->offsetFromMouse;
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
			WindowContext context = WindowContext(window, windowStyle, false, window->renderBuffer);
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
				uiState.windowsToClose.add(windowIndex);
			}

			if (!context.closeRequested && ((window->flags & WindowFlags::Pause) != 0))
			{
				uiState.isAPauseWindowOpen = true;
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
				String titleString = window->title.getString();

				drawSingleRect(window->renderBuffer, barArea, renderer->shaderIds.untextured, barColor);
				V2I titleSize = calculateTextSize(titleFont, titleString, barArea.w);
				Rect2I titleBounds = irectAligned(barArea.pos + v2i(0, barArea.h / 2), titleSize, ALIGN_V_CENTRE | ALIGN_LEFT);
				drawText(window->renderBuffer, titleFont, titleString, titleBounds, ALIGN_V_CENTRE | ALIGN_LEFT, windowStyle->titleColor, renderer->shaderIds.text);

				// TODO: Replace this with an actual Button?
				if (hoveringOverCloseButton
				 && (!isMouseInputHandled() || windowIndex == 0))
				{
					drawSingleRect(window->renderBuffer, closeButtonRect, renderer->shaderIds.untextured, closeButtonColorHover);
				}
				V2I closeTextSize = calculateTextSize(titleFont, closeButtonString);
				Rect2I closeTextBounds = irectAligned(centreOfI(closeButtonRect), closeTextSize, ALIGN_CENTRE);
				drawText(window->renderBuffer, titleFont, closeButtonString, closeTextBounds, ALIGN_CENTRE, windowStyle->titleColor, renderer->shaderIds.text);

				if ((!isMouseInputHandled() || windowIndex == 0)
					 && contains(wholeWindowArea, mousePos)
					 && mouseButtonJustPressed(MouseButton_Left))
				{
					if (hoveringOverCloseButton)
					{
						// If we're inside the X, close it!
						uiState.windowsToClose.add(windowIndex);
					}
					else
					{
						if (!isModal && contains(barArea, mousePos))
						{
							// If we're inside the title bar, start dragging!

							// @Hack: We're pretending the window ID is a pointer, which does work, but could conflict
							// with anything else that uses an ID instead of a pointer! (Or, I suppose, with real
							// pointers if they're in low memory or we spawn a ridiculous number of windows.)
							//
							// The solution would be to either allocate windows individually, or store their order in
							// an array that's separate abd just uses pointers.
							startDragging((void*)window->id, window->area.pos);
						}

						// Make this the active window!
						uiState.windowsToMakeActive.add(windowIndex);
					}

					// Tooltips don't take mouse input
					if (!isTooltip)
					{
						markMouseInputHandled();
					}
				}
			}

			// Prevent anything behind a modal window from interacting with the mouse
			if (isModal)
			{
				markMouseInputHandled(); 
			}
			// Prevent anything behind this window from interacting with the mouse
			else if (contains(wholeWindowArea, mousePos))
			{
				// Tooltips don't take mouse input
				if (!isTooltip)
				{
					markMouseInputHandled();
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
			uiState.windowsToMakeActive.add(tooltipIndex);
		}

		// Close any windows that were requested
		if (!uiState.windowsToClose.isEmpty())
		{
			Array<s32> windowsToClose = uiState.windowsToClose.asSortedArray();

			for (s32 i = windowsToClose.count - 1; i >= 0; i--)
			{
				s32 windowIndex = windowsToClose[i];
				closeWindow(windowIndex);
			}

			uiState.windowsToClose.clear();
		}

		// Activate any windows
		if (!uiState.windowsToMakeActive.isEmpty())
		{
			Array<s32> windowsToMakeActive = uiState.windowsToMakeActive.asSortedArray();

			// NB: Because the windowsToMakeActive are in ascending index order, and we always move them to a lower position,
			// we are guaranteed to not perturb the positions of any later window indices, so this operation is completely
			// safe, and we can do it really simply. Hooray!
			// - Sam, 07/04/2021
			for (s32 i = 0; i < windowsToMakeActive.count; i++)
			{
				s32 windowIndex = windowsToMakeActive[i];

				// Don't do anything if it's already the active window.
				if (windowIndex == 0) continue;

				uiState.openWindows.moveItemKeepingOrder(windowIndex, 0);
			}

			uiState.windowsToMakeActive.clear();
		}

		// Now, render the windows in the correct order!
		for (auto it = uiState.openWindows.iterateBackwards();
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
			ASSERT((uiState.openWindows.get(tooltipIndex)->flags & WindowFlags::Tooltip) != 0);
			closeWindow(tooltipIndex);
		}
	}

	inline Rect2I getWindowContentArea(Rect2I windowArea, s32 barHeight, s32 margin)
	{
		return irectXYWH(windowArea.x + margin,
						windowArea.y + barHeight + margin,
						windowArea.w - (margin * 2),
						windowArea.h - barHeight - (margin * 2));
	}

	inline void closeWindow(s32 windowIndex)
	{
		uiState.windowsToMakeActive.remove(windowIndex);

		Window *window = uiState.openWindows.get(windowIndex);
		
		if (window->onClose != null)
		{
			window->onClose(null, window->userData);
		}

		if (window->renderBuffer != null)
		{
			returnTemporaryRenderBuffer(window->renderBuffer);
			window->renderBuffer = null;
		}
		
		uiState.openWindows.removeIndex(windowIndex, true);
	}
}
