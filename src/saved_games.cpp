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

	catalogue->selectedSavedGameFilename = nullString;
	catalogue->selectedSavedGameIndex = -1;

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

		savedGame->filename = intern(&catalogue->stringsTable, fileInfo->filename);
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

			if (!fileHeaderIsValid(fileHeader, savedGame->filename))
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
	catalogue->selectedSavedGameFilename = nullString;
	catalogue->selectedSavedGameIndex = -1;

	showWindow(uiState, getText("title_load_game"_s), 400, 400, {}, "default"_s, WinFlag_Unique|WinFlag_Modal, loadGameWindowProc, null);
}

void loadGameWindowProc(WindowContext *context, void * /*userdata*/)
{
	SavedGamesCatalogue *catalogue = &savedGamesCatalogue;

	SavedGameInfo *selectedSavedGame = null;

	window_beginColumns(context);

	// List of saved games
	window_column(context, 0.4f);

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

			if (window_button(context, savedGame->filename, -1, (catalogue->selectedSavedGameIndex == index)))
			{
				// Select it and show information in the details pane
				catalogue->selectedSavedGameIndex = index;
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
		window_label(context, selectedSavedGame->filename);
		window_label(context, myprintf("Saved {0}"_s, {formatDateTime(selectedSavedGame->saveTime, DateTime_ShortDateTime)}));
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

		if (window_button(context, getText("button_load"_s)) && (selectedSavedGame != null))
		{
			loadGame(context->uiState, selectedSavedGame);
			context->closeRequested = true;
		}
	}
}

void loadGame(UIState *uiState, SavedGameInfo *savedGame)
{
	globalAppState.gameState = beginNewGame();

	City *city = &globalAppState.gameState->city;
	FileHandle saveFile = openFile(savedGame->fullPath, FileAccess_Read);
	bool loadSucceeded = loadSaveFile(&saveFile, city, &globalAppState.gameState->gameArena);
	closeFile(&saveFile);

	if (loadSucceeded)
	{
		pushUiMessage(uiState, myprintf(getText("msg_load_success"_s), {saveFile.path}));
		globalAppState.appStatus = AppStatus_Game;
	}
	else
	{
		pushUiMessage(uiState, myprintf(getText("msg_load_failure"_s), {saveFile.path}));
	}
}

void showSaveGameWindow(UIState *uiState)
{
	SavedGamesCatalogue *catalogue = &savedGamesCatalogue;
	catalogue->selectedSavedGameFilename = nullString;
	catalogue->selectedSavedGameIndex = -1;

	showWindow(uiState, getText("title_save_game"_s), 400, 400, {}, "default"_s, WinFlag_Unique|WinFlag_Modal, saveGameWindowProc, null);
}

void saveGameWindowProc(WindowContext *context, void * /*userData*/)
{
	window_label(context, "Save game stuff goes here"_s);
	if (window_button(context, getText("button_save"_s)))
	{
		saveGame(context->uiState, "fantastico.sav"_s);
	}
}

void saveGame(UIState *uiState, String saveName)
{
	SavedGamesCatalogue *catalogue = &savedGamesCatalogue;
	City *city = &globalAppState.gameState->city;

	String saveFilename = saveName;
	if (!endsWith(saveFilename, ".sav"_s))
	{
		saveFilename = concatenate({saveName, ".sav"_s});
	}

	String savePath = constructPath({catalogue->savedGamesPath, saveFilename});
	FileHandle saveFile = openFile(savePath, FileAccess_Write);
	bool saveSucceeded = writeSaveFile(&saveFile, city);
	closeFile(&saveFile);

	if (saveSucceeded)
	{
		pushUiMessage(uiState, myprintf(getText("msg_save_success"_s), {saveFile.path}));
	}
	else
	{
		pushUiMessage(uiState, myprintf(getText("msg_save_failure"_s), {saveFile.path}));
	}
}
