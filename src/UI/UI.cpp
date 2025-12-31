/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AppState.h"
#include "input.h"
#include <Assets/AssetManager.h>
#include <Gfx/BitmapFont.h>
#include <Gfx/Renderer.h>
#include <UI/Drawable.h>
#include <UI/Panel.h>
#include <UI/TextInput.h>
#include <UI/Toast.h>
#include <UI/UI.h>
#include <UI/UITheme.h>
#include <UI/Window.h>
#include <Util/Rectangle.h>

namespace UI {

void init(MemoryArena* arena)
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
    auto& renderer = the_renderer();
    auto& ui_camera = renderer.ui_camera();
    uiState.uiRects.clear();
    uiState.mouseInputHandled = false;

    // Clear the drag if the mouse button isn't pressed
    if (!mouseButtonPressed(MouseButton::Left)) {
        uiState.currentDragObject = nullptr;
    }

    windowSize = v2i(ui_camera.size());
    mousePos = v2i(ui_camera.mouse_position());
    if (mouseButtonPressed(MouseButton::Left)) {
        mouseClickStartPos = v2i(getClickStartPos(MouseButton::Left, &ui_camera));
    }
}

void endFrame()
{
    auto current_toast = uiState.toasts.peek();
    if (current_toast.has_value()) {
        Toast* toast = current_toast.value();
        if (toast->update_and_draw(AppState::the().deltaTime))
            (void)uiState.toasts.pop();
    }

    if (uiState.openDropDownListRenderBuffer != nullptr) {
        // If the buffer is empty, we didn't draw the dropdown this frame, so we should mark it as closed
        if (uiState.openDropDownListRenderBuffer->firstChunk == nullptr) {
            // FIXME: Reintroduce this when drop-downs are re-enabled
            // closeDropDownList();
        } else {
            // NB: We transfer to the *window* buffer, not the *ui* buffer, because we
            // want the drop-down to appear in front of any windows.
            the_renderer().window_buffer().take_from(*uiState.openDropDownListRenderBuffer);
        }
    }
}

bool isMouseInputHandled()
{
    return uiState.mouseInputHandled;
}

void markMouseInputHandled()
{
    uiState.mouseInputHandled = true;
}

bool isMouseInUIBounds(Rect2I bounds)
{
    return isMouseInUIBounds(bounds, mousePos);
}

bool isMouseInUIBounds(Rect2I bounds, V2I pos)
{
    Rect2I clippedBounds = isInputScissorActive() ? bounds.intersected(getInputScissorRect()) : bounds;

    bool result = clippedBounds.contains(pos);

    return result;
}

bool justClickedOnUI(Rect2I bounds)
{
    Rect2I clippedBounds = isInputScissorActive() ? bounds.intersected(getInputScissorRect()) : bounds;

    bool result = !isMouseInputHandled()
        && clippedBounds.contains(mousePos)
        && mouseButtonJustReleased(MouseButton::Left)
        && clippedBounds.contains(mouseClickStartPos);

    return result;
}

WidgetMouseState getWidgetMouseState(Rect2I widgetBounds)
{
    WidgetMouseState result = {};

    if (!isMouseInputHandled() && isMouseInUIBounds(widgetBounds)) {
        result.isHovered = true;

        // Mouse pressed: must have started and currently be inside the bounds to show anything
        // Mouse unpressed: show hover if in bounds
        if (mouseButtonPressed(MouseButton::Left)
            && isMouseInUIBounds(widgetBounds, mouseClickStartPos)) {
            result.isPressed = true;
        }
    }

    return result;
}

bool isDragging(void* object)
{
    return (object == uiState.currentDragObject);
}

void startDragging(void* object, V2I objectPos)
{
    uiState.currentDragObject = object;
    uiState.dragObjectStartPos = objectPos;
}

V2I getDraggingObjectPos()
{
    return uiState.dragObjectStartPos + (mousePos - mouseClickStartPos);
}

