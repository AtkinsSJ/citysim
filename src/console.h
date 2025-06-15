/*
 * Copyright (c) 2017-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "input.h"
#include <UI/Forward.h>
#include <UI/TextInput.h>
#include <UI/UI.h>
#include <UI/UITheme.h>
#include <Util/ChunkedArray.h>

struct ConsoleOutputLine {
    String text;
    ConsoleLineStyleID style;
};

struct CommandShortcut {
    KeyboardShortcut shortcut;
    String command;
};

struct Console;

struct Command {
    String name;
    void (*function)(Console*, s32, String);
    s32 minArgs, maxArgs;

    Command(String name, void (*function)(Console*, s32, String), s32 minArgs = 0, s32 maxArgs = 0)
    {
        this->name = name;
        this->function = function;
        this->minArgs = minArgs;
        this->maxArgs = maxArgs;
    }
};

struct Console {
    AssetRef style;

    f32 currentHeight;
    f32 targetHeight;
    f32 openHeight;      // % of screen height
    f32 maximisedHeight; // % of screen height
    f32 openSpeed;       // % per second

    UI::TextInput input;
    ChunkedArray<String> inputHistory;
    s32 inputHistoryCursor;

    ChunkedArray<ConsoleOutputLine> outputLines;
    UI::ScrollbarState scrollbar;

    HashTable<Command> commands;
    ChunkedArray<CommandShortcut> commandShortcuts;
};

inline Console* globalConsole;
s32 const consoleLineLength = 255;

void initConsole(MemoryArena* debugArena, f32 openHeight, f32 maximisedHeight, f32 openSpeed);
void updateAndRenderConsole(Console* console);

void initCommands(Console* console); // Implementation in commands.cpp
void loadConsoleKeyboardShortcuts(Console* console, Blob data, String filename);
void consoleHandleCommand(Console* console, String commandInput);

void consoleWriteLine(String text, ConsoleLineStyleID style = CLS_Default);

// Private
Rect2I getConsoleScrollbarBounds(Console* console);

inline s32 consoleMaxScrollPos(Console* console)
{
    return truncate32(console->outputLines.count - 1);
}
