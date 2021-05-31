#pragma once

UIPanel::UIPanel(Rect2I bounds, UIPanelStyle *panelStyle, u32 flags, RenderBuffer *renderBuffer)
{
	DEBUG_FUNCTION_T(DCDT_UI);

	if (panelStyle == null)
	{
		this->style = findStyle<UIPanelStyle>("default"_s);
	}
	else
	{
		this->style = panelStyle;
	}

	this->flags = flags;

	this->hasAddedWidgets = false;
	this->allowClickThrough = (flags & Panel_AllowClickThrough) != 0;

	this->hideWidgets = (flags & Panel_HideWidgets) != 0;
	this->renderBuffer = renderBuffer;
	
	this->bounds = bounds;
	this->contentArea = shrink(bounds, this->style->margin);
	this->layoutBottomToTop = (flags & Panel_LayoutBottomToTop) != 0;

	// Default to left-aligned
	u32 hAlignment = this->style->widgetAlignment & ALIGN_H;
	if (hAlignment == 0) hAlignment = ALIGN_LEFT;
	this->widgetAlignment = hAlignment | (layoutBottomToTop ? ALIGN_BOTTOM : ALIGN_TOP);

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

bool UIPanel::addTextButton(String text, ButtonState state, String styleName)
{
	DEBUG_FUNCTION_T(DCDT_UI);
	
	prepareForWidgets();

	UIButtonStyle *buttonStyle = findStyle<UIButtonStyle>(styleName, &this->style->buttonStyle);

	Rect2I space = getCurrentLayoutPosition();
	V2I buttonOrigin = alignWithinRectangle(space, widgetAlignment);

	bool fillWidth = ((widgetAlignment & ALIGN_H) == ALIGN_EXPAND_H);
	V2I buttonSize = UI::calculateButtonSize(text, buttonStyle, space.w, fillWidth);
	Rect2I buttonBounds = irectAligned(buttonOrigin, buttonSize, widgetAlignment);

	bool wasClicked = false;

	if (!hideWidgets)
	{
		wasClicked = UI::putTextButton(text, buttonBounds, buttonStyle, state, renderBuffer);
	}

	completeWidget(buttonBounds.size);

	return wasClicked;
}

bool UIPanel::addImageButton(Sprite *sprite, ButtonState state, String styleName)
{
	DEBUG_FUNCTION_T(DCDT_UI);
	
	prepareForWidgets();

	UIButtonStyle *buttonStyle = findStyle<UIButtonStyle>(styleName, &this->style->buttonStyle);

	Rect2I space = getCurrentLayoutPosition();
	V2I buttonOrigin = alignWithinRectangle(space, widgetAlignment);

	bool fillWidth = ((widgetAlignment & ALIGN_H) == ALIGN_EXPAND_H);
	V2I spriteSize = v2i(sprite->pixelWidth, sprite->pixelHeight);
	V2I buttonSize = UI::calculateButtonSize(spriteSize, buttonStyle, space.w, fillWidth);
	Rect2I buttonBounds = irectAligned(buttonOrigin, buttonSize, widgetAlignment);

	bool wasClicked = false;

	if (!hideWidgets)
	{
		wasClicked = UI::putImageButton(sprite, buttonBounds, buttonStyle, state, renderBuffer);
	}

	completeWidget(buttonBounds.size);

	return wasClicked;
}

void UIPanel::addCheckbox(bool *checked, String styleName)
{
	DEBUG_FUNCTION_T(DCDT_UI);

	prepareForWidgets();

	UICheckboxStyle *checkboxStyle = findStyle<UICheckboxStyle>(styleName, &style->checkboxStyle);

	Rect2I space = getCurrentLayoutPosition();
	V2I checkboxOrigin = alignWithinRectangle(space, widgetAlignment);
	V2I checkboxSize = UI::calculateCheckboxSize(checkboxStyle);
	Rect2I checkboxBounds = irectAligned(checkboxOrigin, checkboxSize, widgetAlignment);

	if (!hideWidgets)
	{
		UI::putCheckbox(checked, checkboxBounds, checkboxStyle, false, renderBuffer);
	}

	completeWidget(checkboxSize);
}

template <typename T>
void UIPanel::addDropDownList(Array<T> *listOptions, s32 *currentSelection, String (*getDisplayName)(T *data), String styleName)
{
	DEBUG_FUNCTION_T(DCDT_UI);

	prepareForWidgets();

	UIDropDownListStyle *widgetStyle = findStyle<UIDropDownListStyle>(styleName, &style->dropDownListStyle);

	Rect2I space = getCurrentLayoutPosition();
	V2I widgetOrigin = alignWithinRectangle(space, widgetAlignment);

	bool fillWidth = ((widgetAlignment & ALIGN_H) == ALIGN_EXPAND_H);
	V2I widgetSize = UI::calculateDropDownListSize(listOptions, getDisplayName, widgetStyle, space.w, fillWidth);
	Rect2I widgetBounds = irectAligned(widgetOrigin, widgetSize, widgetAlignment);

	if (!hideWidgets)
	{
		UI::putDropDownList(listOptions, currentSelection, getDisplayName, widgetBounds, widgetStyle, renderBuffer);
	}

	completeWidget(widgetSize);
}

void UIPanel::addSlider(f32 *currentValue, f32 minValue, f32 maxValue, String styleName)
{
	DEBUG_FUNCTION_T(DCDT_UI);

	prepareForWidgets();

	UISliderStyle *widgetStyle = findStyle<UISliderStyle>(styleName, &style->sliderStyle);

	Rect2I space = getCurrentLayoutPosition();
	V2I widgetOrigin = alignWithinRectangle(space, widgetAlignment);

	bool fillWidth = ((widgetAlignment & ALIGN_H) == ALIGN_EXPAND_H);
	V2I widgetSize = UI::calculateSliderSize(widgetStyle, space.w, fillWidth);
	Rect2I widgetBounds = irectAligned(widgetOrigin, widgetSize, widgetAlignment);

	if (!hideWidgets)
	{
		UI::putSlider(currentValue, minValue, maxValue, widgetBounds, widgetStyle, false, renderBuffer);
	}

	completeWidget(widgetSize);
}

void UIPanel::addSprite(Sprite *sprite, s32 width, s32 height)
{
	DEBUG_FUNCTION_T(DCDT_UI);

	prepareForWidgets();

	Rect2I layoutPosition = getCurrentLayoutPosition();
	V2I origin = alignWithinRectangle(layoutPosition, widgetAlignment);
	V2I size = v2i(width, height);

	if (sprite != null)
	{
		if (size.x == -1) size.x = sprite->pixelWidth;
		if (size.y == -1) size.y = sprite->pixelHeight;

		if (!hideWidgets)
		{
			Rect2I spriteBounds = irectAligned(origin, size, widgetAlignment);
			drawSingleSprite(renderBuffer, sprite, rect2(spriteBounds), renderer->shaderIds.pixelArt, makeWhite());
		}
	}

	completeWidget(size);
}

void UIPanel::addText(String text, String styleName)
{
	DEBUG_FUNCTION_T(DCDT_UI);
	
	prepareForWidgets();

	UILabelStyle *labelStyle = findStyle<UILabelStyle>(styleName, &this->style->labelStyle);

	Rect2I space = getCurrentLayoutPosition();
	V2I origin = alignWithinRectangle(space, this->widgetAlignment);

	BitmapFont *font = getFont(&labelStyle->font);
	if (font)
	{
		V2I textSize = calculateTextSize(font, text, space.w);
		V2I topLeft = calculateTextPosition(origin, textSize, this->widgetAlignment);
		Rect2I textBounds = irectPosSize(topLeft, textSize);

		if (!hideWidgets)
		{
			drawText(renderBuffer, font, text, textBounds, this->widgetAlignment, labelStyle->textColor, renderer->shaderIds.text);
		}

		completeWidget(textSize);
	}
}

bool UIPanel::addTextInput(TextInput *textInput, String styleName)
{
	DEBUG_FUNCTION_T(DCDT_UI);
	
	prepareForWidgets();

	UITextInputStyle *textInputStyle = findStyle<UITextInputStyle>(styleName, &this->style->textInputStyle);

	Rect2I space = getCurrentLayoutPosition();
	V2I origin = alignWithinRectangle(space, this->widgetAlignment);

	bool fillWidth = ((this->widgetAlignment & ALIGN_H) == ALIGN_EXPAND_H);
	V2I textInputSize = UI::calculateTextInputSize(textInput, textInputStyle, space.w, fillWidth);
	Rect2I textInputBounds = irectAligned(origin, textInputSize, this->widgetAlignment);

	bool result = false;
	
	if (!hideWidgets)
	{
		result = UI::putTextInput(textInput, textInputBounds, textInputStyle, renderBuffer);
	}
	
	completeWidget(textInputSize);

	return result;
}

Rect2I UIPanel::addBlank(s32 width, s32 height)
{
	DEBUG_FUNCTION_T(DCDT_UI);

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
	DEBUG_FUNCTION_T(DCDT_UI);

	s32 lineWidth = (widgetAlignment & ALIGN_RIGHT) ? currentRight : currentLeft;
	largestLineWidth = max(largestLineWidth, lineWidth);

	currentLeft = 0;
	currentRight = contentArea.w;

	if (largestItemHeightOnLine > 0)
	{
		if (layoutBottomToTop)
		{
			currentBottom -= largestItemHeightOnLine + style->contentPadding;
		}
		else
		{
			currentTop += largestItemHeightOnLine + style->contentPadding;
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
	DEBUG_FUNCTION_T(DCDT_UI);
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

		if (layoutBottomToTop)
		{
			contentArea.h -= height + style->contentPadding;
			contentArea.y += height + style->contentPadding;
		}
		else
		{
			completeWidget(rowBounds.size);
		}

		updateLayoutPosition();

		return UIPanel(rowBounds, rowStyle, flags, renderBuffer);
	}
	else
	{
		Rect2I rowBounds = irectXYWH(
			contentArea.x, contentArea.y + currentBottom - height,
			contentArea.w, height
		);

		if (layoutBottomToTop)
		{
			completeWidget(rowBounds.size);
		}
		else
		{
			contentArea.h -= height + style->contentPadding;
		}
		
		updateLayoutPosition();

		return UIPanel(rowBounds, rowStyle, flags, renderBuffer);
	}
}

UIPanel UIPanel::column(s32 width, Alignment hAlignment, String styleName)
{
	DEBUG_FUNCTION_T(DCDT_UI);
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

		return UIPanel(columnBounds, columnStyle, flags, renderBuffer);
	}
	else
	{
		Rect2I columnBounds = irectXYWH(
			contentArea.x + contentArea.w - width, contentArea.y + currentTop,
			width, contentArea.h
		);

		contentArea.w -= width + style->contentPadding;
		updateLayoutPosition();

		return UIPanel(columnBounds, columnStyle, flags, renderBuffer);
	}
}

void UIPanel::end(bool shrinkToContentHeight, bool shrinkToContentWidth)
{
	DEBUG_FUNCTION_T(DCDT_UI);

	// Make sure the current line's height is taken into account
	startNewLine();

	// @Hack! I don't at all understand why we get a trailing space of 2x the contentPadding at the end.
	if (layoutBottomToTop && hasAddedWidgets)
	{
		currentBottom += (style->contentPadding * 2);
	}

	s32 contentHeight = (layoutBottomToTop ? (contentArea.h - currentBottom)
									 : (currentTop + style->margin));

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
		if (layoutBottomToTop)
		{
			contentHeight = (contentArea.h - currentBottom) + style->contentPadding + (2 *style->margin);
			s32 newHeight = clamp(contentHeight, (2 *style->margin), bounds.h);
			bounds.y += (bounds.h - newHeight);
			bounds.h = newHeight;
			boundsChanged = true;
		}
		else
		{
			contentHeight = currentTop - style->contentPadding + (2 *style->margin);
			bounds.h = clamp(contentHeight, (2 *style->margin), bounds.h);
			boundsChanged = true;
		}

		// Make sure there is space for the scrollbar if we have one
		if (hScrollbar)
		{
			bounds.h += hScrollbarBounds.h;
			if (layoutBottomToTop) bounds.y -= hScrollbarBounds.h;
			boundsChanged = true;
		}
	}

	if (boundsChanged)
	{
		hScrollbarBounds.w = bounds.w;
		hScrollbarBounds.x = bounds.x;
		// In case the height has shrunk, move the scrollbar to the bottom edge
		hScrollbarBounds.y = bounds.y + bounds.h - hScrollbarBounds.h;

		vScrollbarBounds.h = bounds.h;
		vScrollbarBounds.y = bounds.y;
	}

	if (!hasAddedWidgets)
	{
		if (!hideWidgets)
		{
			addBeginScissor(renderBuffer, bounds);

			// Prepare to render background
			background.preparePlaceholder(renderBuffer);
		}
	}

	// Handle scrollbars
	if (hScrollbar)
	{
		UIScrollbarStyle *scrollbarStyle = findStyle<UIScrollbarStyle>(&style->scrollbarStyle);

		if (!hideWidgets)
		{
			updateScrollbar(hScrollbar, largestLineWidth + (style->margin * 2), hScrollbarBounds, scrollbarStyle);

			drawScrollbar(renderBuffer, hScrollbar, hScrollbarBounds, scrollbarStyle);
		}
	}

	if (vScrollbar)
	{
		UIScrollbarStyle *scrollbarStyle = findStyle<UIScrollbarStyle>(&style->scrollbarStyle);

		if (!hideWidgets)
		{
			updateScrollbar(vScrollbar, contentHeight, vScrollbarBounds, scrollbarStyle);

			drawScrollbar(renderBuffer, vScrollbar, vScrollbarBounds, scrollbarStyle);
		}
	}

	if (!hideWidgets)
	{
		// Fill in the background
		background.fillPlaceholder(bounds);

		// Clear any scissor stuff
		addEndScissor(renderBuffer);

		UI::popInputScissorRect();
	}

	// Add a UI rect if we're top level. Otherwise, our parent already added one that encompasses us!
	if (!allowClickThrough)
	{
		UI::addUIRect(bounds);
	}
}

