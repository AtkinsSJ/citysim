#pragma once

UIPanel::UIPanel(Rect2I bounds, UIPanelStyle *panelStyle, u32 flags)
{
	DEBUG_FUNCTION();

	if (panelStyle == null)
	{
		this->style = findPanelStyle(assets->theme, "default"_s);
	}
	else
	{
		this->style = panelStyle;
	}

	this->hasAddedWidgets = false;
	this->blocksMouse = (flags & Panel_BlocksMouse) != 0;

	this->doUpdate = (flags & Panel_DoUpdate) != 0;
	this->doRender = (flags & Panel_DoRender) != 0;
	
	this->bounds = bounds;
	this->contentArea = shrink(bounds, this->style->margin);
	this->topToBottom = (flags & Panel_LayoutTopToBottom) != 0;

	// Default to left-aligned
	u32 hAlignment = this->style->widgetAlignment & ALIGN_H;
	if (hAlignment == 0) hAlignment = ALIGN_LEFT;
	this->widgetAlignment = hAlignment | (topToBottom ? ALIGN_TOP : ALIGN_BOTTOM);

	this->hScrollbar = null;
	this->vScrollbar = null;

	// Relative to contentArea
	this->currentLeft = 0;
	this->currentRight = this->contentArea.w;
	this->currentTop = 0;
	this->currentBottom = this->contentArea.h;

	this->largestItemWidth = 0;
	this->largestItemHeightOnLine = 0;
	this->largestLineWidth = 0;

	this->background = UIDrawable(&this->style->background);
}

void UIPanel::enableHorizontalScrolling(ScrollbarState *scrollbarState)
{
	UIScrollbarStyle *scrollbarStyle = findStyle<UIScrollbarStyle>(&style->scrollbarStyle);
	ASSERT(scrollbarStyle != null);

	this->hScrollbar = scrollbarState;
	this->hScrollbarBounds = irectXYWH(bounds.x, bounds.y + bounds.h - scrollbarStyle->width, bounds.w, scrollbarStyle->width);

	this->contentArea.h -= scrollbarStyle->width;
	updateLayoutPosition();
}

void UIPanel::enableVerticalScrolling(ScrollbarState *scrollbarState, bool expandWidth)
{
	UIScrollbarStyle *scrollbarStyle = findStyle<UIScrollbarStyle>(&style->scrollbarStyle);
	ASSERT(scrollbarStyle != null);

	this->vScrollbar = scrollbarState;

	s32 scrollbarX = bounds.x + bounds.w - (expandWidth ? 0 : scrollbarStyle->width);
	this->vScrollbarBounds = irectXYWH(scrollbarX, bounds.y, scrollbarStyle->width, bounds.h);

	if (expandWidth)
	{
		this->bounds.w += scrollbarStyle->width;
	}
	else
	{
		this->contentArea.w -= scrollbarStyle->width;
		this->currentRight -= scrollbarStyle->width;
		updateLayoutPosition();
	}
}

bool UIPanel::addButton(String text, ButtonState state, String styleName)
{
	DEBUG_FUNCTION();
	
	prepareForWidgets();

	UIButtonStyle *buttonStyle = null;
	if (!isEmpty(styleName))  buttonStyle = findButtonStyle(assets->theme, styleName);
	if (buttonStyle == null)  buttonStyle = findStyle<UIButtonStyle>(&this->style->buttonStyle);

	Rect2I space = getCurrentLayoutPosition();
	s32 availableButtonContentSize = space.w - (buttonStyle->padding * 2);

	BitmapFont *font = getFont(&buttonStyle->font);
	V2I textSize = calculateTextSize(font, text, availableButtonContentSize);
	AddButtonInternalResult buttonResult = addButtonInternal(textSize, state, buttonStyle);

	drawText(&renderer->uiBuffer, font, text, buttonResult.contentBounds, buttonStyle->textAlignment, buttonStyle->textColor, renderer->shaderIds.text);

	return buttonResult.wasClicked;
}

