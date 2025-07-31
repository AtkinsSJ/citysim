/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "input.h"
#include <Assets/AssetManager.h>
#include <Gfx/Forward.h>
#include <UI/Forward.h>
#include <Util/Array.h>
#include <Util/Basic.h>
#include <Util/Orientation.h>
#include <Util/Queue.h>
#include <Util/Set.h>
#include <Util/Stack.h>
#include <Util/Vector.h>

enum class ButtonState : u8 {
    Normal,
    Disabled,
    Active,
};
inline ButtonState buttonIsActive(bool isActive)
{
    return isActive ? ButtonState::Active : ButtonState::Normal;
}

struct BitmapFont;
struct Sprite;

namespace UI {

// Annoyingly, we have to predeclare all the style types. Yay for C++
// (I tried rearranging our includes, it got messy fast.)
// I guess the alternative is switching to an individual-files build. Might be a good idea.

struct ScrollbarState {
    Orientation orientation;
    s32 contentSize;
    float scrollPercent;

    s32 mouseWheelStepSize;
};

float const TOAST_APPEAR_TIME = 0.2f;
float const TOAST_DISPLAY_TIME = 2.0f;
float const TOAST_DISAPPEAR_TIME = 0.2f;
s32 const MAX_TOAST_LENGTH = 1024;
struct Toast {
    float duration;
    float time; // In seconds, from 0 to duration

    String text;
    char _chars[MAX_TOAST_LENGTH];
};

typedef void (*WindowProc)(WindowContext*, void*);

struct UIState {

    // UI elements that react to the mouse should only do so if this is false - and then
    // they should set it to true.
    bool mouseInputHandled;
    Stack<Rect2I> inputScissorRects;

    // Dragging stuff
    void* currentDragObject;
    V2I dragObjectStartPos;

    String tooltipText;

    // TODO: Replace this with better "this input has already been used" code!
    ChunkedArray<Rect2I> uiRects;

    s32 openMenu;
    ScrollbarState openMenuScrollbar;

    // DropDownList stuff
    void* openDropDownList; // We use the pointer to the options array
    ScrollbarState openDropDownListScrollbar;
    RenderBuffer* openDropDownListRenderBuffer;

    // Toast stuff
    Queue<Toast> toasts;

    // Window stuff
    s32 nextWindowID;
    ChunkedArray<Window> openWindows; // Order: index 0 is the top, then each one is below the previous
    Set<s32> windowsToClose;
    Set<s32> windowsToMakeActive;
    bool isAPauseWindowOpen; // Do any open windows have the WindowFlags::Pause flag?
};
inline UIState uiState;

// Cached input values
// These are updated in startFrame()
inline V2I windowSize;
inline V2I mousePos;
inline V2I mouseClickStartPos;

// METHODS

void init(MemoryArena* arena);
void startFrame();
void endFrame();

// Input
bool isMouseInputHandled();
void markMouseInputHandled();
bool isMouseInUIBounds(Rect2I bounds);
bool isMouseInUIBounds(Rect2I bounds, V2I mousePos);
bool justClickedOnUI(Rect2I bounds);

struct WidgetMouseState {
    bool isHovered;
    bool isPressed;
};
WidgetMouseState getWidgetMouseState(Rect2I bounds);

// Dragging!
// After calling startDragging() with a start position, you can repeatedly call
// getDraggingObjectPos() to find out where the object should be now. You'll have to
// clamp the position yourself before using it.
// The *object pointer can be anything that uniquely identifies what you want to drag.
// (eg, in Sliders, we are using the *currentValue pointer.)
// NB: The drag is automatically ended as soon as MouseButton::Left is released, hence
// there is no stopDragging() function.
bool isDragging(void* object);
void startDragging(void* object, V2I objectPos);
V2I getDraggingObjectPos();

// Input Scissor
void pushInputScissorRect(Rect2I bounds);
void popInputScissorRect();
bool isInputScissorActive();
// NB: This is safe to call whether or not a scissor is active. It returns
// either the scissor or a functionally infinite rectangle.
Rect2I getInputScissorRect();

// UI Rects
// (Rectangle areas which block the mouse from interacting with the game
// world. eg, stops you clicking through windows)
void addUIRect(Rect2I bounds);
bool mouseIsWithinUIRects();

// Buttons
V2I calculateButtonSize(String text, ButtonStyle* style = nullptr, s32 maxWidth = 0, bool fillWidth = true);
V2I calculateButtonSize(V2I contentSize, ButtonStyle* style = nullptr, s32 maxWidth = 0, bool fillWidth = true);
Rect2I calculateButtonContentBounds(Rect2I bounds, ButtonStyle* style = nullptr);
bool putButton(Rect2I bounds, ButtonStyle* style = nullptr, ButtonState state = ButtonState::Normal, RenderBuffer* renderBuffer = nullptr, String tooltip = nullString);
bool putTextButton(String text, Rect2I bounds, ButtonStyle* style = nullptr, ButtonState state = ButtonState::Normal, RenderBuffer* renderBuffer = nullptr, String tooltip = nullString);
bool putImageButton(Sprite* sprite, Rect2I bounds, ButtonStyle* style = nullptr, ButtonState state = ButtonState::Normal, RenderBuffer* renderBuffer = nullptr, String tooltip = nullString);

// Checkboxes
V2I calculateCheckboxSize(CheckboxStyle* style = nullptr);
void putCheckbox(bool* checked, Rect2I bounds, CheckboxStyle* style = nullptr, bool isDisabled = false, RenderBuffer* renderBuffer = nullptr);

// Drop-down lists
#if 0
template<typename T>
V2I calculateDropDownListSize(Array<T>* listOptions, String (*getDisplayName)(T* data), DropDownListStyle* style = nullptr, s32 maxWidth = 0, bool fillWidth = true)
{
    if (style == nullptr)
        style = getStyle<DropDownListStyle>("default"_s);
    ButtonStyle* buttonStyle = getStyle<ButtonStyle>(&style->buttonStyle);

    // I don't have a smarter way to do this, so, we'll just go through every option
    // and find the maximum width and height.
    // This feels really wasteful, but maybe it's ok?
    s32 widest = 0;
    s32 tallest = 0;

    for (s32 optionIndex = 0; optionIndex < listOptions->count; optionIndex++) {
        String optionText = getDisplayName(&listOptions->get(optionIndex));
        V2I optionSize = calculateButtonSize(optionText, buttonStyle, maxWidth, fillWidth);
        widest = max(widest, optionSize.x);
        tallest = max(tallest, optionSize.y);
    }

    return v2i(widest, tallest);
}

void openDropDownList(void* pointer);
void closeDropDownList();
bool isDropDownListOpen(void* pointer);

template<typename T>
void putDropDownList(Array<T>* listOptions, s32* currentSelection, String (*getDisplayName)(T* data), Rect2I bounds, DropDownListStyle* style = nullptr, RenderBuffer* renderBuffer = nullptr)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    if (style == nullptr)
        style = getStyle<DropDownListStyle>("default"_s);
    if (renderBuffer == nullptr)
        renderBuffer = &the_renderer().ui_buffer();

