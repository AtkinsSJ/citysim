#pragma once

void showLoadGameWindow(UIState *uiState)
{
	showWindow(uiState, getText("title_load_game"_s), 400, 400, {}, "default"_s, WinFlag_Unique|WinFlag_Modal, loadGameWindowProc, null);
}

void loadGameWindowProc(WindowContext *context, void * /*userData*/)
{
	if (window_button(context, getText("button_load"_s)))
	{
		loadGame(context->uiState);
		context->closeRequested = true;
	}
}

void loadGame(UIState *uiState)
{
	globalAppState.gameState = beginNewGame();

	City *city = &globalAppState.gameState->city;
	FileHandle saveFile = openFile("whatever.sav\0"_s, FileAccess_Read);
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