bool UIPanel::addImageButton(Sprite *sprite, ButtonState state, String styleName)
{
	DEBUG_FUNCTION();
	
	prepareForWidgets();

	UIButtonStyle *buttonStyle = null;
	if (!isEmpty(styleName))  buttonStyle = findButtonStyle(assets->theme, styleName);
	if (buttonStyle == null)  buttonStyle = findStyle<UIButtonStyle>(&this->style->buttonStyle);

	Rect2I space = getCurrentLayoutPosition();
	s32 availableButtonContentSize = space.w - (buttonStyle->padding * 2);

	V2I spriteSize = v2i(sprite->pixelWidth, sprite->pixelHeight);

	if (availableButtonContentSize < sprite->pixelWidth)
	{
		// Scale the image so it fits
		f32 ratio = (f32)availableButtonContentSize / (f32)sprite->pixelWidth;

		spriteSize.x = availableButtonContentSize;
		spriteSize.y = floor_s32(ratio * (f32)sprite->pixelHeight);
	}

	AddButtonInternalResult buttonResult = addButtonInternal(spriteSize, state, buttonStyle);
	drawSingleSprite(&renderer->uiBuffer, sprite, rect2(buttonResult.contentBounds), renderer->shaderIds.textured, makeWhite());

	return buttonResult.wasClicked;
}

void UIPanel::addSprite(Sprite *sprite, s32 width, s32 height)
{
	DEBUG_FUNCTION();

	prepareForWidgets();

	Rect2I layoutPosition = getCurrentLayoutPosition();
	V2I origin = alignWithinRectangle(layoutPosition, widgetAlignment);
	V2I size = v2i(width, height);

	if (sprite != null)
	{
		if (size.x == -1) size.x = sprite->pixelWidth;
		if (size.y == -1) size.y = sprite->pixelHeight;

		Rect2I spriteBounds = irectAligned(origin, size, widgetAlignment);
		drawSingleSprite(&renderer->uiBuffer, sprite, rect2(spriteBounds), renderer->shaderIds.pixelArt, makeWhite());
	}

	completeWidget(size);
}

void UIPanel::addText(String text, String styleName)
{
	DEBUG_FUNCTION();
	
	prepareForWidgets();

	UILabelStyle *labelStyle = null;
	if (!isEmpty(styleName))  labelStyle = findLabelStyle(assets->theme, styleName);
	if (labelStyle == null)   labelStyle = findStyle<UILabelStyle>(&this->style->labelStyle);

	Rect2I space = getCurrentLayoutPosition();
	V2I origin = alignWithinRectangle(space, this->widgetAlignment);

	BitmapFont *font = getFont(&labelStyle->font);
	if (font)
	{
		V2I textSize = calculateTextSize(font, text, space.w);
		V2I topLeft = calculateTextPosition(origin, textSize, this->widgetAlignment);
		Rect2I textBounds = irectPosSize(topLeft, textSize);

		if (doRender)
		{
			drawText(&renderer->uiBuffer, font, text, textBounds, this->widgetAlignment, labelStyle->textColor, renderer->shaderIds.text);
		}

		completeWidget(textSize);
	}
}

bool UIPanel::addTextInput(TextInput *textInput, String styleName)
{
	DEBUG_FUNCTION();
	
	prepareForWidgets();

	bool result = false;
	if (doUpdate)
	{
		result = updateTextInput(textInput);
	}

	UITextInputStyle *textInputStyle = null;
	if (!isEmpty(styleName))    textInputStyle = findTextInputStyle(assets->theme, styleName);
	if (textInputStyle == null) textInputStyle = findStyle<UITextInputStyle>(&this->style->textInputStyle);

	s32 alignment = this->widgetAlignment;
	Rect2I space = getCurrentLayoutPosition();
	V2I origin = alignWithinRectangle(space, alignment);

	BitmapFont *font = getFont(&textInputStyle->font);
	if (font)
	{
		bool fillWidth = ((alignment & ALIGN_H) == ALIGN_EXPAND_H);
		V2I textInputSize = calculateTextInputSize(textInput, textInputStyle, space.w, fillWidth);
		Rect2I textInputBounds = irectAligned(origin, textInputSize, alignment);

		if (doRender)
		{
			drawTextInput(&renderer->uiBuffer, textInput, textInputStyle, textInputBounds);
		}

		if (doUpdate)
		{
			// Capture the input focus if we just clicked on this TextInput
			if (justClickedOnUI(globalAppState.uiState, textInputBounds))
			{
				globalAppState.uiState->mouseInputHandled = true;
				captureInput(textInput);
				textInput->caretFlashCounter = 0;
			}
		}

		completeWidget(textInputSize);
	}

	return result;
}

