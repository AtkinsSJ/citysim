#pragma once

UIPanel::UIPanel(Rect2I bounds, UIPanelStyle *panelStyle, bool thisIsTopLevel)
{
	DEBUG_FUNCTION();

	if (panelStyle == null)
	{
		this->style = findPanelStyle(&assets->theme, "default"_s);
	}
	else
	{
		this->style = panelStyle;
	}

	this->isTopLevel = thisIsTopLevel;
	this->hasAddedWidgets = false;
	
	this->bounds = bounds;
	this->contentArea = shrink(bounds, this->style->margin);

	// Default to top-left alignment if they're not specified
	this->widgetAlignment = this->style->widgetAlignment;
	if ((this->widgetAlignment & ALIGN_H) == 0)
	{
		this->widgetAlignment |= ALIGN_LEFT;
	}
	if ((this->widgetAlignment & ALIGN_V) == 0)
	{
		this->widgetAlignment |= ALIGN_TOP;
	}

	this->hScrollbar = null;
	this->vScrollbar = null;

	// Relative to contentArea
	this->currentLeft= 0;
	this->currentRight = this->contentArea.w;
	this->currentY = 0;

	this->largestItemWidth = 0;
	this->largestItemHeightOnLine = 0;
}

void UIPanel::enableHorizontalScrolling(ScrollbarState *scrollbarState)
{
	UIScrollbarStyle *scrollbarStyle = findStyle<UIScrollbarStyle>(&assets->theme, &style->scrollbarStyle);
	ASSERT(scrollbarStyle != null);

	this->hScrollbar = scrollbarState;
	this->hScrollbarBounds = irectXYWH(bounds.x, bounds.y + bounds.h - scrollbarStyle->width, bounds.w, scrollbarStyle->width);

	this->contentArea.h -= scrollbarStyle->width;
}

void UIPanel::enableVerticalScrolling(ScrollbarState *scrollbarState, bool expandWidth)
{
	UIScrollbarStyle *scrollbarStyle = findStyle<UIScrollbarStyle>(&assets->theme, &style->scrollbarStyle);
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
	}
}

void UIPanel::addText(String text, String styleName)
{
	DEBUG_FUNCTION();
	
	prepareForWidgets();

	UILabelStyle *labelStyle = null;
	if (!isEmpty(styleName))  labelStyle = findLabelStyle(&assets->theme, styleName);
	if (labelStyle == null)   labelStyle = findStyle<UILabelStyle>(&assets->theme, &this->style->labelStyle);

	Rect2I space = getCurrentLayoutPosition();
	V2I origin = alignWithinRectangle(space, this->widgetAlignment);

	BitmapFont *font = getFont(&labelStyle->font);
	if (font)
	{
		V2I textSize = calculateTextSize(font, text, space.w);
		V2I topLeft = calculateTextPosition(origin, textSize, this->widgetAlignment);
		Rect2I textBounds = irectPosSize(topLeft, textSize);

		// if (context->doRender)
		{
			drawText(&renderer->uiBuffer, font, text, textBounds, this->widgetAlignment, labelStyle->textColor, renderer->shaderIds.text);
		}

		completeWidget(textSize);
	}
}

