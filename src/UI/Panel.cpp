/*
 * Copyright (c) 2021-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Panel.h"
#include <UI/TextInput.h>
#include <Util/String.h>

namespace UI {

Panel::Panel(Rect2I bounds, PanelStyle* panelStyle, u32 flags, RenderBuffer* renderBuffer)
    : style(panelStyle ? panelStyle : getStyle<PanelStyle>("default"_s))
    , flags(flags)
    , allowClickThrough((flags & PanelFlags::AllowClickThrough) != 0)
    , hideWidgets((flags & PanelFlags::HideWidgets) != 0)
    , layoutBottomToTop((flags & PanelFlags::LayoutBottomToTop) != 0)
    , renderBuffer(renderBuffer)
    , bounds(bounds)
    , contentArea(shrink(bounds, this->style->padding))
    , widgetAlignment({ style->widgetAlignment.horizontal, layoutBottomToTop ? VAlign::Bottom : VAlign::Top })
    , currentLeft(0)
    , currentRight(this->contentArea.w)
    , currentTop(0)
    , currentBottom(this->contentArea.h)
    , background(Drawable(&this->style->background))
{
}

void Panel::enableHorizontalScrolling(ScrollbarState* scrollbarState)
{
    ScrollbarStyle* scrollbarStyle = getStyle<ScrollbarStyle>(&style->scrollbarStyle);
    ASSERT(scrollbarStyle != nullptr);

    this->hScrollbar = scrollbarState;
    this->hScrollbarBounds = { bounds.x, bounds.y + bounds.h - scrollbarStyle->width, bounds.w, scrollbarStyle->width };

    this->contentArea.h -= scrollbarStyle->width;
    updateLayoutPosition();
}

void Panel::enableVerticalScrolling(ScrollbarState* scrollbarState, bool expandWidth)
{
    ScrollbarStyle* scrollbarStyle = getStyle<ScrollbarStyle>(&style->scrollbarStyle);
    ASSERT(scrollbarStyle != nullptr);

    this->vScrollbar = scrollbarState;

    s32 scrollbarX = bounds.x + bounds.w - (expandWidth ? 0 : scrollbarStyle->width);
    this->vScrollbarBounds = { scrollbarX, bounds.y, scrollbarStyle->width, bounds.h };

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
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    prepareForWidgets();

    ButtonStyle* widgetStyle = getStyle<ButtonStyle>(styleName, &this->style->buttonStyle);

    Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I space, bool fillWidth) {
        return calculateButtonSize(text, widgetStyle, space.w, fillWidth);
    });

    bool wasClicked = false;

    if (!hideWidgets) {
        wasClicked = putTextButton(text, widgetBounds, widgetStyle, state, renderBuffer);
    }

    completeWidget(widgetBounds.size());

    return wasClicked;
}

bool Panel::addImageButton(Sprite* sprite, ButtonState state, String styleName)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

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

    completeWidget(widgetBounds.size());

    return wasClicked;
}

void Panel::addCheckbox(bool* checked, String styleName)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    prepareForWidgets();

    CheckboxStyle* widgetStyle = getStyle<CheckboxStyle>(styleName, &style->checkboxStyle);

    Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I, bool) {
        return calculateCheckboxSize(widgetStyle);
    });

    if (!hideWidgets) {
        putCheckbox(checked, widgetBounds, widgetStyle, false, renderBuffer);
    }

    completeWidget(widgetBounds.size());
}

void Panel::addLabel(String text, String styleName)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    prepareForWidgets();

    LabelStyle* widgetStyle = getStyle<LabelStyle>(styleName, &this->style->labelStyle);

    Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I space, bool fillWidth) {
        return calculateLabelSize(text, widgetStyle, space.w, fillWidth);
    });

    if (!hideWidgets) {
        putLabel(text, widgetBounds, widgetStyle, renderBuffer);
    }

    completeWidget(widgetBounds.size());
}

void Panel::addRadioButton(s32* currentValue, s32 myValue, String styleName)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    prepareForWidgets();

    RadioButtonStyle* widgetStyle = getStyle<RadioButtonStyle>(styleName, &style->radioButtonStyle);

    Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I, bool) {
        return calculateRadioButtonSize(widgetStyle);
    });

    if (!hideWidgets) {
        putRadioButton(currentValue, myValue, widgetBounds, widgetStyle, false, renderBuffer);
    }

    completeWidget(widgetBounds.size());
}

void Panel::addSprite(Sprite* sprite, s32 width, s32 height)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    prepareForWidgets();

    V2I size = v2i(width, height);
    if (sprite != nullptr) {
        if (size.x == -1)
            size.x = sprite->pixelWidth;
        if (size.y == -1)
            size.y = sprite->pixelHeight;
    }

    Rect2I widgetBounds = calculateWidgetBounds(size);

    if (sprite != nullptr) {
        if (!hideWidgets) {
            drawSingleSprite(renderBuffer, sprite, widgetBounds, the_renderer().shaderIds.pixelArt, Colour::white());
        }
    }

    completeWidget(widgetBounds.size());
}

bool Panel::addTextInput(TextInput* textInput, String styleName)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    prepareForWidgets();

    TextInputStyle* widgetStyle = getStyle<TextInputStyle>(styleName, &this->style->textInputStyle);

    Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I space, bool fillWidth) {
        return calculateTextInputSize(textInput, widgetStyle, space.w, fillWidth);
    });

    bool result = false;

    if (!hideWidgets) {
        result = putTextInput(textInput, widgetBounds, widgetStyle, renderBuffer);
    }

    completeWidget(widgetBounds.size());

    return result;
}

Rect2I Panel::addBlank(s32 width, s32 height)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    prepareForWidgets();

    Rect2I widgetBounds = calculateWidgetBounds(v2i(width, height));

    completeWidget(widgetBounds.size());

    return widgetBounds;
}

void Panel::alignWidgets(HAlign h_alignment)
{
    widgetAlignment.horizontal = h_alignment;
}

void Panel::startNewLine(Optional<HAlign> h_align)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    s32 lineWidth = widgetAlignment.horizontal == HAlign::Right ? currentRight : currentLeft;
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
    if (h_align.has_value())
        widgetAlignment.horizontal = h_align.release_value();
}

Panel Panel::row(s32 height, VAlign vAlignment, String styleName)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);
    ASSERT(vAlignment == VAlign::Top || vAlignment == VAlign::Bottom);

    prepareForWidgets();

    startNewLine();

    PanelStyle* rowStyle = getPanelStyle(styleName);

    if (vAlignment == VAlign::Top) {
        Rect2I rowBounds { contentArea.x, contentArea.y + currentTop, contentArea.w, height };

        if (layoutBottomToTop) {
            contentArea.h -= height + style->contentPadding;
            contentArea.y += height + style->contentPadding;
        } else {
            completeWidget(rowBounds.size());
        }

        updateLayoutPosition();

        return Panel(rowBounds, rowStyle, flags, renderBuffer);
    } else {
        Rect2I rowBounds { contentArea.x, contentArea.y + currentBottom - height, contentArea.w, height };

        if (layoutBottomToTop) {
            completeWidget(rowBounds.size());
        } else {
            contentArea.h -= height + style->contentPadding;
        }

        updateLayoutPosition();

        return Panel(rowBounds, rowStyle, flags, renderBuffer);
    }
}

Panel Panel::column(s32 width, HAlign hAlignment, String styleName)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);
    ASSERT(hAlignment == HAlign::Left || hAlignment == HAlign::Right);

    prepareForWidgets();

    startNewLine();

    PanelStyle* columnStyle = getPanelStyle(styleName);

    if (hAlignment == HAlign::Left) {
        Rect2I columnBounds { contentArea.x, contentArea.y + currentTop, width, contentArea.h };

        contentArea.w -= width + style->contentPadding;
        contentArea.x += width + style->contentPadding;
        updateLayoutPosition();

        return Panel(columnBounds, columnStyle, flags, renderBuffer);
    } else {
        Rect2I columnBounds { contentArea.x + contentArea.w - width, contentArea.y + currentTop, width, contentArea.h };

        contentArea.w -= width + style->contentPadding;
        updateLayoutPosition();

        return Panel(columnBounds, columnStyle, flags, renderBuffer);
    }
}

void Panel::end(bool shrinkToContentHeight, bool shrinkToContentWidth)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

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
        if (widgetAlignment.horizontal == HAlign::Left) {
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

Rect2I Panel::calculateWidgetBounds(V2I size)
{
    return calculateWidgetBounds([&](Rect2I, bool) { return size; });
}

void Panel::completeWidget(V2I widgetSize)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    bool lineIsFull = false;

    s32 lineWidth = 0;

    switch (widgetAlignment.horizontal) {
    case HAlign::Left: {
        currentLeft += widgetSize.x + style->contentPadding;
        lineWidth = currentLeft;

        // Check for a full line
        // NB: We might want to do something smarter when there's only a small remainder.
        // Though, for now we'll just be smart about not intentionally wrapping a line.
        if (currentLeft >= currentRight) {
            lineIsFull = true;
        }
    } break;

    case HAlign::Right: {
        currentRight -= widgetSize.x + style->contentPadding;
        lineWidth = currentRight;

        // Check for a full line
        // NB: We might want to do something smarter when there's only a small remainder.
        // Though, for now we'll just be smart about not intentionally wrapping a line.
        if (currentLeft >= currentRight) {
            lineIsFull = true;
        }
    } break;

    case HAlign::Centre:
    case HAlign::Fill:
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

PanelStyle* Panel::getPanelStyle(String styleName)
{
    PanelStyle* result = nullptr;
    if (!isEmpty(styleName))
        result = getStyle<PanelStyle>(styleName);
    if (result == nullptr)
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
