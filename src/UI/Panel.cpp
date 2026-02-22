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
    : style(panelStyle ? panelStyle : &PanelStyle::get("default"_s))
    , flags(flags)
    , allowClickThrough((flags & PanelFlags::AllowClickThrough) != 0)
    , hideWidgets((flags & PanelFlags::HideWidgets) != 0)
    , layoutBottomToTop((flags & PanelFlags::LayoutBottomToTop) != 0)
    , renderBuffer(renderBuffer)
    , bounds(bounds)
    , contentArea(bounds.shrunk(this->style->padding))
    , widgetAlignment({ style->widgetAlignment.horizontal, layoutBottomToTop ? VAlign::Bottom : VAlign::Top })
    , currentLeft(0)
    , currentRight(this->contentArea.width())
    , currentTop(0)
    , currentBottom(this->contentArea.height())
    , background(Drawable(&this->style->background))
{
}

void Panel::enableHorizontalScrolling(ScrollbarState* scrollbarState)
{
    auto& scrollbar_style = style->scrollbarStyle.get();

    this->hScrollbar = scrollbarState;
    this->hScrollbarBounds = { bounds.x(), bounds.y() + bounds.height() - scrollbar_style.width, bounds.width(), scrollbar_style.width };

    this->contentArea.set_height(this->contentArea.height() - scrollbar_style.width);
    updateLayoutPosition();
}

void Panel::enableVerticalScrolling(ScrollbarState* scrollbarState, bool expandWidth)
{
    auto& scrollbar_style = style->scrollbarStyle.get();

    this->vScrollbar = scrollbarState;

    s32 scrollbarX = bounds.x() + bounds.width() - (expandWidth ? 0 : scrollbar_style.width);
    this->vScrollbarBounds = { scrollbarX, bounds.y(), scrollbar_style.width, bounds.height() };

    if (expandWidth) {
        this->bounds.set_width(this->bounds.width() + scrollbar_style.width);
    } else {
        this->contentArea.set_width(this->contentArea.width() - scrollbar_style.width);
        this->currentRight -= scrollbar_style.width;
        updateLayoutPosition();
    }
}

bool Panel::addTextButton(StringView text, ButtonState state, Optional<StringView> style_name)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    prepareForWidgets();

    auto& button_style = ButtonStyle::get_with_fallback(style_name, style->buttonStyle);

    Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I space, bool fillWidth) {
        return calculateButtonSize(text, &button_style, space.width(), fillWidth);
    });

    bool wasClicked = false;

    if (!hideWidgets) {
        wasClicked = putTextButton(text, widgetBounds, &button_style, state, renderBuffer);
    }

    completeWidget(widgetBounds.size());

    return wasClicked;
}

bool Panel::addImageButton(Sprite* sprite, ButtonState state, Optional<StringView> style_name)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    prepareForWidgets();

    auto& widgetStyle = ButtonStyle::get_with_fallback(style_name, style->buttonStyle);
    V2I spriteSize = v2i(sprite->pixelWidth, sprite->pixelHeight);

    Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I space, bool fillWidth) {
        return calculateButtonSize(spriteSize, &widgetStyle, space.width(), fillWidth);
    });

    bool wasClicked = false;

    if (!hideWidgets) {
        wasClicked = putImageButton(sprite, widgetBounds, &widgetStyle, state, renderBuffer);
    }

    completeWidget(widgetBounds.size());

    return wasClicked;
}

void Panel::addCheckbox(bool* checked, Optional<StringView> style_name)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    prepareForWidgets();

    auto& widgetStyle = CheckboxStyle::get_with_fallback(style_name, style->checkboxStyle);

    Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I, bool) {
        return calculateCheckboxSize(&widgetStyle);
    });

    if (!hideWidgets) {
        putCheckbox(checked, widgetBounds, &widgetStyle, false, renderBuffer);
    }

    completeWidget(widgetBounds.size());
}

void Panel::addLabel(StringView text, Optional<StringView> style_name)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    prepareForWidgets();

    auto& widgetStyle = LabelStyle::get_with_fallback(style_name, style->labelStyle);

    Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I space, bool fillWidth) {
        return calculateLabelSize(text, &widgetStyle, space.width(), fillWidth);
    });

    if (!hideWidgets) {
        putLabel(text, widgetBounds, &widgetStyle, renderBuffer);
    }

    completeWidget(widgetBounds.size());
}

