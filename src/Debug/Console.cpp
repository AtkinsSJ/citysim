/*
 * Copyright (c) 2017-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Console.h"
#include "../AppState.h"
#include <Assets/AssetManager.h>
#include <Gfx/Renderer.h>
#include <IO/LineReader.h>
#include <UI/Drawable.h>
#include <UI/TextInput.h>
#include <Util/Orientation.h>

static Console theConsole;

void initConsole(MemoryArena* debugArena, float openHeight, float maximisedHeight, float openSpeed)
{
    Console* console = &theConsole;

    // NB: Console->style is initialised later, in updateConsole(), because initConsole() happens
    // before the asset system is initialised!

    console->currentHeight = 0;

    console->openHeight = openHeight;
    console->maximisedHeight = maximisedHeight;
    console->openSpeed = openSpeed;

    console->input = UI::newTextInput(debugArena, consoleLineLength);
    initChunkedArray(&console->inputHistory, debugArena, 256);
    console->inputHistoryCursor = -1;

    initChunkedArray(&console->outputLines, debugArena, 1024);
    UI::initScrollbar(&console->scrollbar, Orientation::Vertical);

    initChunkedArray(&console->commandShortcuts, &AppState::the().systemArena, 64);

    globalConsole = console;
    initCommands(console);
    consoleWriteLine(myprintf("Loaded {} commands. Type 'help' to list them."_s, { formatInt(console->commands.count) }), ConsoleLineStyle::Default);

    consoleWriteLine("GREETINGS PROFESSOR FALKEN.\nWOULD YOU LIKE TO PLAY A GAME?"_s);
}

void updateAndRenderConsole(Console* console)
{
    bool scrollToBottom = false;
    auto& renderer = the_renderer();

    // Late-init the console style
    if (console->style.type != AssetType::ConsoleStyle) {
        console->style = getAssetRef(AssetType::ConsoleStyle, "default"_s);
    }

    // Keyboard shortcuts for commands
    for (auto it = console->commandShortcuts.iterate();
        it.hasNext();
        it.next()) {
        CommandShortcut* shortcut = it.get();

        if (wasShortcutJustPressed(shortcut->shortcut)) {
            consoleHandleCommand(console, shortcut->command);
            scrollToBottom = true;
        }
    }

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

    if (console->currentHeight != console->targetHeight) {
        console->currentHeight = approach(console->currentHeight, console->targetHeight, console->openSpeed * AppState::the().deltaTime);
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
            consoleHandleCommand(console, console->input.toString());
            scrollToBottom = true;
            console->input.clear();
        } else {
            // Tab completion
            if (keyJustPressed(SDLK_TAB)) {
                String wordToComplete = console->input.getLastWord();
                if (!wordToComplete.isEmpty()) {
                    // Search through our commands to find one that matches
                    // Eventually, we might want to cache an array of commands, but that's a low priority for now
                    String completeCommand = nullString;

                    for (auto it = console->commands.iterate();
                        it.hasNext();
                        it.next()) {
                        Command* c = it.get();
                        if (c->name.startsWith(wordToComplete)) {
                            completeCommand = c->name;
                            break;
                        }
                    }

                    if (!completeCommand.isEmpty()) {
                        String completion = makeString(completeCommand.chars + wordToComplete.length, completeCommand.length - wordToComplete.length);
                        console->input.insert(completion);
                    }
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
                    String oldInput = *console->inputHistory.get(console->inputHistoryCursor);
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
                    String oldInput = *console->inputHistory.get(console->inputHistoryCursor);
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

        UI::ConsoleStyle* consoleStyle = getStyle<UI::ConsoleStyle>(&console->style);
        UI::ScrollbarStyle* scrollbarStyle = getStyle<UI::ScrollbarStyle>(&consoleStyle->scrollbarStyle);
        UI::TextInputStyle* textInputStyle = getStyle<UI::TextInputStyle>(&consoleStyle->textInputStyle);

        // Text input
        V2I textInputSize = UI::calculateTextInputSize(&console->input, textInputStyle, screenWidth);
        V2I textInputPos = v2i(0, actualConsoleHeight - textInputSize.y);
        Rect2I textInputBounds = irectPosSize(textInputPos, textInputSize);
        UI::drawTextInput(renderBuffer, &console->input, textInputStyle, textInputBounds);

        // Output area
        V2I textPos = v2i(consoleStyle->padding.left, textInputBounds.y);
        s32 textMaxWidth = screenWidth - (consoleStyle->padding.left + consoleStyle->padding.right);
        s32 heightOfOutputArea = textPos.y;
        Rect2I consoleBackRect = irectXYWH(0, 0, screenWidth, heightOfOutputArea);
        UI::Drawable(&consoleStyle->background).draw(renderBuffer, consoleBackRect);

        // Scrollbar
        Rect2I scrollbarBounds = getConsoleScrollbarBounds(console);
        if (hasPositiveArea(scrollbarBounds)) {
            s32 fontLineHeight = getFont(&consoleStyle->font)->lineHeight;
            s32 contentHeight = ((console->outputLines.count - 1) * fontLineHeight) + scrollbarBounds.h;
            if (scrollToBottom) {
                console->scrollbar.scrollPercent = 1.0f;
            }

            console->scrollbar.mouseWheelStepSize = 3 * fontLineHeight;

            UI::putScrollbar(&console->scrollbar, contentHeight, scrollbarBounds, scrollbarStyle, false, renderBuffer);
        }

        textPos.y -= consoleStyle->padding.bottom;

        // print output lines
        BitmapFont* consoleFont = getFont(&consoleStyle->font);
        s32 scrollLinePos = clamp(floor_s32(console->scrollbar.scrollPercent * console->outputLines.count), 0, console->outputLines.count - 1);
        Alignment outputLinesAlign { HAlign::Left, VAlign::Bottom };
        for (auto it = console->outputLines.iterate(scrollLinePos, false, true);
            it.hasNext();
            it.next()) {
            ConsoleOutputLine* line = it.get();
            auto outputTextColor = consoleStyle->outputTextColors[line->style];

            V2I textSize = calculateTextSize(consoleFont, line->text, textMaxWidth);
            Rect2I textBounds = irectAligned(textPos, textSize, outputLinesAlign);
            drawText(renderBuffer, consoleFont, line->text, textBounds, outputLinesAlign, outputTextColor, renderer.shaderIds.text);
            textPos.y -= (textSize.y + consoleStyle->contentPadding);

            // If we've gone off the screen, stop!
            if ((textPos.y < 0) || (textPos.y > UI::windowSize.y)) {
                break;
            }
        }
    }
}

void loadConsoleKeyboardShortcuts(Console* console, Blob data, String filename)
{
    LineReader reader = readLines(filename, data);

    console->commandShortcuts.clear();

    while (loadNextLine(&reader)) {
        String shortcutString = readToken(&reader);
        String command = getRemainderOfLine(&reader);

        KeyboardShortcut shortcut = parseKeyboardShortcut(shortcutString);
        if (shortcut.key == SDLK_UNKNOWN) {
            error(&reader, "Unrecognised key in keyboard shortcut sequence '{0}'"_s, { shortcutString });
        } else {
            CommandShortcut* commandShortcut = console->commandShortcuts.appendBlank();
            commandShortcut->shortcut = shortcut;
            commandShortcut->command = command;
        }
    }
}

void consoleHandleCommand(Console* console, String commandInput)
{
    // copy input to output, for readability
    consoleWriteLine(myprintf("> {0}"_s, { commandInput }), ConsoleLineStyle::InputEcho);

    if (!isEmpty(commandInput)) {
        // Add to history
        console->inputHistory.append(pushString(console->inputHistory.memoryArena, commandInput));
        console->inputHistoryCursor = -1;

        s32 tokenCount = countTokens(commandInput);
        if (tokenCount > 0) {
            String arguments;
            String firstToken = nextToken(commandInput, &arguments);

            // Find the command
            Command* command = nullptr;
            for (auto it = console->commands.iterate();
                it.hasNext();
                it.next()) {
                Command* c = it.get();
                if (c->name == firstToken) {
                    command = c;
                    break;
                }
            }

            // Run the command
            if (command != nullptr) {
                s32 argCount = tokenCount - 1;
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
                    command->function(console, argCount, arguments);
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
        line->text = pushString(globalConsole->outputLines.memoryArena, text);
    }
}

Rect2I getConsoleScrollbarBounds(Console* console)
{
    auto& renderer = the_renderer();
    UI::ConsoleStyle* consoleStyle = getStyle<UI::ConsoleStyle>(&console->style);
    UI::ScrollbarStyle* scrollbarStyle = getStyle<UI::ScrollbarStyle>(&consoleStyle->scrollbarStyle);
    UI::TextInputStyle* textInputStyle = getStyle<UI::TextInputStyle>(&consoleStyle->textInputStyle);

    V2I textInputSize = UI::calculateTextInputSize(&console->input, textInputStyle, renderer.window_width());

    Rect2I scrollbarBounds = irectXYWH(renderer.window_width() - scrollbarStyle->width, 0, scrollbarStyle->width, floor_s32(console->currentHeight * renderer.window_height()) - textInputSize.y);

    return scrollbarBounds;
}
