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

		clear(&catalogue->savedGames);
		readSavedGamesInfo(catalogue);
	}
}

void readSavedGamesInfo(SavedGamesCatalogue *catalogue)
{
	Blob tempBuffer = allocateBlob(tempArena, KB(4));

	for (auto it = iterateDirectoryListing(constructPath({catalogue->savedGamesPath}, true));
		hasNextFile(&it);
		findNextFile(&it))
	{
		FileInfo *fileInfo = getFileInfo(&it);
		if (fileInfo->flags & (FileFlag_Directory | FileFlag_Hidden)) continue;

		SavedGameInfo *savedGame = appendBlank(&catalogue->savedGames);

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
			SAVFileHeader *fileHeader = (SAVFileHeader *) pos;
			pos += sizeof(SAVFileHeader);
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
			SAVChunkHeader *header = (SAVChunkHeader *) pos;
			pos += sizeof(SAVChunkHeader);
			if (pos > eof)
			{
				savedGame->isReadable = false;
				continue;
			}

			if (identifiersAreEqual(header->identifier, SAV_META_ID))
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
	sortChunkedArray(&catalogue->savedGames, [](SavedGameInfo *a, SavedGameInfo *b) {
		return a->saveTime.unixTimestamp > b->saveTime.unixTimestamp;
	});

}

void showLoadGameWindow(UIState *uiState)
{
	SavedGamesCatalogue *catalogue = &savedGamesCatalogue;
	catalogue->selectedSavedGameName = nullString;
	catalogue->selectedSavedGameIndex = -1;

	showWindow(uiState, getText("title_load_game"_s), 400, 400, {}, "default"_s, WinFlag_Unique|WinFlag_Modal|WinFlag_AutomaticHeight, savedGamesWindowProc, null);
}

void showSaveGameWindow(UIState *uiState)
{
	SavedGamesCatalogue *catalogue = &savedGamesCatalogue;

	clear(&catalogue->saveGameName);
	captureInput(&catalogue->saveGameName);

	// If we're playing a save file, select that by default
	Indexed<SavedGameInfo *> selectedSavedGame = findFirst(&catalogue->savedGames, [&](SavedGameInfo *info) {
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

	showWindow(uiState, getText("title_save_game"_s), 400, 400, {}, "default"_s, WinFlag_Unique|WinFlag_Modal|WinFlag_AutomaticHeight, savedGamesWindowProc, (void*)true, saveGameWindowOnClose);
}

void saveGameWindowOnClose(WindowContext * /*context*/, void * /*userData*/)
{
	SavedGamesCatalogue *catalogue = &savedGamesCatalogue;
	releaseInput(&catalogue->saveGameName);
}

void savedGamesWindowProc(WindowContext *context, void *userData)
{
	SavedGamesCatalogue *catalogue = &savedGamesCatalogue;
	bool isSaveWindow = userData != null;

	SavedGameInfo *selectedSavedGame = null;
	bool justClickedSavedGame = false;

	window_beginColumns(context, 250);

	// List of saved games
	window_columnPercent(context, 0.4f, &catalogue->savedGamesListScrollbar);

	if (catalogue->savedGames.count == 0)
	{
		window_label(context, getText("msg_no_saved_games"_s));
	}
	else
	{
		for (auto it = iterate(&catalogue->savedGames); hasNext(&it); next(&it))
		{
			SavedGameInfo *savedGame = get(&it);
			s32 index = getIndex(&it);

			if (window_button(context, savedGame->shortName, -1, (catalogue->selectedSavedGameIndex == index)))
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

	// Details about the selected saved game
	window_column(context);

	if (selectedSavedGame)
	{
		window_label(context, selectedSavedGame->shortName);
		window_label(context, myprintf("Saved {0}"_s, {formatDateTime(selectedSavedGame->saveTime, DateTime_LongDateTime)}));
		window_label(context, selectedSavedGame->cityName);
		window_label(context, myprintf("Mayor {0}"_s, {selectedSavedGame->playerName}));
		window_label(context, myprintf("£{0}"_s, {formatInt(selectedSavedGame->funds)}));
		window_label(context, myprintf("{0} population"_s, {formatInt(selectedSavedGame->population)}));

		if (selectedSavedGame->problems != 0)
		{
			if (selectedSavedGame->problems & SAVE_IS_FROM_NEWER_VERSION)
			{
				window_label(context, getText("msg_load_version_too_new"_s));
			}
		}
	}

	window_endColumns(context);

	if (isSaveWindow)
	{
		// 'Save' buttons
		window_label(context, "Save game name:"_s);

		if (justClickedSavedGame)
		{
			clear(&catalogue->saveGameName);
			append(&catalogue->saveGameName, selectedSavedGame->shortName);
		}

		window_startNewLine(context, ALIGN_RIGHT);

		// Now we try and do multiple things on one line! Wish me luck.
		bool pressedSave = window_button(context, getText("button_save"_s));

		window_alignWidgets(context, ALIGN_EXPAND_H);
		bool pressedEnterInTextInput = window_textInput(context, &catalogue->saveGameName);
		String inputName = trim(textInputToString(&catalogue->saveGameName));

		// Show a warning if we're overwriting an existing save that ISN'T the active one
		if (!isEmpty(inputName) && !equals(inputName, catalogue->activeSavedGameName))
		{
			Indexed<SavedGameInfo *> fileToOverwrite = findFirst(&catalogue->savedGames, [&](SavedGameInfo *info) {
				return equals(inputName, info->shortName);
			});

			if (fileToOverwrite.index != -1)
			{
				window_label(context, getText("msg_save_warning_overwrite"_s), "warning"_s);
			}
		}

		if (pressedSave || pressedEnterInTextInput)
		{
			if (!isEmpty(inputName))
			{
				if (saveGame(context->uiState, inputName))
				{
					context->closeRequested = true;
				}
			}
		}
	}
	else
	{
		// 'Load' buttons
		if (window_button(context, getText("button_load"_s)) && (selectedSavedGame != null))
		{
			loadGame(context->uiState, selectedSavedGame);
			context->closeRequested = true;
		}
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

		pushUiMessage(uiState, myprintf(getText("msg_load_success"_s), {savedGame->shortName}));

		globalAppState.appStatus = AppStatus_Game;

		// Filename is interned so it's safe to copy it
		savedGamesCatalogue.activeSavedGameName = savedGame->shortName;
	}
	else
	{
		pushUiMessage(uiState, myprintf(getText("msg_load_failure"_s), {savedGame->shortName}));
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
		pushUiMessage(uiState, myprintf(getText("msg_save_success"_s), {saveFile.path}));

		// Store that we saved it
		savedGamesCatalogue.activeSavedGameName = intern(&catalogue->stringsTable, saveName);
	}
	else
	{
		pushUiMessage(uiState, myprintf(getText("msg_save_failure"_s), {saveFile.path}));
	}

	return saveSucceeded;
}
