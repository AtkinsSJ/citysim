/*
 * Copyright (c) 2017-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Console.h"
#include <App/App.h>
#include <Assets/AssetManager.h>
#include <Debug/Keymap.h>
#include <Gfx/BitmapFont.h>
#include <Gfx/Renderer.h>
#include <IO/LineReader.h>
#include <Input/KeyboardShortcut.h>
#include <Settings/Settings.h>
#include <UI/Drawable.h>
#include <UI/TextInput.h>
#include <UI/Toast.h>
#include <Util/Orientation.h>
#include <Util/TokenReader.h>

static Console theConsole;

/*
 * Our custom logger posts the message into the built-in console, AND then sends it off to SDL's
 * usual logger. This way it's still useful even if our console is broken. (Which will happen!)
 */

static SDL_LogOutputFunction default_logger;
static void* default_logger_user_data;

void console_log_output_function(void* /*userdata*/, int category, SDL_LogPriority priority, char const* message)
{
    default_logger(default_logger_user_data, category, priority, message);

    ConsoleLineStyle style = ConsoleLineStyle::Default;

    switch (priority) {
    case SDL_LOG_PRIORITY_WARN:
        style = ConsoleLineStyle::Warning;
        break;

    case SDL_LOG_PRIORITY_ERROR:
    case SDL_LOG_PRIORITY_CRITICAL:
        style = ConsoleLineStyle::Error;
        break;

    default:
        style = ConsoleLineStyle::Default;
        break;
    }

    consoleWriteLine(String::from_null_terminated(message), style);
}

void initConsole(MemoryArena* debugArena, float openHeight, float maximisedHeight, float openSpeed)
{
    Console* console = &theConsole;

    console->currentHeight = 0;

    console->openHeight = openHeight;
    console->maximisedHeight = maximisedHeight;
    console->openSpeed = openSpeed;

    console->input = UI::newTextInput(debugArena, consoleLineLength);
    console->inputHistory = ChunkedArray<String>(*debugArena, 256);
    console->inputHistoryCursor = -1;

    console->outputLines = ChunkedArray<ConsoleOutputLine>(*debugArena, 1024);

    console->commandShortcuts = ChunkedArray<CommandShortcut>(*debugArena, 64);
    console->register_default_commands();

    globalConsole = console;
    SDL_LogGetOutputFunction(&default_logger, &default_logger_user_data);
    SDL_LogSetOutputFunction(&console_log_output_function, nullptr);
}

