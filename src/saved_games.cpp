#pragma once

void initSavedGamesCatalogue()
{
	SavedGamesCatalogue *catalogue = &savedGamesCatalogue;
	*catalogue = {};

	initMemoryArena(&catalogue->savedGamesArena, MB(1));

	initStringTable(&catalogue->stringsTable);

	catalogue->savedGamesPath = intern(&catalogue->stringsTable, constructPath({getUserDataPath(), "saves"_s}));
	createDirectory(catalogue->savedGamesPath);
	catalogue->savedGamesChangeHandle = beginWatchingDirectory(catalogue->savedGamesPath);

	initChunkedArray(&catalogue->savedGames, &catalogue->savedGamesArena, 64);

	// Window-related stuff
	catalogue->selectedSavedGameName = nullString;
	catalogue->selectedSavedGameIndex = -1;
	catalogue->saveGameName = newTextInput(&catalogue->savedGamesArena, 64, "\\/:*?\"'`<>|[]()^#%&!@+={}~."_s);

	// Initial saved-games scan
	readSavedGamesInfo(catalogue);
}

void updateSavedGamesCatalogue()
{
	SavedGamesCatalogue *catalogue = &savedGamesCatalogue;

	if (hasDirectoryChanged(&catalogue->savedGamesChangeHandle))
	{
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

void readSavedGamesInfo(SavedGamesCatalogue *catalogue)
{
	// We picked 4KB somewhat arbitrarily. It should be way more than enough, given that the META
	// chunk comes first, which we require.
	Blob tempBuffer = allocateBlob(tempArena, KB(4));

	for (auto it = iterateDirectoryListing(constructPath({catalogue->savedGamesPath}, true));
		hasNextFile(&it);
		findNextFile(&it))
	{
		FileInfo *fileInfo = getFileInfo(&it);
		if (fileInfo->flags & (FileFlag_Directory | FileFlag_Hidden)) continue;

		SavedGameInfo *savedGame = catalogue->savedGames.appendBlank();

		savedGame->shortName = intern(&catalogue->stringsTable, getFileName(fileInfo->filename));
		savedGame->fullPath = intern(&catalogue->stringsTable, constructPath({catalogue->savedGamesPath, fileInfo->filename}));

		// Now we have to read the file a little to get the META stuff out of it. Hmmmm.

		FileHandle savedFile = openFile(savedGame->fullPath, FileAccess_Read);
		savedGame->isReadable = savedFile.isOpen;
		if (savedGame->isReadable)
		{
			smm sizeRead = readFromFile(&savedFile, tempBuffer.size, tempBuffer.memory);

			u8 *pos = tempBuffer.memory;
			u8 *eof = pos + sizeRead;

			// @Copypasta from loadSaveFile() - might want to extract things out at some point!

			// File Header
			FileHeader *fileHeader = (FileHeader *) pos;
			pos += sizeof(FileHeader);
			if (pos > eof)
			{
				savedGame->isReadable = false;
				continue;
			}

			if (!fileHeaderIsValid(fileHeader, savedGame->shortName, SAV_FILE_ID))
			{
				savedGame->isReadable = false;
				continue;
			}

			if (fileHeader->version > SAV_VERSION)
			{
				savedGame->problems |= SAVE_IS_FROM_NEWER_VERSION;
			}

			// META chunk
			FileChunkHeader *header = (FileChunkHeader *) pos;
			pos += sizeof(FileChunkHeader);
			if (pos > eof)
			{
				savedGame->isReadable = false;
				continue;
			}

			if (header->identifier == SAV_META_ID)
			{
				// Load Meta
				if (header->version > SAV_META_VERSION)
				{
					savedGame->problems |= SAVE_IS_FROM_NEWER_VERSION;
				}

				u8 *startOfChunk = pos;
				SAVChunk_Meta *cMeta = (SAVChunk_Meta *) pos;
				pos += header->length;
				if (pos > eof)
				{
					savedGame->isReadable = false;
					continue;
				}

				savedGame->saveTime   = getLocalTimeFromTimestamp(cMeta->saveTimestamp);
				savedGame->cityName   = intern(&catalogue->stringsTable, readString(cMeta->cityName, startOfChunk));
				savedGame->playerName = intern(&catalogue->stringsTable, readString(cMeta->playerName, startOfChunk));
				savedGame->citySize   = v2i(cMeta->cityWidth, cMeta->cityHeight);
				savedGame->funds      = cMeta->funds;
				savedGame->population = cMeta->population;
			}
		}
		closeFile(&savedFile);
	}

	// Sort the saved games by most-recent first
	catalogue->savedGames.sort([](SavedGameInfo *a, SavedGameInfo *b) {
		return a->saveTime.unixTimestamp > b->saveTime.unixTimestamp;
	});

}

void showLoadGameWindow(UIState *uiState)
{
	SavedGamesCatalogue *catalogue = &savedGamesCatalogue;
	catalogue->selectedSavedGameName = nullString;
	catalogue->selectedSavedGameIndex = -1;

	showWindow(uiState, getText("title_load_game"_s), 780, 580, {}, "default"_s, WinFlag_Unique|WinFlag_Modal, savedGamesWindowProc);
}

void showSaveGameWindow(UIState *uiState)
{
	SavedGamesCatalogue *catalogue = &savedGamesCatalogue;

	clear(&catalogue->saveGameName);
	captureInput(&catalogue->saveGameName);

	// If we're playing a save file, select that by default
	Indexed<SavedGameInfo *> selectedSavedGame = catalogue->savedGames.findFirst([&](SavedGameInfo *info) {
		return equals(info->shortName, catalogue->activeSavedGameName);
	});
	if (selectedSavedGame.index != -1)
	{
		catalogue->selectedSavedGameName = catalogue->activeSavedGameName;
		catalogue->selectedSavedGameIndex = selectedSavedGame.index;
		append(&catalogue->saveGameName, catalogue->selectedSavedGameName);
	}
	else
	{
		catalogue->selectedSavedGameName = nullString;
		catalogue->selectedSavedGameIndex = -1;
	}

	showWindow(uiState, getText("title_save_game"_s), 780, 580, {}, "default"_s, WinFlag_Unique|WinFlag_Modal, savedGamesWindowProc, (void*)true, saveGameWindowOnClose);
}

void saveGameWindowOnClose(WindowContext * /*context*/, void * /*userData*/)
{
	SavedGamesCatalogue *catalogue = &savedGamesCatalogue;
	releaseInput(&catalogue->saveGameName);
}

void savedGamesWindowProc(WindowContext *context, void *userData)
{
	UIPanel *ui = &context->windowPanel;
	SavedGamesCatalogue *catalogue = &savedGamesCatalogue;
	bool isSaveWindow = userData != null;

	SavedGameInfo *selectedSavedGame = null;
	bool justClickedSavedGame = false;

	UIPanel bottomBar = ui->row(26, ALIGN_BOTTOM, "plain"_s);

	UIPanel savesList = ui->column(320, ALIGN_LEFT, "inset"_s);
	{
		savesList.enableVerticalScrolling(&catalogue->savedGamesListScrollbar);
		savesList.alignWidgets(ALIGN_EXPAND_H);

		if (catalogue->savedGames.isEmpty())
		{
			savesList.addText(getText("msg_no_saved_games"_s));
		}
		else
		{
			for (auto it = catalogue->savedGames.iterate(); it.hasNext(); it.next())
			{
				SavedGameInfo *savedGame = it.get();
				s32 index = it.getIndex();

				if (savesList.addButton(savedGame->shortName, buttonIsActive(catalogue->selectedSavedGameIndex == index)))
				{
					// Select it and show information in the details pane
					catalogue->selectedSavedGameIndex = index;
					justClickedSavedGame = true;
				}

				if (catalogue->selectedSavedGameIndex == index)
				{
					selectedSavedGame = savedGame;
				}
			}
		}
	}
	savesList.end();

	// Now we have the saved-game info
	if (selectedSavedGame)
	{
		ui->alignWidgets(ALIGN_RIGHT);
		if (ui->addImageButton(getSprite("icon_delete"_s), Button_Normal, "delete"_s))
		{
			showWindow(globalAppState.uiState, getText("title_delete_save"_s), 300, 300, v2i(0,0), "default"_s, WinFlag_AutomaticHeight | WinFlag_Modal, confirmDeleteSaveWindowProc);
		}

		ui->alignWidgets(ALIGN_EXPAND_H);
		ui->addText(selectedSavedGame->shortName);
		ui->addText(myprintf("Saved {0}"_s, {formatDateTime(selectedSavedGame->saveTime, DateTime_LongDateTime)}));
		ui->addText(selectedSavedGame->cityName);
		ui->addText(myprintf("Mayor {0}"_s, {selectedSavedGame->playerName}));
		ui->addText(myprintf("£{0}"_s, {formatInt(selectedSavedGame->funds)}));
		ui->addText(myprintf("{0} population"_s, {formatInt(selectedSavedGame->population)}));

		if (selectedSavedGame->problems != 0)
		{
			if (selectedSavedGame->problems & SAVE_IS_FROM_NEWER_VERSION)
			{
				ui->addText(getText("msg_load_version_too_new"_s));
			}
		}
	}

	// Bottom bar
	{
		if (isSaveWindow)
		{
			// 'Save' buttons
			bottomBar.alignWidgets(ALIGN_LEFT);
			if (bottomBar.addButton(getText("button_back"_s)))
			{
				context->closeRequested = true;
			}

			bottomBar.addText("Save game name:"_s);

			bottomBar.alignWidgets(ALIGN_RIGHT);
			bool pressedSave = bottomBar.addButton(getText("button_save"_s), !isEmpty(&catalogue->saveGameName) ? Button_Normal : Button_Disabled);

			bottomBar.alignWidgets(ALIGN_EXPAND_H);
			if (justClickedSavedGame)
			{
				clear(&catalogue->saveGameName);
				append(&catalogue->saveGameName, selectedSavedGame->shortName);
			}

			bool pressedEnterInTextInput = bottomBar.addTextInput(&catalogue->saveGameName);
			String inputName = trim(textInputToString(&catalogue->saveGameName));

			// Show a warning if we're overwriting an existing save that ISN'T the active one
			if (!isEmpty(inputName) && !equals(inputName, catalogue->activeSavedGameName))
			{
				Indexed<SavedGameInfo *> fileToOverwrite = catalogue->savedGames.findFirst([&](SavedGameInfo *info) {
					return equals(inputName, info->shortName);
				});

				if (fileToOverwrite.index != -1)
				{
					bottomBar.addText(getText("msg_save_warning_overwrite"_s), "warning"_s);
				}
			}

			if (pressedSave || pressedEnterInTextInput)
			{
				if (!isEmpty(inputName))
				{
					if (saveGame(globalAppState.uiState, inputName))
					{
						context->closeRequested = true;
					}
				}
			}
		}
		else
		{
			bottomBar.alignWidgets(ALIGN_LEFT);
			if (bottomBar.addButton(getText("button_back"_s)))
			{
				context->closeRequested = true;
			}

			bottomBar.alignWidgets(ALIGN_RIGHT);
			if (bottomBar.addButton(getText("button_load"_s), selectedSavedGame ? Button_Normal : Button_Disabled))
			{
				loadGame(globalAppState.uiState, selectedSavedGame);
				context->closeRequested = true;
			}
		}
	}
	bottomBar.end();
}

void confirmDeleteSaveWindowProc(WindowContext *context, void * /*userData*/)
{
	UIPanel *ui = &context->windowPanel;

	SavedGameInfo *savedGame = savedGamesCatalogue.savedGames.get(savedGamesCatalogue.selectedSavedGameIndex);

	ui->addText(getText("msg_delete_save_confirm"_s, {savedGame->shortName}));
	ui->startNewLine(ALIGN_RIGHT);

	if (ui->addButton(getText("button_delete"_s), Button_Normal, "delete"_s))
	{
		deleteSave(globalAppState.uiState, savedGame);
		context->closeRequested = true;
	}

	if (ui->addButton(getText("button_cancel"_s)))
	{
		context->closeRequested = true;
	}
}

void loadGame(UIState *uiState, SavedGameInfo *savedGame)
{
	if (globalAppState.gameState != null)
	{
		freeGameState(globalAppState.gameState);
	}

	globalAppState.gameState = newGameState();
	GameState *gameState = globalAppState.gameState;

	u32 startTicks = SDL_GetTicks();

	FileHandle saveFile = openFile(savedGame->fullPath, FileAccess_Read);
	bool loadSucceeded = loadSaveFile(&saveFile, gameState);
	closeFile(&saveFile);

	if (loadSucceeded)
	{
		u32 endTicks = SDL_GetTicks();
		logInfo("Loaded save '{0}' in {1} milliseconds."_s, {savedGame->shortName, formatInt(endTicks - startTicks)});

		pushToast(uiState, getText("msg_load_success"_s, {savedGame->shortName}));

		globalAppState.appStatus = AppStatus_Game;

		closeAllWindows();

		// Filename is interned so it's safe to copy it
		savedGamesCatalogue.activeSavedGameName = savedGame->shortName;
	}
	else
	{
		pushToast(uiState, getText("msg_load_failure"_s, {savedGame->shortName}));
	}
}

bool saveGame(UIState *uiState, String saveName)
{
	SavedGamesCatalogue *catalogue = &savedGamesCatalogue;

	String saveFilename = saveName;
	if (!endsWith(saveFilename, ".sav"_s))
	{
		saveFilename = concatenate({saveName, ".sav"_s});
	}

	String savePath = constructPath({catalogue->savedGamesPath, saveFilename});
	FileHandle saveFile = openFile(savePath, FileAccess_Write);
	bool saveSucceeded = writeSaveFile(&saveFile, globalAppState.gameState);
	closeFile(&saveFile);

	if (saveSucceeded)
	{
		pushToast(uiState, getText("msg_save_success"_s, {saveFile.path}));

		// Store that we saved it
		savedGamesCatalogue.activeSavedGameName = intern(&catalogue->stringsTable, saveName);
	}
	else
	{
		pushToast(uiState, getText("msg_save_failure"_s, {saveFile.path}));
	}

	return saveSucceeded;
}

bool deleteSave(UIState *uiState, SavedGameInfo *savedGame)
{
	bool success = deleteFile(savedGame->fullPath);

	if (success)
	{
		pushToast(uiState, getText("msg_delete_save_success"_s, {savedGame->shortName}));
	}
	else
	{
		pushToast(uiState, getText("msg_delete_save_failure"_s, {savedGame->shortName}));
	}

	return success;
}
