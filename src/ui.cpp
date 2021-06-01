// ui.cpp

#include "ui_drawable.cpp"
#include "ui_panel.cpp"
#include "ui_window.cpp"

void UI::init(MemoryArena *arena)
{
	uiState = {};

	initChunkedArray(&uiState.uiRects, arena, 64);
	initStack(&uiState.inputScissorRects, arena);

	initQueue(&uiState.toasts, arena);

	initChunkedArray(&uiState.openWindows, arena, 64);
	initSet(&uiState.windowsToClose, arena);
	initSet(&uiState.windowsToMakeActive, arena);

	initScrollbar(&uiState.openMenuScrollbar, false);

	initScrollbar(&uiState.openDropDownListScrollbar, false);
}

void UI::startFrame()
{
	uiState.uiRects.clear();
	uiState.mouseInputHandled = false;

	// Clear the drag if the mouse button isn't pressed
	if (!mouseButtonPressed(MouseButton_Left))
	{
		uiState.currentDragObject = null;
	}

	windowSize = v2i(renderer->uiCamera.size);
	mousePos = v2i(renderer->uiCamera.mousePos);
	if (mouseButtonPressed(MouseButton_Left))
	{
		mouseClickStartPos = v2i(getClickStartPos(MouseButton_Left, &renderer->uiCamera));
	}
}

void UI::endFrame()
{
	drawToast();

	if (uiState.openDropDownListRenderBuffer != null)
	{
		// If the buffer is empty, we didn't draw the dropdown this frame, so we should mark it as closed
		if (uiState.openDropDownListRenderBuffer->firstChunk == null)
		{
			closeDropDownList();
		}
		else
		{
			// NB: We transfer to the *window* buffer, not the *ui* buffer, because we
			// want the drop-down to appear in front of any windows.
			transferRenderBufferData(uiState.openDropDownListRenderBuffer, &renderer->windowBuffer);
		}
	}
}

inline bool UI::isMouseInputHandled()
{
	return uiState.mouseInputHandled;
}

inline void UI::markMouseInputHandled()
{
	uiState.mouseInputHandled = true;
}

inline bool UI::isMouseInUIBounds(Rect2I bounds)
{
	return isMouseInUIBounds(bounds, mousePos);
}

inline bool UI::isMouseInUIBounds(Rect2I bounds, V2I pos)
{
	Rect2I clippedBounds = isInputScissorActive() ? intersect(bounds, getInputScissorRect()) : bounds;

	bool result = contains(clippedBounds, pos);

	return result;
}

inline bool UI::justClickedOnUI(Rect2I bounds)
{
	Rect2I clippedBounds = isInputScissorActive() ? intersect(bounds, getInputScissorRect()) : bounds;

	bool result = !UI::isMouseInputHandled()
			   && contains(clippedBounds, mousePos)
			   && mouseButtonJustReleased(MouseButton_Left)
			   && contains(clippedBounds, mouseClickStartPos);

	return result;
}

WidgetMouseState UI::getWidgetMouseState(Rect2I widgetBounds)
{
	WidgetMouseState result = {};

	if (!isMouseInputHandled() && isMouseInUIBounds(widgetBounds))
	{
		result.isHovered = true;

		// Mouse pressed: must have started and currently be inside the bounds to show anything
		// Mouse unpressed: show hover if in bounds
		if (mouseButtonPressed(MouseButton_Left)
		 && isMouseInUIBounds(widgetBounds, mouseClickStartPos))
		{
			result.isPressed = true;
		}
	}

	return result;
}

inline bool UI::isDragging(void *object)
{
	return (object == uiState.currentDragObject);
}

void UI::startDragging(void *object, V2I objectPos)
{
	uiState.currentDragObject = object;
	uiState.dragObjectStartPos = objectPos;
}

V2I UI::getDraggingObjectPos()
{
	return uiState.dragObjectStartPos + (mousePos - mouseClickStartPos);
}