Rect2I UIPanel::addBlank(s32 width, s32 height)
{
	DEBUG_FUNCTION();

	prepareForWidgets();

	Rect2I layoutPosition = getCurrentLayoutPosition();
	V2I origin = alignWithinRectangle(layoutPosition, widgetAlignment);
	V2I size = v2i(width, height);

	completeWidget(size);

	return irectAligned(origin, size, widgetAlignment);
}

void UIPanel::alignWidgets(u32 alignment)
{
	widgetAlignment = (widgetAlignment & ALIGN_V) | (alignment & ALIGN_H);
}

void UIPanel::startNewLine(u32 hAlignment)
{
	DEBUG_FUNCTION();

	s32 lineWidth = (widgetAlignment & ALIGN_RIGHT) ? currentRight : currentLeft;
	largestLineWidth = max(largestLineWidth, lineWidth);

	currentLeft = 0;
	currentRight = contentArea.w;

	if (largestItemHeightOnLine > 0)
	{
		if (topToBottom)
		{
			currentTop += largestItemHeightOnLine + style->contentPadding;
		}
		else
		{
			currentBottom -= largestItemHeightOnLine + style->contentPadding;
		}
	}

	largestItemHeightOnLine = 0;

	// Only change alignment if we passed one
	if (hAlignment != 0)
	{
		widgetAlignment = (widgetAlignment & ALIGN_V) | (hAlignment & ALIGN_H);
	}
}

UIPanel UIPanel::row(s32 height, Alignment vAlignment, String styleName)
{
	DEBUG_FUNCTION();
	ASSERT(vAlignment == ALIGN_TOP || vAlignment == ALIGN_BOTTOM);
	
	prepareForWidgets();

	startNewLine();

	UIPanelStyle *rowStyle = getPanelStyle(styleName);

	if (vAlignment == ALIGN_TOP)
	{
		Rect2I rowBounds = irectXYWH(
			contentArea.x, contentArea.y + currentTop,
			contentArea.w, height
		);

		if (topToBottom)
		{
			completeWidget(rowBounds.size);
		}
		else
		{
			contentArea.h -= height + style->contentPadding;
			contentArea.y += height + style->contentPadding;
		}

		updateLayoutPosition();

		return UIPanel(rowBounds, rowStyle, getFlagsForChild());
	}
	else
	{
		Rect2I rowBounds = irectXYWH(
			contentArea.x, contentArea.y + currentBottom - height,
			contentArea.w, height
		);

		if (topToBottom)
		{
			contentArea.h -= height + style->contentPadding;
		}
		else
		{
			completeWidget(rowBounds.size);
		}
		
		updateLayoutPosition();

		return UIPanel(rowBounds, rowStyle, getFlagsForChild());
	}
}

UIPanel UIPanel::column(s32 width, Alignment hAlignment, String styleName)
{
	DEBUG_FUNCTION();
	ASSERT(hAlignment == ALIGN_LEFT || hAlignment == ALIGN_RIGHT);
	
	prepareForWidgets();

	startNewLine();

	UIPanelStyle *columnStyle = getPanelStyle(styleName);

	if (hAlignment == ALIGN_LEFT)
	{
		Rect2I columnBounds = irectXYWH(
			contentArea.x, contentArea.y + currentTop,
			width, contentArea.h
		);

		contentArea.w -= width + style->contentPadding;
		contentArea.x += width + style->contentPadding;
		updateLayoutPosition();

		return UIPanel(columnBounds, columnStyle, getFlagsForChild());
	}
	else
	{
		Rect2I columnBounds = irectXYWH(
			contentArea.x + contentArea.w - width, contentArea.y + currentTop,
			width, contentArea.h
		);

		contentArea.w -= width + style->contentPadding;
		updateLayoutPosition();

		return UIPanel(columnBounds, columnStyle, getFlagsForChild());
	}
}