void updateAndRenderConsole(Console* console)
{
    bool scrollToBottom = false;
    auto& renderer = the_renderer();

    if (!isInputCaptured()) {
        // Keyboard shortcuts for commands
        for (auto it = console->commandShortcuts.iterate();
            it.hasNext();
            it.next()) {
            auto& shortcut = it.get();

            if (shortcut.shortcut.was_just_pressed()) {
                consoleHandleCommand(console, shortcut.command);
                scrollToBottom = true;
            }
        }
    }

    if (!isInputCaptured() || hasCapturedInput(&console->input)) {
        // Show/hide the console
        if (keyJustPressed(SDLK_BACKQUOTE)) {
            if (modifierKeyIsPressed(ModifierKey::Ctrl)) {
                if (console->targetHeight == console->maximisedHeight) {
                    console->targetHeight = console->openHeight;
                } else {
                    console->targetHeight = console->maximisedHeight;
                }
                scrollToBottom = true;
            } else {
                if (console->targetHeight > 0) {
                    console->targetHeight = 0;
                } else {
                    console->targetHeight = console->openHeight;
                    scrollToBottom = true;
                }
            }
        }
    }

    if (console->currentHeight != console->targetHeight) {
        console->currentHeight = approach(console->currentHeight, console->targetHeight, console->openSpeed * App::the().delta_time());
    }

    // This is a little hacky... I think we want the console to ALWAYS consume input if it is open.
    // Other things can't take control from it.
    // Maybe this is a terrible idea? Not sure, but we'll see.
    // - Sam, 16/01/2020
    if (console->currentHeight > 0) {
        captureInput(&console->input);
    } else {
        releaseInput(&console->input);
    }

    if (hasCapturedInput(&console->input)) {
        if (UI::updateTextInput(&console->input)) {
            consoleHandleCommand(console, console->input.text());
            scrollToBottom = true;
            console->input.clear();
        } else {
            // Tab completion
            // FIXME: Move this behaviour into TextInput?
            if (keyJustPressed(SDLK_TAB)) {
                auto word_to_complete = console->input.last_word();
                if (!word_to_complete.is_empty()) {
                    // Search through our commands to find one that matches
                    // Eventually, we might want to cache an array of commands, but that's a low priority for now
                    Optional<StringView> command_to_complete;

                    for (auto& [name, command] : console->commands) {
                        if (name.starts_with(word_to_complete)) {
                            command_to_complete = name.view();
                            break;
                        }
                    }

                    if (command_to_complete.has_value())
                        console->input.insert(command_to_complete.value().substring(word_to_complete.length()));
                }
            }
            // Command history
            else if (keyJustPressed(SDLK_UP)) {
                if (console->inputHistory.count > 0) {
                    if (console->inputHistoryCursor == -1) {
                        console->inputHistoryCursor = truncate32(console->inputHistory.count - 1);
                    } else {
                        console->inputHistoryCursor = max(console->inputHistoryCursor - 1, 0);
                    }

                    console->input.clear();
                    String const& oldInput = console->inputHistory.get(console->inputHistoryCursor);
                    console->input.append(oldInput);
                }
            }
            // Command history
            else if (keyJustPressed(SDLK_DOWN)) {
                if (console->inputHistory.count > 0) {
                    if (console->inputHistoryCursor == -1) {
                        console->inputHistoryCursor = truncate32(console->inputHistory.count - 1);
                    } else {
                        console->inputHistoryCursor = truncate32(min(console->inputHistoryCursor + 1, (s32)console->inputHistory.count - 1));
                    }

                    console->input.clear();
                    String const& oldInput = console->inputHistory.get(console->inputHistoryCursor);
                    console->input.append(oldInput);
                }
            }
        }
    }

    // Display the console
    if (console->currentHeight > 0) {
        RenderBuffer* renderBuffer = &renderer.debug_buffer();
        s32 actualConsoleHeight = floor_s32(console->currentHeight * (float)UI::windowSize.y);
        s32 screenWidth = renderer.window_width();

        UI::ConsoleStyle& consoleStyle = console->style.get();
        UI::ScrollbarStyle& scrollbarStyle = consoleStyle.scrollbarStyle.get();
        UI::TextInputStyle& textInputStyle = consoleStyle.textInputStyle.get();

        // Text input
        V2I textInputSize = UI::calculateTextInputSize(&console->input, &textInputStyle, screenWidth);
        V2I textInputPos = v2i(0, actualConsoleHeight - textInputSize.y);
        Rect2I textInputBounds { textInputPos, textInputSize };
        UI::drawTextInput(renderBuffer, &console->input, &textInputStyle, textInputBounds);

        // Output area
        V2I textPos = v2i(consoleStyle.padding.left, textInputBounds.y());
        s32 textMaxWidth = screenWidth - (consoleStyle.padding.left + consoleStyle.padding.right);
        s32 heightOfOutputArea = textPos.y;
        Rect2I consoleBackRect { 0, 0, screenWidth, heightOfOutputArea };
        UI::Drawable(&consoleStyle.background).draw(renderBuffer, consoleBackRect);

        // Scrollbar
        Rect2I scrollbarBounds = getConsoleScrollbarBounds(console);
        if (scrollbarBounds.has_positive_area()) {
            s32 fontLineHeight = consoleStyle.font.get().line_height();
            s32 contentHeight = ((console->outputLines.count - 1) * fontLineHeight) + scrollbarBounds.height();
            if (scrollToBottom) {
                console->scrollbar.scroll_to_bottom();
            }

            console->scrollbar.set_mouse_wheel_step_size(3 * fontLineHeight);

            console->scrollbar.place(contentHeight, scrollbarBounds, &scrollbarStyle, false, renderBuffer);
        }

        textPos.y -= consoleStyle.padding.bottom;

        // print output lines
        auto& consoleFont = consoleStyle.font.get();
        s32 scrollLinePos = clamp(floor_s32(console->scrollbar.scroll_percent() * console->outputLines.count), 0, console->outputLines.count - 1);
        Alignment outputLinesAlign { HAlign::Left, VAlign::Bottom };
        for (auto it = console->outputLines.iterate(scrollLinePos, false, true);
            it.hasNext();
            it.next()) {
            auto& line = it.get();
            auto outputTextColor = consoleStyle.outputTextColors[line.style];

            V2I textSize = consoleFont.calculate_text_size(line.text, textMaxWidth);
            Rect2I textBounds = Rect2I::create_aligned(textPos, textSize, outputLinesAlign);
            drawText(renderBuffer, &consoleFont, line.text, textBounds, outputLinesAlign, outputTextColor, renderer.shaderIds.text);
            textPos.y -= (textSize.y + consoleStyle.contentPadding);

            // If we've gone off the screen, stop!
            if ((textPos.y < 0) || (textPos.y > UI::windowSize.y)) {
                break;
            }
        }
    }
}

