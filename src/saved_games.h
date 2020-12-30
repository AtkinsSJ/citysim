#pragma once

enum SavedGameProblem
{
	SAVE_IS_FROM_NEWER_VERSION = 1 << 0,
};

struct SavedGameInfo
{
	bool isReadable;
	String shortName; // Has no extension
	String fullPath;

	u32 problems;
	
	DateTime saveTime;
	String cityName;
	String playerName;
	V2I citySize;
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

	String activeSavedGameName; // The one that's currently loaded, so we know it's safe to save over it. (shortName)

	// Fields used by the save/load windows
	String selectedSavedGameName; // (shortName)
	s32 selectedSavedGameIndex;
	TextInput saveGameName;

	ScrollbarState savedGamesListScrollbar;
};

SavedGamesCatalogue savedGamesCatalogue;

void initSavedGamesCatalogue();
void updateSavedGamesCatalogue();
void readSavedGamesInfo(SavedGamesCatalogue *catalogue);

bool saveGame(UIState *uiState, String saveName);
void loadGame(UIState *uiState, SavedGameInfo *savedGame);

void savedGamesWindowProc(WindowContext *context, void *userData);
void showSaveGameWindow(UIState *uiState);
void showLoadGameWindow(UIState *uiState);
void saveGameWindowOnClose(WindowContext *context, void *userData);