void UIPanel::end(bool shrinkToContentHeight, bool shrinkToContentWidth)
{
	DEBUG_FUNCTION();
	UIState *uiState = globalAppState.uiState;

	// Make sure the current line's height is taken into account
	startNewLine();

	// @Hack! I don't at all understand why we get a trailing space of 2x the contentPadding at the end.
	if (!topToBottom && hasAddedWidgets)
	{
		currentBottom += (style->contentPadding * 2);
	}

	s32 contentHeight = (topToBottom ? (currentTop + style->margin)
									 : (contentArea.h - currentBottom));

	bool boundsChanged = false;

	if (shrinkToContentWidth)
	{
		if (widgetAlignment & ALIGN_LEFT)
		{
			s32 widthDifference = contentArea.w - largestLineWidth;
			if (widthDifference > 0)
			{
				bounds.w -= widthDifference;
				boundsChanged = true;
			}
		}
		else
		{
			s32 widthDifference = contentArea.w - largestLineWidth;
			if (widthDifference > 0)
			{
				bounds.x += widthDifference;
				bounds.w -= widthDifference;
				boundsChanged = true;
			}
		}
	}

	if (shrinkToContentHeight)
	{
		if (topToBottom)
		{
			contentHeight = currentTop - style->contentPadding + (2 *style->margin);
			bounds.h = clamp(contentHeight, (2 *style->margin), bounds.h);
		}
		else
		{
			contentHeight = (contentArea.h - currentBottom) + style->contentPadding + (2 *style->margin);
			s32 newHeight = clamp(contentHeight, (2 *style->margin), bounds.h);
			bounds.y += (bounds.h - newHeight);
			bounds.h = newHeight;
		}
	}

	if (boundsChanged)
	{
		hScrollbarBounds.w = bounds.w;
		hScrollbarBounds.x = bounds.x;

		vScrollbarBounds.h = bounds.h;
		vScrollbarBounds.y = bounds.y;
	}

	if (!hasAddedWidgets)
	{
		if (doRender)
		{
			addBeginScissor(&renderer->uiBuffer, bounds);

			// Prepare to render background
			background.preparePlaceholder(&renderer->uiBuffer);
		}
	}

	// Handle scrollbars
	if (hScrollbar)
	{
		UIScrollbarStyle *scrollbarStyle = findStyle<UIScrollbarStyle>(&style->scrollbarStyle);

		if (doUpdate)
		{
			updateScrollbar(uiState, hScrollbar, currentLeft + style->margin, hScrollbarBounds, scrollbarStyle);
		}

		f32 scrollPercent = getScrollbarPercent(hScrollbar, hScrollbarBounds.h);

		if (doRender)
		{
			drawScrollbar(&renderer->uiBuffer, scrollPercent, hScrollbarBounds.pos, hScrollbarBounds.h, scrollbarStyle);
		}
	}

	if (vScrollbar)
	{
		UIScrollbarStyle *scrollbarStyle = findStyle<UIScrollbarStyle>(&style->scrollbarStyle);

		if (doUpdate)
		{
			updateScrollbar(uiState, vScrollbar, contentHeight, vScrollbarBounds, scrollbarStyle);
		}

		f32 scrollPercent = getScrollbarPercent(vScrollbar, vScrollbarBounds.h);

		if (doRender)
		{
			drawScrollbar(&renderer->uiBuffer, scrollPercent, vScrollbarBounds.pos, vScrollbarBounds.h, scrollbarStyle);
		}
	}

	if (doRender)
	{
		// Fill in the background
		background.fillPlaceholder(bounds);

		// Clear any scissor stuff
		addEndScissor(&renderer->uiBuffer);

		popInputScissorRect(uiState);
	}

	// Add a UI rect if we're top level. Otherwise, our parent already added one that encompasses us!
	if (blocksMouse)
	{
		addUIRect(uiState, bounds);
	}
}

UIPanel::AddButtonInternalResult UIPanel::addButtonInternal(V2I contentSize, ButtonState state, UIButtonStyle *buttonStyle)
{
	DEBUG_FUNCTION();

	UIState *uiState = globalAppState.uiState;

	AddButtonInternalResult result = {};

	s32 buttonAlignment = this->widgetAlignment;
	Rect2I space = getCurrentLayoutPosition();
	V2I buttonOrigin = alignWithinRectangle(space, buttonAlignment);

	bool fillWidth = ((buttonAlignment & ALIGN_H) == ALIGN_EXPAND_H);
	V2I buttonSize = calculateButtonSize(contentSize, buttonStyle, space.w, fillWidth);
	Rect2I buttonBounds = irectAligned(buttonOrigin, buttonSize, buttonAlignment);

	result.contentBounds = irectAligned(
		alignWithinRectangle(buttonBounds, buttonStyle->textAlignment, buttonStyle->padding),
		contentSize,
		buttonStyle->textAlignment
	);

	if (doRender)
	{
		UIDrawableStyle *backgroundStyle = &buttonStyle->background;

		if (state == Button_Disabled)
		{
			backgroundStyle = &buttonStyle->disabledBackground;
		}
		else if (isMouseInUIBounds(uiState, buttonBounds))
		{
			// Mouse pressed: must have started and currently be inside the bounds to show anything
			// Mouse unpressed: show hover if in bounds
			if (mouseButtonPressed(MouseButton_Left))
			{
				if (isMouseInUIBounds(uiState, buttonBounds, getClickStartPos(MouseButton_Left, &renderer->uiCamera)))
				{
					backgroundStyle = &buttonStyle->pressedBackground;
				}
			}
			else
			{
				backgroundStyle = &buttonStyle->hoverBackground;
			}
		}
		else if (state == Button_Active)
		{
			backgroundStyle = &buttonStyle->hoverBackground;
		}

		UIDrawable(backgroundStyle).draw(&renderer->uiBuffer, buttonBounds);
	}

	if (doUpdate)
	{
		if ((state != Button_Disabled)
		 && justClickedOnUI(uiState, buttonBounds))
		{
			result.wasClicked = true;
			uiState->mouseInputHandled = true;
		}
	}

	completeWidget(buttonBounds.size);

	return result;
}

