#pragma once

namespace UI {

Panel::Panel(Rect2I bounds, PanelStyle* panelStyle, u32 flags, RenderBuffer* renderBuffer)
{
    DEBUG_FUNCTION_T(DCDT_UI);

    if (panelStyle == null) {
        this->style = getStyle<PanelStyle>("default"_s);
    } else {
        this->style = panelStyle;
    }

    this->flags = flags;

    this->hasAddedWidgets = false;
    this->allowClickThrough = (flags & PanelFlags::AllowClickThrough) != 0;

    this->hideWidgets = (flags & PanelFlags::HideWidgets) != 0;
    this->renderBuffer = renderBuffer;

    this->bounds = bounds;
    this->contentArea = shrink(bounds, this->style->padding);
    this->layoutBottomToTop = (flags & PanelFlags::LayoutBottomToTop) != 0;

    // Default to left-aligned
    u32 hAlignment = this->style->widgetAlignment & ALIGN_H;
    if (hAlignment == 0)
        hAlignment = ALIGN_LEFT;
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

    this->background = Drawable(&this->style->background);
}

void Panel::enableHorizontalScrolling(ScrollbarState* scrollbarState)
{
    ScrollbarStyle* scrollbarStyle = getStyle<ScrollbarStyle>(&style->scrollbarStyle);
    ASSERT(scrollbarStyle != null);

    this->hScrollbar = scrollbarState;
    this->hScrollbarBounds = irectXYWH(bounds.x, bounds.y + bounds.h - scrollbarStyle->width, bounds.w, scrollbarStyle->width);

    this->contentArea.h -= scrollbarStyle->width;
    updateLayoutPosition();
}

void Panel::enableVerticalScrolling(ScrollbarState* scrollbarState, bool expandWidth)
{
    ScrollbarStyle* scrollbarStyle = getStyle<ScrollbarStyle>(&style->scrollbarStyle);
    ASSERT(scrollbarStyle != null);

    this->vScrollbar = scrollbarState;

    s32 scrollbarX = bounds.x + bounds.w - (expandWidth ? 0 : scrollbarStyle->width);
    this->vScrollbarBounds = irectXYWH(scrollbarX, bounds.y, scrollbarStyle->width, bounds.h);

    if (expandWidth) {
        this->bounds.w += scrollbarStyle->width;
    } else {
        this->contentArea.w -= scrollbarStyle->width;
        this->currentRight -= scrollbarStyle->width;
        updateLayoutPosition();
    }
}

bool Panel::addTextButton(String text, ButtonState state, String styleName)
{
    DEBUG_FUNCTION_T(DCDT_UI);

    prepareForWidgets();

    ButtonStyle* widgetStyle = getStyle<ButtonStyle>(styleName, &this->style->buttonStyle);

    Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I space, bool fillWidth) {
        return calculateButtonSize(text, widgetStyle, space.w, fillWidth);
    });

    bool wasClicked = false;

    if (!hideWidgets) {
        wasClicked = putTextButton(text, widgetBounds, widgetStyle, state, renderBuffer);
    }

    completeWidget(widgetBounds.size);

    return wasClicked;
}

bool Panel::addImageButton(Sprite* sprite, ButtonState state, String styleName)
{
    DEBUG_FUNCTION_T(DCDT_UI);

    prepareForWidgets();

    ButtonStyle* widgetStyle = getStyle<ButtonStyle>(styleName, &this->style->buttonStyle);
    V2I spriteSize = v2i(sprite->pixelWidth, sprite->pixelHeight);

    Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I space, bool fillWidth) {
        return calculateButtonSize(spriteSize, widgetStyle, space.w, fillWidth);
    });

    bool wasClicked = false;

    if (!hideWidgets) {
        wasClicked = putImageButton(sprite, widgetBounds, widgetStyle, state, renderBuffer);
    }

    completeWidget(widgetBounds.size);

    return wasClicked;
}

void Panel::addCheckbox(bool* checked, String styleName)
{
    DEBUG_FUNCTION_T(DCDT_UI);

    prepareForWidgets();

    CheckboxStyle* widgetStyle = getStyle<CheckboxStyle>(styleName, &style->checkboxStyle);

    Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I, bool) {
        return calculateCheckboxSize(widgetStyle);
    });

    if (!hideWidgets) {
        putCheckbox(checked, widgetBounds, widgetStyle, false, renderBuffer);
    }

    completeWidget(widgetBounds.size);
}

