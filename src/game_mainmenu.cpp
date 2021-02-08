#pragma once

AppStatus updateAndRenderMainMenu(UIState *uiState, f32 /*deltaTime*/)
{
	DEBUG_FUNCTION();
	
	AppStatus result = AppStatus_MainMenu;

	s32 windowWidth = round_s32(renderer->uiCamera.size.x);
	// s32 windowHeight = round_s32(renderer->uiCamera.size.y);
	UITheme *theme = &assets->theme;

	V2I position = v2i(windowWidth / 2, 157);
	s32 maxLabelWidth = windowWidth - 256;

	UILabelStyle *labelStyle = findLabelStyle(theme, "title"_s);
	BitmapFont *font = getFont(&labelStyle->font);

	position.y += (uiText(&renderer->uiBuffer, font, getText("game_title"_s),
			position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;

	position.y += (uiText(&renderer->uiBuffer, font, getText("game_subtitle"_s),
			position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;

	UIButtonStyle *style = findButtonStyle(&assets->theme, "default"_s);

	s32 buttonPadding = 8;

	String newGameText  = getText("button_new_game"_s);
	String loadText     = getText("button_load"_s);
	String creditsText  = getText("button_credits"_s);
	String settingsText = getText("button_settings"_s);
	String aboutText    = getText("button_about"_s);
	String exitText     = getText("button_exit"_s);

	V2I newGameSize  = calculateButtonSize(newGameText,  style);
	V2I loadSize     = calculateButtonSize(loadText,     style);
	V2I creditsSize  = calculateButtonSize(creditsText,  style);
	V2I settingsSize = calculateButtonSize(settingsText, style);
	V2I aboutSize    = calculateButtonSize(aboutText,    style);
	V2I exitSize     = calculateButtonSize(exitText,     style);

	s32 buttonWidth = max(
		newGameSize.x,
		loadSize.x,
		creditsSize.x,
		settingsSize.x,
		aboutSize.x,
		exitSize.x
	);
	s32 buttonHeight = max(
		newGameSize.y,
		loadSize.y,
		creditsSize.y,
		settingsSize.y,
		aboutSize.y,
		exitSize.y
	);

	Rect2I buttonRect = irectXYWH(position.x - (buttonWidth/2), position.y + buttonPadding, buttonWidth, buttonHeight);
	if (uiButton(uiState, newGameText, buttonRect, style))
	{
		beginNewGame();

		result = AppStatus_Game;
	}
	buttonRect.y += buttonHeight + buttonPadding;
	if (uiButton(uiState, loadText, buttonRect, style))
	{
		showLoadGameWindow(uiState);
	}
	buttonRect.y += buttonHeight + buttonPadding;
	if (uiButton(uiState, creditsText, buttonRect, style))
	{
		result = AppStatus_Credits;
	}
	buttonRect.y += buttonHeight + buttonPadding;
	if (uiButton(uiState, settingsText, buttonRect, style))
	{
		result = AppStatus_SettingsMenu;
	}
	buttonRect.y += buttonHeight + buttonPadding;
	if (uiButton(uiState, aboutText, buttonRect, style))
	{
		showAboutWindow(uiState);
	}
	buttonRect.y += buttonHeight + buttonPadding;
	if (uiButton(uiState, exitText, buttonRect, style))
	{
		result = AppStatus_Quit;
	}

	// This is a copy of savedGamesWindowProc() in our new system, for testing
	if (false)
	{
		UIPanel testPanel(irectXYWH(32, 4, 780, 580));
		{
			SavedGamesCatalogue *catalogue = &savedGamesCatalogue;
			bool isSaveWindow = true;

			SavedGameInfo *selectedSavedGame = null;
			bool justClickedSavedGame = false;

			UIPanel bottomBar = testPanel.row(26, ALIGN_BOTTOM, "plain"_s);

			UIPanel savesList = testPanel.column(320, ALIGN_LEFT, "inset"_s);
			{
				savesList.enableVerticalScrolling(&catalogue->savedGamesListScrollbar);
				savesList.alignWidgets(ALIGN_EXPAND_H);

				if (catalogue->savedGames.count == 0)
				{
					savesList.addText(getText("msg_no_saved_games"_s));
				}
				else
				{
					// TEMPORARY: Multiply the list so that we can test scrolling
					for (int iteration = 0; iteration < 20; iteration++)
					{
						for (auto it = iterate(&catalogue->savedGames); hasNext(&it); next(&it))
						{
							SavedGameInfo *savedGame = get(&it);
							s32 index = getIndex(&it);

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
			}
			savesList.end();

			// Now we have the saved-game info
			if (selectedSavedGame)
			{
				testPanel.alignWidgets(ALIGN_RIGHT);
				if (testPanel.addButton(getText("button_delete_save"_s), Button_Normal, "delete"_s))
				{
					deleteSave(globalAppState.uiState, selectedSavedGame);
				}

				testPanel.alignWidgets(ALIGN_EXPAND_H);
				testPanel.addText(selectedSavedGame->shortName);
				testPanel.addText(myprintf("Saved {0}"_s, {formatDateTime(selectedSavedGame->saveTime, DateTime_LongDateTime)}));
				testPanel.addText(selectedSavedGame->cityName);
				testPanel.addText(myprintf("Mayor {0}"_s, {selectedSavedGame->playerName}));
				testPanel.addText(myprintf("£{0}"_s, {formatInt(selectedSavedGame->funds)}));
				testPanel.addText(myprintf("{0} population"_s, {formatInt(selectedSavedGame->population)}));

				if (selectedSavedGame->problems != 0)
				{
					if (selectedSavedGame->problems & SAVE_IS_FROM_NEWER_VERSION)
					{
						testPanel.addText(getText("msg_load_version_too_new"_s));
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
						// context->closeRequested = true;
					}

					bottomBar.addText("Save game name:"_s);

					bottomBar.alignWidgets(ALIGN_RIGHT);
					bool pressedSave = bottomBar.addButton(getText("button_save"_s), selectedSavedGame ? Button_Normal : Button_Disabled);

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
						Indexed<SavedGameInfo *> fileToOverwrite = findFirst(&catalogue->savedGames, [&](SavedGameInfo *info) {
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
								// context->closeRequested = true;
							}
						}
					}
				}
				else
				{
					bottomBar.alignWidgets(ALIGN_LEFT);
					if (bottomBar.addButton(getText("button_back"_s)))
					{
						// context->closeRequested = true;
					}

					bottomBar.alignWidgets(ALIGN_RIGHT);
					if (bottomBar.addButton(getText("button_load"_s), selectedSavedGame ? Button_Normal : Button_Disabled))
					{
						loadGame(globalAppState.uiState, selectedSavedGame);
						// context->closeRequested = true;
					}
				}
			}
			bottomBar.end();
		}
		testPanel.end();
	}
	
	return result;
}