    bool isOpen = isDropDownListOpen(listOptions);

    // We always draw the current-selection box,
    // and then show the panel if needed.

    // Show the selection box
    String selectionText = getDisplayName(&listOptions->get(*currentSelection));
    bool clicked = putTextButton(selectionText, bounds, getStyle<ButtonStyle>(&style->buttonStyle), buttonIsActive(isOpen), renderBuffer);
    if (clicked) {
        // Toggle things
        if (isOpen) {
            isOpen = false;
            closeDropDownList();
        } else {
            isOpen = true;
            openDropDownList(listOptions);
        }
    }

    // Show the panel
    if (isOpen) {
        s32 panelTop = bounds.y + bounds.h;
        s32 panelMaxHeight = windowSize.y - panelTop;
        Rect2I panelBounds = irectXYWH(bounds.x, panelTop, bounds.w, panelMaxHeight);
        Panel panel = Panel(panelBounds, getStyle<PanelStyle>(&style->panelStyle), 0, uiState.openDropDownListRenderBuffer);
        panel.enableVerticalScrolling(&uiState.openDropDownListScrollbar, false);
        for (s32 optionIndex = 0; optionIndex < listOptions->count; optionIndex++) {
            if (panel.addTextButton(getDisplayName(&listOptions->get(optionIndex)), buttonIsActive(optionIndex == *currentSelection))) {
                *currentSelection = optionIndex;

                isOpen = false;
                closeDropDownList();
            }
        }
        panel.end(true);

        // If we clicked somewhere outside of the panel, close it
        if (isOpen && !clicked
            && mouseButtonJustReleased(MouseButton::Left)
            && !panel.bounds.contains(mouseClickStartPos)) {
            closeDropDownList();
        }
    }
}
#endif

// Labels
V2I calculateLabelSize(String text, LabelStyle* style = nullptr, s32 maxWidth = 0, bool fillWidth = true);
void putLabel(String text, Rect2I bounds, LabelStyle* style = nullptr, RenderBuffer* renderBuffer = nullptr);

// Menus
void showMenu(s32 menuID);
void hideMenus();
void toggleMenuVisible(s32 menuID);
bool isMenuVisible(s32 menuID);
ScrollbarState* getMenuScrollbar();

// Radio Buttons
V2I calculateRadioButtonSize(RadioButtonStyle* style = nullptr);
void putRadioButton(s32* selectedValue, s32 value, Rect2I bounds, RadioButtonStyle* style = nullptr, bool isDisabled = false, RenderBuffer* renderBuffer = nullptr);

// Scrollbars
void initScrollbar(ScrollbarState* state, Orientation orientation, s32 mouseWheelStepSize = 64);
// NB: When the viewport is larger than the content, there's no thumb rect so nothing is returned
Maybe<Rect2I> getScrollbarThumbBounds(ScrollbarState* state, Rect2I scrollbarBounds, ScrollbarStyle* style);
void putScrollbar(ScrollbarState* state, s32 contentSize, Rect2I bounds, ScrollbarStyle* style = nullptr, bool isDisabled = false, RenderBuffer* renderBuffer = nullptr);
s32 getScrollbarContentOffset(ScrollbarState* state, s32 scrollbarSize);

// Sliders
// NB: fillSpace only applies to the "length" of the slider, not its "thickness"
V2I calculateSliderSize(Orientation orientation, SliderStyle* style = nullptr, V2I availableSpace = {}, bool fillSpace = false);
void putSlider(float* currentValue, float minValue, float maxValue, Orientation orientation, Rect2I bounds, SliderStyle* style = nullptr, bool isDisabled = false, RenderBuffer* renderBuffer = nullptr, bool snapToWholeNumbers = false);
void putSlider(s32* currentValue, s32 minValue, s32 maxValue, Orientation orientation, Rect2I bounds, SliderStyle* style = nullptr, bool isDisabled = false, RenderBuffer* renderBuffer = nullptr);

// TextInputs
bool putTextInput(TextInput* textInput, Rect2I bounds, TextInputStyle* style = nullptr, RenderBuffer* renderBuffer = nullptr);

// Toasts
// NB: `message` is copied into the UIState, so it can be a temporary allocation
void pushToast(String message);
void drawToast();

// Tooltip
void showTooltip(String text);
void showTooltip(WindowProc tooltipProc, void* userData = nullptr);
void basicTooltipWindowProc(WindowContext* context, void* userData);

}
