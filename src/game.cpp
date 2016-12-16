#pragma once

#include "game_mainmenu.cpp"

GameState *initialiseGameState()
{
	GameState *result;
	bootstrapArena(GameState, result, gameArena);

	initCity(&result->gameArena, &result->city, 2000 / ITILE_SIZE, 1517 / ITILE_SIZE, "City Name Here", gameStartFunds);

	result->status = GameStatus_Playing;

	return result;
}

void inputMoveCamera(Camera *camera, InputState *inputState, int32 cityWidth, int32 cityHeight) // City size in tiles
{ 
	// Zooming
	if (canZoom && inputState->wheelY) {
		// round()ing the zoom so it doesn't gradually drift due to float imprecision
		camera->zoom = clamp(round(10 * camera->zoom - inputState->wheelY) * 0.1f, 0.1f, 10.0f);
	}

	// Panning
	real32 scrollSpeed = (CAMERA_PAN_SPEED * sqrt(camera->zoom)) * SECONDS_PER_FRAME;
	if (mouseButtonPressed(inputState, SDL_BUTTON_MIDDLE))
	{
		// Click-panning!
		float scale = scrollSpeed * 0.01f;
		V2 clickStartPos = inputState->clickStartPosition[mouseButtonIndex(SDL_BUTTON_MIDDLE)];
		camera->pos += (inputState->mousePosNormalised - clickStartPos) * scale;
	}
	else
	{
		if (inputState->keyDown[SDL_SCANCODE_LEFT]
			|| inputState->keyDown[SDL_SCANCODE_A]
			|| (inputState->mousePosNormalised.x < (-1.0f + CAMERA_EDGE_SCROLL_MARGIN)))
		{
			camera->pos.x -= scrollSpeed;
		}
		else if (inputState->keyDown[SDL_SCANCODE_RIGHT]
			|| inputState->keyDown[SDL_SCANCODE_D]
			|| (inputState->mousePosNormalised.x > (1.0f - CAMERA_EDGE_SCROLL_MARGIN)))
		{
			camera->pos.x += scrollSpeed;
		}

		if (inputState->keyDown[SDL_SCANCODE_UP]
			|| inputState->keyDown[SDL_SCANCODE_W]
			|| (inputState->mousePosNormalised.y > (1.0f - CAMERA_EDGE_SCROLL_MARGIN)))
		{
			camera->pos.y -= scrollSpeed;
		}
		else if (inputState->keyDown[SDL_SCANCODE_DOWN]
			|| inputState->keyDown[SDL_SCANCODE_S]
			|| (inputState->mousePosNormalised.y < (-1.0f + CAMERA_EDGE_SCROLL_MARGIN)))
		{
			camera->pos.y += scrollSpeed;
		}
	}

	// Clamp camera
	V2 cameraSize = camera->size / camera->zoom;
	if (cityWidth < cameraSize.x) {
		// City smaller than camera, so centre on it
		camera->pos.x = cityWidth * 0.5f;
	} else {
		real32 minX = (cameraSize.x * 0.5f) - CAMERA_MARGIN;
		camera->pos.x = clamp( camera->pos.x, minX, (real32)cityWidth - minX );
	}

	if (cityHeight < camera->size.y / camera->zoom) {
		// City smaller than camera, so centre on it
		camera->pos.y = cityHeight * 0.5f;
	} else {
		real32 minY = (cameraSize.y * 0.5f) - CAMERA_MARGIN;
		camera->pos.y = clamp( camera->pos.y, minY, (real32)cityHeight - minY );
	}
}

void showCostTooltip(UIState *uiState, AssetManager *assets, int32 cost, int32 cityFunds) {
	UITheme *theme = &assets->theme;

	if (cost > cityFunds) {
		uiState->tooltip.color = theme->tooltipStyle.textColorBad;
	} else {
		uiState->tooltip.color = theme->tooltipStyle.textColorNormal;
	}
	sprintf(uiState->tooltip.text, "-£%d", cost);
	uiState->tooltip.show = true;
}