inline void UI::pushInputScissorRect(Rect2I bounds)
{
	push(&uiState.inputScissorRects, bounds);
}

inline void UI::popInputScissorRect()
{
	pop(&uiState.inputScissorRects);
}

inline bool UI::isInputScissorActive()
{
	return !isEmpty(&uiState.inputScissorRects);
}

inline Rect2I UI::getInputScissorRect()
{
	Rect2I result;

	if (isInputScissorActive())
	{
		result = *peek(&uiState.inputScissorRects);
	}
	else
	{
		result = irectInfinity();
	}

	return result;
}

inline void UI::addUIRect(Rect2I bounds)
{
	uiState.uiRects.append(bounds);
}

bool UI::mouseIsWithinUIRects()
{
	bool result = false;

	for (auto it = uiState.uiRects.iterate(); it.hasNext(); it.next())
	{
		if (contains(it.getValue(), mousePos))
		{
			result = true;
			break;
		}
	}

	return result;
}

Rect2I UI::drawText(RenderBuffer *renderBuffer, BitmapFont *font, String text, V2I origin, u32 align, V4 color, s32 maxWidth)
{
	DEBUG_FUNCTION_T(DCDT_UI);

	V2I textSize = calculateTextSize(font, text, maxWidth);
	V2I topLeft  = calculateTextPosition(origin, textSize, align);

	Rect2I bounds = irectPosSize(topLeft, textSize);

	drawText(renderBuffer, font, text, bounds, align, color, renderer->shaderIds.text);

	return bounds;
}

V2I UI::calculateButtonSize(String text, UIButtonStyle *buttonStyle, s32 maxWidth, bool fillWidth)
{
	s32 doublePadding = (buttonStyle->padding * 2);

	// If we have icons, take them into account
	V2I startIconSize = buttonStyle->startIcon.getSize();
	if (startIconSize.x > 0) startIconSize.x += buttonStyle->contentPadding;

	V2I endIconSize =  buttonStyle->endIcon.getSize();
	if (endIconSize.x > 0) endIconSize.x += buttonStyle->contentPadding;

	s32 textMaxWidth = 0;
	if (maxWidth != 0)
	{
		textMaxWidth = maxWidth - doublePadding - startIconSize.x - endIconSize.x;
	}

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
			resultWidth = (textSize.x + startIconSize.x + endIconSize.x + doublePadding);
		}

		result = v2i(resultWidth, max(textSize.y, startIconSize.y, endIconSize.y) + doublePadding);
	}

	return result;
}

V2I UI::calculateButtonSize(V2I contentSize, UIButtonStyle *buttonStyle, s32 maxWidth, bool fillWidth)
{
	s32 doublePadding = (buttonStyle->padding * 2);

	// If we have icons, take them into account
	V2I startIconSize = buttonStyle->startIcon.getSize();
	if (startIconSize.x > 0) startIconSize.x += buttonStyle->contentPadding;

	V2I endIconSize =  buttonStyle->endIcon.getSize();
	if (endIconSize.x > 0) endIconSize.x += buttonStyle->contentPadding;

	V2I result = {};

	if (fillWidth && (maxWidth > 0))
	{
		result.x = maxWidth;
	}
	else
	{
		result.x = (contentSize.x + doublePadding + startIconSize.x + endIconSize.x);
	}

	result.y = max(contentSize.y, startIconSize.y, endIconSize.y) + doublePadding;

	return result;
}

Rect2I UI::calculateButtonContentBounds(Rect2I bounds, UIButtonStyle *style)
{
	Rect2I contentBounds = shrink(bounds, style->padding);

	V2I startIconSize = style->startIcon.getSize();
	if (startIconSize.x > 0) startIconSize.x += style->contentPadding;
	V2I endIconSize = style->endIcon.getSize();
	if (endIconSize.x > 0) endIconSize.x += style->contentPadding;

	contentBounds.x += startIconSize.x;
	contentBounds.w -= (startIconSize.x + endIconSize.x);

	return contentBounds;
}

