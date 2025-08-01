/*
 * Copyright (c) 2021-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <Assets/AssetManager.h>
#include <UI/Drawable.h>
#include <UI/Forward.h>
#include <UI/UI.h>
#include <Util/Orientation.h>
#include <Util/Rectangle.h>
#include <Util/String.h>

// Going to try out using a more OOP style.
// I didn't like putting "window_" at the start of all the Window-UI functions, or
// having to pass it as the parameter all the time.
// Maybe this will feel awkward too, who knows.

//
// NB: Panels push a BeginScissor when their first widget is added.
// This means that you can't alternate between adding widgets to two different ones, as
// they'll both use the scissor of whichever Panel had its first child added last. eg this:
//
//    UI::Panel a = UI::Panel(...);
//    UI::Panel b = UI::Panel(...);
//    a.addLabel("First"_s);
//    b.addLabel("Second"_s);
//    a.addLabel("Third"_s);
//
// will not work, as "Third" will be clipped to the boundaries of b, not a! But as long as
// you avoid code like that, it's fine - adding all the 'a' widgets, then all of 'b', or
// vice-versa.
//
// - Sam, 04/02/2021
//

namespace UI {
namespace PanelFlags {

enum {
    LayoutBottomToTop = 1 << 0,
    AllowClickThrough = 1 << 1,
    HideWidgets = 1 << 2, // Widgets are not updated or rendered, just laid out
};

}

struct Panel {
    Panel(Rect2I bounds, PanelStyle* style = nullptr, u32 flags = 0, RenderBuffer* renderBuffer = &the_renderer().ui_buffer());
    Panel(Rect2I bounds, String styleName, u32 flags = 0, RenderBuffer* renderBuffer = &the_renderer().ui_buffer())
        : Panel(bounds, getStyle<PanelStyle>(styleName), flags, renderBuffer)
    {
    }

    // Configuration functions, which should be called before adding any widgets
    void enableHorizontalScrolling(ScrollbarState* hScrollbar);
    void enableVerticalScrolling(ScrollbarState* vScrollbar, bool expandWidth = false);

    // Add stuff to the panel
    bool addTextButton(String text, ButtonState state = ButtonState::Normal, String styleName = nullString);
    bool addImageButton(Sprite* sprite, ButtonState state = ButtonState::Normal, String styleName = nullString);

    void addCheckbox(bool* checked, String styleName = nullString);

    template<typename T>
    void addDropDownList(Array<T>* listOptions, s32* currentSelection, String (*getDisplayName)(T* data), String styleName = nullString)
    {
        DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

        prepareForWidgets();

        DropDownListStyle* widgetStyle = getStyle<DropDownListStyle>(styleName, &style->dropDownListStyle);

        Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I space, bool fillWidth) {
            return calculateDropDownListSize(listOptions, getDisplayName, widgetStyle, space.width(), fillWidth);
        });

        if (!hideWidgets) {
            putDropDownList(listOptions, currentSelection, getDisplayName, widgetBounds, widgetStyle, renderBuffer);
        }

        completeWidget(widgetBounds.size());
    }

    void addLabel(String text, String styleName = nullString);

    void addRadioButton(s32* currentValue, s32 myValue, String styleName = nullString);

    template<typename T>
    void addRadioButtonGroup(Array<T>* listOptions, s32* currentSelection, String (*getDisplayName)(T* data), String styleName = nullString, String labelStyleName = nullString)
    {
        DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

        prepareForWidgets();

        RadioButtonStyle* radioButtonStyle = getStyle<RadioButtonStyle>(styleName, &style->radioButtonStyle);
        LabelStyle* labelStyle = getStyle<LabelStyle>(labelStyleName, &style->labelStyle);

        V2I radioButtonSize = calculateRadioButtonSize(radioButtonStyle);

        Rect2I buttonGroupBounds = calculateWidgetBounds([&](Rect2I space, bool fillWidth) {
            s32 textWidth = space.width() - (radioButtonSize.x + style->contentPadding);

            // Calculate the overall size
            // This means addRadioButtonGroup() has to loop through everything twice, but what can you do
            V2I widgetSize = v2i(0, 0);
            for (s32 optionIndex = 0; optionIndex < listOptions->count; optionIndex++) {
                String optionText = getDisplayName(&listOptions->get(optionIndex));
                V2I labelSize = calculateLabelSize(optionText, labelStyle, textWidth, fillWidth);

                widgetSize.x = max(widgetSize.x, radioButtonSize.x + style->contentPadding + labelSize.x);

                if (optionIndex > 0)
                    widgetSize.y += style->contentPadding;
                widgetSize.y += max(labelSize.y, radioButtonSize.y);
            }

            ASSERT(widgetSize.x <= space.width());
            return widgetSize;
        });

        Rect2I radioButtonBounds = buttonGroupBounds.create_aligned_within(radioButtonSize, { HAlign::Left, VAlign::Top });
        s32 textWidth = buttonGroupBounds.width() - (radioButtonSize.x + style->contentPadding);
        bool fillWidth = widgetAlignment.horizontal == HAlign::Fill;

        for (s32 optionIndex = 0; optionIndex < listOptions->count; optionIndex++) {
            if (!hideWidgets) {
                putRadioButton(currentSelection, optionIndex, radioButtonBounds, radioButtonStyle, false, renderBuffer);
            }

            String optionText = getDisplayName(&listOptions->get(optionIndex));
            V2I labelSize = calculateLabelSize(optionText, labelStyle, textWidth, fillWidth);
            Rect2I labelBounds {
                radioButtonBounds.x() + radioButtonBounds.width() + style->contentPadding,
                radioButtonBounds.y(),
                labelSize.x,
                labelSize.y
            };

            if (!hideWidgets) {
                putLabel(optionText, labelBounds, labelStyle, renderBuffer);
            }

            radioButtonBounds.set_y(labelBounds.y() + labelBounds.height() + style->contentPadding);
        }

        completeWidget(buttonGroupBounds.size());
    }

    template<typename T>
    void addSlider(T* currentValue, T minValue, T maxValue, String styleName = nullString)
    {
        DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

        prepareForWidgets();

        SliderStyle* widgetStyle = getStyle<SliderStyle>(styleName, &style->sliderStyle);

        Rect2I widgetBounds = calculateWidgetBounds([&](Rect2I space, bool fillWidth) {
            return calculateSliderSize(Orientation::Horizontal, widgetStyle, space.size(), fillWidth);
        });

        if (!hideWidgets) {
            putSlider(currentValue, minValue, maxValue, Orientation::Horizontal, widgetBounds, widgetStyle, false, renderBuffer);
        }

        completeWidget(widgetBounds.size());
    }

    void addSprite(Sprite* sprite, s32 width = -1, s32 height = -1);

    bool addTextInput(TextInput* textInput, String styleName = nullString);

    // Add a blank rectangle as if it were a widget. (So, leaving padding between
    // it and other widgets.) The bounds are returned so you can draw your own
    // contents.
    Rect2I addBlank(s32 width, s32 height = 0);

    // Layout options
    void alignWidgets(HAlign h_align);
    void startNewLine(Optional<HAlign> h_align = {});

    // Slice the remaining content area in two, with one part being the new Panel,
    // and the rest becoming the existing panel's content area.
    Panel row(s32 height, VAlign vAlignment, String styleName = nullString);
    Panel column(s32 width, HAlign hAlignment, String styleName = nullString);

    void end(bool shinkToContentHeight = false, bool shrinkToContentWidth = false);

    // "Private"
    void prepareForWidgets();

    template<typename Func>
    Rect2I calculateWidgetBounds(Func calculateSize)
    {
        // Calculate the available space
        Rect2I space = {};

        space.set_x(contentArea.x() + currentLeft);
        space.set_width(currentRight - currentLeft);

        if (layoutBottomToTop) {
            space.set_y(contentArea.y() + currentBottom);

            if (vScrollbar != nullptr) {
                space.set_y(space.y() + (vScrollbar->contentSize - getScrollbarContentOffset(vScrollbar, bounds.height()) - bounds.height()));
            }
        } else {
            space.set_y(contentArea.y() + currentTop);

            if (vScrollbar != nullptr) {
                space.set_y(space.y() - getScrollbarContentOffset(vScrollbar, bounds.height()));
            }
        }

        // Adjust if we're in a scrolling area
        if (hScrollbar != nullptr) {
            space.set_width(s16Max); // Not s32 because then we'd have overflow issues. s16 should be plenty large enough.
            space.set_x(space.x() - getScrollbarContentOffset(hScrollbar, bounds.width()));
        }
        ASSERT(space.width() > 0);

        bool fillWidth = ((widgetAlignment.horizontal) == HAlign::Fill);

        V2I widgetSize = calculateSize(space, fillWidth);

        Rect2I widgetBounds = space.create_aligned_within(widgetSize, widgetAlignment);

        return widgetBounds;
    }

    Rect2I calculateWidgetBounds(V2I size);
    void completeWidget(V2I widgetSize);
    PanelStyle* getPanelStyle(String styleName);

    // Call after modifying the contentArea. Updates the positions fields to match.
    void updateLayoutPosition();

    PanelStyle* style { nullptr };

    // Flags, and bool versions for easier access
    u32 flags { 0 };
    bool allowClickThrough { false };
    bool hideWidgets { false }; // Widgets are not updated or rendered, just laid out
    bool layoutBottomToTop { false };

    bool hasAddedWidgets { false };

    RenderBuffer* renderBuffer { nullptr };

    Rect2I bounds {};
    Rect2I contentArea {};
    Alignment widgetAlignment;

    ScrollbarState* hScrollbar { nullptr };
    Rect2I hScrollbarBounds {};
    ScrollbarState* vScrollbar { nullptr };
    Rect2I vScrollbarBounds {};

    // Relative to contentArea
    s32 currentLeft { 0 };
    s32 currentRight { 0 };
    s32 currentTop { 0 };
    s32 currentBottom { 0 };

    s32 largestItemWidth { 0 };
    s32 largestItemHeightOnLine { 0 };
    s32 largestLineWidth { 0 };

    Drawable background;
};

}
