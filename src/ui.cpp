// ui.cpp

void initUIState(UIState *uiState, MemoryArena *arena)
{
	*uiState = {};

	uiState->message = {};
	uiState->message.text = pushString(arena, 256);
	uiState->message.countdown = -1;

	initChunkedArray(&uiState->uiRects, arena, 64);

	initChunkedArray(&uiState->openWindows, arena, 64);
}

inline bool isMouseInUIBounds(UIState *uiState, Rect2I bounds)
{
	return isMouseInUIBounds(uiState, bounds, renderer->uiCamera.mousePos);
}

inline bool isMouseInUIBounds(UIState *uiState, Rect2I bounds, V2 mousePos)
{
	Rect2I clippedBounds = uiState->isInputScissorActive ? intersect(bounds, uiState->inputScissorBounds) : bounds;

	bool result = contains(clippedBounds, mousePos);

	return result;
}

inline bool justClickedOnUI(UIState *uiState, Rect2I bounds)
{
	V2I mousePos = v2i(renderer->uiCamera.mousePos);
	Rect2I clippedBounds = uiState->isInputScissorActive ? intersect(bounds, uiState->inputScissorBounds) : bounds;

	bool result = contains(clippedBounds, mousePos)
			   && mouseButtonJustReleased(MouseButton_Left)
			   && contains(clippedBounds, getClickStartPos(MouseButton_Left, &renderer->uiCamera));

	return result;
}

void addUIRect(UIState *uiState, Rect2I bounds)
{
	append(&uiState->uiRects, bounds);
}

Rect2I uiText(RenderBuffer *renderBuffer, BitmapFont *font, String text, V2I origin, u32 align, V4 color, s32 maxWidth)
{
	DEBUG_FUNCTION();

	V2I textSize = calculateTextSize(font, text, maxWidth);
	V2I topLeft  = calculateTextPosition(origin, textSize, align);

	Rect2I bounds = irectPosSize(topLeft, textSize);

	drawText(renderBuffer, font, text, bounds, align, color, renderer->shaderIds.text);

	return bounds;
}

void basicTooltipWindowProc(WindowContext *context, void * /*userData*/)
{
	window_label(context, context->uiState->tooltipText);
}

void showTooltip(UIState *uiState, WindowProc tooltipProc, void *userData)
{
	static String styleName = "tooltip"_s;
	showWindow(uiState, nullString, 300, 0, v2i(0,0), styleName, WinFlag_AutomaticHeight | WinFlag_ShrinkWidth | WinFlag_Unique | WinFlag_Tooltip | WinFlag_Headless, tooltipProc, userData);
}

V2I calculateButtonSize(String text, UIButtonStyle *buttonStyle, s32 maxWidth, bool fillWidth)
{
	s32 doublePadding = (buttonStyle->padding * 2);
	s32 textMaxWidth = (maxWidth == 0) ? 0 : (maxWidth - doublePadding);

	V2I result = {};
	BitmapFont *font = getFont(&buttonStyle->font);

	if (textMaxWidth < 0)
	{
		// ERROR! Negative text width means we can't fit any so give up.
		// (NB: 0 means "whatever", so we only worry if it's less than that.)
		DEBUG_BREAK();

		result.x = maxWidth;
		result.y = font->lineHeight;
	}
	else
	{
		V2I textSize = calculateTextSize(font, text, textMaxWidth);

		s32 resultWidth = 0;

		if (fillWidth && (maxWidth > 0))
		{
			resultWidth = maxWidth;
		}
		else
		{
			resultWidth = (textSize.x + doublePadding);
		}

		result = v2i(resultWidth, textSize.y + doublePadding);
	}

	return result;
}