void pushInputScissorRect(Rect2I bounds)
{
    push(&uiState.inputScissorRects, bounds);
}

void popInputScissorRect()
{
    (void)pop(&uiState.inputScissorRects);
}

bool isInputScissorActive()
{
    return !isEmpty(&uiState.inputScissorRects);
}

Rect2I getInputScissorRect()
{
    if (isInputScissorActive())
        return *peek(&uiState.inputScissorRects);

    return Rect2I::create_infinity();
}

void addUIRect(Rect2I bounds)
{
    uiState.uiRects.append(bounds);
}

bool mouseIsWithinUIRects()
{
    bool result = false;

    for (auto it = uiState.uiRects.iterate(); it.hasNext(); it.next()) {
        if (it.getValue().contains(mousePos)) {
            result = true;
            break;
        }
    }

    return result;
}

V2I calculateButtonSize(StringView text, ButtonStyle* style, s32 maxWidth, bool fillWidth)
{
    // If we have icons, take them into account
    V2I startIconSize = style->startIcon.getSize();
    if (startIconSize.x > 0)
        startIconSize.x += style->contentPadding;

    V2I endIconSize = style->endIcon.getSize();
    if (endIconSize.x > 0)
        endIconSize.x += style->contentPadding;

    s32 textMaxWidth = 0;
    if (maxWidth != 0) {
        textMaxWidth = maxWidth - (style->padding.left + style->padding.right) - startIconSize.x - endIconSize.x;
    }

    V2I result = {};
    BitmapFont* font = getFont(style->font);

    if (textMaxWidth < 0) {
        // ERROR! Negative text width means we can't fit any so give up.
        // (NB: 0 means "whatever", so we only worry if it's less than that.)
        DEBUG_BREAK();

        result.x = maxWidth;
        result.y = font->lineHeight;
    } else {
        V2I textSize = calculateTextSize(font, text, textMaxWidth);

        s32 resultWidth = 0;

        if (fillWidth && (maxWidth > 0)) {
            resultWidth = maxWidth;
        } else {
            resultWidth = (textSize.x + startIconSize.x + endIconSize.x + style->padding.left + style->padding.right);
        }

        result = v2i(resultWidth, max(textSize.y, startIconSize.y, endIconSize.y) + (style->padding.top + style->padding.bottom));
    }

    return result;
}

V2I calculateButtonSize(V2I contentSize, ButtonStyle* style, s32 maxWidth, bool fillWidth)
{
    // If we have icons, take them into account
    V2I startIconSize = style->startIcon.getSize();
    if (startIconSize.x > 0)
        startIconSize.x += style->contentPadding;

    V2I endIconSize = style->endIcon.getSize();
    if (endIconSize.x > 0)
        endIconSize.x += style->contentPadding;

    V2I result = {};

    if (fillWidth && (maxWidth > 0)) {
        result.x = maxWidth;
    } else {
        result.x = (contentSize.x + style->padding.left + style->padding.right + startIconSize.x + endIconSize.x);
    }

    result.y = max(contentSize.y, startIconSize.y, endIconSize.y) + (style->padding.top + style->padding.bottom);

    return result;
}

Rect2I calculateButtonContentBounds(Rect2I bounds, ButtonStyle* style)
{
    Rect2I contentBounds = bounds.shrunk(style->padding);

    V2I startIconSize = style->startIcon.getSize();
    if (startIconSize.x > 0) {
        contentBounds.set_x(contentBounds.x() + startIconSize.x + style->contentPadding);
        contentBounds.set_width(contentBounds.width() - (startIconSize.x + style->contentPadding));
    }

    V2I endIconSize = style->endIcon.getSize();
    if (endIconSize.x > 0) {
        contentBounds.set_width(contentBounds.width() - (endIconSize.x + style->contentPadding));
    }

    return contentBounds;
}

