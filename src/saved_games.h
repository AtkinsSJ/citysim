#pragma once

enum SavedGameProblem
{
	SAVE_IS_FROM_NEWER_VERSION = 1 << 0,
};

struct SavedGameInfo
{
	bool isReadable;
	String filename;
	String fullPath;

	u32 problems;
	DateTime saveTime;
	String cityName;
	String playerName;
	s32 funds;
	s32 population;
};

struct SavedGamesCatalogue
{
	// Rather than scan the save files directory every time we need it, we maintain
	// a list of saved games here, which updates when the files change.

	MemoryArena savedGamesArena;
	StringTable stringsTable;

	DirectoryChangeWatchingHandle savedGamesChangeHandle;
	String savedGamesPath;

	ChunkedArray<SavedGameInfo> savedGames;

	// Fields used by the save/load windows
	String selectedSavedGameFilename;
	s32 selectedSavedGameIndex;
	TextInput saveGameName;

	ScrollbarState savedGamesListScrollbar;
};

SavedGamesCatalogue savedGamesCatalogue;

void initSavedGamesCatalogue();
void updateSavedGamesCatalogue();
void readSavedGamesInfo(SavedGamesCatalogue *catalogue);

void showSaveGameWindow(UIState *uiState);
void saveGameWindowProc(WindowContext *context, void *userData);
void saveGameWindowOnClose(WindowContext *context, void *userData);
bool saveGame(UIState *uiState, String saveName);

void showLoadGameWindow(UIState *uiState);
void loadGameWindowProc(WindowContext *context, void *userData);
void loadGame(UIState *uiState, SavedGameInfo *savedGame);