bool UIPanel::addButton(String text, ButtonState state, String styleName)
{
	DEBUG_FUNCTION();
	
	prepareForWidgets();

	bool buttonWasClicked = false;
	UIButtonStyle *buttonStyle = null;
	if (!isEmpty(styleName))  buttonStyle = findButtonStyle(&assets->theme, styleName);
	if (buttonStyle == null)  buttonStyle = findStyle<UIButtonStyle>(&assets->theme, &this->style->buttonStyle);

	u32 textAlignment = buttonStyle->textAlignment;
	s32 buttonPadding = buttonStyle->padding;
	V2I mousePos = v2i(renderer->uiCamera.mousePos);

	s32 buttonAlignment = this->widgetAlignment;
	Rect2I space = getCurrentLayoutPosition();
	V2I buttonOrigin = alignWithinRectangle(space, buttonAlignment);

	BitmapFont *font = getFont(&buttonStyle->font);
	if (font)
	{
		bool fillWidth = ((buttonAlignment & ALIGN_H) == ALIGN_EXPAND_H);
		s32 buttonWidth = calculateButtonSize(text, buttonStyle, space.w, fillWidth).x;

		V2I textSize = calculateTextSize(font, text, buttonWidth - (buttonStyle->padding * 2));
		Rect2I buttonBounds = irectAligned(buttonOrigin, v2i(buttonWidth, textSize.y + (buttonPadding * 2)), buttonAlignment);

		// if (this->doRender)
		{
			V4 backColor = buttonStyle->backgroundColor;
			RenderItem_DrawSingleRect *background = appendDrawRectPlaceholder(&renderer->uiBuffer, renderer->shaderIds.untextured);

			V2I textOrigin = alignWithinRectangle(buttonBounds, textAlignment, buttonPadding);
			V2I textTopLeft = calculateTextPosition(textOrigin, textSize, textAlignment);
			Rect2I textBounds = irectPosSize(textTopLeft, textSize);
			drawText(&renderer->uiBuffer, font, text, textBounds, textAlignment, buttonStyle->textColor, renderer->shaderIds.text);

			if (state == Button_Disabled)
			{
				backColor = buttonStyle->disabledBackgroundColor;
			}
			else if (contains(buttonBounds, mousePos))
			{
				// Mouse pressed: must have started and currently be inside the bounds to show anything
				// Mouse unpressed: show hover if in bounds
				if (mouseButtonPressed(MouseButton_Left))
				{
					if (contains(buttonBounds, getClickStartPos(MouseButton_Left, &renderer->uiCamera)))
					{
						backColor = buttonStyle->pressedBackgroundColor;
					}
				}
				else
				{
					backColor = buttonStyle->hoverBackgroundColor;
				}
			}
			else if (state == Button_Active)
			{
				backColor = buttonStyle->hoverBackgroundColor;
			}

			fillDrawRectPlaceholder(background, buttonBounds, backColor);
		}

		// if (this->doUpdate)
		{
			UIState *uiState = globalAppState.uiState;
			if ((state != Button_Disabled)
			 && justClickedOnUI(uiState, buttonBounds))
			{
				buttonWasClicked = true;
				uiState->mouseInputHandled = true;
			}
		}

		completeWidget(buttonBounds.size);
	}

	return buttonWasClicked;
}