void updateAndRenderGameUI(Renderer *renderer, AssetManager *assets, UIState *uiState, GameState *gameState, InputState *inputState)
{
	V2 centre = renderer->uiBuffer.camera.pos;
	real32 windowWidth = (real32) renderer->uiBuffer.camera.size.x;
	UITheme *theme = &assets->theme;
	BitmapFont *font = getFont(assets, theme->labelStyle.font);

	uiState->uiRectCount = 0;

	real32 left = uiPadding;
	char stringBuffer[256];

	RealRect uiRect = uiState->uiRects[uiState->uiRectCount++] = rectXYWH(0,0, windowWidth, 64);
	drawRect(&renderer->uiBuffer, uiRect, 0, theme->overlayColor);

	uiText(uiState, renderer, font, gameState->city.name, v2(left, uiPadding), ALIGN_LEFT, 1, theme->labelStyle.textColor);

	sprintf(stringBuffer, "£%d", gameState->city.funds);
	uiText(uiState, renderer, font, stringBuffer, v2(centre.x, uiPadding), ALIGN_RIGHT, 1, theme->labelStyle.textColor);
	sprintf(stringBuffer, "(-£%d/month)\0", gameState->city.monthlyExpenditure);
	uiText(uiState, renderer, font, stringBuffer, v2(centre.x, uiPadding), ALIGN_LEFT, 1, theme->labelStyle.textColor);

	// Build UI
	{
		RealRect buttonRect = rectXYWH(uiPadding, 28 + uiPadding, 80, 24);

		if (uiMenuButton(uiState, renderer, assets, inputState, "Build...", buttonRect, 1, UIMenu_Build))
		{
			RealRect menuButtonRect = buttonRect;
			menuButtonRect.y += menuButtonRect.h + uiPadding;

			RealRect menuRect = expandRect(menuButtonRect, uiPadding);

			if (uiButton(uiState, renderer, assets, inputState, "Build HQ", menuButtonRect, 1,
					(uiState->actionMode == ActionMode_Build) && (uiState->selectedBuildingArchetype == BA_Farmhouse),
					SDL_SCANCODE_Q, "(Q)"))
			{
				uiState->openMenu = UIMenu_None;
				uiState->selectedBuildingArchetype = BA_Farmhouse;
				uiState->actionMode = ActionMode_Build;
				setCursor(uiState, assets, Cursor_Build);
			}
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			menuRect.h += menuButtonRect.h + uiPadding;

			if (uiButton(uiState, renderer, assets, inputState, "Build Field", menuButtonRect, 1,
						(uiState->actionMode == ActionMode_Build) && (uiState->selectedBuildingArchetype == BA_Field),
						SDL_SCANCODE_F, "(F)"))
			{
				uiState->openMenu = UIMenu_None;
				uiState->selectedBuildingArchetype = BA_Field;
				uiState->actionMode = ActionMode_Build;
				setCursor(uiState, assets, Cursor_Build);
			}
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			menuRect.h += menuButtonRect.h + uiPadding;

			if (uiButton(uiState, renderer, assets, inputState, "Build Barn", menuButtonRect, 1,
						(uiState->actionMode == ActionMode_Build) && (uiState->selectedBuildingArchetype == BA_Barn),
						SDL_SCANCODE_B, "(B)"))
			{
				uiState->openMenu = UIMenu_None;
				uiState->selectedBuildingArchetype = BA_Barn;
				uiState->actionMode = ActionMode_Build;
				setCursor(uiState, assets, Cursor_Build);
			}
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			menuRect.h += menuButtonRect.h + uiPadding;

			if (uiButton(uiState, renderer, assets, inputState, "Build Road", menuButtonRect, 1,
						(uiState->actionMode == ActionMode_Build) && (uiState->selectedBuildingArchetype == BA_Path),
						SDL_SCANCODE_R, "(R)"))
			{
				uiState->openMenu = UIMenu_None;
				uiState->selectedBuildingArchetype = BA_Path;
				uiState->actionMode = ActionMode_Build;
				setCursor(uiState, assets, Cursor_Build);
			}

			uiState->uiRects[uiState->uiRectCount++] = menuRect;
			drawRect(&renderer->uiBuffer, menuRect, 0, theme->overlayColor);
		}

		buttonRect.x += buttonRect.w + uiPadding;
		if (uiButton(uiState, renderer, assets, inputState, "Demolish", buttonRect, 1,
					(uiState->actionMode == ActionMode_Demolish),
					SDL_SCANCODE_X, "(X)"))
		{
			uiState->actionMode = ActionMode_Demolish;
			setCursor(uiState, assets, Cursor_Demolish);
		}
		buttonRect.x += buttonRect.w + uiPadding;
		if (uiButton(uiState, renderer, assets, inputState, "Plant", buttonRect, 1,
					(uiState->actionMode == ActionMode_Plant),
					SDL_SCANCODE_P, "(P)"))
		{
			uiState->actionMode = ActionMode_Plant;
			setCursor(uiState, assets, Cursor_Plant);
		}
		buttonRect.x += buttonRect.w + uiPadding;
		if (uiButton(uiState, renderer, assets, inputState, "Harvest", buttonRect, 1,
					(uiState->actionMode == ActionMode_Harvest),
					SDL_SCANCODE_H, "(H)"))
		{
			uiState->actionMode = ActionMode_Harvest;
			setCursor(uiState, assets, Cursor_Harvest);
		}
		buttonRect.x += buttonRect.w + uiPadding;
		if (uiButton(uiState, renderer, assets, inputState, "Hire Worker", buttonRect, 1,
					(uiState->actionMode == ActionMode_Hire),
					SDL_SCANCODE_G, "(G)"))
		{
			uiState->actionMode = ActionMode_Hire;
			setCursor(uiState, assets, Cursor_Hire);
		}
	}
}

