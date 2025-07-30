/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "saved_games.h"
#include "AppState.h"
#include "input.h"
#include "save_file.h"
#include "settings.h"
#include <UI/Window.h>
#include <Util/Orientation.h>

void initSavedGamesCatalogue()
{
    SavedGamesCatalogue* catalogue = &savedGamesCatalogue;
    *catalogue = {};

    catalogue->savedGamesArena = { "SavedGames"_s };

    initStringTable(&catalogue->stringsTable);

    catalogue->savedGamesPath = intern(&catalogue->stringsTable, constructPath({ getUserDataPath(), "saves"_s }));
    createDirectory(catalogue->savedGamesPath);
    catalogue->savedGamesChangeHandle = beginWatchingDirectory(catalogue->savedGamesPath);

    initChunkedArray(&catalogue->savedGames, &catalogue->savedGamesArena, 64);

    // Window-related stuff
    catalogue->selectedSavedGameName = nullString;
    catalogue->selectedSavedGameIndex = -1;
    catalogue->saveGameName = UI::newTextInput(&catalogue->savedGamesArena, 64, "\\/:*?\"'`<>|[]()^#%&!@+={}~."_s);

    UI::initScrollbar(&catalogue->savedGamesListScrollbar, Orientation::Vertical);

    // Initial saved-games scan
    readSavedGamesInfo(catalogue);
}

void updateSavedGamesCatalogue()
{
    SavedGamesCatalogue* catalogue = &savedGamesCatalogue;

    if (hasDirectoryChanged(&catalogue->savedGamesChangeHandle)) {
        // TODO: @Speed Throwing the list away and starting over is unnecessary - if we could
        // detect which file changed and only update that, it'd be much better!
        // That'd mean using ReadDirectoryChangesW() instead of FindFirstChangeNotification() on win32.
        // We probably want to do that, but for now we'll do the simple solution of using what
        // we already have.
        // - Sam, 13/11/2019

        // Another option would be to scan the file-change times, and only update the ones that
        // changed since the last read. (And also add any ones that were missing, and remove
        // any that have been deleted.) This might not take much effort?
        // - Sam, 25/03/2019

        catalogue->savedGames.clear();
        readSavedGamesInfo(catalogue);
    }
}

void readSavedGamesInfo(SavedGamesCatalogue* catalogue)
{
    // Load the save game metadata
    for (auto it = iterateDirectoryListing(constructPath({ catalogue->savedGamesPath }));
        hasNextFile(&it);
        findNextFile(&it)) {
        FileInfo* fileInfo = getFileInfo(&it);
        if (fileInfo->flags.has(FileFlags::Directory) || fileInfo->flags.has(FileFlags::Hidden))
            continue;

        SavedGameInfo* savedGame = catalogue->savedGames.appendBlank();

        savedGame->shortName = intern(&catalogue->stringsTable, getFileName(fileInfo->filename));
        savedGame->fullPath = intern(&catalogue->stringsTable, constructPath({ catalogue->savedGamesPath, fileInfo->filename }));
        savedGame->isReadable = false;

        FileHandle savedFile = openFile(savedGame->fullPath, FileAccessMode::Read);
        if (savedFile.isOpen) {
            BinaryFileReader reader = readBinaryFile(&savedFile, SAV_FILE_ID, &temp_arena());
            savedGame->problems = reader.problems;

            if (reader.isValidFile) {
                // Read the META section, which is all we care about
                bool readSection = reader.startSection(SAV_META_ID, SAV_META_VERSION);
                if (readSection) {
                    SAVSection_Meta* meta = reader.readStruct<SAVSection_Meta>(0);

                    savedGame->saveTime = getLocalTimeFromTimestamp(meta->saveTimestamp);
                    savedGame->cityName = intern(&catalogue->stringsTable, reader.readString(meta->cityName));
                    savedGame->playerName = intern(&catalogue->stringsTable, reader.readString(meta->playerName));
                    savedGame->citySize = v2i(meta->cityWidth, meta->cityHeight);
                    savedGame->funds = meta->funds;
                    savedGame->population = meta->population;
                    savedGame->isReadable = true;
                }
            }
        }
        closeFile(&savedFile);
    }

    // Sort the saved games by most-recent first
    catalogue->savedGames.sort([](SavedGameInfo* a, SavedGameInfo* b) {
        return a->saveTime.unixTimestamp > b->saveTime.unixTimestamp;
    });
}