inline u32 UIPanel::getFlagsForChild()
{
	u32 flags = 0;

	if (doUpdate)    flags |= Panel_DoUpdate;
	if (doRender)    flags |= Panel_DoRender;
	if (topToBottom) flags |= Panel_LayoutTopToBottom;

	return flags;
}

void UIPanel::prepareForWidgets()
{
	if (!hasAddedWidgets)
	{
		if (doRender)
		{
			addBeginScissor(&renderer->uiBuffer, bounds);

			pushInputScissorRect(globalAppState.uiState, bounds);

			// Prepare to render background
			background.preparePlaceholder(&renderer->uiBuffer);
		}

		hasAddedWidgets = true;
	}
}

Rect2I UIPanel::getCurrentLayoutPosition()
{
	Rect2I result = {};

	result.x = contentArea.x + currentLeft;
	result.w = currentRight - currentLeft;

	if (topToBottom)
	{
		result.y = contentArea.y + currentTop;
		
		if (vScrollbar != null)
		{
			result.y -= vScrollbar->scrollPosition;
		}
	}
	else
	{
		result.y = contentArea.y + currentBottom;

		if (vScrollbar != null)
		{
			result.y += (vScrollbar->contentSize - vScrollbar->scrollPosition - bounds.h);
		}
	}

	// Adjust if we're in a scrolling area
	if (hScrollbar != null)
	{
		result.x = result.x - hScrollbar->scrollPosition;
	}

	ASSERT(result.w > 0);

	return result;
}

void UIPanel::completeWidget(V2I widgetSize)
{
	DEBUG_FUNCTION();
	
	bool lineIsFull = false;

	switch (widgetAlignment & ALIGN_H)
	{
		case ALIGN_LEFT: {
			currentLeft += widgetSize.x + style->contentPadding;
			// Check for a full line
			// NB: We might want to do something smarter when there's only a small remainder.
			// Though, for now we'll just be smart about not intentionally wrapping a line.
			if (currentLeft >= currentRight)
			{
				lineIsFull = true;
			}
		} break;

		case ALIGN_RIGHT: {
			currentRight -= widgetSize.x + style->contentPadding;
			// Check for a full line
			// NB: We might want to do something smarter when there's only a small remainder.
			// Though, for now we'll just be smart about not intentionally wrapping a line.
			if (currentLeft >= currentRight)
			{
				lineIsFull = true;
			}
		} break;

		case ALIGN_H_CENTRE:
		case ALIGN_EXPAND_H:
		default: {
			// Just start a new line
			lineIsFull = true;
		} break;
	}

	largestItemWidth        = max(largestItemWidth,        widgetSize.x);
	largestItemHeightOnLine = max(largestItemHeightOnLine, widgetSize.y);

	if (lineIsFull)
	{
		// New line with the same alignment
		startNewLine();
	}
}

inline UIPanelStyle *UIPanel::getPanelStyle(String styleName)
{
	UIPanelStyle *result = null;
	if (!isEmpty(styleName)) result = findPanelStyle(assets->theme, styleName);
	if (result == null)      result = this->style;

	return result;
}

void UIPanel::updateLayoutPosition()
{
	currentLeft = max(0, currentLeft);
	currentRight = min(contentArea.w, currentRight);

	currentTop = max(0, currentTop);
	currentBottom = min(contentArea.h, currentBottom);
}
