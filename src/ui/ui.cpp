// ui.cpp

namespace UI
{
	void init(MemoryArena *arena)
	{
		uiState = {};

		initChunkedArray(&uiState.uiRects, arena, 64);
		initStack(&uiState.inputScissorRects, arena);

		initQueue(&uiState.toasts, arena);

		initChunkedArray(&uiState.openWindows, arena, 64);
		initSet(&uiState.windowsToClose, arena);
		initSet(&uiState.windowsToMakeActive, arena);

		initScrollbar(&uiState.openMenuScrollbar, Orientation::Vertical);

		initScrollbar(&uiState.openDropDownListScrollbar, Orientation::Vertical);
	}

	void startFrame()
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

	void endFrame()
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

	inline bool isMouseInputHandled()
	{
		return uiState.mouseInputHandled;
	}

	inline void markMouseInputHandled()
	{
		uiState.mouseInputHandled = true;
	}

	inline bool isMouseInUIBounds(Rect2I bounds)
	{
		return isMouseInUIBounds(bounds, mousePos);
	}

	inline bool isMouseInUIBounds(Rect2I bounds, V2I pos)
	{
		Rect2I clippedBounds = isInputScissorActive() ? intersect(bounds, getInputScissorRect()) : bounds;

		bool result = contains(clippedBounds, pos);

		return result;
	}

	inline bool justClickedOnUI(Rect2I bounds)
	{
		Rect2I clippedBounds = isInputScissorActive() ? intersect(bounds, getInputScissorRect()) : bounds;

		bool result = !isMouseInputHandled()
				   && contains(clippedBounds, mousePos)
				   && mouseButtonJustReleased(MouseButton_Left)
				   && contains(clippedBounds, mouseClickStartPos);

		return result;
	}

	WidgetMouseState getWidgetMouseState(Rect2I widgetBounds)
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

	inline bool isDragging(void *object)
	{
		return (object == uiState.currentDragObject);
	}

	void startDragging(void *object, V2I objectPos)
	{
		uiState.currentDragObject = object;
		uiState.dragObjectStartPos = objectPos;
	}

	V2I getDraggingObjectPos()
	{
		return uiState.dragObjectStartPos + (mousePos - mouseClickStartPos);
	}

	inline void pushInputScissorRect(Rect2I bounds)
	{
		push(&uiState.inputScissorRects, bounds);
	}

	inline void popInputScissorRect()
	{
		pop(&uiState.inputScissorRects);
	}

	inline bool isInputScissorActive()
	{
		return !isEmpty(&uiState.inputScissorRects);
	}

	inline Rect2I getInputScissorRect()
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

	inline void addUIRect(Rect2I bounds)
	{
		uiState.uiRects.append(bounds);
	}

	bool mouseIsWithinUIRects()
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

	V2I calculateButtonSize(String text, ButtonStyle *style, s32 maxWidth, bool fillWidth)
	{
		// If we have icons, take them into account
		V2I startIconSize = style->startIcon.getSize();
		if (startIconSize.x > 0) startIconSize.x += style->contentPadding;

		V2I endIconSize =  style->endIcon.getSize();
		if (endIconSize.x > 0) endIconSize.x += style->contentPadding;

		s32 textMaxWidth = 0;
		if (maxWidth != 0)
		{
			textMaxWidth = maxWidth - (style->padding.left + style->padding.right) - startIconSize.x - endIconSize.x;
		}

		V2I result = {};
		BitmapFont *font = getFont(&style->font);

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
				resultWidth = (textSize.x + startIconSize.x + endIconSize.x + style->padding.left + style->padding.right);
			}