bool uiButton(UIState *uiState, String text, Rect2I bounds, UIButtonStyle *style, ButtonState state, SDL_Keycode shortcutKey, String tooltip)
{
	DEBUG_FUNCTION();
	
	if (style == null)
	{
		style = findButtonStyle(&assets->theme, "default"_s);
	}
	
	bool buttonClicked = false;

	V4 backColor = style->backgroundColor;
	u32 textAlignment = style->textAlignment;

	if (state == Button_Disabled)
	{
		backColor = style->disabledBackgroundColor;
	}
	else if (!uiState->mouseInputHandled && isMouseInUIBounds(uiState, bounds))
	{
		uiState->mouseInputHandled = true;

		// Mouse pressed: must have started and currently be inside the bounds to show anything
		// Mouse unpressed: show hover if in bounds
		if (mouseButtonPressed(MouseButton_Left))
		{
			if (isMouseInUIBounds(uiState, bounds, getClickStartPos(MouseButton_Left, &renderer->uiCamera)))
			{
				backColor = style->pressedBackgroundColor;
			}
		}
		else
		{
			if (mouseButtonJustReleased(MouseButton_Left)
			 && isMouseInUIBounds(uiState, bounds, getClickStartPos(MouseButton_Left, &renderer->uiCamera)))
			{
				buttonClicked = true;
			}
			backColor = style->hoverBackgroundColor;
		}

		if (tooltip.length)
		{
			uiState->tooltipText = tooltip;
			showTooltip(uiState, basicTooltipWindowProc, null);
		}
	}
	else if (state == Button_Active)
	{
		backColor = style->hoverBackgroundColor;
	}

	drawSingleRect(&renderer->uiBuffer, bounds, renderer->shaderIds.untextured, backColor);
	V2I textOrigin = alignWithinRectangle(bounds, textAlignment, style->padding);
	uiText(&renderer->uiBuffer, getFont(&style->font), text, textOrigin, textAlignment, style->textColor);

	// Keyboard shortcut!
	if (state != Button_Disabled
	&& !isInputCaptured()
	&& (shortcutKey != SDLK_UNKNOWN) && keyJustPressed(shortcutKey))
	{
		buttonClicked = true;
	}

	return buttonClicked;
}

bool uiMenuButton(UIState *uiState, String text, Rect2I bounds, s32 menuID, UIButtonStyle *style, SDL_Keycode shortcutKey, String tooltip)
{
	DEBUG_FUNCTION();
	
	bool currentlyOpen = uiState->openMenu == menuID;
	if (uiButton(uiState, text, bounds, style, buttonIsActive(currentlyOpen), shortcutKey, tooltip))
	{
		if (currentlyOpen)
		{
			uiState->openMenu = 0;
			currentlyOpen = false;
		}
		else
		{
			// NB: Do all menu-state-initialisation here!
			uiState->openMenu = menuID;
			currentlyOpen = true;

			uiState->openMenuScrollbar = {};
		}
	}

	return currentlyOpen;
}

void pushUiMessage(UIState *uiState, String message)
{
	copyString(message, &uiState->message.text);
	uiState->message.countdown = uiMessageDisplayTime;
}

void drawUiMessage(UIState *uiState)
{
	DEBUG_FUNCTION();
	
	if (uiState->message.countdown > 0)
	{
		uiState->message.countdown -= SECONDS_PER_FRAME;

		if (uiState->message.countdown > 0)
		{
			UIMessageStyle *style = findMessageStyle(&assets->theme, "default"_s);

			f32 t = (f32)uiState->message.countdown / uiMessageDisplayTime;

			V4 backgroundColor = style->backgroundColor;
			V4 textColor = style->textColor;

			if (t < 0.2f)
			{
				// Fade out
				f32 tt = t / 0.2f;
				backgroundColor *= lerp<f32>(0, backgroundColor.a, tt);
				textColor *= lerp<f32>(0, textColor.a, tt);
			}
			else if (t > 0.8f)
			{
				// Fade in
				f32 tt = (t - 0.8f) / 0.2f;
				backgroundColor *= lerp<f32>(backgroundColor.a, 0, tt);
				textColor *= lerp<f32>(textColor.a, 0, tt);
			}

			V2I origin = v2i(floor_s32(renderer->uiCamera.size.x / 2), floor_s32(renderer->uiCamera.size.y - 8));

			RenderItem_DrawSingleRect *backgroundRI = appendDrawRectPlaceholder(&renderer->uiBuffer, renderer->shaderIds.untextured);

			Rect2I labelRect = uiText(&renderer->uiBuffer, getFont(&style->font), uiState->message.text, origin,
										 ALIGN_H_CENTRE | ALIGN_BOTTOM, textColor);

			labelRect = expand(labelRect, style->padding);

			fillDrawRectPlaceholder(backgroundRI, labelRect, backgroundColor);
		}
	}
}

