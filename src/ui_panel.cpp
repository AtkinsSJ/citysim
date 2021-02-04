#pragma once

UIPanel::UIPanel(Rect2I bounds, UIPanelStyle *panelStyle)
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
	
	this->bounds = bounds;
	this->contentArea = shrink(bounds, this->style->margin);

	// TODO: make this a parameter to pass in?
	// TODO: maybe make it a style property too?
	this->widgetAlignment = ALIGN_TOP | ALIGN_LEFT;

	// Relative to contentArea
	this->currentLeft= 0;
	this->currentRight = this->contentArea.w;
	this->currentY = 0;

	this->largestItemWidth = 0;
	this->largestItemHeightOnLine = 0;

	// Prepare to render background
	// TODO: Handle backgrounds that aren't a solid colour
	this->backgroundPlaceholder = appendDrawRectPlaceholder(&renderer->uiBuffer, renderer->shaderIds.untextured);

	// TODO: Scissor?
	// TODO: UI Rect?
}

void UIPanel::addText(String text, String styleName)
{
	DEBUG_FUNCTION();

	UILabelStyle *labelStyle = null;
	if (!isEmpty(styleName))  labelStyle = findLabelStyle(&assets->theme, styleName);
	if (labelStyle == null)        labelStyle = findStyle<UILabelStyle>(&assets->theme, &this->style->labelStyle);

	Rect2I space = getCurrentLayoutPosition();
	V2I origin = alignWithinRectangle(space, this->widgetAlignment);

	BitmapFont *font = getFont(&labelStyle->font);
	if (font)
	{
		V2I textSize = calculateTextSize(font, text, space.w);
		V2I topLeft = calculateTextPosition(origin, textSize, this->widgetAlignment);
		Rect2I textBounds = irectPosSize(topLeft, textSize);

		drawText(&renderer->uiBuffer, font, text, textBounds, this->widgetAlignment, labelStyle->textColor, renderer->shaderIds.text);

		completeWidget(textSize);
	}
}

bool UIPanel::addButton(String text, s32 textWidth, ButtonState state, String styleName)
{
	DEBUG_FUNCTION();

	bool buttonClicked = false;
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
		s32 buttonWidth;
		if (textWidth == -1)
		{
			bool fillWidth = ((buttonAlignment & ALIGN_H) == ALIGN_EXPAND_H);
			buttonWidth = calculateButtonSize(text, buttonStyle, space.w, fillWidth).x;
		}
		else
		{
			buttonWidth = textWidth + (buttonStyle->padding * 2);
		}

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
				buttonClicked = true;
				uiState->mouseInputHandled = true;
			}
		}

		completeWidget(buttonBounds.size);
	}

	return buttonClicked;
}

bool UIPanel::addTextInput(TextInput *textInput, String styleName)
{
	DEBUG_FUNCTION();

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

	this->currentLeft = 0;
	this->currentRight = this->contentArea.w;
	if (this->largestItemHeightOnLine > 0)
	{
		this->currentY += this->largestItemHeightOnLine + this->style->contentPadding;
	}

	this->largestItemHeightOnLine = 0;

	// Only change alignment if we passed one
	if (hAlignment != 0)
	{
		this->widgetAlignment = (this->widgetAlignment & ALIGN_V) | (hAlignment & ALIGN_H);
	}
}

UIPanel UIPanel::row(s32 height, Alignment vAlignment, String styleName)
{
	DEBUG_FUNCTION();
	ASSERT(vAlignment == ALIGN_TOP || vAlignment == ALIGN_BOTTOM);

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

		return UIPanel(rowBounds, rowStyle);
	}
	else
	{
		Rect2I rowBounds = irectXYWH(
			contentArea.x, contentArea.y + contentArea.h - height,
			contentArea.w, height
		);

		contentArea.h -= height + style->contentPadding;

		return UIPanel(rowBounds, rowStyle);
	}
}

UIPanel UIPanel::column(s32 width, Alignment hAlignment, String styleName)
{
	DEBUG_FUNCTION();
	ASSERT(hAlignment == ALIGN_LEFT || hAlignment == ALIGN_RIGHT);

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

		return UIPanel(columnBounds, columnStyle);
	}
	else
	{
		Rect2I columnBounds = irectXYWH(
			contentArea.x + contentArea.w - width, contentArea.y + currentY,
			width, contentArea.h
		);

		contentArea.w -= width + style->contentPadding;
		startNewLine();

		return UIPanel(columnBounds, columnStyle);
	}
}

void UIPanel::end()
{
	DEBUG_FUNCTION();

	// Fill in the background
	fillDrawRectPlaceholder(this->backgroundPlaceholder, this->bounds, this->style->backgroundColor);

	// Draw the scrollbar if we have one


	// Clear any scissor stuff
}

Rect2I UIPanel::getCurrentLayoutPosition()
{
	Rect2I result = {};

	result.x = this->contentArea.x + this->currentLeft;
	result.y = this->contentArea.y + this->currentY;
	result.w = this->currentRight - this->currentLeft;

	// Adjust if we're in a scrolling column area
	// if (this->columnScrollbarState != null)
	// {
	// 	result.y = result.y - this->columnScrollbarState->scrollPosition;
	// }

	ASSERT(result.w > 0);

	return result;
}

void UIPanel::completeWidget(V2I widgetSize)
{
	DEBUG_FUNCTION();
	
	bool lineIsFull = false;

	switch (this->widgetAlignment & ALIGN_H)
	{
		case ALIGN_LEFT: {
			this->currentLeft += widgetSize.x + this->style->contentPadding;
			// Check for a full line
			// NB: We might want to do something smarter when there's only a small remainder.
			// Though, for now we'll just be smart about not intentionally wrapping a line.
			if (this->currentLeft >= this->currentRight)
			{
				lineIsFull = true;
			}
		} break;

		case ALIGN_RIGHT: {
			this->currentRight -= widgetSize.x + this->style->contentPadding;
			// Check for a full line
			// NB: We might want to do something smarter when there's only a small remainder.
			// Though, for now we'll just be smart about not intentionally wrapping a line.
			if (this->currentLeft >= this->currentRight)
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

	this->largestItemWidth        = max(this->largestItemWidth,        widgetSize.x);
	this->largestItemHeightOnLine = max(this->largestItemHeightOnLine, widgetSize.y);

	if (lineIsFull)
	{
		// New line with the same alignment
		startNewLine();
	}
}