bool UI::putButton(Rect2I bounds, UIButtonStyle *style, ButtonState state, RenderBuffer *renderBuffer, String tooltip)
{
	DEBUG_FUNCTION_T(DCDT_UI);

	if (style == null) 			style = findStyle<UIButtonStyle>("default"_s);
	if (renderBuffer == null) 	renderBuffer = &renderer->uiBuffer;
	
	bool buttonClicked = false;

	WidgetMouseState mouseState = getWidgetMouseState(bounds);

	UIDrawableStyle *backgroundStyle = &style->background;

	if (state == Button_Disabled)
	{
		backgroundStyle = &style->disabledBackground;
	}
	else if (mouseState.isHovered)
	{
		if (mouseState.isPressed)
		{
			backgroundStyle = &style->pressedBackground;
		}
		else
		{
			backgroundStyle = &style->hoverBackground;
		}

		if (tooltip.length)
		{
			showTooltip(tooltip);
		}
	}
	else if (state == Button_Active)
	{
		backgroundStyle = &style->hoverBackground;
	}

	UIDrawable buttonBackground = UIDrawable(backgroundStyle);
	buttonBackground.draw(renderBuffer, bounds);

	// Icons!
	if (style->startIcon.type != Drawable_None)
	{
		V2I startIconSize = style->startIcon.getSize();
		V2I startIconOrigin = alignWithinRectangle(bounds, style->startIconAlignment, style->padding);
		Rect2I startIconBounds = irectAligned(startIconOrigin, startIconSize, style->startIconAlignment);

		UIDrawable(&style->startIcon).draw(renderBuffer, startIconBounds);
	}
	if (style->endIcon.type != Drawable_None)
	{
		V2I endIconSize = style->endIcon.getSize();
		V2I endIconOrigin = alignWithinRectangle(bounds, style->endIconAlignment, style->padding);
		Rect2I endIconBounds = irectAligned(endIconOrigin, endIconSize, style->endIconAlignment);

		UIDrawable(&style->endIcon).draw(renderBuffer, endIconBounds);
	}

	// Respond to click
	if ((state != Button_Disabled) && justClickedOnUI(bounds))
	{
		buttonClicked = true;
		markMouseInputHandled();
	}

	return buttonClicked;
}

bool UI::putTextButton(String text, Rect2I bounds, UIButtonStyle *style, ButtonState state, RenderBuffer *renderBuffer, String tooltip)
{
	if (renderBuffer == null)
	{
		renderBuffer = &renderer->uiBuffer;
	}

	bool result = putButton(bounds, style, state, renderBuffer, tooltip);

	Rect2I contentBounds = calculateButtonContentBounds(bounds, style);

	u32 textAlignment = style->textAlignment;
	V2I textOrigin = alignWithinRectangle(contentBounds, textAlignment, 0);
	drawText(renderBuffer, getFont(&style->font), text, textOrigin, textAlignment, style->textColor);

	return result;
}

bool UI::putImageButton(Sprite *sprite, Rect2I bounds, UIButtonStyle *style, ButtonState state, RenderBuffer *renderBuffer, String tooltip)
{
	if (renderBuffer == null) renderBuffer = &renderer->uiBuffer;

	bool result = putButton(bounds, style, state, renderBuffer, tooltip);

	Rect2I contentBounds = calculateButtonContentBounds(bounds, style);
	Rect2 spriteBounds = rect2(contentBounds);
	drawSingleSprite(renderBuffer, sprite, spriteBounds, renderer->shaderIds.textured, makeWhite());

	return result;
}

V2I UI::calculateCheckboxSize(UICheckboxStyle *style)
{
	s32 doublePadding = (style->padding * 2);
	
	V2I result = v2i(style->contentSize.x + doublePadding, style->contentSize.y + doublePadding);

	return result;
}

