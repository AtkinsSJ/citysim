/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "binary_file_reader.h"
#include "file.h"
#include <UI/TextInput.h>
#include <UI/UI.h>
#include <Util/ChunkedArray.h>
#include <Util/String.h>
#include <Util/StringTable.h>
#include <Util/Time.h>
#include <Util/Vector.h>

struct SavedGameInfo {
    // File properties
    bool isReadable;
    String shortName; // Has no extension
    String fullPath;
    DateTime saveTime;

    Flags<BinaryFileReader::Problems> problems;

    // City properties
    String cityName;
    String playerName;
    V2I citySize;
    s32 funds;
    s32 population;
};

struct SavedGamesCatalogue {
    // Rather than scan the save files directory every time we need it, we maintain
    // a list of saved games here, which updates when the files change.

    MemoryArena savedGamesArena;
    StringTable stringsTable;

    DirectoryChangeWatchingHandle savedGamesChangeHandle;
    String savedGamesPath;

    ChunkedArray<SavedGameInfo> savedGames;

    String activeSavedGameName; // The one that's currently loaded, so we know it's safe to save over it. (shortName)

    // Fields used by the save/load windows
    String selectedSavedGameName; // (shortName)
    s32 selectedSavedGameIndex;
    UI::TextInput saveGameName;

    UI::ScrollbarState savedGamesListScrollbar;
};

inline SavedGamesCatalogue savedGamesCatalogue;

void initSavedGamesCatalogue();
void updateSavedGamesCatalogue();
void readSavedGamesInfo(SavedGamesCatalogue* catalogue);

bool saveGame(String saveName);
void loadGame(SavedGameInfo* savedGame);
bool deleteSave(SavedGameInfo* savedGame);

void showSaveGameWindow();
void showLoadGameWindow();

// Internal
void savedGamesWindowProc(UI::WindowContext* context, void* userData);
void confirmOverwriteSaveWindowProc(UI::WindowContext* context, void* userData);
void confirmDeleteSaveWindowProc(UI::WindowContext* context, void* userData);
void saveGameWindowOnClose(UI::WindowContext* context, void* userData);
