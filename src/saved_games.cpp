#pragma once

void initSavedGamesCatalogue()
{
	SavedGamesCatalogue *catalogue = &savedGamesCatalogue;
	*catalogue = {};

	initMemoryArena(&catalogue->savedGamesArena, MB(1));

	catalogue->savedGamesPath = pushString(&catalogue->savedGamesArena, constructPath({getUserDataPath(), "saves"_s}));
	createDirectory(catalogue->savedGamesPath);
	catalogue->savedGamesChangeHandle = beginWatchingDirectory(catalogue->savedGamesPath);

	initChunkedArray(&catalogue->savedGames, &catalogue->savedGamesArena, 64);

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

		readSavedGamesInfo(catalogue);
	}
}

void showLoadGameWindow(UIState *uiState)
{
	showWindow(uiState, getText("title_load_game"_s), 400, 400, {}, "default"_s, WinFlag_Unique|WinFlag_Modal, loadGameWindowProc, null);
}

void loadGameWindowProc(WindowContext *context, void * /*userdata*/)
{
	SavedGamesCatalogue *catalogue = &savedGamesCatalogue;

	if (catalogue->savedGames.count == 0)
	{
		window_label(context, getText("msg_no_saved_games"_s));
	}
	else
	{
		for (auto it = iterate(&catalogue->savedGames); hasNext(&it); next(&it))
		{
			SavedGameInfo *savedGame = get(&it);

			if (window_button(context, savedGame->filename))
			{
				loadGame(context->uiState, savedGame);
				context->closeRequested = true;
			}
		}
	}
}

void readSavedGamesInfo(SavedGamesCatalogue *catalogue)
{
	clear(&catalogue->savedGames);

	for (auto it = iterateDirectoryListing(constructPath({catalogue->savedGamesPath}, true));
		hasNextFile(&it);
		findNextFile(&it))
	{
		FileInfo *fileInfo = getFileInfo(&it);
		if (fileInfo->flags & (FileFlag_Directory | FileFlag_Hidden)) continue;

		SavedGameInfo *savedGame = appendBlank(&catalogue->savedGames);

		// TODO: @InternedStrings We really want to keep the Strings in the catalogue, in some
		// kind of special-purpose StringSet or something.
		// @Leak We re-allocate the strings every time readSavedGamesInfo() is called, and we never free them!

		savedGame->filename = pushString(&catalogue->savedGamesArena, fileInfo->filename);
		savedGame->fullPath = pushString(&catalogue->savedGamesArena, constructPath({catalogue->savedGamesPath, fileInfo->filename}));
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