void Panel::addRadioButton(s32* currentValue, s32 myValue, Optional<StringView> style_name)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    prepareForWidgets();

    auto& widgetStyle = RadioButtonStyle::get_with_fallback(style_name, style->radioButtonStyle);

    Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I, bool) {
        return calculateRadioButtonSize(&widgetStyle);
    });

    if (!hideWidgets) {
        putRadioButton(currentValue, myValue, widgetBounds, &widgetStyle, false, renderBuffer);
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

bool Panel::addTextInput(TextInput* textInput, Optional<StringView> style_name)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    prepareForWidgets();

    auto& widgetStyle = TextInputStyle::get_with_fallback(style_name, style->textInputStyle);

    Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I space, bool fillWidth) {
        return calculateTextInputSize(textInput, &widgetStyle, space.width(), fillWidth);
    });

    bool result = false;

    if (!hideWidgets) {
        result = putTextInput(textInput, widgetBounds, &widgetStyle, renderBuffer);
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
    currentRight = contentArea.width();

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

Panel Panel::row(s32 height, VAlign vAlignment, Optional<StringView> style_name)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);
    ASSERT(vAlignment == VAlign::Top || vAlignment == VAlign::Bottom);

    prepareForWidgets();

    startNewLine();

    PanelStyle* rowStyle = getPanelStyle(style_name);

    if (vAlignment == VAlign::Top) {
        Rect2I rowBounds { contentArea.x(), contentArea.y() + currentTop, contentArea.width(), height };

        if (layoutBottomToTop) {
            contentArea.set_height(contentArea.height() - (height + style->contentPadding));
            contentArea.set_y(contentArea.y() + height + style->contentPadding);
        } else {
            completeWidget(rowBounds.size());
        }

        updateLayoutPosition();

        return Panel(rowBounds, rowStyle, flags, renderBuffer);
    } else {
        Rect2I rowBounds { contentArea.x(), contentArea.y() + currentBottom - height, contentArea.width(), height };

        if (layoutBottomToTop) {
            completeWidget(rowBounds.size());
        } else {
            contentArea.set_height(contentArea.height() - (height + style->contentPadding));
        }

        updateLayoutPosition();

        return Panel(rowBounds, rowStyle, flags, renderBuffer);
    }
}

Panel Panel::column(s32 width, HAlign hAlignment, Optional<StringView> style_name)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);
    ASSERT(hAlignment == HAlign::Left || hAlignment == HAlign::Right);

    prepareForWidgets();

    startNewLine();

    PanelStyle* columnStyle = getPanelStyle(style_name);

    if (hAlignment == HAlign::Left) {
        Rect2I columnBounds { contentArea.x(), contentArea.y() + currentTop, width, contentArea.height() };

        contentArea.set_width(contentArea.width() - (width + style->contentPadding));
        contentArea.set_x(contentArea.x() + width + style->contentPadding);
        updateLayoutPosition();

        return Panel(columnBounds, columnStyle, flags, renderBuffer);
    } else {
        Rect2I columnBounds { contentArea.x() + contentArea.width() - width, contentArea.y() + currentTop, width, contentArea.height() };

        contentArea.set_width(contentArea.width() - (width + style->contentPadding));
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

    s32 contentHeight = (layoutBottomToTop ? (contentArea.height() - currentBottom)
                                           : (currentTop + style->padding.bottom));

    bool boundsChanged = false;

    if (shrinkToContentWidth) {
        if (widgetAlignment.horizontal == HAlign::Left) {
            s32 widthDifference = contentArea.width() - largestLineWidth;
            if (widthDifference > 0) {
                bounds.set_width(bounds.width() - widthDifference);
                boundsChanged = true;
            }
        } else {
            s32 widthDifference = contentArea.width() - largestLineWidth;
            if (widthDifference > 0) {
                bounds.set_x(bounds.x() + widthDifference);
                bounds.set_width(bounds.width() - widthDifference);
                boundsChanged = true;
            }
        }
    }

    if (shrinkToContentHeight) {
        s32 totalVPadding = style->padding.top + style->padding.bottom;
        if (layoutBottomToTop) {
            contentHeight = (contentArea.height() - currentBottom) + style->contentPadding + totalVPadding;
            s32 newHeight = clamp(contentHeight, totalVPadding, bounds.height());
            bounds.set_y(bounds.y() + (bounds.height() - newHeight));
            bounds.set_height(newHeight);
            boundsChanged = true;
        } else {
            contentHeight = currentTop - style->contentPadding + totalVPadding;
            bounds.set_height(clamp(contentHeight, totalVPadding, bounds.height()));
            boundsChanged = true;
        }

        // Make sure there is space for the scrollbar if we have one
        if (hScrollbar) {
            bounds.set_height(bounds.height() + hScrollbarBounds.height());
            if (layoutBottomToTop)
                bounds.set_y(bounds.y() - hScrollbarBounds.height());
            boundsChanged = true;
        }
    }

    if (boundsChanged) {
        hScrollbarBounds.set_width(bounds.width());
        hScrollbarBounds.set_x(bounds.x());
        // In case the height has shrunk, move the scrollbar to the bottom edge
        hScrollbarBounds.set_y(bounds.y() + bounds.height() - hScrollbarBounds.height());

        vScrollbarBounds.set_height(bounds.height());
        vScrollbarBounds.set_y(bounds.y());
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
        auto& scrollbar_style = style->scrollbarStyle.get();

        if (!hideWidgets) {
            putScrollbar(hScrollbar, largestLineWidth + style->padding.left + style->padding.right, hScrollbarBounds, &scrollbar_style, false, renderBuffer);
        }
    }

    if (vScrollbar) {
        auto& scrollbar_style = style->scrollbarStyle.get();

        if (!hideWidgets) {
            putScrollbar(vScrollbar, contentHeight, vScrollbarBounds, &scrollbar_style, false, renderBuffer);
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

PanelStyle* Panel::getPanelStyle(Optional<StringView> style_name) const
{
    PanelStyle* result = nullptr;
    if (style_name.has_value())
        result = &PanelStyle::get(style_name.value());
    if (result == nullptr)
        result = this->style;

    return result;
}

void Panel::updateLayoutPosition()
{
    currentLeft = max(0, currentLeft);
    currentRight = min(contentArea.width(), currentRight);

    currentTop = max(0, currentTop);
    currentBottom = min(contentArea.height(), currentBottom);
}

}