// Returns true if game should restart
bool updateAndRenderGameOverUI(Renderer *renderer, AssetManager *assets, UIState *uiState, InputState *inputState, bool won)
{
	bool result = false;
	real32 windowWidth = (real32) renderer->uiBuffer.camera.size.x;
	real32 windowHeight = (real32) renderer->uiBuffer.camera.size.y;

	UITheme *theme = &assets->theme;
	BitmapFont *font = getFont(assets, FontAssetType_Main);

	V2 cameraCentre = renderer->uiBuffer.camera.pos;
	drawRect(&renderer->uiBuffer, rectXYWH(0, 0, windowWidth, windowHeight), 10, theme->overlayColor);

	char gameOverText[256];
	if (won)
	{
		sprintf(gameOverText, "You won! You earned £%d in ??? days", gameWinFunds);
	} else {
		sprintf(gameOverText, "Game over! You ran out of money! :(");
	}

	uiText(uiState, renderer, font, gameOverText, cameraCentre - v2(0, 32), ALIGN_CENTRE, 11, theme->labelStyle.textColor);

	if (uiButton(uiState, renderer, assets, inputState, "Menu", rectCentreSize(cameraCentre, v2(80, 24)), 11))
	{
		result = true;
	}

	return result;
}

void updateAndRenderGame(AppState *appState, InputState *inputState, Renderer *renderer, AssetManager *assets)
{
	if (appState->gameState == 0)
	{
		appState->gameState = initialiseGameState();
		renderer->worldBuffer.camera.pos = v2(appState->gameState->city.width/2, appState->gameState->city.height/2);
	}

	GameState *gameState = appState->gameState;

	//UIState *uiState = &gameState->uiState;

	// Win and Lose!
	if (gameState->city.funds >= gameWinFunds) {
		gameState->status = GameStatus_Won;
	} else if (gameState->city.funds < 0) {
		gameState->status = GameStatus_Lost;
	}

	// CAMERA!
	Camera *worldCamera = &renderer->worldBuffer.camera;
	Camera *uiCamera = &renderer->uiBuffer.camera;
	
	// TODO: Unproject should happen somewhere outside of the game code.
	V2 mouseWorldPos = unproject(worldCamera, inputState->mousePosNormalised);
	Coord mouseTilePos = tilePosition(mouseWorldPos);

	V2 mouseUIPos = unproject(uiCamera, inputState->mousePosNormalised);

	#if 0 // UiButton/Mouse interaction
	if (gameState->status == GameStatus_Playing) {

		if (keyJustPressed(inputState, SDL_SCANCODE_INSERT)) {
			gameState->city.funds += 10000;
		} else if (keyJustPressed(inputState, SDL_SCANCODE_DELETE)) {
			gameState->city.funds -= 10000;
		}

		// Camera controls
		// HOME resets the camera and centres on the HQ
		if (keyJustPressed(inputState, SDL_SCANCODE_HOME)) {
			worldCamera->zoom = 1;
			// Jump to the farmhouse if we have one!
			if (gameState->city.firstBuildingOfType[BA_Farmhouse]) {
				worldCamera->pos = centre(gameState->city.firstBuildingOfType[BA_Farmhouse]->footprint);
			} else {
				pushUiMessage(uiState, "Build an HQ, then pressing [Home] will take you there.");
			}
		}

		// SDL_Log("Mouse world position: %f, %f", mouseWorldPos.x, mouseWorldPos.y);
		// This is a very basic check for "is the user clicking on the UI?"
		if (!inRects(uiState->uiRects, uiState->uiRectCount, mouseUIPos)) {
			switch (uiState->actionMode) {
				case ActionMode_Build: {
					if (mouseButtonPressed(inputState, SDL_BUTTON_LEFT)) {
						placeBuilding(&uiState, &gameState->city, uiState->selectedBuildingArchetype, mouseTilePos);
					}

					int32 buildCost = buildingDefinitions[uiState->selectedBuildingArchetype].buildCost;
					showCostTooltip(renderer, &uiState, buildCost, gameState->city.funds);
				} break;

				case ActionMode_Demolish: {
					if (mouseButtonJustPressed(inputState, SDL_BUTTON_LEFT)) {
						mouseDragStartPos = mouseWorldPos;
						dragRect = irectXYWH(mouseTilePos.x, mouseTilePos.y, 1, 1);
					} else if (mouseButtonPressed(inputState, SDL_BUTTON_LEFT)) {
						dragRect = irectCovering(mouseDragStartPos, mouseWorldPos);
						int32 demolitionCost = calculateDemolitionCost(&gameState->city, dragRect);
						showCostTooltip(renderer, &uiState, demolitionCost, gameState->city.funds);
					}	

					if (mouseButtonJustReleased(inputState, SDL_BUTTON_LEFT)) {
						// Demolish everything within dragRect!
						demolishRect(&uiState, &gameState->city, dragRect);
						dragRect = irectXYWH(-1,-1,0,0);
					}
				} break;

				case ActionMode_None: {
					if (mouseButtonJustPressed(inputState, SDL_BUTTON_LEFT)) {
						int tileI = tileIndex(&gameState->city, mouseTilePos.x, mouseTilePos.y);
						int buildingID = gameState->city.tileBuildings[tileI];

						SDL_Log("Building ID at position (%d,%d) = %d", mouseTilePos.x, mouseTilePos.y, buildingID);
					}
				} break;
			}
		}

		if (mouseButtonJustPressed(inputState, SDL_BUTTON_RIGHT)) {
			// Unselect current thing
			// setActiveButton(&actionButtonGroup, null);
			uiState->actionMode = ActionMode_None;
			setCursor(uiState, assets, Cursor_Main);
		}
	}
	#endif

	// RENDERING

	// Draw terrain
	V2 backgroundSize = v2(gameState->city.width, gameState->city.height); //v2(2000.0f / TILE_SIZE, 1517.0f / TILE_SIZE);
	drawTextureRegion(&renderer->worldBuffer, getTextureRegion(assets, TextureAssetType_Map1, 0),
		rectXYWH(0, 0, backgroundSize.x, backgroundSize.y), 0);

	#if 0 // Terrain, which we don't use
	for (int32 y = (cameraBounds.y < 0) ? 0 : (int32)cameraBounds.y;
		(y < gameState->city.height) && (y < cameraBounds.y + cameraBounds.h);
		y++)
	{
		for (int32 x = (cameraBounds.x < 0) ? 0 : (int32)cameraBounds.x;
			(x < gameState->city.width) && (x < cameraBounds.x + cameraBounds.w);
			x++)
		{
			Terrain t = terrainAt(&gameState->city,x,y);
			GL_TextureAtlasItem textureAtlasItem;
			switch (t) {
				case Terrain_Forest: {
					textureAtlasItem = GL_TextureAtlasItem_ForestTile;
				} break;
				case Terrain_Water: {
					textureAtlasItem = GL_TextureAtlasItem_WaterTile;
				} break;
				case Terrain_Ground:
				default: {
					textureAtlasItem = GL_TextureAtlasItem_GroundTile;
				} break;
			}

			drawGL_TextureAtlasItem(renderer, false, textureAtlasItem,
				v2(x+0.5f,y+0.5f), v2(1.0f, 1.0f), -1000);

			#if 0 // Data layer rendering
			int32 pathGroup = pathGroupAt(&gameState->city, x, y);
			if (pathGroup > 0)
			{
				Color color = {};
				switch (pathGroup)
				{
					case 1: color = {0, 0, 255, 63}; break;
					case 2: color = {0, 255, 0, 63}; break;
					case 3: color = {255, 0, 0, 63}; break;
					case 4: color = {0, 255, 255, 63}; break;
					case 5: color = {255, 255, 0, 63}; break;
					case 6: color = {255, 0, 255, 63}; break;

					default: color = {255, 255, 255, 63}; break;
				}

				drawRect(renderer, false, rectXYWH(x, y, 1, 1), depthFromY(y) + 100.0f, &color);
			}
			#endif
		}
	}
	#endif
	
	#if 0 // Draw buildings
	for (uint32 i=1; i<gameState->city.buildingCount; i++)
	{
		Building building = gameState->city.buildings[i];

		BuildingDefinition *def = buildingDefinitions + building.archetype;

		V4 drawColor = makeWhite();

		if (gameState->uiState.actionMode == ActionMode_Demolish
			&& rectsOverlap(building.footprint, dragRect)) {
			// Draw building red to preview demolition
			drawColor = color255(255,128,128,255);
		}

		switch (building.archetype) {

			default: {
				V2 drawPos = centre(building.footprint);
				drawTextureAtlasItem(renderer, &renderer->worldBuffer, def->textureAtlasItem,
				 	drawPos, v2(building.footprint.dim), depthFromY(drawPos.y), drawColor);
			} break;
		}
	}

	// Building preview
	if (gameState->uiState.actionMode == ActionMode_Build
		&& gameState->uiState.selectedBuildingArchetype != BA_None) {

		V4 ghostColor = color255(128,255,128,255);
		if (!canPlaceBuilding(&gameState->uiState, &gameState->city, gameState->uiState.selectedBuildingArchetype, mouseTilePos)) {
			ghostColor = color255(255,0,0,128);
		}
		Rect footprint = irectCentreDim(mouseTilePos, buildingDefinitions[gameState->uiState.selectedBuildingArchetype].size);
		drawGL_TextureAtlasItem(
			renderer,
			false,
			buildingDefinitions[gameState->uiState.selectedBuildingArchetype].textureAtlasItem,
			centre(footprint),
			v2(footprint.dim),
			depthFromY(mouseTilePos.y) + 100,
			ghostColor
		);
	} else if (gameState->uiState.actionMode == ActionMode_Demolish
		&& mouseButtonPressed(inputState, SDL_BUTTON_LEFT)) {
		// Demolition outline
		drawRect(renderer, false, realRect(dragRect), 0, color255(128, 0, 0, 128));
	}
	#endif

	// Draw the UI!
	switch (gameState->status)
	{
		case GameStatus_Playing:
		{
			updateAndRenderGameUI(renderer, assets, &appState->uiState, gameState, inputState);
		}
		break;

		case GameStatus_Won:
		case GameStatus_Lost:
		{
			updateAndRenderGameUI(renderer, assets, &appState->uiState, gameState, inputState);

			if (updateAndRenderGameOverUI(renderer, assets, &appState->uiState, inputState, gameState->status == GameStatus_Won))
			{
				appState->appStatus = AppStatus_MainMenu;
				// // End the game. Hmmm.
				// gameState = startGame(gameState->gameArena);

				// gameState->status = GameStatus_Setup;
			}
		}
		break;
	}

	if (appState->appStatus == AppStatus_Game)
	{
		drawTooltip(&appState->uiState, renderer, assets, inputState);
		drawUiMessage(&appState->uiState, renderer, assets);

		inputMoveCamera(worldCamera, inputState, gameState->city.width, gameState->city.height);
	}
}

void updateAndRender(AppState *appState, InputState *inputState, Renderer *renderer, AssetManager *assets)
{
	switch (appState->appStatus)
	{
		case AppStatus_MainMenu:
		{
			updateAndRenderMainMenu(appState, inputState, renderer, assets);
		} break;
		case AppStatus_Game:
		{
			updateAndRenderGame(appState, inputState, renderer, assets);
		} break;
	}
}