void UI::putCheckbox(bool *checked, Rect2I bounds, UICheckboxStyle *style, bool isDisabled, RenderBuffer *renderBuffer)
{
	DEBUG_FUNCTION_T(DCDT_UI);

	if (style == null) 			style = findStyle<UICheckboxStyle>("default"_s);
	if (renderBuffer == null) 	renderBuffer = &renderer->uiBuffer;

	WidgetMouseState mouseState = getWidgetMouseState(bounds);

	UIDrawableStyle *backgroundStyle = &style->background;
	UIDrawableStyle *checkStyle = &style->checkImage;

	if (isDisabled)
	{
		backgroundStyle = &style->disabledBackground;
		checkStyle = &style->disabledCheckImage;
	}
	else if (mouseState.isHovered)
	{
		if (mouseState.isPressed)
		{
			backgroundStyle = &style->pressedBackground;
			checkStyle = &style->pressedCheckImage;
		}
		else
		{
			backgroundStyle = &style->hoverBackground;
			checkStyle = &style->hoverCheckImage;
		}
	}

	if (!isDisabled && justClickedOnUI(bounds))
	{
		*checked = !(*checked);
		markMouseInputHandled();
	}

	// Render it
	UIDrawable(backgroundStyle).draw(renderBuffer, bounds);

	if (*checked)
	{
		Rect2I checkBounds = shrink(bounds, style->padding);
		UIDrawable(checkStyle).draw(renderBuffer, checkBounds);
	}
}

template <typename T>
V2I UI::calculateDropDownListSize(Array<T> *listOptions, String (*getDisplayName)(T *data), UIDropDownListStyle *style, s32 maxWidth, bool fillWidth)
{
	if (style == null) style = findStyle<UIDropDownListStyle>("default"_s);
	UIButtonStyle *buttonStyle = findStyle<UIButtonStyle>(&style->buttonStyle);

	// I don't have a smarter way to do this, so, we'll just go through every option
	// and find the maximum width and height.
	// This feels really wasteful, but maybe it's ok?
	s32 widest = 0;
	s32 tallest = 0;

	for (s32 optionIndex = 0; optionIndex < listOptions->count; optionIndex++)
	{
		String optionText = getDisplayName(listOptions->get(optionIndex));
		V2I optionSize = calculateButtonSize(optionText, buttonStyle, maxWidth, fillWidth);
		widest = max(widest, optionSize.x);
		tallest = max(tallest, optionSize.y);
	}

	return v2i(widest, tallest);
}

template <typename T>
void UI::putDropDownList(Array<T> *listOptions, s32 *currentSelection, String (*getDisplayName)(T *data), Rect2I bounds, UIDropDownListStyle *style, RenderBuffer *renderBuffer)
{
	DEBUG_FUNCTION_T(DCDT_UI);

	if (style == null) 			style = findStyle<UIDropDownListStyle>("default"_s);
	if (renderBuffer == null) 	renderBuffer = &renderer->uiBuffer;

	bool isOpen = isDropDownListOpen(listOptions);

	// We always draw the current-selection box,
	// and then show the panel if needed.

	// Show the selection box
	String selectionText = getDisplayName(listOptions->get(*currentSelection));
	bool clicked = putTextButton(selectionText, bounds, findStyle<UIButtonStyle>(&style->buttonStyle), buttonIsActive(isOpen), renderBuffer);
	if (clicked)
	{
		// Toggle things
		if (isOpen)
		{
			isOpen = false;
			closeDropDownList();
		}
		else
		{
			isOpen = true;
			openDropDownList(listOptions);
		}
	}

	// Show the panel
	if (isOpen)
	{
		s32 panelTop = bounds.y + bounds.h;
		s32 panelMaxHeight = windowSize.y - panelTop;
		Rect2I panelBounds = irectXYWH(bounds.x, panelTop, bounds.w, panelMaxHeight);
		UIPanel panel = UIPanel(panelBounds, findStyle<UIPanelStyle>(&style->panelStyle), 0, uiState.openDropDownListRenderBuffer);
		panel.enableVerticalScrolling(&uiState.openDropDownListScrollbar, false);
		for (s32 optionIndex = 0; optionIndex < listOptions->count; optionIndex++)
		{
			if (panel.addTextButton(getDisplayName(listOptions->get(optionIndex)), buttonIsActive(optionIndex == *currentSelection)))
			{
				*currentSelection = optionIndex;

				isOpen = false;
				closeDropDownList();
			}
		}
		panel.end(true);

		// If we clicked somewhere outside of the panel, close it
		if (isOpen && !clicked
		 && mouseButtonJustReleased(MouseButton_Left)
		 && !contains(panel.bounds, mouseClickStartPos))
		{
			closeDropDownList();
		}
	}
}

