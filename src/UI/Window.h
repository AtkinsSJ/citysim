/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <UI/Forward.h>
#include <UI/Panel.h>
#include <UI/UI.h>
#include <Util/Forward.h>

namespace WindowFlags {

enum {
    AutomaticHeight = 1 << 0,    // Height determined by content
    Unique = 1 << 1,             // Only one window with the same WindowProc will be allowed. A new one will replace the old.
    UniqueKeepPosition = 1 << 2, // Requires Unique. Keeps the old window's position and size.
    Modal = 1 << 3,              // Prevent interaction with anything else while open
    Tooltip = 1 << 4,            // Follows the mouse cursor
    Headless = 1 << 5,           // No title bar
    ShrinkWidth = 1 << 6,        // Reduces the width to match the content of the window
    Pause = 1 << 7,              // Pauses the game while open
};

};

namespace UI {

// NB: A WindowContext only exists for a single call of a WindowProc.
// It isn't kept around between update and render, so attempting to save
// state here that you want to carry over from one to the other will not work!!!
// AKA, I messed up.
struct WindowContext {
    WindowContext(Window* window, WindowStyle* windowStyle, bool hideWidgets, RenderBuffer* renderBuffer);

    Window* window;
    WindowStyle* windowStyle;

    bool hideWidgets;
    RenderBuffer* renderBuffer;

    Panel windowPanel;

    // Results
    bool closeRequested = false;
};

struct WindowTitle {
    static WindowTitle none();
    static WindowTitle fromTextAsset(String assetID);
    static WindowTitle fromLambda(String (*lambda)());

    enum class Type {
        None,
        TextAsset,
        Calculated,
    };
    Type type;
    union {
        String assetID;
        String (*calculateTitle)();
    };

    String getString();
};

struct Window {
    s64 id;

    WindowTitle title;
    u32 flags;

    Rect2I area;
    String styleName;

    WindowProc windowProc;
    void* userData;
    WindowProc onClose;

    // Only used temporarily within updateAndRenderWindows()!
    // In user code, use WindowContext::renderBuffer instead
    RenderBuffer* renderBuffer;

    bool isInitialised;
};

// TODO: I don't like the API for this. Ideally we'd just say
// "showWindow(proc)" and it would figure out its size and stuff on its own.
// As it is, the user code has to know way too much about the window. We end
// up creating showXWindow() functions to wrap the showWindow call for each
// window type.
void showWindow(WindowTitle title, s32 width, s32 height, V2I position, String style_name, u32 flags, WindowProc windowProc, void* user_data = nullptr, WindowProc on_close = nullptr);

bool hasPauseWindowOpen();
bool isWindowOpen(WindowProc windowProc);

// Close any open windows that use the given WindowProc.
// Generally, there will only be one window per WindowProc, so that's an easy 1-to-1 mapping.
// If we later want better granularity, I'll have to figure something else out!
void closeWindow(WindowProc windowProc);
void closeAllWindows();

void updateAndRenderWindows();

//
// INTERNAL
//
inline Rect2I getWindowContentArea(Rect2I windowArea, s32 barHeight, s32 contentPadding);
inline void closeWindow(s32 windowIndex);

}