void updateScrollbar(UIState *uiState, ScrollbarState *state, s32 contentSize, Rect2I bounds, UIScrollbarStyle *style)
{
	DEBUG_FUNCTION();

	state->contentSize = contentSize;

	// If the content is smaller than the scrollbar, then snap it to position 0 and don't allow interaction.
	if (bounds.h > state->contentSize)
	{
		state->scrollPosition = 0;
	}
	else
	{
		if (style == null)
		{
			style = findScrollbarStyle(&assets->theme, "default"_s);
		}

		if (!uiState->mouseInputHandled)
		{
			// Scrollwheel stuff
			// (It's weird that we're putting this within mouseInputHandled, but eh)
			s32 mouseWheelDelta = inputState->wheelY;
			if (mouseWheelDelta != 0)
			{
				// One scroll step is usually 3 lines of text.
				// 64px seems reasonable?
				state->scrollPosition = clamp(state->scrollPosition - (64 * mouseWheelDelta), 0, (state->contentSize - bounds.h));
			}

			// Mouse stuff
			if (mouseButtonPressed(MouseButton_Left)
			 && isMouseInUIBounds(uiState, bounds, getClickStartPos(MouseButton_Left, &renderer->uiCamera)))
			{
				V2 relativeMousePos = renderer->uiCamera.mousePos - v2(bounds.pos);

				f32 range = (f32)(bounds.h - style->width);

				f32 scrollPercent = clamp01((relativeMousePos.y - 0.5f * style->width) / range);
				state->scrollPosition = round_s32(scrollPercent * (state->contentSize - bounds.h));

				uiState->mouseInputHandled = true;
			}
		}
	}
}

// TODO: We really want a version of this that just takes a ScrollbarState*, it'd make many things simpler.
// However, we use this without a ScrollbarState in the console, currently. May or may not want to move that
// over to using a ScrollbarState too.
// NB: Roll the "just draw the background if the content is too small for a scrollbar" stuff from 
// window_completeColumn() into that, too.
void drawScrollbar(RenderBuffer *uiBuffer, f32 scrollPercent, V2I topLeft, s32 height, V2I knobSize, V4 knobColor, V4 backgroundColor, s8 shaderID)
{
	Rect2 backgroundRect = rectXYWHi(topLeft.x, topLeft.y, knobSize.x, height);
	drawSingleRect(uiBuffer, backgroundRect, shaderID, backgroundColor);

	knobSize.y = min(knobSize.y, height); // force knob to fit
	s32 scrollY = round_s32(scrollPercent * (height - knobSize.y));
	Rect2 knobRect = rectXYWHi(topLeft.x, topLeft.y + scrollY, knobSize.x, knobSize.y);
	drawSingleRect(uiBuffer, knobRect, shaderID, knobColor);
}

inline f32 getScrollbarPercent(ScrollbarState *scrollbar, s32 scrollbarHeight)
{
	f32 result = 0.0f;

	// We only do this if the content is large enough to need scrolling.
	// If it's not, we default to 0 above.
	if (scrollbarHeight <= scrollbar->contentSize)
	{
		result = (f32)scrollbar->scrollPosition / (f32)(scrollbar->contentSize - scrollbarHeight);
	}

	result = clamp01(result);

	return result;
}