template<typename T>
void Panel::addDropDownList(Array<T>* listOptions, s32* currentSelection, String (*getDisplayName)(T* data), String styleName)
{
    DEBUG_FUNCTION_T(DCDT_UI);

    prepareForWidgets();

    DropDownListStyle* widgetStyle = getStyle<DropDownListStyle>(styleName, &style->dropDownListStyle);

    Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I space, bool fillWidth) {
        return calculateDropDownListSize(listOptions, getDisplayName, widgetStyle, space.w, fillWidth);
    });

    if (!hideWidgets) {
        putDropDownList(listOptions, currentSelection, getDisplayName, widgetBounds, widgetStyle, renderBuffer);
    }

    completeWidget(widgetBounds.size);
}

void Panel::addLabel(String text, String styleName)
{
    DEBUG_FUNCTION_T(DCDT_UI);

    prepareForWidgets();

    LabelStyle* widgetStyle = getStyle<LabelStyle>(styleName, &this->style->labelStyle);

    Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I space, bool fillWidth) {
        return calculateLabelSize(text, widgetStyle, space.w, fillWidth);
    });

    if (!hideWidgets) {
        putLabel(text, widgetBounds, widgetStyle, renderBuffer);
    }

    completeWidget(widgetBounds.size);
}

void Panel::addRadioButton(s32* currentValue, s32 myValue, String styleName)
{
    DEBUG_FUNCTION_T(DCDT_UI);

    prepareForWidgets();

    RadioButtonStyle* widgetStyle = getStyle<RadioButtonStyle>(styleName, &style->radioButtonStyle);

    Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I, bool) {
        return calculateRadioButtonSize(widgetStyle);
    });

    if (!hideWidgets) {
        putRadioButton(currentValue, myValue, widgetBounds, widgetStyle, false, renderBuffer);
    }

    completeWidget(widgetBounds.size);
}

template<typename T>
void Panel::addRadioButtonGroup(Array<T>* listOptions, s32* currentSelection, String (*getDisplayName)(T* data), String styleName, String labelStyleName)
{
    DEBUG_FUNCTION_T(DCDT_UI);

    prepareForWidgets();

    RadioButtonStyle* radioButtonStyle = getStyle<RadioButtonStyle>(styleName, &style->radioButtonStyle);
    LabelStyle* labelStyle = getStyle<LabelStyle>(labelStyleName, &style->labelStyle);

    V2I radioButtonSize = calculateRadioButtonSize(radioButtonStyle);

    Rect2I buttonGroupBounds = calculateWidgetBounds([&](Rect2I space, bool fillWidth) {
        s32 textWidth = space.w - (radioButtonSize.x + style->contentPadding);

        // Calculate the overall size
        // This means addRadioButtonGroup() has to loop through everything twice, but what can you do
        V2I widgetSize = v2i(0, 0);
        for (s32 optionIndex = 0; optionIndex < listOptions->count; optionIndex++) {
            String optionText = getDisplayName(listOptions->get(optionIndex));
            V2I labelSize = calculateLabelSize(optionText, labelStyle, textWidth, fillWidth);

            widgetSize.x = max(widgetSize.x, radioButtonSize.x + style->contentPadding + labelSize.x);

            if (optionIndex > 0)
                widgetSize.y += style->contentPadding;
            widgetSize.y += max(labelSize.y, radioButtonSize.y);
        }

        ASSERT(widgetSize.x <= space.w);
        return widgetSize;
    });

    Rect2I radioButtonBounds = alignWithinRectangle(buttonGroupBounds, radioButtonSize, ALIGN_TOP | ALIGN_LEFT);
    s32 textWidth = buttonGroupBounds.w - (radioButtonSize.x + style->contentPadding);
    bool fillWidth = ((widgetAlignment & ALIGN_H) == ALIGN_EXPAND_H);

    for (s32 optionIndex = 0; optionIndex < listOptions->count; optionIndex++) {
        if (!hideWidgets) {
            putRadioButton(currentSelection, optionIndex, radioButtonBounds, radioButtonStyle, false, renderBuffer);
        }

        String optionText = getDisplayName(listOptions->get(optionIndex));
        V2I labelSize = calculateLabelSize(optionText, labelStyle, textWidth, fillWidth);
        Rect2I labelBounds = irectXYWH(
            radioButtonBounds.x + radioButtonBounds.w + style->contentPadding,
            radioButtonBounds.y,
            labelSize.x,
            labelSize.y);

        if (!hideWidgets) {
            putLabel(optionText, labelBounds, labelStyle, renderBuffer);
        }

        radioButtonBounds.y = labelBounds.y + labelBounds.h + style->contentPadding;
    }

    completeWidget(buttonGroupBounds.size);
}