bool putButton(Rect2I bounds, ButtonStyle* style, ButtonState state, RenderBuffer* renderBuffer, String tooltip)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    if (style == nullptr)
        style = getStyle<ButtonStyle>("default"_s);
    if (renderBuffer == nullptr)
        renderBuffer = &the_renderer().ui_buffer();

    bool buttonClicked = false;

    WidgetMouseState mouseState = getWidgetMouseState(bounds);

    DrawableStyle* backgroundStyle = &style->background;

    if (state == ButtonState::Disabled) {
        backgroundStyle = &style->backgroundDisabled;
    } else if (mouseState.isHovered) {
        if (mouseState.isPressed) {
            backgroundStyle = &style->backgroundPressed;
        } else {
            backgroundStyle = &style->backgroundHover;
        }

        if (!tooltip.is_empty())
            showTooltip(tooltip);
    } else if (state == ButtonState::Active) {
        backgroundStyle = &style->backgroundHover;
    }

    Drawable buttonBackground = Drawable(backgroundStyle);
    buttonBackground.draw(renderBuffer, bounds);

    // Icons!
    if (style->startIcon.type != DrawableType::None) {
        V2I startIconSize = style->startIcon.getSize();
        Rect2I startIconBounds = bounds.create_aligned_within(startIconSize, style->startIconAlignment, style->padding);

        Drawable(&style->startIcon).draw(renderBuffer, startIconBounds);
    }
    if (style->endIcon.type != DrawableType::None) {
        V2I endIconSize = style->endIcon.getSize();
        Rect2I endIconBounds = bounds.create_aligned_within(endIconSize, style->endIconAlignment, style->padding);

        Drawable(&style->endIcon).draw(renderBuffer, endIconBounds);
    }

    // Respond to click
    if ((state != ButtonState::Disabled) && justClickedOnUI(bounds)) {
        buttonClicked = true;
        markMouseInputHandled();
    }

    return buttonClicked;
}

bool putTextButton(StringView text, Rect2I bounds, ButtonStyle* style, ButtonState state, RenderBuffer* renderBuffer, String tooltip)
{
    auto& renderer = the_renderer();
    if (renderBuffer == nullptr) {
        renderBuffer = &renderer.ui_buffer();
    }

    bool result = putButton(bounds, style, state, renderBuffer, tooltip);

    Rect2I contentBounds = calculateButtonContentBounds(bounds, style);

    drawText(renderBuffer, getFont(style->font), text, contentBounds, style->textAlignment, style->textColor, renderer.shaderIds.text);

    return result;
}

bool putImageButton(Sprite* sprite, Rect2I bounds, ButtonStyle* style, ButtonState state, RenderBuffer* renderBuffer, String tooltip)
{
    auto& renderer = the_renderer();
    if (renderBuffer == nullptr)
        renderBuffer = &renderer.ui_buffer();

    bool result = putButton(bounds, style, state, renderBuffer, tooltip);

    Rect2I contentBounds = calculateButtonContentBounds(bounds, style);
    Rect2 spriteBounds = contentBounds;
    drawSingleSprite(renderBuffer, sprite, spriteBounds, renderer.shaderIds.textured, Colour::white());

    return result;
}

V2I calculateCheckboxSize(CheckboxStyle* style)
{
    V2I result = v2i(
        style->checkSize.x + style->padding.left + style->padding.right,
        style->checkSize.y + style->padding.top + style->padding.bottom);

    return result;
}