void UI::openDropDownList(void *pointer)
{
	uiState.openDropDownList = pointer;
	if (uiState.openDropDownListRenderBuffer == null)
	{
		uiState.openDropDownListRenderBuffer = getTemporaryRenderBuffer("DropDownList"_s);
	}
	clearRenderBuffer(uiState.openDropDownListRenderBuffer);
	initScrollbar(&uiState.openDropDownListScrollbar, false);
}

void UI::closeDropDownList()
{
	uiState.openDropDownList = null;
	returnTemporaryRenderBuffer(uiState.openDropDownListRenderBuffer);
	uiState.openDropDownListRenderBuffer = null;
}

inline bool UI::isDropDownListOpen(void *pointer)
{
	return (uiState.openDropDownList == pointer);
}

void UI::showMenu(s32 menuID)
{
	// NB: Do all menu-state-initialisation here!
	uiState.openMenu = menuID;
	uiState.openMenuScrollbar = {};
}

void UI::hideMenus()
{
	uiState.openMenu = 0;
}

void UI::toggleMenuVisible(s32 menuID)
{
	if (isMenuVisible(menuID))
	{
		hideMenus();
	}
	else
	{
		showMenu(menuID);
	}
}

inline bool UI::isMenuVisible(s32 menuID)
{
	return (uiState.openMenu == menuID);
}

inline ScrollbarState *UI::getMenuScrollbar()
{
	return &uiState.openMenuScrollbar;
}

void UI::initScrollbar(ScrollbarState *state, bool isHorizontal, s32 mouseWheelStepSize)
{
	*state = {};

	state->isHorizontal = isHorizontal;
	state->mouseWheelStepSize = mouseWheelStepSize;
}

Maybe<Rect2I> UI::getScrollbarThumbBounds(ScrollbarState *state, Rect2I scrollbarBounds, UIScrollbarStyle *style)
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

