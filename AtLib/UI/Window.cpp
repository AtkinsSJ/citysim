/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Window.h"
#include <Debug/Debug.h>
#include <Gfx/BitmapFont.h>
#include <Input/Input.h>
#include <Util/String.h>

namespace UI {

static void* drag_object_pointer_for_window(Window& window)
{
    // This is hacky. We assume that the combination of a windowProc and its userData pointers should be unique,
    // and also doesn't collide with some other object we want to drag. The collision chance is very slim though.
    // FIXME: Actually allocate Windows somewhere so they have a unique address.
    return (u8*)window.windowProc + (u64)window.userData;
}

WindowContext::WindowContext(Window* window, WindowStyle* windowStyle, bool hideWidgets, RenderBuffer* renderBuffer)
    : window(window)
    , windowStyle(windowStyle)
    , hideWidgets(hideWidgets)
    , renderBuffer(renderBuffer)
    , windowPanel(
          Rect2I {
              window->area.x(),
              window->area.y() + ((window->flags & WindowFlags::Headless) ? 0 : windowStyle->titleBarHeight),
              window->area.width(),
              (window->flags & WindowFlags::AutomaticHeight) ? 10000 : (window->area.height() - ((window->flags & WindowFlags::Headless) ? 0 : windowStyle->titleBarHeight)),
          },
          &windowStyle->panelStyle.get(),
          ((window->flags & WindowFlags::Tooltip) ? PanelFlags::AllowClickThrough : 0)
              | (hideWidgets ? PanelFlags::HideWidgets : 0),
          renderBuffer)
{
}

WindowTitle WindowTitle::none()
{
    return WindowTitle { Empty {} };
}

WindowTitle WindowTitle::from_text_asset(String asset_id)
{
    return WindowTitle { TextAsset { .asset_id = move(asset_id) } };
}

WindowTitle WindowTitle::from_lambda(Function<String()> lambda)
{
    return WindowTitle { move(lambda) };
}

WindowTitle::WindowTitle(Value&& value)
    : m_value(move(value))
{
}

String WindowTitle::get_string()
{
    return m_value.visit(
        [](Empty const&) { return ""_s; },
        [](TextAsset const& asset) { return getText(asset.asset_id); },
        [](Function<String()>& lambda) { return lambda(); });
}

/**
 * Creates an (in-game) window in the centre of the screen, and puts it in front of all other windows.
 */
void showWindow(WindowTitle title, s32 width, s32 height, V2I position, String style_name, u32 flags, WindowProc window_proc, void* user_data, WindowProc on_close)
{
    if (window_proc == nullptr) {
        logError("showWindow() called with a null WindowProc. That doesn't make sense? Title: {0}"_s, { title.get_string() });
        return;
    }

    // TODO: Have a proper "automatic position" parameter instead.
    if (position.x == 0 && position.y == 0) {
        auto game_window_size = the_renderer().window_size();
        position.x = (game_window_size.x - width) / 2;
        position.y = (game_window_size.y - height) / 2;
    }

    // If the window wants to be unique, then we search for an existing one with the same WindowProc
    if (uiState.openWindows.count > 0 && flags & WindowFlags::Unique) {
        Window* existing_window = nullptr;

        s32 existing_window_index = 0;
        for (auto it = uiState.openWindows.iterate();
            it.hasNext();
            it.next()) {
            auto& old_window = it.get();
            if (old_window.windowProc == window_proc) {
                existing_window = &old_window;
                existing_window_index = it.getIndex();
                break;
            }
        }

        if (existing_window) {
            existing_window->id = uiState.nextWindowID++,
            existing_window->title = title;
            existing_window->flags = flags;
            if (!(flags & WindowFlags::UniqueKeepPosition))
                existing_window->area = { position, v2i(width, height) };
            existing_window->styleName = style_name;
            existing_window->userData = user_data;
            existing_window->onClose = on_close;
            // NB: existing_window->renderBuffer is left as it is.

            uiState.windowsToMakeActive.put(existing_window_index);
            return;
        }
    }

    auto title_string = title.get_string();

    Window new_window = {
        .id = uiState.nextWindowID++,
        .title = title,
        .flags = flags,
        .area = { position, v2i(width, height) },
        .styleName = style_name,
        .windowProc = window_proc,
        .userData = user_data,
        .onClose = on_close,
        .renderBuffer = the_renderer().get_temporary_render_buffer(title_string),
    };
    uiState.openWindows.append(new_window);
    uiState.windowsToMakeActive.put(truncate32(uiState.openWindows.count - 1));
}

bool hasPauseWindowOpen()
{
    return uiState.isAPauseWindowOpen;
}

bool isWindowOpen(WindowProc windowProc)
{
    auto window_to_remove = uiState.openWindows.find_first([&](Window& window) { return window.windowProc == windowProc; });

    return window_to_remove.has_value();
}

void closeWindow(WindowProc windowProc)
{
    auto window_to_remove = uiState.openWindows.find_first([&](Window& window) { return window.windowProc == windowProc; });

    if (window_to_remove.has_value()) {
        uiState.windowsToClose.put(window_to_remove.value().index());
    } else if (!uiState.openWindows.is_empty()) {
        logInfo("closeWindow() call didn't find any windows that matched the WindowProc."_s);
    }
}

void closeAllWindows()
{
    for (s32 windowIndex = 0; windowIndex < uiState.openWindows.count; windowIndex++) {
        uiState.windowsToClose.put(windowIndex);
    }
}

void updateAndRenderWindows()
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);
    auto& renderer = the_renderer();

    // This is weird, the UI camera should always be positioned with 0,0 being the bottom-left I thought?
    Rect2I validWindowArea = Rect2I::create_centre_size(v2i(renderer.ui_camera().position()), v2i(renderer.ui_camera().size()));

    uiState.isAPauseWindowOpen = false;
    s32 tooltipIndex = -1;

    bool isActive = true;
    for (auto it = uiState.openWindows.iterate();
        it.hasNext();
        it.next()) {
        auto& window = it.get();
        s32 windowIndex = it.getIndex();

        // Skip this Window if we've requested to close it.
        if (uiState.windowsToClose.contains(windowIndex))
            continue;

        bool isModal = (window.flags & WindowFlags::Modal) != 0;
        bool hasTitleBar = (window.flags & WindowFlags::Headless) == 0;
        bool isTooltip = (window.flags & WindowFlags::Tooltip) != 0;

        if (isTooltip) {
            ASSERT(tooltipIndex == -1); // Multiple tooltips???
            tooltipIndex = windowIndex;
        }

        bool shrinkWidth = (window.flags & WindowFlags::ShrinkWidth) != 0;
        bool shrinkHeight = (window.flags & WindowFlags::AutomaticHeight) != 0;

        auto& window_style = WindowStyle::get(window.styleName);

        s32 barHeight = hasTitleBar ? window_style.titleBarHeight : 0;

        // Modal windows get a translucent colour over everything behind them
        if (isModal) {
            drawSingleRect(window.renderBuffer, Rect2 { v2(0, 0), renderer.ui_camera().size() }, renderer.shaderIds.untextured, Colour::from_rgb_255(64, 64, 64, 128));
        }

        // If the window is new, make sure it has a valid area by running the WindowProc once
        // (Otherwise, windows may appear in the wrong place or at the wrong size on the frame they are created.)
        if (!window.isInitialised) {
            window.isInitialised = true;

            WindowContext context = WindowContext(&window, &window_style, true, nullptr);
            window.windowProc(&context, window.userData);
            context.windowPanel.end(shrinkHeight, shrinkWidth);

            if (shrinkHeight) {
                window.area.set_height(barHeight + context.windowPanel.bounds.height());
            }

            if (shrinkWidth) {
                window.area.set_width(context.windowPanel.bounds.width());
                window.area.set_x(context.windowPanel.bounds.x());
            }
        }

        // Handle dragging the window
        if (isModal) {
            // Modal windows can't be moved, they just auto-centre
            window.area = validWindowArea.create_centred_within(window.area.size());
        } else if (isDragging(drag_object_pointer_for_window(window))) {
            window.area.set_position(getDraggingObjectPos());
        } else if (isTooltip) {
            window.area.set_position(mousePos + window_style.offsetFromMouse);
        }

        // Keep window on screen
        {
            // X
            if (window.area.width() > validWindowArea.width()) {
                // If it's too big, centre it.
                window.area.set_x(validWindowArea.x() - ((window.area.width() - validWindowArea.width()) / 2));
            } else if (window.area.x() < validWindowArea.x()) {
                window.area.set_x(validWindowArea.x());
            } else if ((window.area.x() + window.area.width()) > (validWindowArea.x() + validWindowArea.width())) {
                window.area.set_x(validWindowArea.x() + validWindowArea.width() - window.area.width());
            }

            // Y
            if (window.area.height() > validWindowArea.height()) {
                // If it's too big, centre it.
                window.area.set_y(validWindowArea.y() - ((window.area.height() - validWindowArea.height()) / 2));
            } else if (window.area.y() < validWindowArea.y()) {
                window.area.set_y(validWindowArea.y());
            } else if ((window.area.y() + window.area.height()) > (validWindowArea.y() + validWindowArea.height())) {
                window.area.set_y(validWindowArea.y() + validWindowArea.height() - window.area.height());
            }
        }

        // Actually run the window proc
        WindowContext context = WindowContext(&window, &window_style, false, window.renderBuffer);
        window.windowProc(&context, window.userData);
        context.windowPanel.end(shrinkHeight, shrinkWidth);

        if (shrinkHeight) {
            window.area.set_height(barHeight + context.windowPanel.bounds.height());
        }

        if (shrinkWidth) {
            window.area.set_width(context.windowPanel.bounds.width());
            window.area.set_x(context.windowPanel.bounds.x());
        }

        if (context.closeRequested) {
            uiState.windowsToClose.put(windowIndex);
        }

        if (!context.closeRequested && ((window.flags & WindowFlags::Pause) != 0)) {
            uiState.isAPauseWindowOpen = true;
        }

        Rect2I wholeWindowArea = window.area;
        Rect2I barArea { wholeWindowArea.x(), wholeWindowArea.y(), wholeWindowArea.width(), barHeight };
        Rect2I closeButtonRect { wholeWindowArea.x() + wholeWindowArea.width() - barHeight, wholeWindowArea.y(), barHeight, barHeight };

        if (hasTitleBar) {
            bool hoveringOverCloseButton = closeButtonRect.contains(mousePos);

            auto barColor = (isActive ? window_style.titleBarColor : window_style.titleBarColorInactive);

            String closeButtonString = "X"_s;
            auto closeButtonColorHover = window_style.titleBarButtonHoverColor;

            drawSingleRect(window.renderBuffer, barArea, renderer.shaderIds.untextured, barColor);
            String titleString = window.title.get_string();

            auto& title_style = window_style.titleLabelStyle.get();
            // TODO: Take close-button size into account
            V2I titleSize = calculateLabelSize(titleString, &title_style, barArea.width(), false);
            putLabel(titleString, barArea.create_aligned_within(titleSize, title_style.textAlignment), &title_style, window.renderBuffer);

            // TODO: Replace this with an actual Button?
            auto& titleFont = title_style.font.get();
            if (hoveringOverCloseButton
                && (!isMouseInputHandled() || windowIndex == 0)) {
                drawSingleRect(window.renderBuffer, closeButtonRect, renderer.shaderIds.untextured, closeButtonColorHover);
            }
            V2I closeTextSize = titleFont.calculate_text_size(closeButtonString);
            Rect2I closeTextBounds = Rect2I::create_aligned(v2i(closeButtonRect.centre()), closeTextSize, Alignment::centre());
            drawText(window.renderBuffer, &titleFont, closeButtonString, closeTextBounds, Alignment::centre(), title_style.textColor, renderer.shaderIds.text);

            if ((!isMouseInputHandled() || windowIndex == 0)
                && wholeWindowArea.contains(mousePos)
                && mouseButtonJustPressed(MouseButton::Left)) {
                if (hoveringOverCloseButton) {
                    // If we're inside the X, close it!
                    uiState.windowsToClose.put(windowIndex);
                } else {
                    if (!isModal && barArea.contains(mousePos)) {
                        // If we're inside the title bar, start dragging!

                        // @Hack: We're pretending the window ID is a pointer, which does work, but could conflict
                        // with anything else that uses an ID instead of a pointer! (Or, I suppose, with real
                        // pointers if they're in low memory or we spawn a ridiculous number of windows.)
                        //
                        // The solution would be to either allocate windows individually, or store their order in
                        // an array that's separate abd just uses pointers.
                        startDragging(drag_object_pointer_for_window(window), window.area.position());
                    }

                    // Make this the active window!
                    uiState.windowsToMakeActive.put(windowIndex);
                }

                // Tooltips don't take mouse input
                if (!isTooltip) {
                    markMouseInputHandled();
                }
            }
        }

        // Prevent anything behind a modal window from interacting with the mouse
        if (isModal) {
            markMouseInputHandled();
        }
        // Prevent anything behind this window from interacting with the mouse
        else if (wholeWindowArea.contains(mousePos)) {
            // Tooltips don't take mouse input
            if (!isTooltip) {
                markMouseInputHandled();
            }
        }

        //
        // NB: This is a little confusing, so some explanation:
        // Tooltips are windows, theoretically the front-most window, because they're shown fresh each frame.
        // We take the front-most window as the active one. Problem is, we don't want the "real" front-most
        // window to appear inactive just because a tooltip is visible. It feels weird and glitchy.
        // So, instead of "isActive = (i == 0)", weset it to true before the loop, and then set it to false
        // the first time we finish a window that wasn't a tooltip, which will be the front-most non-tooltip
        // window!
        // Actually, a similar thing will apply to UI messages once we port those, so I'll have to break it
        // into a separate WindowFlag.
        //
        // - Sam, 02/06/2019
        //
        if (!isTooltip) {
            isActive = false;
        }
    }

    // Put the tooltip on top of everything else
    // FIXME: Actually, this won't do that! windowsToMakeActive is not FIFO! But, it's better than nothing
    if (tooltipIndex != -1) {
        uiState.windowsToMakeActive.put(tooltipIndex);
    }

    auto as_sorted_array = [](HashSet<s32> const& set) -> Array<s32> {
        if (set.is_empty())
            return {};

        auto result = temp_arena().allocate_array<s32>(set.count());

        // Gather
        for (auto it : set)
            result.append(it);

        // Sort
        result.sort([](s32 a, s32 b) { return a < b; });
        return result;
    };

    Array<s32> windowsToMakeActive = as_sorted_array(uiState.windowsToMakeActive);

    // Close any windows that were requested
    if (!uiState.windowsToClose.is_empty()) {
        Array<s32> windowsToClose = as_sorted_array(uiState.windowsToClose);

        for (s32 i = windowsToClose.count() - 1; i >= 0; i--) {
            s32 windowIndex = windowsToClose[i];
            closeWindow(windowIndex);

            // Any indices in windowsToMakeActive that are >windowIndex are now inaccurate.
            // Subtract 1 from them to account for this one being removed from the list.
            for (auto j = 0; j < windowsToMakeActive.count(); ++j) {
                if (windowsToMakeActive[j] > windowIndex)
                    windowsToMakeActive[j]--;
            }
        }

        uiState.windowsToClose.clear();
    }

    // Activate any windows
    if (!uiState.windowsToMakeActive.is_empty()) {
        // NB: Because the windowsToMakeActive are in ascending index order, and we always move them to a lower position,
        // we are guaranteed to not perturb the positions of any later window indices, so this operation is completely
        // safe, and we can do it really simply. Hooray!
        // - Sam, 07/04/2021
        for (auto const& windowIndex : windowsToMakeActive) {
            // Don't do anything if it's already the active window.
            if (windowIndex == 0)
                continue;

            uiState.openWindows.moveItemKeepingOrder(windowIndex, 0);
        }

        uiState.windowsToMakeActive.clear();
    }

    // Now, render the windows in the correct order!
    for (auto it = uiState.openWindows.iterateBackwards();
        it.hasNext();
        it.next()) {
        auto& window = it.get();
        if (window.renderBuffer)
            the_renderer().window_buffer().take_from(*window.renderBuffer);
    }

    // Remove the tooltip now that it's been shown
    if (tooltipIndex != -1) {
        // We moved the tooltip to index 0 when we did the activate-windows loop!
        tooltipIndex = 0;
        ASSERT((uiState.openWindows.get(tooltipIndex).flags & WindowFlags::Tooltip) != 0);
        closeWindow(tooltipIndex);
    }
}

Rect2I getWindowContentArea(Rect2I windowArea, s32 barHeight, s32 margin)
{
    return { windowArea.x() + margin,
        windowArea.y() + barHeight + margin,
        windowArea.width() - (margin * 2),
        windowArea.height() - barHeight - (margin * 2) };
}

void closeWindow(s32 windowIndex)
{
    uiState.windowsToMakeActive.remove(windowIndex);

    auto& window = uiState.openWindows.get(windowIndex);

    if (window.onClose != nullptr) {
        window.onClose(nullptr, window.userData);
    }

    if (window.renderBuffer != nullptr) {
        the_renderer().return_temporary_render_buffer(*window.renderBuffer);
        window.renderBuffer = nullptr;
    }

    uiState.openWindows.take_index(windowIndex, true);
}

}