void putCheckbox(bool* checked, Rect2I bounds, CheckboxStyle* style, bool isDisabled, RenderBuffer* renderBuffer)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    if (style == nullptr)
        style = getStyle<CheckboxStyle>("default"_s);
    if (renderBuffer == nullptr)
        renderBuffer = &the_renderer().ui_buffer();

    WidgetMouseState mouseState = getWidgetMouseState(bounds);

    DrawableStyle* backgroundStyle = &style->background;
    DrawableStyle* checkStyle = &style->check;

    if (isDisabled) {
        backgroundStyle = &style->backgroundDisabled;
        checkStyle = &style->checkDisabled;
    } else if (mouseState.isHovered) {
        if (mouseState.isPressed) {
            backgroundStyle = &style->backgroundPressed;
            checkStyle = &style->checkPressed;
        } else {
            backgroundStyle = &style->backgroundHover;
            checkStyle = &style->checkHover;
        }
    }

    if (!isDisabled && justClickedOnUI(bounds)) {
        *checked = !(*checked);
        markMouseInputHandled();
    }

    // Render it
    Drawable(backgroundStyle).draw(renderBuffer, bounds);

    if (*checked) {
        Rect2I checkBounds = bounds.shrunk(style->padding);
        Drawable(checkStyle).draw(renderBuffer, checkBounds);
    }
}

void openDropDownList(void* pointer)
{
    uiState.openDropDownList = pointer;
    if (uiState.openDropDownListRenderBuffer == nullptr) {
        uiState.openDropDownListRenderBuffer = the_renderer().get_temporary_render_buffer("DropDownList"_s);
    }
    uiState.openDropDownListRenderBuffer->clear_for_pool();
    initScrollbar(&uiState.openDropDownListScrollbar, Orientation::Vertical);
}

void closeDropDownList()
{
    uiState.openDropDownList = nullptr;
    the_renderer().return_temporary_render_buffer(*uiState.openDropDownListRenderBuffer);
    uiState.openDropDownListRenderBuffer = nullptr;
}

bool isDropDownListOpen(void* pointer)
{
    return (uiState.openDropDownList == pointer);
}

V2I calculateLabelSize(StringView text, LabelStyle* style, s32 maxWidth, bool fillWidth)
{
    if (style == nullptr)
        style = getStyle<LabelStyle>("default"_s);

    s32 maxTextWidth = maxWidth - (style->padding.left + style->padding.right);

    V2I textSize = calculateTextSize(getFont(style->font), text, maxTextWidth);

    // Add padding
    V2I result = v2i(
        textSize.x + style->padding.left + style->padding.right,
        textSize.y + style->padding.top + style->padding.bottom);

    if (fillWidth) {
        result.x = maxWidth;
    }

    return result;
}

void putLabel(StringView text, Rect2I bounds, LabelStyle* style, RenderBuffer* renderBuffer)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    auto& renderer = the_renderer();
    if (style == nullptr)
        style = getStyle<LabelStyle>("default"_s);
    if (renderBuffer == nullptr)
        renderBuffer = &renderer.ui_buffer();

    Rect2I textBounds = bounds.shrunk(style->padding);

    Drawable(&style->background).draw(renderBuffer, bounds);

    drawText(renderBuffer, getFont(style->font), text, textBounds, style->textAlignment, style->textColor, renderer.shaderIds.text);
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
    if (isMenuVisible(menuID)) {
        hideMenus();
    } else {
        showMenu(menuID);
    }
}

bool isMenuVisible(s32 menuID)
{
    return (uiState.openMenu == menuID);
}

ScrollbarState* getMenuScrollbar()
{
    return &uiState.openMenuScrollbar;
}

V2I calculateRadioButtonSize(RadioButtonStyle* style)
{
    if (style == nullptr)
        style = getStyle<RadioButtonStyle>("default"_s);

    V2I result = style->size;

    return result;
}

