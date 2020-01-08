#pragma once

struct SavedGameInfo
{
	String filename;
	String fullPath;
};

struct SavedGamesCatalogue
{
	// Rather than scan the save files directory every time we need it, we maintain
	// a list of saved games here, which updates when the files change.

	MemoryArena savedGamesArena;

	DirectoryChangeWatchingHandle savedGamesChangeHandle;
	String savedGamesPath;

	ChunkedArray<SavedGameInfo> savedGames;

	StringTable filenamesTable;
};

SavedGamesCatalogue savedGamesCatalogue;

void initSavedGamesCatalogue();
void updateSavedGamesCatalogue();
void readSavedGamesInfo(SavedGamesCatalogue *catalogue);

void showLoadGameWindow(UIState *uiState);
void loadGameWindowProc(WindowContext *context, void *userData);
void loadGame(UIState *uiState, SavedGameInfo *savedGame);