void loadConsoleKeyboardShortcuts(Console* console, Blob data, String filename)
{
    LineReader reader { filename, data };

    console->commandShortcuts.clear();

    while (reader.load_next_line()) {
        auto shortcut_string = reader.next_token();
        if (!shortcut_string.has_value())
            continue;
        auto command = reader.remainder_of_current_line();

        if (auto shortcut = KeyboardShortcut::from_string(shortcut_string.value()); shortcut.has_value()) {
            console->commandShortcuts.append({
                .shortcut = shortcut.release_value(),
                .command = command,
            });
        } else {
            reader.error("Unrecognised key in keyboard shortcut sequence '{0}'"_s, { shortcut_string.value() });
        }
    }
}

void consoleHandleCommand(Console* console, StringView commandInput)
{
    // copy input to output, for readability
    consoleWriteLine(myprintf("> {0}"_s, { commandInput }), ConsoleLineStyle::InputEcho);

    if (!commandInput.is_empty()) {
        // Add to history
        console->inputHistory.append(console->inputHistory.memoryArena->allocate_string(commandInput));
        console->inputHistoryCursor = -1;

        TokenReader tokens { commandInput };
        if (auto token_count = tokens.remaining_token_count(); token_count > 0) {
            auto firstToken = tokens.next_token().release_value();

            // Find the command
            auto command = console->commands.get(firstToken.deprecated_to_string());

            // Run the command
            if (command.has_value()) {
                auto argCount = token_count - 1;
                bool tooManyArgs = (argCount > command->maxArgs) && (command->maxArgs != -1);
                if ((argCount < command->minArgs) || tooManyArgs) {
                    if (command->minArgs == command->maxArgs) {
                        consoleWriteLine(myprintf("Command '{0}' requires exactly {1} argument(s), but {2} given."_s,
                                             { firstToken, formatInt(command->minArgs), formatInt(argCount) }),
                            ConsoleLineStyle::Error);
                    } else if (command->maxArgs == -1) {
                        consoleWriteLine(myprintf("Command '{0}' requires at least {1} argument(s), but {2} given."_s,
                                             { firstToken, formatInt(command->minArgs), formatInt(argCount) }),
                            ConsoleLineStyle::Error);
                    } else {
                        consoleWriteLine(myprintf("Command '{0}' requires between {1} and {2} arguments, but {3} given."_s,
                                             { firstToken, formatInt(command->minArgs), formatInt(command->maxArgs), formatInt(argCount) }),
                            ConsoleLineStyle::Error);
                    }
                } else {
                    u32 commandStartTime = SDL_GetTicks();
                    command->function(console, argCount, tokens.remaining_input());
                    u32 commandEndTime = SDL_GetTicks();

                    consoleWriteLine(myprintf("Command executed in {0}ms"_s, { formatInt(commandEndTime - commandStartTime) }));
                }
            } else {
                consoleWriteLine(myprintf("I don't understand '{0}'. Try 'help' for a list of commands."_s, { firstToken }), ConsoleLineStyle::Error);
            }
        }
    }
}