void putRadioButton(s32* selectedValue, s32 value, Rect2I bounds, RadioButtonStyle* style, bool isDisabled, RenderBuffer* renderBuffer)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    auto& renderer = the_renderer();
    if (style == nullptr)
        style = getStyle<RadioButtonStyle>("default"_s);
    if (renderBuffer == nullptr)
        renderBuffer = &renderer.ui_buffer();

    WidgetMouseState mouseState = getWidgetMouseState(bounds);

    DrawableStyle* backgroundStyle = &style->background;
    DrawableStyle* dotStyle = &style->dot;

    if (isDisabled) {
        backgroundStyle = &style->backgroundDisabled;
        dotStyle = &style->dotDisabled;
    } else if (mouseState.isHovered) {
        if (mouseState.isPressed) {
            backgroundStyle = &style->backgroundPressed;
            dotStyle = &style->dotPressed;
        } else {
            backgroundStyle = &style->backgroundHover;
            dotStyle = &style->dotHover;
        }
    }

    if (!isDisabled && justClickedOnUI(bounds)) {
        *selectedValue = value;
        markMouseInputHandled();
    }

    // Render it
    Drawable(backgroundStyle).draw(renderBuffer, bounds);

    if (*selectedValue == value) {
        Rect2I dotBounds = bounds.create_centred_within(style->dotSize);
        Drawable(dotStyle).draw(renderBuffer, dotBounds);
    }
}

void initScrollbar(ScrollbarState* state, Orientation orientation, s32 mouseWheelStepSize)
{
    *state = {};

    state->orientation = orientation;
    state->mouseWheelStepSize = mouseWheelStepSize;
}

Optional<Rect2I> get_scrollbar_thumb_bounds(ScrollbarState* state, Rect2I scrollbarBounds, ScrollbarStyle* style)
{
    // NB: This algorithm for thumb size came from here: https://ux.stackexchange.com/a/85698
    // (Which is ultimately taken from Microsoft's .NET documentation.)
    // Anyway, it's simple enough, but distinguishes between the size of visible content, and
    // the size of the scrollbar's track. For us, for now, those are the same value, but I'm
    // keeping them as separate variables, which is why viewportSize and trackSize are
    // the same. If we add scrollbar buttons, that'll reduce the track size.
    // - Sam, 01/04/2021
    if (state->orientation == Orientation::Horizontal) {
        s32 thumbHeight = style->width;
        s32 viewportSize = scrollbarBounds.width();

        if (viewportSize < state->contentSize) {
            s32 trackSize = scrollbarBounds.width();
            s32 desiredThumbSize = trackSize * viewportSize / (state->contentSize + viewportSize);
            s32 thumbWidth = clamp(desiredThumbSize, style->width, scrollbarBounds.width());

            s32 thumbPos = round_s32(state->scrollPercent * (scrollbarBounds.width() - thumbWidth));

            return Rect2I { scrollbarBounds.x() + thumbPos, scrollbarBounds.y(), thumbWidth, thumbHeight };
        }
    } else {
        ASSERT(state->orientation == Orientation::Vertical);

        s32 thumbWidth = style->width;
        s32 viewportSize = scrollbarBounds.height();

        if (viewportSize < state->contentSize) {
            s32 trackSize = scrollbarBounds.height();
            s32 desiredThumbSize = trackSize * viewportSize / (state->contentSize + viewportSize);
            s32 thumbHeight = clamp(desiredThumbSize, style->width, scrollbarBounds.height());

            s32 thumbPos = round_s32(state->scrollPercent * (scrollbarBounds.height() - thumbHeight));

            return Rect2I { scrollbarBounds.x(), scrollbarBounds.y() + thumbPos, thumbWidth, thumbHeight };
        }
    }

    return {};
}