void showLoadGameWindow()
{
    SavedGamesCatalogue* catalogue = &savedGamesCatalogue;
    catalogue->selectedSavedGameName = nullString;
    catalogue->selectedSavedGameIndex = -1;

    UI::showWindow(UI::WindowTitle::fromTextAsset("title_load_game"_s), 780, 580, {}, "default"_s, WindowFlags::Unique | WindowFlags::Modal, savedGamesWindowProc);
}

void showSaveGameWindow()
{
    SavedGamesCatalogue* catalogue = &savedGamesCatalogue;

    catalogue->saveGameName.clear();
    captureInput(&catalogue->saveGameName);

    // If we're playing a save file, select that by default
    Indexed<SavedGameInfo*> selectedSavedGame = catalogue->savedGames.findFirst([&](SavedGameInfo* info) {
        return equals(info->shortName, catalogue->activeSavedGameName);
    });
    if (selectedSavedGame.index != -1) {
        catalogue->selectedSavedGameName = catalogue->activeSavedGameName;
        catalogue->selectedSavedGameIndex = selectedSavedGame.index;
        catalogue->saveGameName.append(catalogue->selectedSavedGameName);
    } else {
        catalogue->selectedSavedGameName = nullString;
        catalogue->selectedSavedGameIndex = -1;
    }

    UI::showWindow(UI::WindowTitle::fromTextAsset("title_save_game"_s), 780, 580, {}, "default"_s, WindowFlags::Unique | WindowFlags::Modal, savedGamesWindowProc, (void*)true, saveGameWindowOnClose);
}

void saveGameWindowOnClose(UI::WindowContext* /*context*/, void* /*userData*/)
{
    SavedGamesCatalogue* catalogue = &savedGamesCatalogue;
    releaseInput(&catalogue->saveGameName);
}

