// ui.cpp

#include "ui_drawable.cpp"
#include "ui_panel.cpp"
#include "ui_window.cpp"

void initUIState(UIState *uiState, MemoryArena *arena)
{
	*uiState = {};

	uiState->message = {};
	uiState->message.text = pushString(arena, 256);
	uiState->message.countdown = -1;

	initChunkedArray(&uiState->uiRects, arena, 64);

	initChunkedArray(&uiState->openWindows, arena, 64);

	initStack(&uiState->inputScissorRects, arena);
}

inline bool isMouseInUIBounds(UIState *uiState, Rect2I bounds)
{
	return isMouseInUIBounds(uiState, bounds, renderer->uiCamera.mousePos);
}

inline bool isMouseInUIBounds(UIState *uiState, Rect2I bounds, V2 mousePos)
{
	Rect2I clippedBounds = isInputScissorActive(uiState) ? intersect(bounds, getInputScissorRect(uiState)) : bounds;

	bool result = contains(clippedBounds, mousePos);

	return result;
}

inline bool justClickedOnUI(UIState *uiState, Rect2I bounds)
{
	V2I mousePos = v2i(renderer->uiCamera.mousePos);
	Rect2I clippedBounds = isInputScissorActive(uiState) ? intersect(bounds, getInputScissorRect(uiState)) : bounds;

	bool result = contains(clippedBounds, mousePos)
			   && mouseButtonJustReleased(MouseButton_Left)
			   && contains(clippedBounds, getClickStartPos(MouseButton_Left, &renderer->uiCamera));

	return result;
}

inline void pushInputScissorRect(UIState *uiState, Rect2I bounds)
{
	push(&uiState->inputScissorRects, bounds);
}

inline void popInputScissorRect(UIState *uiState)
{
	pop(&uiState->inputScissorRects);
}

inline bool isInputScissorActive(UIState *uiState)
{
	return !isEmpty(&uiState->inputScissorRects);
}

inline Rect2I getInputScissorRect(UIState *uiState)
{
	Rect2I result;

	if (isInputScissorActive(uiState))
	{
		result = *peek(&uiState->inputScissorRects);
	}
	else
	{
		result = irectInfinity();
	}

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
	UIPanel *ui = &context->windowPanel;
	ui->addText(context->uiState->tooltipText);
}

void showTooltip(UIState *uiState, WindowProc tooltipProc, void *userData)
{
	static String styleName = "tooltip"_s;
	showWindow(uiState, nullString, 300, 100, v2i(0,0), styleName, WinFlag_AutomaticHeight | WinFlag_ShrinkWidth | WinFlag_Unique | WinFlag_Tooltip | WinFlag_Headless, tooltipProc, userData);
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

V2I calculateButtonSize(V2I contentSize, UIButtonStyle *buttonStyle, s32 maxWidth, bool fillWidth)
{
	s32 doublePadding = (buttonStyle->padding * 2);

	V2I result = {};

	if (fillWidth && (maxWidth > 0))
	{
		result.x = maxWidth;
	}
	else
	{
		result.x = (contentSize.x + doublePadding);
	}

	result.y = contentSize.y + doublePadding;

	return result;
}

bool uiButton(UIState *uiState, String text, Rect2I bounds, UIButtonStyle *style, ButtonState state, SDL_Keycode shortcutKey, String tooltip)
{
	DEBUG_FUNCTION();
	
	if (style == null)
	{
		style = findButtonStyle(assets->theme, "default"_s);
	}
	
	bool buttonClicked = false;

	UIDrawableStyle *backgroundStyle = &style->background;

	u32 textAlignment = style->textAlignment;

	if (state == Button_Disabled)
	{
		backgroundStyle = &style->disabledBackground;
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
				backgroundStyle = &style->pressedBackground;
			}
		}
		else
		{
			if (mouseButtonJustReleased(MouseButton_Left)
			 && isMouseInUIBounds(uiState, bounds, getClickStartPos(MouseButton_Left, &renderer->uiCamera)))
			{
				buttonClicked = true;
			}
			backgroundStyle = &style->hoverBackground;
		}

		if (tooltip.length)
		{
			uiState->tooltipText = tooltip;
			showTooltip(uiState, basicTooltipWindowProc, null);
		}
	}
	else if (state == Button_Active)
	{
		backgroundStyle = &style->hoverBackground;
	}

	UIDrawable buttonBackground = UIDrawable(backgroundStyle);
	buttonBackground.draw(&renderer->uiBuffer, bounds);

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

void pushToast(UIState *uiState, String message)
{
	copyString(message, &uiState->message.text);
	uiState->message.countdown = uiMessageDisplayTime;
}

void drawToast(UIState *uiState)
{
	DEBUG_FUNCTION();
	
	if (uiState->message.countdown > 0)
	{
		uiState->message.countdown -= SECONDS_PER_FRAME;

		if (uiState->message.countdown > 0)
		{
			UIPanelStyle *style = findPanelStyle(assets->theme, "toast"_s);

			// f32 t = (f32)uiState->message.countdown / uiMessageDisplayTime;
			// TODO: Animate this based on t.
			// Though probably we want to keep a little animation-state enum instead of just a timer.
			V2I origin = v2i(floor_s32(renderer->uiCamera.size.x / 2), floor_s32(renderer->uiCamera.size.y - 8));

			UILabelStyle *labelStyle = findStyle<UILabelStyle>(&style->labelStyle);
			s32 maxWidth = min(floor_s32(renderer->uiCamera.size.x * 0.8f), 500);
			V2I textSize = calculateTextSize(getFont(&labelStyle->font), uiState->message.text, maxWidth - (2 * style->margin));

			V2I toastSize = v2i(textSize.x + (2 * style->margin), textSize.y + (2 * style->margin));
			Rect2I toastBounds = irectAligned(origin, toastSize, ALIGN_BOTTOM | ALIGN_H_CENTRE);

			UIPanel toast = UIPanel(toastBounds, style);
			toast.addText(uiState->message.text);
			toast.end();
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
			style = findScrollbarStyle(assets->theme, "default"_s);
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
void drawScrollbar(RenderBuffer *uiBuffer, f32 scrollPercent, V2I topLeft, s32 height, UIScrollbarStyle *style)
{
	UIDrawable background = UIDrawable(&style->background);
	Rect2I backgroundRect = irectXYWH(topLeft.x, topLeft.y, style->width, height);
	background.draw(uiBuffer, backgroundRect);

	s32 knobWidth = style->width;
	s32 knobHeight = min(knobWidth, height); // TODO: make it larger?

	s32 scrollY = round_s32(scrollPercent * (height - knobHeight));

	UIDrawable knob = UIDrawable(&style->knob);
	Rect2I knobRect = irectXYWH(topLeft.x, topLeft.y + scrollY, knobWidth, knobHeight);
	knob.draw(uiBuffer, knobRect);
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

inline void toggleMenuVisible(UIState *uiState, s32 menuID)
{
	if (isMenuVisible(uiState, menuID))
	{
		hideMenus(uiState);
	}
	else
	{
		showMenu(uiState, menuID);
	}
}

inline bool isMenuVisible(UIState *uiState, s32 menuID)
{
	return (uiState->openMenu == menuID);
}