void putScrollbar(ScrollbarState* state, s32 contentSize, Rect2I bounds, ScrollbarStyle* style, bool isDisabled, RenderBuffer* renderBuffer)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    ASSERT(bounds.has_positive_area());

    auto& renderer = the_renderer();
    if (style == nullptr)
        style = getStyle<ScrollbarStyle>("default"_s);
    if (renderBuffer == nullptr)
        renderBuffer = &renderer.ui_buffer();

    Drawable(&style->background).draw(renderBuffer, bounds);

    state->contentSize = contentSize;

    // If the content is smaller than the scrollbar, then snap it to position 0 and don't allow interaction.
    if (bounds.height() > state->contentSize) {
        state->scrollPercent = 0.0f;
    } else {
        if (!isMouseInputHandled()) {
            bool isHorizontal = (state->orientation == Orientation::Horizontal);
            ASSERT(isHorizontal || (state->orientation == Orientation::Vertical));

            auto maybe_thumb = get_scrollbar_thumb_bounds(state, bounds, style);
            if (maybe_thumb.has_value()) {
                Rect2I thumb = maybe_thumb.release_value();
                s32 overflowSize = state->contentSize - (isHorizontal ? bounds.width() : bounds.height());
                s32 thumbSize = isHorizontal ? thumb.width() : thumb.height();
                s32 gutterSize = isHorizontal ? bounds.width() : bounds.height();
                s32 thumbRange = gutterSize - thumbSize;

                // Scrollwheel stuff
                // (It's weird that we're putting this within mouseInputHandled, but eh)
                s32 mouseWheelDelta = (isHorizontal ? input_state().wheelX : -input_state().wheelY);
                if (mouseWheelDelta != 0) {
                    s32 oldScrollOffset = getScrollbarContentOffset(state, gutterSize);
                    s32 scrollOffset = oldScrollOffset + (state->mouseWheelStepSize * mouseWheelDelta);

                    state->scrollPercent = clamp01((float)scrollOffset / (float)overflowSize);
                }

                // Mouse stuff
                DrawableStyle* thumbStyle = &style->thumb;
                if (isDisabled) {
                    thumbStyle = &style->thumbDisabled;
                } else if (isDragging(state)) {
                    // Move
                    V2I thumbPos = getDraggingObjectPos();

                    // @Copypasta We duplicate this code below, because there are two states where we need to set
                    // the new thumb position. It's really awkward but I don't know how to pull the logic out.
                    if (isHorizontal) {
                        thumb.set_x(clamp(thumbPos.x, bounds.x(), bounds.x() + thumbRange));
                        state->scrollPercent = clamp01((float)(thumb.x() - bounds.x()) / (float)thumbRange);
                    } else {
                        thumb.set_y(clamp(thumbPos.y, bounds.y(), bounds.y() + thumbRange));
                        state->scrollPercent = clamp01((float)(thumb.y() - bounds.y()) / (float)thumbRange);
                    }

                    thumbStyle = &style->thumbPressed;
                } else if (isMouseInUIBounds(bounds)) {
                    bool inThumbBounds = isMouseInUIBounds(thumb);

                    if (mouseButtonJustPressed(MouseButton::Left)) {
                        // If we're not on the thumb, jump the thumb to where we are!
                        if (!inThumbBounds) {
                            if (isHorizontal) {
                                thumb.set_x(clamp(mousePos.x - (thumb.width() / 2), bounds.x(), bounds.x() + thumbRange));
                                state->scrollPercent = clamp01((float)(thumb.x() - bounds.x()) / (float)thumbRange);
                            } else {
                                thumb.set_y(clamp(mousePos.y - (thumb.height() / 2), bounds.y(), bounds.y() + thumbRange));
                                state->scrollPercent = clamp01((float)(thumb.y() - bounds.y()) / (float)thumbRange);
                            }
                        }

                        // Start drag
                        startDragging(state, thumb.position());

                        thumbStyle = &style->thumbPressed;
                    } else if (inThumbBounds) {
                        // Hovering thumb
                        thumbStyle = &style->thumbHover;
                    }
                }

                Drawable(thumbStyle).draw(renderBuffer, thumb);
            }
        }
    }
}

s32 getScrollbarContentOffset(ScrollbarState* state, s32 scrollbarSize)
{
    s32 overflowSize = state->contentSize - scrollbarSize;

    s32 result = round_s32(state->scrollPercent * overflowSize);

    return result;
}