bool UIPanel::addTextInput(TextInput *textInput, String styleName)
{
	DEBUG_FUNCTION();
	
	prepareForWidgets();

	bool result = false;
	// if (context->doUpdate)
	{
		result = updateTextInput(textInput);
	}

	UITextInputStyle *textInputStyle = null;
	if (!isEmpty(styleName))  textInputStyle = findTextInputStyle(&assets->theme, styleName);
	if (textInputStyle == null)        textInputStyle = findStyle<UITextInputStyle>(&assets->theme, &this->style->textInputStyle);

	s32 alignment = this->widgetAlignment;
	Rect2I space = getCurrentLayoutPosition();
	V2I origin = alignWithinRectangle(space, alignment);

	BitmapFont *font = getFont(&textInputStyle->font);
	if (font)
	{
		bool fillWidth = ((alignment & ALIGN_H) == ALIGN_EXPAND_H);
		V2I textInputSize = calculateTextInputSize(textInput, textInputStyle, space.w, fillWidth);
		Rect2I textInputBounds = irectAligned(origin, textInputSize, alignment);

		// if (context->doRender)
		{
			drawTextInput(&renderer->uiBuffer, textInput, textInputStyle, textInputBounds);
		}

		// if (context->doUpdate)
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

void UIPanel::alignWidgets(u32 alignment)
{
	widgetAlignment = (widgetAlignment & ALIGN_V) | (alignment & ALIGN_H);
}

void UIPanel::startNewLine(u32 hAlignment)
{
	DEBUG_FUNCTION();

	currentLeft = 0;
	currentRight = contentArea.w;
	if (largestItemHeightOnLine > 0)
	{
		currentY += largestItemHeightOnLine + style->contentPadding;
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

	UIPanelStyle *rowStyle = null;
	if (!isEmpty(styleName)) rowStyle = findPanelStyle(&assets->theme, styleName);
	if (rowStyle == null)    rowStyle = this->style;

	if (vAlignment == ALIGN_TOP)
	{
		Rect2I rowBounds = irectXYWH(
			contentArea.x, contentArea.y + currentY,
			contentArea.w, height
		);

		completeWidget(rowBounds.size);

		return UIPanel(rowBounds, rowStyle, false);
	}
	else
	{
		Rect2I rowBounds = irectXYWH(
			contentArea.x, contentArea.y + contentArea.h - height,
			contentArea.w, height
		);

		contentArea.h -= height + style->contentPadding;

		return UIPanel(rowBounds, rowStyle, false);
	}
}

UIPanel UIPanel::column(s32 width, Alignment hAlignment, String styleName)
{
	DEBUG_FUNCTION();
	ASSERT(hAlignment == ALIGN_LEFT || hAlignment == ALIGN_RIGHT);
	
	prepareForWidgets();

	startNewLine();

	UIPanelStyle *columnStyle = null;
	if (!isEmpty(styleName)) columnStyle = findPanelStyle(&assets->theme, styleName);
	if (columnStyle == null) columnStyle = this->style;

	if (hAlignment == ALIGN_LEFT)
	{
		Rect2I columnBounds = irectXYWH(
			contentArea.x, contentArea.y + currentY,
			width, contentArea.h
		);

		contentArea.w -= width + style->contentPadding;
		contentArea.x += width + style->contentPadding;
		startNewLine();

		return UIPanel(columnBounds, columnStyle, false);
	}
	else
	{
		Rect2I columnBounds = irectXYWH(
			contentArea.x + contentArea.w - width, contentArea.y + currentY,
			width, contentArea.h
		);

		contentArea.w -= width + style->contentPadding;
		startNewLine();

		return UIPanel(columnBounds, columnStyle, false);
	}
}

void UIPanel::shrinkToContent()
{
	s32 contentHeight = currentY - style->contentPadding + (2 *style->margin);
	bounds.h = clamp(contentHeight, (2 *style->margin), bounds.h);

	hScrollbarBounds.w = bounds.w;
	vScrollbarBounds.h = bounds.h;
}

void UIPanel::end()
{
	DEBUG_FUNCTION();
	UIState *uiState = globalAppState.uiState;

	if (!hasAddedWidgets)
	{
		// if (context->doRender)
		{
			addBeginScissor(&renderer->uiBuffer, rect2(bounds));

			// Prepare to render background
			// TODO: Handle backgrounds that aren't a solid colour
			this->backgroundPlaceholder = appendDrawRectPlaceholder(&renderer->uiBuffer, renderer->shaderIds.untextured);
		}
	}

	// Handle scrollbars
	if (hScrollbar)
	{
		UIScrollbarStyle *scrollbarStyle = findStyle<UIScrollbarStyle>(&assets->theme, &style->scrollbarStyle);

		// if (context->doUpdate)
		{
			updateScrollbar(uiState, hScrollbar, currentLeft + style->margin, hScrollbarBounds, scrollbarStyle);
		}

		f32 scrollPercent = getScrollbarPercent(hScrollbar, hScrollbarBounds.h);

		// if (context->doRender)
		{
			drawScrollbar(&renderer->uiBuffer, scrollPercent, hScrollbarBounds.pos, hScrollbarBounds.h, v2i(scrollbarStyle->width, scrollbarStyle->width), scrollbarStyle->knobColor, scrollbarStyle->backgroundColor, renderer->shaderIds.untextured);
		}
	}

	if (vScrollbar)
	{
		UIScrollbarStyle *scrollbarStyle = findStyle<UIScrollbarStyle>(&assets->theme, &style->scrollbarStyle);

		// if (context->doUpdate)
		{
			updateScrollbar(uiState, vScrollbar, currentY + style->margin, vScrollbarBounds, scrollbarStyle);
		}

		f32 scrollPercent = getScrollbarPercent(vScrollbar, vScrollbarBounds.h);

		// if (context->doRender)
		{
			drawScrollbar(&renderer->uiBuffer, scrollPercent, vScrollbarBounds.pos, vScrollbarBounds.h, v2i(scrollbarStyle->width, scrollbarStyle->width), scrollbarStyle->knobColor, scrollbarStyle->backgroundColor, renderer->shaderIds.untextured);
		}
	}

	// if (context->doRender)
	{
		// Fill in the background
		fillDrawRectPlaceholder(backgroundPlaceholder, bounds, style->backgroundColor);

		// Clear any scissor stuff
		addEndScissor(&renderer->uiBuffer);
	}

	// Add a UI rect if we're top level. Otherwise, our parent already added one that encompasses us!
	if (isTopLevel)
	{
		addUIRect(uiState, bounds);
	}
}

void UIPanel::prepareForWidgets()
{
	if (!hasAddedWidgets)
	{
		// if (context->doRender)
		{
			addBeginScissor(&renderer->uiBuffer, rect2(bounds));

			// Prepare to render background
			// TODO: Handle backgrounds that aren't a solid colour
			this->backgroundPlaceholder = appendDrawRectPlaceholder(&renderer->uiBuffer, renderer->shaderIds.untextured);
		}

		hasAddedWidgets = true;
	}
}

Rect2I UIPanel::getCurrentLayoutPosition()
{
	Rect2I result = {};

	result.x = contentArea.x + currentLeft;
	result.y = contentArea.y + currentY;
	result.w = currentRight - currentLeft;

	// Adjust if we're in a scrolling column area
	if (hScrollbar != null)
	{
		result.x = result.x - hScrollbar->scrollPosition;
	}

	if (vScrollbar != null)
	{
		result.y = result.y - vScrollbar->scrollPosition;
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