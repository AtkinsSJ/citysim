// ui.cpp

#include "ui_drawable.cpp"
#include "ui_panel.cpp"
#include "ui_window.cpp"

void initUIState(UIState *uiState, MemoryArena *arena)
{
	*uiState = {};

	initChunkedArray(&uiState->uiRects, arena, 64);
	initStack(&uiState->inputScissorRects, arena);

	initQueue(&uiState->toasts, arena);

	initChunkedArray(&uiState->openWindows, arena, 64);
	initSet(&uiState->windowsToClose, arena);
	initSet(&uiState->windowsToMakeActive, arena);

	initScrollbar(&uiState->openMenuScrollbar, false);
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

	bool result = !uiState->mouseInputHandled
			   && contains(clippedBounds, mousePos)
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

inline void addUIRect(UIState *uiState, Rect2I bounds)
{
	uiState->uiRects.append(bounds);
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
		style = findStyle<UIButtonStyle>("default"_s);
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
			showTooltip(uiState, basicTooltipWindowProc);
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
	Toast *newToast = uiState->toasts.push();

	*newToast = {};
	newToast->text = makeString(newToast->_chars, MAX_TOAST_LENGTH);
	copyString(message, &newToast->text);

	newToast->duration = TOAST_APPEAR_TIME + TOAST_DISPLAY_TIME + TOAST_DISAPPEAR_TIME;
	newToast->time = 0;
}

void drawToast(UIState *uiState)
{
	DEBUG_FUNCTION();

	Maybe<Toast*> currentToast = uiState->toasts.peek();
	if (currentToast.isValid)
	{
		Toast *toast = currentToast.value;

		toast->time += globalAppState.deltaTime;

		if (toast->time >= toast->duration)
		{
			uiState->toasts.pop();
		}
		else
		{
			UIPanelStyle *style = findStyle<UIPanelStyle>("toast"_s);
			V2I origin = v2i(floor_s32(renderer->uiCamera.size.x / 2), floor_s32(renderer->uiCamera.size.y - 8));

			UILabelStyle *labelStyle = findStyle<UILabelStyle>(&style->labelStyle);
			s32 maxWidth = min(floor_s32(renderer->uiCamera.size.x * 0.8f), 500);
			V2I textSize = calculateTextSize(getFont(&labelStyle->font), toast->text, maxWidth - (2 * style->margin));

			V2I toastSize = v2i(textSize.x + (2 * style->margin), textSize.y + (2 * style->margin));

			f32 animationDistance = toastSize.y + 16.0f;

			if (toast->time < TOAST_APPEAR_TIME)
			{
				// Animate in
				f32 t = toast->time / TOAST_APPEAR_TIME;
				origin.y += round_s32(interpolate(animationDistance, 0, t, Interpolate_SineOut));
			}
			else if (toast->time > (TOAST_APPEAR_TIME + TOAST_DISPLAY_TIME))
			{
				// Animate out
				f32 t = (toast->time - (TOAST_APPEAR_TIME + TOAST_DISPLAY_TIME)) / TOAST_DISAPPEAR_TIME;
				origin.y += round_s32(interpolate(0, animationDistance, t, Interpolate_SineIn));
			}
			Rect2I toastBounds = irectAligned(origin, toastSize, ALIGN_BOTTOM | ALIGN_H_CENTRE);

			UIPanel panel = UIPanel(toastBounds, style);
			panel.addText(toast->text);
			panel.end();
		}
	}
}

void initScrollbar(ScrollbarState *state, bool isHorizontal, s32 mouseWheelStepSize)
{
	*state = {};

	state->isHorizontal = isHorizontal;
	state->mouseWheelStepSize = mouseWheelStepSize;
}

Maybe<Rect2I> getScrollbarThumbBounds(ScrollbarState *state, Rect2I scrollbarBounds, UIScrollbarStyle *style)
{
	Maybe<Rect2I> result = makeFailure<Rect2I>();

	// NB: This algorithm for thumb size came from here: https://ux.stackexchange.com/a/85698
	// (Which is ultimately taken from Microsoft's .NET documentation.)
	// Anyway, it's simple enough, but distinguishes between the size of visible content, and
	// the size of the scrollbar's track. For us, for now, those are the same value, but I'm
	// keeping them as separate variables, which is why viewportSize and trackSize are
	// the same. If we add scrollbar buttons, that'll reduce the track size.
	// - Sam, 01/04/2021
	if (state->isHorizontal)
	{
		s32 thumbHeight = style->width;
		s32 viewportSize = scrollbarBounds.w;

		if (viewportSize < state->contentSize)
		{
			s32 trackSize = scrollbarBounds.w;
			s32 desiredThumbSize = trackSize * viewportSize / (state->contentSize + viewportSize);
			s32 thumbWidth = clamp(desiredThumbSize, style->width, scrollbarBounds.w);

			s32 thumbPos = round_s32(state->scrollPercent * (scrollbarBounds.w - thumbWidth));

			result = makeSuccess(irectXYWH(scrollbarBounds.x + thumbPos, scrollbarBounds.y, thumbWidth, thumbHeight));
		}
	}
	else
	{
		s32 thumbWidth = style->width;
		s32 viewportSize = scrollbarBounds.h;

		if (viewportSize < state->contentSize)
		{
			s32 trackSize = scrollbarBounds.h;
			s32 desiredThumbSize = trackSize * viewportSize / (state->contentSize + viewportSize);
			s32 thumbHeight = clamp(desiredThumbSize, style->width, scrollbarBounds.h);

			s32 thumbPos = round_s32(state->scrollPercent * (scrollbarBounds.h - thumbHeight));

			result = makeSuccess(irectXYWH(scrollbarBounds.x, scrollbarBounds.y + thumbPos, thumbWidth, thumbHeight));
		}
	}

	return result;
}

void updateScrollbar(UIState *uiState, ScrollbarState *state, s32 contentSize, Rect2I bounds, UIScrollbarStyle *style)
{
	DEBUG_FUNCTION();

	state->contentSize = contentSize;

	// If the content is smaller than the scrollbar, then snap it to position 0 and don't allow interaction.
	if (bounds.h > state->contentSize)
	{
		state->scrollPercent = 0.0f;
	}
	else
	{
		if (style == null)
		{
			style = findStyle<UIScrollbarStyle>("default"_s);
		}

		if (!uiState->mouseInputHandled)
		{
			Maybe<Rect2I> thumb = getScrollbarThumbBounds(state, bounds, style);
			if (thumb.isValid)
			{
				s32 overflowSize   = state->contentSize - (state->isHorizontal ? bounds.w : bounds.h);
				Rect2I thumbBounds = thumb.value;
				s32 thumbSize      = state->isHorizontal ? thumbBounds.w : thumbBounds.h;
				s32 gutterSize     = state->isHorizontal ? bounds.w : bounds.h;
				s32 thumbRange     = gutterSize - thumbSize;

				// Scrollwheel stuff
				// (It's weird that we're putting this within mouseInputHandled, but eh)
				s32 mouseWheelDelta = (state->isHorizontal ? inputState->wheelX : -inputState->wheelY);
				if (mouseWheelDelta != 0)
				{
					s32 oldScrollOffset = getScrollbarContentOffset(state, gutterSize);
					s32 scrollOffset = oldScrollOffset + (state->mouseWheelStepSize * mouseWheelDelta);

					state->scrollPercent = clamp01((f32)scrollOffset / (f32)overflowSize);
				}

				// Mouse stuff
				if (mouseButtonPressed(MouseButton_Left))
				{
					V2 clickStartPos = getClickStartPos(MouseButton_Left, &renderer->uiCamera);
					V2I mousePos = v2i(renderer->uiCamera.mousePos);

					if (isMouseInUIBounds(uiState, bounds, clickStartPos))
					{
						// If we are dragging the thumb, move it
						if (state->isDraggingThumb)
						{
							s32 dragDistance = 0;

							if (state->isHorizontal)
							{
								dragDistance = mousePos.x - (s32)clickStartPos.x;
							}
							else
							{
								dragDistance = mousePos.y - (s32)clickStartPos.y;
							}

							f32 dragPercent = (f32)dragDistance / thumbRange;
							state->scrollPercent = clamp01(state->thumbDragStartPercent + dragPercent);
						}
						// Else, if we clicked on the thumb, begin dragging
						else if (mouseButtonJustPressed(MouseButton_Left) && isMouseInUIBounds(uiState, thumbBounds, clickStartPos))
						{
							state->isDraggingThumb = true;
							state->thumbDragStartPercent = state->scrollPercent;
						}
						// Else, jump to mouse position
						else
						{
							V2 relativeMousePos = renderer->uiCamera.mousePos - v2(bounds.pos);

							state->scrollPercent = clamp01(( (state->isHorizontal ? relativeMousePos.x : relativeMousePos.y) - 0.5f * thumbSize) / (f32)thumbRange);
						}

						uiState->mouseInputHandled = true;
					}
				}
				else
				{
					state->isDraggingThumb = false;
				}
			}
		}
	}
}

void drawScrollbar(RenderBuffer *uiBuffer, ScrollbarState *state, Rect2I bounds, UIScrollbarStyle *style)
{
	ASSERT(hasPositiveArea(bounds));

	UIDrawable background = UIDrawable(&style->background);
	background.draw(uiBuffer, bounds);

	Maybe<Rect2I> thumbBounds = getScrollbarThumbBounds(state, bounds, style);
	if (thumbBounds.isValid)
	{
		UIDrawable thumb = UIDrawable(&style->thumb);
		thumb.draw(uiBuffer, thumbBounds.value);
	}
}

s32 getScrollbarContentOffset(ScrollbarState *state, s32 scrollbarSize)
{
	s32 overflowSize = state->contentSize - scrollbarSize;

	s32 result = round_s32(state->scrollPercent * overflowSize);

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