void UIPanel::prepareForWidgets()
{
	if (!hasAddedWidgets)
	{
		if (!hideWidgets)
		{
			addBeginScissor(renderBuffer, bounds);

			UI::pushInputScissorRect(bounds);

			// Prepare to render background
			background.preparePlaceholder(renderBuffer);
		}

		hasAddedWidgets = true;
	}
}

Rect2I UIPanel::getCurrentLayoutPosition()
{
	Rect2I result = {};

	result.x = contentArea.x + currentLeft;
	result.w = currentRight - currentLeft;

	if (layoutBottomToTop)
	{
		result.y = contentArea.y + currentBottom;

		if (vScrollbar != null)
		{
			result.y += (vScrollbar->contentSize - getScrollbarContentOffset(vScrollbar, bounds.h) - bounds.h);
		}
	}
	else
	{
		result.y = contentArea.y + currentTop;
		
		if (vScrollbar != null)
		{
			result.y -= getScrollbarContentOffset(vScrollbar, bounds.h);
		}
	}

	// Adjust if we're in a scrolling area
	if (hScrollbar != null)
	{
		result.w = s16Max; // Not s32 because then we'd have overflow issues. s16 should be plenty large enough.
		result.x = result.x - getScrollbarContentOffset(hScrollbar, bounds.w);
	}

	ASSERT(result.w > 0);

	return result;
}

void UIPanel::completeWidget(V2I widgetSize)
{
	DEBUG_FUNCTION_T(DCDT_UI);
	
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
	if (!isEmpty(styleName)) result = findStyle<UIPanelStyle>(styleName);
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