void consoleWriteLine(String text, ConsoleLineStyle style)
{
    if (globalConsole) {
        ConsoleOutputLine* line = globalConsole->outputLines.appendBlank();
        line->style = style;
        line->text = globalConsole->outputLines.memoryArena->allocate_string(text);
    }
}

Rect2I getConsoleScrollbarBounds(Console* console)
{
    auto& renderer = the_renderer();
    UI::ConsoleStyle& consoleStyle = console->style.get();
    UI::ScrollbarStyle& scrollbarStyle = consoleStyle.scrollbarStyle.get();
    UI::TextInputStyle& textInputStyle = consoleStyle.textInputStyle.get();

    V2I textInputSize = UI::calculateTextInputSize(&console->input, &textInputStyle, renderer.window_width());

    Rect2I scrollbarBounds { static_cast<s32>(renderer.window_width() - scrollbarStyle.width), 0, scrollbarStyle.width, floor_s32(console->currentHeight * renderer.window_height()) - textInputSize.y };

    return scrollbarBounds;
}

void Console::register_command(Command&& command)
{
    auto name = command.name;
    commands.set(name, move(command));
}

void Console::after_assets_loaded()
{
    // Load the dev keymap
    auto const& keymap = Keymap::get("dev.keymap"_sv);
    for (auto const& shortcut : keymap.shortcuts())
        commandShortcuts.append(shortcut);
}

void Console::before_assets_unloaded()
{
    // Unload the dev keymap
    commandShortcuts.clear();
}

#define ConsoleCommand(name) static void cmd_##name([[maybe_unused]] Console* console, [[maybe_unused]] s32 argumentsCount, [[maybe_unused]] StringView arguments)
ConsoleCommand(exit)
{
    consoleWriteLine("Quitting game..."_s, ConsoleLineStyle::Success);
    input_state().receivedQuitSignal = true;
}

ConsoleCommand(hello)
{
    consoleWriteLine("Hello human!"_s);
    consoleWriteLine(myprintf("Testing formatInt bases: 10:{0}, 16:{1}, 36:{2}, 8:{3}, 2:{4}"_s, { formatInt(123456, 10), formatInt(123456, 16), formatInt(123456, 36), formatInt(123456, 8), formatInt(123456, 2) }));
}

ConsoleCommand(help)
{
    consoleWriteLine("Available commands are:"_s);

    for (auto& [name, command] : globalConsole->commands)
        consoleWriteLine(myprintf(" - {0}"_s, { name }));
}

ConsoleCommand(reload_assets)
{
    asset_manager().reload();
}

ConsoleCommand(reload_settings)
{
    Settings::the().load();
    Settings::the().apply();
}