template<typename T>
void Panel::addSlider(T* currentValue, T minValue, T maxValue, String styleName)
{
    DEBUG_FUNCTION_T(DCDT_UI);

    prepareForWidgets();

    SliderStyle* widgetStyle = getStyle<SliderStyle>(styleName, &style->sliderStyle);

    Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I space, bool fillWidth) {
        return calculateSliderSize(Orientation::Horizontal, widgetStyle, space.size, fillWidth);
    });

    if (!hideWidgets) {
        putSlider(currentValue, minValue, maxValue, Orientation::Horizontal, widgetBounds, widgetStyle, false, renderBuffer);
    }

    completeWidget(widgetBounds.size);
}

void Panel::addSprite(Sprite* sprite, s32 width, s32 height)
{
    DEBUG_FUNCTION_T(DCDT_UI);

    prepareForWidgets();

    V2I size = v2i(width, height);
    if (sprite != null) {
        if (size.x == -1)
            size.x = sprite->pixelWidth;
        if (size.y == -1)
            size.y = sprite->pixelHeight;
    }

    Rect2I widgetBounds = calculateWidgetBounds(size);

    if (sprite != null) {
        if (!hideWidgets) {
            drawSingleSprite(renderBuffer, sprite, rect2(widgetBounds), renderer->shaderIds.pixelArt, makeWhite());
        }
    }

    completeWidget(widgetBounds.size);
}

bool Panel::addTextInput(TextInput* textInput, String styleName)
{
    DEBUG_FUNCTION_T(DCDT_UI);

    prepareForWidgets();

    TextInputStyle* widgetStyle = getStyle<TextInputStyle>(styleName, &this->style->textInputStyle);

    Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I space, bool fillWidth) {
        return calculateTextInputSize(textInput, widgetStyle, space.w, fillWidth);
    });

    bool result = false;

    if (!hideWidgets) {
        result = putTextInput(textInput, widgetBounds, widgetStyle, renderBuffer);
    }

    completeWidget(widgetBounds.size);

    return result;
}

Rect2I Panel::addBlank(s32 width, s32 height)
{
    DEBUG_FUNCTION_T(DCDT_UI);

    prepareForWidgets();

    Rect2I widgetBounds = calculateWidgetBounds(v2i(width, height));

    completeWidget(widgetBounds.size);

    return widgetBounds;
}

void Panel::alignWidgets(u32 alignment)
{
    widgetAlignment = (widgetAlignment & ALIGN_V) | (alignment & ALIGN_H);
}

void Panel::startNewLine(u32 hAlignment)
{
    DEBUG_FUNCTION_T(DCDT_UI);

    s32 lineWidth = (widgetAlignment & ALIGN_RIGHT) ? currentRight : currentLeft;
    largestLineWidth = max(largestLineWidth, lineWidth);

    currentLeft = 0;
    currentRight = contentArea.w;

    if (largestItemHeightOnLine > 0) {
        if (layoutBottomToTop) {
            currentBottom -= largestItemHeightOnLine + style->contentPadding;
        } else {
            currentTop += largestItemHeightOnLine + style->contentPadding;
        }
    }

    largestItemHeightOnLine = 0;

    // Only change alignment if we passed one
    if (hAlignment != 0) {
        widgetAlignment = (widgetAlignment & ALIGN_V) | (hAlignment & ALIGN_H);
    }
}

Panel Panel::row(s32 height, Alignment vAlignment, String styleName)
{
    DEBUG_FUNCTION_T(DCDT_UI);
    ASSERT(vAlignment == ALIGN_TOP || vAlignment == ALIGN_BOTTOM);

    prepareForWidgets();

    startNewLine();

    PanelStyle* rowStyle = getPanelStyle(styleName);

    if (vAlignment == ALIGN_TOP) {
        Rect2I rowBounds = irectXYWH(
            contentArea.x, contentArea.y + currentTop,
            contentArea.w, height);

        if (layoutBottomToTop) {
            contentArea.h -= height + style->contentPadding;
            contentArea.y += height + style->contentPadding;
        } else {
            completeWidget(rowBounds.size);
        }

        updateLayoutPosition();

        return Panel(rowBounds, rowStyle, flags, renderBuffer);
    } else {
        Rect2I rowBounds = irectXYWH(
            contentArea.x, contentArea.y + currentBottom - height,
            contentArea.w, height);

        if (layoutBottomToTop) {
            completeWidget(rowBounds.size);
        } else {
            contentArea.h -= height + style->contentPadding;
        }

        updateLayoutPosition();

        return Panel(rowBounds, rowStyle, flags, renderBuffer);
    }
}