void showMenu(UIState *uiState, s32 menuID)
{
	uiState->openMenu = menuID;
	uiState->openMenuScrollbar = {};
}

inline void hideMenus(UIState *uiState)
{
	uiState->openMenu = 0;
}

inline bool isMenuVisible(UIState *uiState, s32 menuID)
{
	return (uiState->openMenu == menuID);
}

PopupMenu beginPopupMenu(UIState *uiState, s32 x, s32 y, s32 width, s32 maxHeight, UIPopupMenuStyle *style)
{
	PopupMenu result = {};

	result.style = style;
	result.scrollbarStyle = findStyle<UIScrollbarStyle>(&assets->theme, &style->scrollbarStyle);
	result.buttonStyle = findStyle<UIButtonStyle>(&assets->theme, &style->buttonStyle);

	result.origin = v2i(x, y);
	result.width = width;
	result.maxHeight = maxHeight;

	result.backgroundRect = appendDrawRectPlaceholder(&renderer->uiBuffer, renderer->shaderIds.untextured);
	result.currentYOffset = result.style->margin;

	s32 scrollbarWidth = (result.scrollbarStyle ? result.scrollbarStyle->width : 0);
	uiState->inputScissorBounds = irectXYWH(x, y, width + scrollbarWidth, maxHeight);
	uiState->isInputScissorActive = true;
	addBeginScissor(&renderer->uiBuffer, rect2(uiState->inputScissorBounds));

	return result;
}

bool popupMenuButton(UIState *uiState, PopupMenu *menu, String text, UIButtonStyle *style, ButtonState state)
{
	V2I buttonSize = calculateButtonSize(text, style, menu->width - (menu->style->margin * 2));

	Rect2I buttonRect = irectXYWH(menu->origin.x + menu->style->margin,
								menu->origin.y + menu->currentYOffset - uiState->openMenuScrollbar.scrollPosition,
								buttonSize.x, buttonSize.y);

	bool result = uiButton(uiState, text, buttonRect, style, state);

	menu->currentYOffset += buttonRect.h + menu->style->contentPadding;

	return result;
}

void endPopupMenu(UIState *uiState, PopupMenu *menu)
{
	// Only show the scrollbar if the content is larger than will fit.
	bool showScrollbar = (menu->currentYOffset > menu->maxHeight);

	s32 scrollbarWidth = (showScrollbar ? menu->scrollbarStyle->width : 0);

	// Handle scrollbar stuff
	if (showScrollbar)
	{
		Rect2I scrollbarBounds = irectXYWH(menu->origin.x + menu->width, menu->origin.y, scrollbarWidth, menu->maxHeight);
		updateScrollbar(uiState, &uiState->openMenuScrollbar, menu->currentYOffset, scrollbarBounds, menu->scrollbarStyle);
		f32 scrollPercent = getScrollbarPercent(&uiState->openMenuScrollbar, scrollbarBounds.h);
		drawScrollbar(&renderer->uiBuffer, scrollPercent, scrollbarBounds.pos, scrollbarBounds.h, v2i(scrollbarWidth, scrollbarWidth), menu->scrollbarStyle->knobColor, menu->scrollbarStyle->backgroundColor, renderer->shaderIds.untextured);
	}

	s32 contentHeight = (menu->currentYOffset > 0) ? (menu->currentYOffset - menu->style->contentPadding) : 0;

	Rect2I menuRect = irectXYWH(menu->origin.x, menu->origin.y, menu->width + scrollbarWidth, min(contentHeight + menu->style->margin, menu->maxHeight));

	addUIRect(uiState, menuRect);
	fillDrawRectPlaceholder(menu->backgroundRect, menuRect, menu->style->backgroundColor);
	uiState->isInputScissorActive = false;
	addEndScissor(&renderer->uiBuffer);
}