V2I calculateSliderSize(Orientation orientation, SliderStyle* style, V2I availableSpace, bool fillSpace)
{
    if (style == nullptr)
        style = getStyle<SliderStyle>("default"_s);

    V2I result = {};

    // This is really arbitrary, but sliders don't have an inherent length, so they need something!
    s32 standardSize = 200;

    if (orientation == Orientation::Horizontal) {
        result = v2i(fillSpace ? availableSpace.x : standardSize, style->thumbSize.y);
    } else {
        ASSERT(orientation == Orientation::Vertical);

        result = v2i(style->thumbSize.x, fillSpace ? availableSpace.y : standardSize);
    }

    return result;
}

void putSlider(float* currentValue, float minValue, float maxValue, Orientation orientation, Rect2I bounds, SliderStyle* style, bool isDisabled, RenderBuffer* renderBuffer, bool snapToWholeNumbers)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);
    ASSERT(maxValue > minValue);

    auto& renderer = the_renderer();
    if (style == nullptr)
        style = getStyle<SliderStyle>("default"_s);
    if (renderBuffer == nullptr)
        renderBuffer = &renderer.ui_buffer();

    // Value ranges
    *currentValue = clamp(*currentValue, minValue, maxValue);
    float valueRange = maxValue - minValue;
    float currentPercent = (*currentValue - minValue) / valueRange;

    // Calculate where the thumb is initially
    s32 travel; // Space available for the thumb to move in
    V2I thumbPos;
    if (orientation == Orientation::Horizontal) {
        travel = (bounds.width() - style->thumbSize.x);
        thumbPos.x = bounds.x() + round_s32((float)travel * currentPercent);
        thumbPos.y = bounds.y() + ((bounds.height() - style->thumbSize.y) / 2);
    } else {
        ASSERT(orientation == Orientation::Vertical);

        travel = (bounds.height() - style->thumbSize.y);
        thumbPos.x = bounds.x() + ((bounds.width() - style->thumbSize.x) / 2);
        thumbPos.y = bounds.y() + bounds.height() - style->thumbSize.y - round_s32((float)travel * currentPercent);
    }
    Rect2I thumbBounds { thumbPos, style->thumbSize };

    // Interact with mouse
    DrawableStyle* thumbStyle = &style->thumb;
    if (isDisabled) {
        thumbStyle = &style->thumbDisabled;
    } else if (isDragging(currentValue)) {
        // Move
        V2I draggedPos = getDraggingObjectPos();
        float positionPercent;
        if (orientation == Orientation::Horizontal) {
            thumbBounds.set_x(clamp(draggedPos.x, bounds.x(), bounds.x() + travel));
            positionPercent = (float)(thumbBounds.x() - bounds.x()) / (float)travel;
        } else {
            ASSERT(orientation == Orientation::Vertical);

            thumbBounds.set_y(clamp(draggedPos.y, bounds.y(), bounds.y() + travel));
            positionPercent = 1.0f - (float)(thumbBounds.y() - bounds.y()) / (float)travel;
        }

        // Apply that to the currentValue
        *currentValue = minValue + (positionPercent * valueRange);

        thumbStyle = &style->thumbPressed;
    } else if (isMouseInUIBounds(bounds)) {
        bool inThumbBounds = isMouseInUIBounds(thumbBounds);

        if (mouseButtonJustPressed(MouseButton::Left)) {
            // If we're not on the thumb, jump the thumb to where we are!
            if (!inThumbBounds) {

                float positionPercent;
                if (orientation == Orientation::Horizontal) {
                    thumbBounds.set_x(clamp(mousePos.x - (thumbBounds.width() / 2), bounds.x(), bounds.x() + travel));
                    positionPercent = (float)(thumbBounds.x() - bounds.x()) / (float)travel;
                } else {
                    ASSERT(orientation == Orientation::Vertical);

                    thumbBounds.set_y(clamp(mousePos.y - (thumbBounds.height() / 2), bounds.y(), bounds.y() + travel));
                    positionPercent = 1.0f - (float)(thumbBounds.y() - bounds.y()) / (float)travel;
                }

                // Apply that to the currentValue
                *currentValue = minValue + (positionPercent * valueRange);
            }

            // Start drag
            startDragging(currentValue, thumbBounds.position());

            thumbStyle = &style->thumbPressed;
        } else if (inThumbBounds) {
            // Hovering thumb
            thumbStyle = &style->thumbHover;
        }
    }

    // Snap the thumb (and currentValue) to a position representing an integer value, if that was requested
    // We do this here, to avoid duplicating this in both thumb-movement paths above. It's a quick enough
    // calculation that it shouldn't matter that we're doing it every frame regardless of if the value did
    // change.
    if (snapToWholeNumbers) {
        *currentValue = round_float(*currentValue);
        currentPercent = (*currentValue - minValue) / valueRange;

        if (orientation == Orientation::Horizontal) {
            thumbBounds.set_x(bounds.x() + round_s32((float)travel * currentPercent));
        } else {
            ASSERT(orientation == Orientation::Vertical);

            thumbBounds.set_y(bounds.y() + bounds.height() - thumbBounds.height() - round_s32((float)travel * currentPercent));
        }
    }

    // Draw things
    Rect2I trackBounds;
    if (orientation == Orientation::Horizontal) {
        s32 trackThickness = (style->trackThickness != 0) ? style->trackThickness : bounds.height();
        trackBounds = Rect2I::create_aligned(bounds.x(), bounds.y() + bounds.height() / 2, bounds.width(), trackThickness, { HAlign::Left, VAlign::Centre });
    } else {
        ASSERT(orientation == Orientation::Vertical);

        s32 trackThickness = (style->trackThickness != 0) ? style->trackThickness : bounds.width();
        trackBounds = Rect2I::create_aligned(bounds.x() + bounds.width() / 2, bounds.y(), trackThickness, bounds.height(), { HAlign::Centre, VAlign::Top });
    }

    Drawable(&style->track).draw(renderBuffer, trackBounds);
    Drawable(thumbStyle).draw(renderBuffer, thumbBounds);
}