Panel Panel::column(s32 width, Alignment hAlignment, String styleName)
{
    DEBUG_FUNCTION_T(DCDT_UI);
    ASSERT(hAlignment == ALIGN_LEFT || hAlignment == ALIGN_RIGHT);

    prepareForWidgets();

    startNewLine();

    PanelStyle* columnStyle = getPanelStyle(styleName);

    if (hAlignment == ALIGN_LEFT) {
        Rect2I columnBounds = irectXYWH(
            contentArea.x, contentArea.y + currentTop,
            width, contentArea.h);

        contentArea.w -= width + style->contentPadding;
        contentArea.x += width + style->contentPadding;
        updateLayoutPosition();

        return Panel(columnBounds, columnStyle, flags, renderBuffer);
    } else {
        Rect2I columnBounds = irectXYWH(
            contentArea.x + contentArea.w - width, contentArea.y + currentTop,
            width, contentArea.h);

        contentArea.w -= width + style->contentPadding;
        updateLayoutPosition();

        return Panel(columnBounds, columnStyle, flags, renderBuffer);
    }
}

void Panel::end(bool shrinkToContentHeight, bool shrinkToContentWidth)
{
    DEBUG_FUNCTION_T(DCDT_UI);

    // Make sure the current line's height is taken into account
    startNewLine();

    // @Hack! I don't at all understand why we get a trailing space of 2x the contentPadding at the end.
    if (layoutBottomToTop && hasAddedWidgets) {
        currentBottom += (style->contentPadding * 2);
    }

    s32 contentHeight = (layoutBottomToTop ? (contentArea.h - currentBottom)
                                           : (currentTop + style->padding.bottom));

    bool boundsChanged = false;

    if (shrinkToContentWidth) {
        if (widgetAlignment & ALIGN_LEFT) {
            s32 widthDifference = contentArea.w - largestLineWidth;
            if (widthDifference > 0) {
                bounds.w -= widthDifference;
                boundsChanged = true;
            }
        } else {
            s32 widthDifference = contentArea.w - largestLineWidth;
            if (widthDifference > 0) {
                bounds.x += widthDifference;
                bounds.w -= widthDifference;
                boundsChanged = true;
            }
        }
    }

    if (shrinkToContentHeight) {
        s32 totalVPadding = style->padding.top + style->padding.bottom;
        if (layoutBottomToTop) {
            contentHeight = (contentArea.h - currentBottom) + style->contentPadding + totalVPadding;
            s32 newHeight = clamp(contentHeight, totalVPadding, bounds.h);
            bounds.y += (bounds.h - newHeight);
            bounds.h = newHeight;
            boundsChanged = true;
        } else {
            contentHeight = currentTop - style->contentPadding + totalVPadding;
            bounds.h = clamp(contentHeight, totalVPadding, bounds.h);
            boundsChanged = true;
        }

        // Make sure there is space for the scrollbar if we have one
        if (hScrollbar) {
            bounds.h += hScrollbarBounds.h;
            if (layoutBottomToTop)
                bounds.y -= hScrollbarBounds.h;
            boundsChanged = true;
        }
    }

    if (boundsChanged) {
        hScrollbarBounds.w = bounds.w;
        hScrollbarBounds.x = bounds.x;
        // In case the height has shrunk, move the scrollbar to the bottom edge
        hScrollbarBounds.y = bounds.y + bounds.h - hScrollbarBounds.h;

        vScrollbarBounds.h = bounds.h;
        vScrollbarBounds.y = bounds.y;
    }

    if (!hasAddedWidgets) {
        if (!hideWidgets) {
            addBeginScissor(renderBuffer, bounds);

            // Prepare to render background
            background.preparePlaceholder(renderBuffer);
        }
    }

    // Handle scrollbars
    if (hScrollbar) {
        ScrollbarStyle* scrollbarStyle = getStyle<ScrollbarStyle>(&style->scrollbarStyle);

        if (!hideWidgets) {
            putScrollbar(hScrollbar, largestLineWidth + style->padding.left + style->padding.right, hScrollbarBounds, scrollbarStyle, false, renderBuffer);
        }
    }

    if (vScrollbar) {
        ScrollbarStyle* scrollbarStyle = getStyle<ScrollbarStyle>(&style->scrollbarStyle);

        if (!hideWidgets) {
            putScrollbar(vScrollbar, contentHeight, vScrollbarBounds, scrollbarStyle, false, renderBuffer);
        }
    }

    if (!hideWidgets) {
        // Fill in the background
        background.fillPlaceholder(bounds);

        // Clear any scissor stuff
        addEndScissor(renderBuffer);

        popInputScissorRect();
    }

    // Add a UI rect if we're top level. Otherwise, our parent already added one that encompasses us!
    if (!allowClickThrough) {
        addUIRect(bounds);
    }
}