ConsoleCommand(setting)
{
    auto& settings = *Settings::the().settings;

    if (argumentsCount == 0) {
        consoleWriteLine("Available settings:"_s, ConsoleLineStyle::Success);
        settings.for_each_setting([](auto& setting) {
            consoleWriteLine(setting.name(), ConsoleLineStyle::Success);
        });
        return;
    }

    // FIXME: This is hacky, surely we can come up with a nice API for this.
    LineReader reader { "console"_s, Blob { static_cast<smm>(arguments.length()), const_cast<u8*>(reinterpret_cast<u8 const*>(arguments.raw_pointer_to_characters())) } };
    reader.load_next_line();

    auto maybe_setting_name = reader.next_token();
    if (!maybe_setting_name.has_value()) {
        consoleWriteLine("Missing setting name."_s, ConsoleLineStyle::Error);
        return;
    }
    auto setting_name = maybe_setting_name.release_value();
    auto maybe_setting = settings.get_setting(setting_name.deprecated_to_string());
    if (!maybe_setting.has_value()) {
        consoleWriteLine(myprintf("Unrecognized setting name '{}'."_s, { setting_name }), ConsoleLineStyle::Error);
        return;
    }
    auto& setting = *maybe_setting.release_value();

    if (argumentsCount == 1) {
        consoleWriteLine(myprintf("{} is {}"_s, { setting_name, setting.serialize_value() }), ConsoleLineStyle::Success);
        return;
    }

    if (setting.set_from_file(reader)) {
        Settings::the().apply();
        consoleWriteLine(myprintf("Set {} to {}"_s, { setting_name, setting.serialize_value() }), ConsoleLineStyle::Success);
    } else {
        consoleWriteLine(myprintf("Unable to set {}: invalid format"_s, { setting_name }), ConsoleLineStyle::Error);
    }
}

ConsoleCommand(speed)
{
    auto& app = App::the();

    if (argumentsCount == 0) {
        consoleWriteLine(myprintf("Current game speed: {0}"_s, { formatFloat(app.speed_multiplier(), 3) }), ConsoleLineStyle::Success);
    } else {
        TokenReader tokens { arguments };
        if (auto speed_multiplier = tokens.next_token().value().to_float(); speed_multiplier.has_value()) {
            float multiplier = speed_multiplier.value();
            app.set_speed_multiplier(multiplier);
            consoleWriteLine(myprintf("Set speed to {0}"_s, { formatFloat(multiplier, 3) }), ConsoleLineStyle::Success);
            return;
        }
        consoleWriteLine("Usage: speed (multiplier), where multiplier is a float, or with no argument to list the current speed"_s, ConsoleLineStyle::Error);
    }
}

ConsoleCommand(toast)
{
    UI::Toast::show(arguments);
}

ConsoleCommand(zoom)
{
    auto& renderer = the_renderer();

    if (argumentsCount == 0) {
        // list the zoom
        float zoom = renderer.world_camera().zoom();
        consoleWriteLine(myprintf("Current zoom is {0}"_s, { formatFloat(zoom, 3) }), ConsoleLineStyle::Success);
    } else if (argumentsCount == 1) {
        // set the zoom
        TokenReader tokens { arguments };
        if (auto requested_zoom = tokens.next_token().value().to_float(); requested_zoom.has_value()) {
            float newZoom = requested_zoom.release_value();
            renderer.world_camera().set_zoom(newZoom);
            consoleWriteLine(myprintf("Set zoom to {0}"_s, { formatFloat(newZoom, 3) }), ConsoleLineStyle::Success);
            return;
        }
        consoleWriteLine("Usage: zoom (scale), where scale is a float, or with no argument to list the current zoom"_s, ConsoleLineStyle::Error);
    }
}
#undef ConsoleCommand

void Console::register_default_commands()
{
    register_command(Command { "help"_s, cmd_help, 0, 0 });
    register_command(Command { "exit"_s, cmd_exit, 0, 0 });
    register_command(Command { "hello"_s, cmd_hello, 0, 0 });
    register_command(Command { "reload_assets"_s, cmd_reload_assets, 0, 0 });
    register_command(Command { "reload_settings"_s, cmd_reload_settings, 0, 0 });
    register_command(Command { "setting"_s, cmd_setting, 0, -1 });
    register_command(Command { "speed"_s, cmd_speed, 0, 1 });
    register_command(Command { "toast"_s, cmd_toast, 1, -1 });
    register_command(Command { "zoom"_s, cmd_zoom, 0, 1 });
}