void UI::putScrollbar(ScrollbarState *state, s32 contentSize, Rect2I bounds, UIScrollbarStyle *style, bool isDisabled, RenderBuffer *renderBuffer)
{
	DEBUG_FUNCTION_T(DCDT_UI);

	ASSERT(hasPositiveArea(bounds));

	if (style == null) 			style = findStyle<UIScrollbarStyle>("default"_s);
	if (renderBuffer == null) 	renderBuffer = &renderer->uiBuffer;

	UIDrawable(&style->background).draw(renderBuffer, bounds);

	state->contentSize = contentSize;

	// If the content is smaller than the scrollbar, then snap it to position 0 and don't allow interaction.
	if (bounds.h > state->contentSize)
	{
		state->scrollPercent = 0.0f;
	}
	else
	{
		if (!isMouseInputHandled())
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
				UIDrawableStyle *thumbStyle = &style->thumb;
				if (isDisabled)
				{
					// thumbStyle = &style->thumbDisabled;
				}
				else if (isDragging(state))
				{
					// Move
					V2I thumbPos = getDraggingObjectPos();

					// @Copypasta We duplicate this code below, because there are two states where we need to set
					// the new thumb position. It's really awkward but I don't know how to pull the logic out.
					if (state->isHorizontal)
					{
						thumbBounds.x = clamp(thumbPos.x, bounds.x, bounds.x + thumbRange);
						state->scrollPercent = clamp01((f32)(thumbBounds.x - bounds.x) / (f32)thumbRange);
					}
					else
					{
						thumbBounds.y = clamp(thumbPos.y, bounds.y, bounds.y + thumbRange);
						state->scrollPercent = clamp01((f32)(thumbBounds.y - bounds.y) / (f32)thumbRange);
					}
					
					// thumbStyle = &style->thumbPressed;
				}
				else if (isMouseInUIBounds(bounds))
				{
					bool inThumbBounds = isMouseInUIBounds(thumbBounds);

					if (mouseButtonJustPressed(MouseButton_Left))
					{
						// If we're not on the thumb, jump the thumb to where we are!
						if (!inThumbBounds)
						{
							if (state->isHorizontal)
							{
								thumbBounds.x = clamp(mousePos.x - (thumbBounds.w / 2), bounds.x, bounds.x + thumbRange);
								state->scrollPercent = clamp01((f32)(thumbBounds.x - bounds.x) / (f32)thumbRange);
							}
							else
							{
								thumbBounds.y = clamp(mousePos.y - (thumbBounds.h / 2), bounds.y, bounds.y + thumbRange);
								state->scrollPercent = clamp01((f32)(thumbBounds.y - bounds.y) / (f32)thumbRange);
							}
						}

						// Start drag
						startDragging(state, thumbBounds.pos);

						// thumbStyle = &style->thumbPressed;
					}
					else if (inThumbBounds)
					{
						// Hovering thumb
						// thumbStyle = &style->thumbHover;
					}
				}

				UIDrawable(thumbStyle).draw(renderBuffer, thumbBounds);
			}
		}
	}
}

s32 UI::getScrollbarContentOffset(ScrollbarState *state, s32 scrollbarSize)
{
	s32 overflowSize = state->contentSize - scrollbarSize;

	s32 result = round_s32(state->scrollPercent * overflowSize);

	return result;
}

V2I UI::calculateSliderSize(UISliderStyle *style, s32 maxWidth, bool fillWidth)
{
	if (style == null) 			style = findStyle<UISliderStyle>("default"_s);

	// I think we always fill width? I don't know how to establish our width otherwise!
	V2I result = v2i(maxWidth, style->thumbSize.y);

	return result;
}

void UI::putSlider(f32 *currentValue, f32 minValue, f32 maxValue, Rect2I bounds, UISliderStyle *style, bool isDisabled, RenderBuffer *renderBuffer)
{
	DEBUG_FUNCTION_T(DCDT_UI);
	ASSERT(maxValue > minValue);

	if (style == null) 			style = findStyle<UISliderStyle>("default"_s);
	if (renderBuffer == null) 	renderBuffer = &renderer->uiBuffer;

	// Value ranges
	*currentValue = clamp(*currentValue, minValue, maxValue);
	f32 valueRange = maxValue - minValue;
	f32 currentPercent = (*currentValue - minValue) / valueRange;

	// Calculate where the thumb is initially
	s32 travelX = (bounds.w - style->thumbSize.x); // Space available for the thumb to move in
	s32 thumbX = bounds.x + round_s32((f32)travelX * currentPercent);
	s32 thumbY = bounds.y + ((bounds.h - style->thumbSize.y) / 2);
	Rect2I thumbBounds = irectXYWH(thumbX, thumbY, style->thumbSize.x, style->thumbSize.y);

	// Interact with mouse
	UIDrawableStyle *thumbStyle = &style->thumb;
	if (isDisabled)
	{
		thumbStyle = &style->thumbDisabled;
	}
	else if (isDragging(currentValue))
	{
		// Move
		V2I thumbPos = getDraggingObjectPos();
		thumbBounds.x = clamp(thumbPos.x, bounds.x, bounds.x + travelX);

		// Apply that to the currentValue
		f32 positionPercent = (f32)(thumbBounds.x - bounds.x) / (f32)travelX;
		*currentValue = minValue + (positionPercent * valueRange);

		thumbStyle = &style->thumbPressed;
	}
	else if (isMouseInUIBounds(bounds))
	{
		bool inThumbBounds = isMouseInUIBounds(thumbBounds);

		if (mouseButtonJustPressed(MouseButton_Left))
		{
			// If we're not on the thumb, jump the thumb to where we are!
			if (!inThumbBounds)
			{
				thumbBounds.x = clamp(mousePos.x - (thumbBounds.w / 2), bounds.x, bounds.x + travelX);

				// Apply that to the currentValue
				f32 positionPercent = (f32)(thumbBounds.x - bounds.x) / (f32)travelX;
				*currentValue = minValue + (positionPercent * valueRange);
			}

			// Start drag
			startDragging(currentValue, thumbBounds.pos);

			thumbStyle = &style->thumbPressed;
		}
		else if (inThumbBounds)
		{
			// Hovering thumb
			thumbStyle = &style->thumbHover;
		}
	}

	// Draw things
	s32 trackThickness = (style->trackThickness != 0) ? style->trackThickness : bounds.h;
	Rect2I trackBounds = irectAligned(bounds.x, bounds.y + bounds.h / 2, bounds.w, trackThickness, ALIGN_LEFT | ALIGN_V_CENTRE);
	UIDrawable(&style->track).draw(renderBuffer, trackBounds);
	UIDrawable(thumbStyle).draw(renderBuffer, thumbBounds);
}