void savedGamesWindowProc(UI::WindowContext* context, void* userData)
{
    UI::Panel* ui = &context->windowPanel;
    SavedGamesCatalogue* catalogue = &savedGamesCatalogue;
    bool isSaveWindow = userData != nullptr;

    SavedGameInfo* selectedSavedGame = nullptr;
    bool justClickedSavedGame = false;

    UI::Panel bottomBar = ui->row(26, ALIGN_BOTTOM, "plain"_s);

    UI::Panel savesList = ui->column(320, ALIGN_LEFT, "inset"_s);
    {
        savesList.enableVerticalScrolling(&catalogue->savedGamesListScrollbar);
        savesList.alignWidgets(ALIGN_EXPAND_H);

        if (catalogue->savedGames.isEmpty()) {
            savesList.addLabel(getText("msg_no_saved_games"_s));
        } else {
            // for (s32 duplicates = 0; duplicates < 5; duplicates++)
            for (auto it = catalogue->savedGames.iterate(); it.hasNext(); it.next()) {
                SavedGameInfo* savedGame = it.get();
                s32 index = it.getIndex();

                if (savesList.addTextButton(savedGame->shortName, buttonIsActive(catalogue->selectedSavedGameIndex == index))) {
                    // Select it and show information in the details pane
                    catalogue->selectedSavedGameIndex = index;
                    justClickedSavedGame = true;
                }

                if (catalogue->selectedSavedGameIndex == index) {
                    selectedSavedGame = savedGame;
                }
            }
        }
    }
    savesList.end();

    // Now we have the saved-game info
    if (selectedSavedGame) {
        ui->alignWidgets(ALIGN_RIGHT);
        if (ui->addImageButton(getSprite("icon_delete"_s), ButtonState::Normal, "delete"_s)) {
            UI::showWindow(UI::WindowTitle::fromTextAsset("title_delete_save"_s), 300, 300, v2i(0, 0), "default"_s, WindowFlags::AutomaticHeight | WindowFlags::Modal | WindowFlags::Unique, confirmDeleteSaveWindowProc);
        }

        ui->alignWidgets(ALIGN_EXPAND_H);
        ui->addLabel(selectedSavedGame->shortName);
        ui->addLabel(myprintf("Saved {0}"_s, { formatDateTime(selectedSavedGame->saveTime, DateTimeFormat::LongDateTime) }));
        ui->addLabel(selectedSavedGame->cityName);
        ui->addLabel(myprintf("Mayor {0}"_s, { selectedSavedGame->playerName }));
        ui->addLabel(myprintf("£{0}"_s, { formatInt(selectedSavedGame->funds) }));
        ui->addLabel(myprintf("{0} population"_s, { formatInt(selectedSavedGame->population) }));

        if (selectedSavedGame->problems.has(BinaryFileReader::Problems::VersionTooNew)) {
            ui->addLabel(getText("msg_load_version_too_new"_s));
        }
    }

    // Bottom bar
    {
        if (isSaveWindow) {
            // 'Save' buttons
            bottomBar.alignWidgets(ALIGN_LEFT);
            if (bottomBar.addTextButton(getText("button_back"_s))) {
                context->closeRequested = true;
            }

            bottomBar.addLabel("Save game name:"_s);

            bottomBar.alignWidgets(ALIGN_RIGHT);
            bool pressedSave = bottomBar.addTextButton(getText("button_save"_s), catalogue->saveGameName.isEmpty() ? ButtonState::Disabled : ButtonState::Normal);

            bottomBar.alignWidgets(ALIGN_EXPAND_H);
            if (justClickedSavedGame) {
                catalogue->saveGameName.clear();
                catalogue->saveGameName.append(selectedSavedGame->shortName);
            }

            bool pressedEnterInTextInput = bottomBar.addTextInput(&catalogue->saveGameName);
            String inputName = trim(catalogue->saveGameName.toString());

            // Show a warning if we're overwriting an existing save that ISN'T the active one
            bool showOverwriteWarning = false;
            if (!isEmpty(inputName) && !equals(inputName, catalogue->activeSavedGameName)) {
                Indexed<SavedGameInfo*> fileToOverwrite = catalogue->savedGames.findFirst([&](SavedGameInfo* info) {
                    return equals(inputName, info->shortName);
                });

                if (fileToOverwrite.index != -1) {
                    showOverwriteWarning = true;
                }
            }

            if (pressedSave || pressedEnterInTextInput) {
                if (!isEmpty(inputName)) {
                    if (showOverwriteWarning) {
                        UI::showWindow(UI::WindowTitle::fromTextAsset("title_overwrite_save"_s), 300, 300, v2i(0, 0), "default"_s, WindowFlags::AutomaticHeight | WindowFlags::Modal | WindowFlags::Unique, confirmOverwriteSaveWindowProc);
                    } else if (saveGame(inputName)) {
                        context->closeRequested = true;
                    }
                }
            }
        } else {
            bottomBar.alignWidgets(ALIGN_LEFT);
            if (bottomBar.addTextButton(getText("button_back"_s))) {
                context->closeRequested = true;
            }

            bottomBar.alignWidgets(ALIGN_RIGHT);
            if (bottomBar.addTextButton(getText("button_load"_s), selectedSavedGame ? ButtonState::Normal : ButtonState::Disabled)) {
                loadGame(selectedSavedGame);
                context->closeRequested = true;
            }
        }
    }
    bottomBar.end();
}