void Panel::prepareForWidgets()
{
    if (!hasAddedWidgets) {
        if (!hideWidgets) {
            addBeginScissor(renderBuffer, bounds);

            pushInputScissorRect(bounds);

            // Prepare to render background
            background.preparePlaceholder(renderBuffer);
        }

        hasAddedWidgets = true;
    }
}

template<typename Func>
Rect2I Panel::calculateWidgetBounds(Func calculateSize)
{
    // Calculate the available space
    Rect2I space = {};

    space.x = contentArea.x + currentLeft;
    space.w = currentRight - currentLeft;

    if (layoutBottomToTop) {
        space.y = contentArea.y + currentBottom;

        if (vScrollbar != null) {
            space.y += (vScrollbar->contentSize - getScrollbarContentOffset(vScrollbar, bounds.h) - bounds.h);
        }
    } else {
        space.y = contentArea.y + currentTop;

        if (vScrollbar != null) {
            space.y -= getScrollbarContentOffset(vScrollbar, bounds.h);
        }
    }

    // Adjust if we're in a scrolling area
    if (hScrollbar != null) {
        space.w = s16Max; // Not s32 because then we'd have overflow issues. s16 should be plenty large enough.
        space.x = space.x - getScrollbarContentOffset(hScrollbar, bounds.w);
    }
    ASSERT(space.w > 0);

    bool fillWidth = ((widgetAlignment & ALIGN_H) == ALIGN_EXPAND_H);

    V2I widgetSize = calculateSize(space, fillWidth);

    Rect2I widgetBounds = alignWithinRectangle(space, widgetSize, widgetAlignment);

    return widgetBounds;
}

Rect2I Panel::calculateWidgetBounds(V2I size)
{
    return calculateWidgetBounds([&](Rect2I, bool) { return size; });
}

void Panel::completeWidget(V2I widgetSize)
{
    DEBUG_FUNCTION_T(DCDT_UI);

    bool lineIsFull = false;

    s32 lineWidth = 0;

    switch (widgetAlignment & ALIGN_H) {
    case ALIGN_LEFT: {
        currentLeft += widgetSize.x + style->contentPadding;
        lineWidth = currentLeft;

        // Check for a full line
        // NB: We might want to do something smarter when there's only a small remainder.
        // Though, for now we'll just be smart about not intentionally wrapping a line.
        if (currentLeft >= currentRight) {
            lineIsFull = true;
        }
    } break;

    case ALIGN_RIGHT: {
        currentRight -= widgetSize.x + style->contentPadding;
        lineWidth = currentRight;

        // Check for a full line
        // NB: We might want to do something smarter when there's only a small remainder.
        // Though, for now we'll just be smart about not intentionally wrapping a line.
        if (currentLeft >= currentRight) {
            lineIsFull = true;
        }
    } break;

    case ALIGN_H_CENTRE:
    case ALIGN_EXPAND_H:
    default: {
        lineWidth = widgetSize.x;

        // Just start a new line
        lineIsFull = true;
    } break;
    }

    largestLineWidth = max(largestLineWidth, lineWidth);
    largestItemWidth = max(largestItemWidth, widgetSize.x);
    largestItemHeightOnLine = max(largestItemHeightOnLine, widgetSize.y);

    if (lineIsFull) {
        // New line with the same alignment
        startNewLine();
    }
}

inline PanelStyle* Panel::getPanelStyle(String styleName)
{
    PanelStyle* result = null;
    if (!isEmpty(styleName))
        result = getStyle<PanelStyle>(styleName);
    if (result == null)
        result = this->style;

    return result;
}

void Panel::updateLayoutPosition()
{
    currentLeft = max(0, currentLeft);
    currentRight = min(contentArea.w, currentRight);

    currentTop = max(0, currentTop);
    currentBottom = min(contentArea.h, currentBottom);
}

}