			result = v2i(resultWidth, max(textSize.y, startIconSize.y, endIconSize.y) + (style->padding.top + style->padding.bottom));
		}

		return result;
	}

	V2I calculateButtonSize(V2I contentSize, ButtonStyle *style, s32 maxWidth, bool fillWidth)
	{
		// If we have icons, take them into account
		V2I startIconSize = style->startIcon.getSize();
		if (startIconSize.x > 0) startIconSize.x += style->contentPadding;

		V2I endIconSize =  style->endIcon.getSize();
		if (endIconSize.x > 0) endIconSize.x += style->contentPadding;

		V2I result = {};

		if (fillWidth && (maxWidth > 0))
		{
			result.x = maxWidth;
		}
		else
		{
			result.x = (contentSize.x + style->padding.left + style->padding.right + startIconSize.x + endIconSize.x);
		}

		result.y = max(contentSize.y, startIconSize.y, endIconSize.y) + (style->padding.top + style->padding.bottom);

		return result;
	}

	Rect2I calculateButtonContentBounds(Rect2I bounds, ButtonStyle *style)
	{
		Rect2I contentBounds = shrink(bounds, style->padding);

		V2I startIconSize = style->startIcon.getSize();
		if (startIconSize.x > 0)
		{
			contentBounds.x += startIconSize.x + style->contentPadding;
			contentBounds.w -= startIconSize.x + style->contentPadding;
		}

		V2I endIconSize = style->endIcon.getSize();
		if (endIconSize.x > 0)
		{
			contentBounds.w -= endIconSize.x + style->contentPadding;
		}

		return contentBounds;
	}

	bool putButton(Rect2I bounds, ButtonStyle *style, ButtonState state, RenderBuffer *renderBuffer, String tooltip)
	{
		DEBUG_FUNCTION_T(DCDT_UI);

		if (style == null) 			style = getStyle<ButtonStyle>("default"_s);
		if (renderBuffer == null) 	renderBuffer = &renderer->uiBuffer;
		
		bool buttonClicked = false;

		WidgetMouseState mouseState = getWidgetMouseState(bounds);

		DrawableStyle *backgroundStyle = &style->background;

		if (state == Button_Disabled)
		{
			backgroundStyle = &style->backgroundDisabled;
		}
		else if (mouseState.isHovered)
		{
			if (mouseState.isPressed)
			{
				backgroundStyle = &style->backgroundPressed;
			}
			else
			{
				backgroundStyle = &style->backgroundHover;
			}

			if (tooltip.length)
			{
				showTooltip(tooltip);
			}
		}
		else if (state == Button_Active)
		{
			backgroundStyle = &style->backgroundHover;
		}

		Drawable buttonBackground = Drawable(backgroundStyle);
		buttonBackground.draw(renderBuffer, bounds);

		// Icons!
		if (style->startIcon.type != Drawable_None)
		{
			V2I startIconSize = style->startIcon.getSize();
			Rect2I startIconBounds = alignWithinRectangle(bounds, startIconSize, style->startIconAlignment, style->padding);

			Drawable(&style->startIcon).draw(renderBuffer, startIconBounds);
		}
		if (style->endIcon.type != Drawable_None)
		{
			V2I endIconSize = style->endIcon.getSize();
			Rect2I endIconBounds = alignWithinRectangle(bounds, endIconSize, style->endIconAlignment, style->padding);

			Drawable(&style->endIcon).draw(renderBuffer, endIconBounds);
		}

		// Respond to click
		if ((state != Button_Disabled) && justClickedOnUI(bounds))
		{
			buttonClicked = true;
			markMouseInputHandled();
		}

		return buttonClicked;
	}

	bool putTextButton(String text, Rect2I bounds, ButtonStyle *style, ButtonState state, RenderBuffer *renderBuffer, String tooltip)
	{
		if (renderBuffer == null)
		{
			renderBuffer = &renderer->uiBuffer;
		}

		bool result = putButton(bounds, style, state, renderBuffer, tooltip);

		Rect2I contentBounds = calculateButtonContentBounds(bounds, style);

		drawText(renderBuffer, getFont(&style->font), text, contentBounds, style->textAlignment, style->textColor, renderer->shaderIds.text);

		return result;
	}

	bool putImageButton(Sprite *sprite, Rect2I bounds, ButtonStyle *style, ButtonState state, RenderBuffer *renderBuffer, String tooltip)
	{
		if (renderBuffer == null) renderBuffer = &renderer->uiBuffer;

		bool result = putButton(bounds, style, state, renderBuffer, tooltip);

		Rect2I contentBounds = calculateButtonContentBounds(bounds, style);
		Rect2 spriteBounds = rect2(contentBounds);
		drawSingleSprite(renderBuffer, sprite, spriteBounds, renderer->shaderIds.textured, makeWhite());

		return result;
	}

	V2I calculateCheckboxSize(CheckboxStyle *style)
	{
		V2I result = v2i(
			style->checkSize.x + style->padding.left + style->padding.right,
			style->checkSize.y + style->padding.top + style->padding.bottom
		);

		return result;
	}

	void putCheckbox(bool *checked, Rect2I bounds, CheckboxStyle *style, bool isDisabled, RenderBuffer *renderBuffer)
	{
		DEBUG_FUNCTION_T(DCDT_UI);

		if (style == null) 			style = getStyle<CheckboxStyle>("default"_s);
		if (renderBuffer == null) 	renderBuffer = &renderer->uiBuffer;

		WidgetMouseState mouseState = getWidgetMouseState(bounds);

		DrawableStyle *backgroundStyle = &style->background;
		DrawableStyle *checkStyle = &style->check;

		if (isDisabled)
		{
			backgroundStyle = &style->backgroundDisabled;
			checkStyle = &style->checkDisabled;
		}
		else if (mouseState.isHovered)
		{
			if (mouseState.isPressed)
			{
				backgroundStyle = &style->backgroundPressed;
				checkStyle = &style->checkPressed;
			}
			else
			{
				backgroundStyle = &style->backgroundHover;
				checkStyle = &style->checkHover;
			}
		}

		if (!isDisabled && justClickedOnUI(bounds))
		{
			*checked = !(*checked);
			markMouseInputHandled();
		}

		// Render it
		Drawable(backgroundStyle).draw(renderBuffer, bounds);

		if (*checked)
		{
			Rect2I checkBounds = shrink(bounds, style->padding);
			Drawable(checkStyle).draw(renderBuffer, checkBounds);
		}
	}

	template <typename T>
	V2I calculateDropDownListSize(Array<T> *listOptions, String (*getDisplayName)(T *data), DropDownListStyle *style, s32 maxWidth, bool fillWidth)
	{
		if (style == null) style = getStyle<DropDownListStyle>("default"_s);
		ButtonStyle *buttonStyle = getStyle<ButtonStyle>(&style->buttonStyle);

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
	void putDropDownList(Array<T> *listOptions, s32 *currentSelection, String (*getDisplayName)(T *data), Rect2I bounds, DropDownListStyle *style, RenderBuffer *renderBuffer)
	{
		DEBUG_FUNCTION_T(DCDT_UI);

		if (style == null) 			style = getStyle<DropDownListStyle>("default"_s);
		if (renderBuffer == null) 	renderBuffer = &renderer->uiBuffer;

		bool isOpen = isDropDownListOpen(listOptions);

		// We always draw the current-selection box,
		// and then show the panel if needed.

		// Show the selection box
		String selectionText = getDisplayName(listOptions->get(*currentSelection));
		bool clicked = putTextButton(selectionText, bounds, getStyle<ButtonStyle>(&style->buttonStyle), buttonIsActive(isOpen), renderBuffer);
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
			Panel panel = Panel(panelBounds, getStyle<PanelStyle>(&style->panelStyle), 0, uiState.openDropDownListRenderBuffer);
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

	void openDropDownList(void *pointer)
	{
		uiState.openDropDownList = pointer;
		if (uiState.openDropDownListRenderBuffer == null)
		{
			uiState.openDropDownListRenderBuffer = getTemporaryRenderBuffer("DropDownList"_s);
		}
		clearRenderBuffer(uiState.openDropDownListRenderBuffer);
		initScrollbar(&uiState.openDropDownListScrollbar, Orientation::Vertical);
	}

	void closeDropDownList()
	{
		uiState.openDropDownList = null;
		returnTemporaryRenderBuffer(uiState.openDropDownListRenderBuffer);
		uiState.openDropDownListRenderBuffer = null;
	}

	inline bool isDropDownListOpen(void *pointer)
	{
		return (uiState.openDropDownList == pointer);
	}

	V2I calculateLabelSize(String text, LabelStyle *style, s32 maxWidth, bool fillWidth)
	{
		if (style == null) 			style = getStyle<LabelStyle>("default"_s);

		s32 maxTextWidth = maxWidth - (style->padding.left + style->padding.right);

		V2I textSize = calculateTextSize(getFont(&style->font), text, maxTextWidth);

		// Add padding
		V2I result = v2i(
			textSize.x + style->padding.left + style->padding.right,
			textSize.y + style->padding.top + style->padding.bottom
		);

		if (fillWidth)
		{
			result.x = maxWidth;
		}

		return result;
	}

	void putLabel(String text, Rect2I bounds, LabelStyle *style, RenderBuffer *renderBuffer)
	{
		DEBUG_FUNCTION_T(DCDT_UI);

		if (style == null) 			style = getStyle<LabelStyle>("default"_s);
		if (renderBuffer == null) 	renderBuffer = &renderer->uiBuffer;

		Rect2I textBounds = shrink(bounds, style->padding);

		Drawable(&style->background).draw(renderBuffer, bounds);

		drawText(renderBuffer, getFont(&style->font), text, textBounds, style->textAlignment, style->textColor, renderer->shaderIds.text);
	}

	void showMenu(s32 menuID)
	{
		// NB: Do all menu-state-initialisation here!
		uiState.openMenu = menuID;
		uiState.openMenuScrollbar = {};
	}

	void hideMenus()
	{
		uiState.openMenu = 0;
	}

	void toggleMenuVisible(s32 menuID)
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

	inline bool isMenuVisible(s32 menuID)
	{
		return (uiState.openMenu == menuID);
	}

	inline ScrollbarState *getMenuScrollbar()
	{
		return &uiState.openMenuScrollbar;
	}

	V2I calculateRadioButtonSize(RadioButtonStyle *style)
	{
		if (style == null) 			style = getStyle<RadioButtonStyle>("default"_s);

		V2I result = style->size;

		return result;
	}
	
	void putRadioButton(s32 *selectedValue, s32 value, Rect2I bounds, RadioButtonStyle *style, bool isDisabled, RenderBuffer *renderBuffer)
	{
		DEBUG_FUNCTION_T(DCDT_UI);

		if (style == null) 			style = getStyle<RadioButtonStyle>("default"_s);
		if (renderBuffer == null) 	renderBuffer = &renderer->uiBuffer;

		WidgetMouseState mouseState = getWidgetMouseState(bounds);

		DrawableStyle *backgroundStyle = &style->background;
		DrawableStyle *dotStyle = &style->dot;

		if (isDisabled)
		{
			backgroundStyle = &style->backgroundDisabled;
			dotStyle = &style->dotDisabled;
		}
		else if (mouseState.isHovered)
		{
			if (mouseState.isPressed)
			{
				backgroundStyle = &style->backgroundPressed;
				dotStyle = &style->dotPressed;
			}
			else
			{
				backgroundStyle = &style->backgroundHover;
				dotStyle = &style->dotHover;
			}
		}

		if (!isDisabled && justClickedOnUI(bounds))
		{
			*selectedValue = value;
			markMouseInputHandled();
		}

		// Render it
		Drawable(backgroundStyle).draw(renderBuffer, bounds);

		if (*selectedValue == value)
		{
			Rect2I dotBounds = centreWithin(bounds, style->dotSize);
			Drawable(dotStyle).draw(renderBuffer, dotBounds);
		}
	}

	void initScrollbar(ScrollbarState *state, Orientation orientation, s32 mouseWheelStepSize)
	{
		*state = {};

		state->orientation = orientation;
		state->mouseWheelStepSize = mouseWheelStepSize;
	}

	Maybe<Rect2I> getScrollbarThumbBounds(ScrollbarState *state, Rect2I scrollbarBounds, ScrollbarStyle *style)
	{
		Maybe<Rect2I> result = makeFailure<Rect2I>();

		// NB: This algorithm for thumb size came from here: https://ux.stackexchange.com/a/85698
		// (Which is ultimately taken from Microsoft's .NET documentation.)
		// Anyway, it's simple enough, but distinguishes between the size of visible content, and
		// the size of the scrollbar's track. For us, for now, those are the same value, but I'm
		// keeping them as separate variables, which is why viewportSize and trackSize are
		// the same. If we add scrollbar buttons, that'll reduce the track size.
		// - Sam, 01/04/2021
		if (state->orientation == Orientation::Horizontal)
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
			ASSERT(state->orientation == Orientation::Vertical);

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

	void putScrollbar(ScrollbarState *state, s32 contentSize, Rect2I bounds, ScrollbarStyle *style, bool isDisabled, RenderBuffer *renderBuffer)
	{
		DEBUG_FUNCTION_T(DCDT_UI);

		ASSERT(hasPositiveArea(bounds));

		if (style == null) 			style = getStyle<ScrollbarStyle>("default"_s);
		if (renderBuffer == null) 	renderBuffer = &renderer->uiBuffer;

		Drawable(&style->background).draw(renderBuffer, bounds);

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
				bool isHorizontal = (state->orientation == Orientation::Horizontal);
				ASSERT(isHorizontal || (state->orientation == Orientation::Vertical));

				Maybe<Rect2I> thumb = getScrollbarThumbBounds(state, bounds, style);
				if (thumb.isValid)
				{
					s32 overflowSize   = state->contentSize - (isHorizontal ? bounds.w : bounds.h);
					Rect2I thumbBounds = thumb.value;
					s32 thumbSize      = isHorizontal ? thumbBounds.w : thumbBounds.h;
					s32 gutterSize     = isHorizontal ? bounds.w : bounds.h;
					s32 thumbRange     = gutterSize - thumbSize;

					// Scrollwheel stuff
					// (It's weird that we're putting this within mouseInputHandled, but eh)
					s32 mouseWheelDelta = (isHorizontal ? inputState->wheelX : -inputState->wheelY);
					if (mouseWheelDelta != 0)
					{
						s32 oldScrollOffset = getScrollbarContentOffset(state, gutterSize);
						s32 scrollOffset = oldScrollOffset + (state->mouseWheelStepSize * mouseWheelDelta);

						state->scrollPercent = clamp01((f32)scrollOffset / (f32)overflowSize);
					}

					// Mouse stuff
					DrawableStyle *thumbStyle = &style->thumb;
					if (isDisabled)
					{
						thumbStyle = &style->thumbDisabled;
					}
					else if (isDragging(state))
					{
						// Move
						V2I thumbPos = getDraggingObjectPos();

						// @Copypasta We duplicate this code below, because there are two states where we need to set
						// the new thumb position. It's really awkward but I don't know how to pull the logic out.
						if (isHorizontal)
						{
							thumbBounds.x = clamp(thumbPos.x, bounds.x, bounds.x + thumbRange);
							state->scrollPercent = clamp01((f32)(thumbBounds.x - bounds.x) / (f32)thumbRange);
						}
						else
						{
							thumbBounds.y = clamp(thumbPos.y, bounds.y, bounds.y + thumbRange);
							state->scrollPercent = clamp01((f32)(thumbBounds.y - bounds.y) / (f32)thumbRange);
						}
						
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
								if (isHorizontal)
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

							thumbStyle = &style->thumbPressed;
						}
						else if (inThumbBounds)
						{
							// Hovering thumb
							thumbStyle = &style->thumbHover;
						}
					}

					Drawable(thumbStyle).draw(renderBuffer, thumbBounds);
				}
			}
		}
	}

	s32 getScrollbarContentOffset(ScrollbarState *state, s32 scrollbarSize)
	{
		s32 overflowSize = state->contentSize - scrollbarSize;

		s32 result = round_s32(state->scrollPercent * overflowSize);

		return result;
	}

	V2I calculateSliderSize(Orientation orientation, SliderStyle *style, V2I availableSpace, bool fillSpace)
	{
		if (style == null) 			style = getStyle<SliderStyle>("default"_s);

		V2I result = {};

		// This is really arbitrary, but sliders don't have an inherent length, so they need something!
		s32 standardSize = 200;

		if (orientation == Orientation::Horizontal)
		{
			result = v2i(fillSpace ? availableSpace.x : standardSize, style->thumbSize.y);
		}
		else
		{
			ASSERT(orientation == Orientation::Vertical);

			result = v2i(style->thumbSize.x, fillSpace ? availableSpace.y : standardSize);
		}

		return result;
	}

	void putSlider(f32 *currentValue, f32 minValue, f32 maxValue, Orientation orientation, Rect2I bounds, SliderStyle *style, bool isDisabled, RenderBuffer *renderBuffer, bool snapToWholeNumbers)
	{
		DEBUG_FUNCTION_T(DCDT_UI);
		ASSERT(maxValue > minValue);

		if (style == null) 			style = getStyle<SliderStyle>("default"_s);
		if (renderBuffer == null) 	renderBuffer = &renderer->uiBuffer;

		// Value ranges
		*currentValue = clamp(*currentValue, minValue, maxValue);
		f32 valueRange = maxValue - minValue;
		f32 currentPercent = (*currentValue - minValue) / valueRange;

		// Calculate where the thumb is initially
		s32 travel; // Space available for the thumb to move in
		V2I thumbPos;
		if (orientation == Orientation::Horizontal)
		{
			travel = (bounds.w - style->thumbSize.x);
			thumbPos.x = bounds.x + round_s32((f32)travel * currentPercent);
			thumbPos.y = bounds.y + ((bounds.h - style->thumbSize.y) / 2);
		}
		else
		{
			ASSERT(orientation == Orientation::Vertical);

			travel = (bounds.h - style->thumbSize.y);
			thumbPos.x = bounds.x + ((bounds.w - style->thumbSize.x) / 2);
			thumbPos.y = bounds.y + bounds.h - style->thumbSize.y - round_s32((f32)travel * currentPercent);
		}
		Rect2I thumbBounds = irectPosSize(thumbPos, style->thumbSize);

		// Interact with mouse
		DrawableStyle *thumbStyle = &style->thumb;
		if (isDisabled)
		{
			thumbStyle = &style->thumbDisabled;
		}
		else if (isDragging(currentValue))
		{
			// Move
			V2I draggedPos = getDraggingObjectPos();
			f32 positionPercent;
			if (orientation == Orientation::Horizontal)
			{
				thumbBounds.x = clamp(draggedPos.x, bounds.x, bounds.x + travel);
				positionPercent = (f32)(thumbBounds.x - bounds.x) / (f32)travel;
			}
			else
			{
				ASSERT(orientation == Orientation::Vertical);

				thumbBounds.y = clamp(draggedPos.y, bounds.y, bounds.y + travel);
				positionPercent = 1.0f - (f32)(thumbBounds.y - bounds.y) / (f32)travel;
			}

			// Apply that to the currentValue
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

					f32 positionPercent;
					if (orientation == Orientation::Horizontal)
					{
						thumbBounds.x = clamp(mousePos.x - (thumbBounds.w / 2), bounds.x, bounds.x + travel);
						positionPercent = (f32)(thumbBounds.x - bounds.x) / (f32)travel;
					}
					else
					{
						ASSERT(orientation == Orientation::Vertical);

						thumbBounds.y = clamp(mousePos.y - (thumbBounds.h / 2), bounds.y, bounds.y + travel);
						positionPercent = 1.0f - (f32)(thumbBounds.y - bounds.y) / (f32)travel;
					}

					// Apply that to the currentValue
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

		// Snap the thumb (and currentValue) to a position representing an integer value, if that was requested
		// We do this here, to avoid duplicating this in both thumb-movement paths above. It's a quick enough
		// calculation that it shouldn't matter that we're doing it every frame regardless of if the value did
		// change.
		if (snapToWholeNumbers)
		{
			*currentValue = round_f32(*currentValue);
			currentPercent = (*currentValue - minValue) / valueRange;

			if (orientation == Orientation::Horizontal)
			{
				thumbBounds.x = bounds.x + round_s32((f32)travel * currentPercent);
			}
			else
			{
				ASSERT(orientation == Orientation::Vertical);

				thumbBounds.y = bounds.y + bounds.h - thumbBounds.h - round_s32((f32)travel * currentPercent);
			}
		}

		// Draw things
		Rect2I trackBounds;
		if (orientation == Orientation::Horizontal)
		{
			s32 trackThickness = (style->trackThickness != 0) ? style->trackThickness : bounds.h;
			trackBounds = irectAligned(bounds.x, bounds.y + bounds.h / 2, bounds.w, trackThickness, ALIGN_LEFT | ALIGN_V_CENTRE);
		}
		else
		{
			ASSERT(orientation == Orientation::Vertical);

			s32 trackThickness = (style->trackThickness != 0) ? style->trackThickness : bounds.w;
			trackBounds = irectAligned(bounds.x + bounds.w / 2, bounds.y, trackThickness, bounds.h, ALIGN_TOP | ALIGN_H_CENTRE);
		}

		Drawable(&style->track).draw(renderBuffer, trackBounds);
		Drawable(thumbStyle).draw(renderBuffer, thumbBounds);
	}

	void putSlider(s32 *currentValue, s32 minValue, s32 maxValue, Orientation orientation, Rect2I bounds, SliderStyle *style, bool isDisabled, RenderBuffer *renderBuffer)
	{
		f32 currentValueF = (f32) *currentValue;
		f32 minValueF     = (f32)  minValue;
		f32 maxValueF     = (f32)  maxValue;

		putSlider(&currentValueF, minValueF, maxValueF, orientation, bounds, style, isDisabled, renderBuffer, true);

		*currentValue = round_s32(currentValueF);
	}

	bool putTextInput(TextInput *textInput, Rect2I bounds, TextInputStyle *style, RenderBuffer *renderBuffer)
	{
		if (style == null) style = getStyle<TextInputStyle>("default"_s);
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

	void pushToast(String message)
	{
		Toast *newToast = uiState.toasts.push();

		*newToast = {};
		newToast->text = makeString(newToast->_chars, MAX_TOAST_LENGTH);
		copyString(message, &newToast->text);

		newToast->duration = TOAST_APPEAR_TIME + TOAST_DISPLAY_TIME + TOAST_DISAPPEAR_TIME;
		newToast->time = 0;
	}

	void drawToast()
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
				PanelStyle *style = getStyle<PanelStyle>("toast"_s);
				V2I origin = v2i(windowSize.x / 2, windowSize.y - 8);

				LabelStyle *labelStyle = getStyle<LabelStyle>(&style->labelStyle);
				s32 maxWidth = min(floor_s32(windowSize.x * 0.8f), 500);
				V2I textSize = calculateTextSize(getFont(&labelStyle->font), toast->text, maxWidth - (style->padding.left + style->padding.right));

				V2I toastSize = v2i(
					textSize.x + (style->padding.left + style->padding.right),
					textSize.y + (style->padding.top + style->padding.bottom)
				);

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

				Panel panel = Panel(toastBounds, style);
				panel.addLabel(toast->text);
				panel.end();
			}
		}
	}

	void showTooltip(String text)
	{
		uiState.tooltipText = text;
		showTooltip(basicTooltipWindowProc);
	}

	void showTooltip(WindowProc tooltipProc, void *userData)
	{
		static String styleName = "tooltip"_s;
		showWindow(UI::WindowTitle::none(), 300, 100, v2i(0,0), styleName, WindowFlags::AutomaticHeight | WindowFlags::ShrinkWidth | WindowFlags::Unique | WindowFlags::Tooltip | WindowFlags::Headless, tooltipProc, userData);
	}

	void basicTooltipWindowProc(WindowContext *context, void * /*userData*/)
	{
		Panel *ui = &context->windowPanel;
		ui->addLabel(uiState.tooltipText);
	}
}