void confirmOverwriteSaveWindowProc(UI::WindowContext* context, void* /*userData*/)
{
    UI::Panel* ui = &context->windowPanel;

    String inputName = trim(savedGamesCatalogue.saveGameName.toString());

    ui->addLabel(getText("msg_save_overwrite_confirm"_s, { inputName }));
    ui->startNewLine(ALIGN_RIGHT);

    if (ui->addTextButton(getText("button_overwrite"_s), ButtonState::Normal, "delete"_s)) {
        saveGame(inputName);
        context->closeRequested = true;
    }

    if (ui->addTextButton(getText("button_cancel"_s))) {
        context->closeRequested = true;
    }
}

void confirmDeleteSaveWindowProc(UI::WindowContext* context, void* /*userData*/)
{
    UI::Panel* ui = &context->windowPanel;

    SavedGameInfo* savedGame = savedGamesCatalogue.savedGames.get(savedGamesCatalogue.selectedSavedGameIndex);

    ui->addLabel(getText("msg_delete_save_confirm"_s, { savedGame->shortName }));
    ui->startNewLine(ALIGN_RIGHT);

    if (ui->addTextButton(getText("button_delete"_s), ButtonState::Normal, "delete"_s)) {
        deleteSave(savedGame);
        context->closeRequested = true;
    }

    if (ui->addTextButton(getText("button_cancel"_s))) {
        context->closeRequested = true;
    }
}

void loadGame(SavedGameInfo* savedGame)
{
    auto& app_state = AppState::the();
    if (app_state.gameState != nullptr) {
        freeGameState(app_state.gameState);
    }

    app_state.gameState = newGameState();
    GameState* gameState = app_state.gameState;

    u32 startTicks = SDL_GetTicks();

    FileHandle saveFile = openFile(savedGame->fullPath, FileAccessMode::Read);
    bool loadSucceeded = loadSaveFile(&saveFile, gameState);
    closeFile(&saveFile);

    if (loadSucceeded) {
        u32 endTicks = SDL_GetTicks();
        logInfo("Loaded save '{0}' in {1} milliseconds."_s, { savedGame->shortName, formatInt(endTicks - startTicks) });

        UI::pushToast(getText("msg_load_success"_s, { savedGame->shortName }));

        app_state.appStatus = AppStatus::Game;

        UI::closeAllWindows();

        // Filename is interned so it's safe to copy it
        savedGamesCatalogue.activeSavedGameName = savedGame->shortName;
    } else {
        UI::pushToast(getText("msg_load_failure"_s, { savedGame->shortName }));
    }
}

bool saveGame(String saveName)
{
    SavedGamesCatalogue* catalogue = &savedGamesCatalogue;

    String saveFilename = saveName;
    if (!saveFilename.endsWith(".sav"_s)) {
        saveFilename = concatenate({ saveName, ".sav"_s });
    }

    String savePath = constructPath({ catalogue->savedGamesPath, saveFilename });
    FileHandle saveFile = openFile(savePath, FileAccessMode::Write);
    bool saveSucceeded = writeSaveFile(&saveFile, AppState::the().gameState);
    closeFile(&saveFile);

    if (saveSucceeded) {
        UI::pushToast(getText("msg_save_success"_s, { saveFile.path }));

        // Store that we saved it
        savedGamesCatalogue.activeSavedGameName = intern(&catalogue->stringsTable, saveName);
    } else {
        UI::pushToast(getText("msg_save_failure"_s, { saveFile.path }));
    }

    return saveSucceeded;
}

bool deleteSave(SavedGameInfo* savedGame)
{
    bool success = deleteFile(savedGame->fullPath);

    if (success) {
        UI::pushToast(getText("msg_delete_save_success"_s, { savedGame->shortName }));
    } else {
        UI::pushToast(getText("msg_delete_save_failure"_s, { savedGame->shortName }));
    }

    return success;
}