bool UI::putTextInput(TextInput *textInput, Rect2I bounds, UITextInputStyle *style, RenderBuffer *renderBuffer)
{
	if (style == null) style = findStyle<UITextInputStyle>("default"_s);
	if (renderBuffer == null) renderBuffer = &renderer->uiBuffer;

	bool submittedInput = updateTextInput(textInput);

	drawTextInput(renderBuffer, textInput, style, bounds);

	// Capture the input focus if we just clicked on this TextInput
	if (justClickedOnUI(bounds))
	{
		markMouseInputHandled();
		captureInput(textInput);
		textInput->caretFlashCounter = 0;
	}

	return submittedInput;
}

void UI::pushToast(String message)
{
	Toast *newToast = uiState.toasts.push();

	*newToast = {};
	newToast->text = makeString(newToast->_chars, MAX_TOAST_LENGTH);
	copyString(message, &newToast->text);

	newToast->duration = TOAST_APPEAR_TIME + TOAST_DISPLAY_TIME + TOAST_DISAPPEAR_TIME;
	newToast->time = 0;
}

void UI::drawToast()
{
	DEBUG_FUNCTION_T(DCDT_UI);

	Maybe<Toast*> currentToast = uiState.toasts.peek();
	if (currentToast.isValid)
	{
		Toast *toast = currentToast.value;

		toast->time += globalAppState.deltaTime;

		if (toast->time >= toast->duration)
		{
			uiState.toasts.pop();
		}
		else
		{
			UIPanelStyle *style = findStyle<UIPanelStyle>("toast"_s);
			V2I origin = v2i(windowSize.x / 2, windowSize.y - 8);

			UILabelStyle *labelStyle = findStyle<UILabelStyle>(&style->labelStyle);
			s32 maxWidth = min(floor_s32(windowSize.x * 0.8f), 500);
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

void UI::showTooltip(String text)
{
	uiState.tooltipText = text;
	showTooltip(basicTooltipWindowProc);
}

void UI::showTooltip(WindowProc tooltipProc, void *userData)
{
	static String styleName = "tooltip"_s;
	showWindow(nullString, 300, 100, v2i(0,0), styleName, WinFlag_AutomaticHeight | WinFlag_ShrinkWidth | WinFlag_Unique | WinFlag_Tooltip | WinFlag_Headless, tooltipProc, userData);
}

void UI::basicTooltipWindowProc(WindowContext *context, void * /*userData*/)
{
	UIPanel *ui = &context->windowPanel;
	ui->addText(uiState.tooltipText);
}