void putSlider(s32* currentValue, s32 minValue, s32 maxValue, Orientation orientation, Rect2I bounds, SliderStyle* style, bool isDisabled, RenderBuffer* renderBuffer)
{
    float currentValueF = (float)*currentValue;
    float minValueF = (float)minValue;
    float maxValueF = (float)maxValue;

    putSlider(&currentValueF, minValueF, maxValueF, orientation, bounds, style, isDisabled, renderBuffer, true);

    *currentValue = round_s32(currentValueF);
}

bool putTextInput(TextInput* textInput, Rect2I bounds, TextInputStyle* style, RenderBuffer* renderBuffer)
{
    auto& renderer = the_renderer();
    if (style == nullptr)
        style = getStyle<TextInputStyle>("default"_s);
    if (renderBuffer == nullptr)
        renderBuffer = &renderer.ui_buffer();

    bool submittedInput = updateTextInput(textInput);

    drawTextInput(renderBuffer, textInput, style, bounds);

    // Capture the input focus if we just clicked on this TextInput
    if (justClickedOnUI(bounds)) {
        markMouseInputHandled();
        captureInput(textInput);
        textInput->caretFlashCounter = 0;
    }

    return submittedInput;
}

void showTooltip(String text)
{
    uiState.tooltipText = text;
    showTooltip(basicTooltipWindowProc);
}

void showTooltip(WindowProc tooltipProc, void* userData)
{
    static String styleName = "tooltip"_s;
    showWindow(UI::WindowTitle::none(), 300, 100, v2i(0, 0), styleName, WindowFlags::AutomaticHeight | WindowFlags::ShrinkWidth | WindowFlags::Unique | WindowFlags::Tooltip | WindowFlags::Headless, tooltipProc, userData);
}

void basicTooltipWindowProc(WindowContext* context, void* /*userData*/)
{
    Panel* ui = &context->windowPanel;
    ui->addLabel(uiState.tooltipText);
}

}
