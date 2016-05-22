

// This is less 'start game' and more 'reset the map and everything so we can show 
// an empty map in the background of the menu'. But also does resetting of things for when you
// click 'Play'. This code is madness, basically.
GameState *startGame(MemoryArena *gameArena)
{
	GameState *result = 0;

	ResetMemoryArena(gameArena);
	result = PushStruct(gameArena, GameState);
	result->arena = gameArena;

	random_seed(&result->rng, 0);

#if 0
	// Test the RNG!
	int32 randomNumbers[1024];
	for (int i=0; i<1024; i++)
	{
		randomNumbers[i] = random_next(&result->rng);
	}
#endif

	initCity(gameArena, &result->city, 100,100, "City Name Here", gameStartFunds);
	generateTerrain(&result->city, &result->rng);

	return result;
}

void gameUpdateAndRender(GameState *gameState, GLRenderer *renderer, InputState *inputState)
{

	// Game simulation

	// Win and Lose!
	if (gameState->city.funds >= gameWinFunds) {
		gameState->status = GameStatus_Won;
	} else if (gameState->city.funds < 0) {
		gameState->status = GameStatus_Lost;
	}

	// CAMERA!
	updateCamera(&renderer->worldCamera, inputState, gameState->city.width, gameState->city.height);

	real32 worldScale = renderer->worldCamera.zoom / TILE_SIZE;
	real32 camWidth = renderer->worldCamera.windowWidth * worldScale,
			camHeight = renderer->worldCamera.windowHeight * worldScale;
	real32 halfCamWidth = camWidth * 0.5f,
			halfCamHeight = camHeight * 0.5f;
	RealRect cameraBounds = {
		renderer->worldCamera.pos.x - halfCamWidth,
		renderer->worldCamera.pos.y - halfCamHeight,
		camWidth, camHeight
	};
	renderer->worldBuffer.projectionMatrix = orthographicMatrix4(
		renderer->worldCamera.pos.x - halfCamWidth, renderer->worldCamera.pos.x + halfCamWidth,
		renderer->worldCamera.pos.y - halfCamHeight, renderer->worldCamera.pos.y + halfCamHeight,
		-1000.0f, 1000.0f
	);
	renderer->uiBuffer.projectionMatrix = orthographicMatrix4(
		0, (GLfloat) renderer->worldCamera.windowWidth,
		0, (GLfloat) renderer->worldCamera.windowHeight,
		-1000.0f, 1000.0f
	);
	
	V2 mouseWorldPos = unproject(renderer, v2(inputState->mousePos));
	Coord mouseTilePos = tilePosition(mouseWorldPos);

// UiButton/Mouse interaction
	if (gameState->status == GameStatus_Playing) {

		if (keyJustPressed(inputState, SDL_SCANCODE_INSERT)) {
			gameState->city.funds += 10000;
		} else if (keyJustPressed(inputState, SDL_SCANCODE_DELETE)) {
			gameState->city.funds -= 10000;
		}

		// Camera controls
		// HOME resets the camera and centres on the HQ
		if (keyJustPressed(inputState, SDL_SCANCODE_HOME)) {
			renderer->worldCamera.zoom = 1;
			// Jump to the farmhouse if we have one!
			if (gameState->city.firstBuildingOfType[BA_Farmhouse]) {
				renderer->worldCamera.pos = centre(gameState->city.firstBuildingOfType[BA_Farmhouse]->footprint);
			} else {
				pushUiMessage(&gameState->uiState, "Build an HQ, then pressing [Home] will take you there.");
			}
		}

		// SDL_Log("Mouse world position: %f, %f", mouseWorldPos.x, mouseWorldPos.y);

		// This is a very basic check for "is the user clicking on the UI?"
		if (!inRects(gameState->uiState.uiRects, gameState->uiState.uiRectCount, v2(inputState->mousePos))) {
			switch (gameState->uiState.actionMode) {
				// case ActionMode_Build: {
				// 	if (mouseButtonPressed(inputState, SDL_BUTTON_LEFT)) {
				// 		placeBuilding(&gameState->uiState, &gameState->city, gameState->uiState.selectedBuildingArchetype, mouseTilePos);
				// 	}

				// 	int32 buildCost = buildingDefinitions[gameState->uiState.selectedBuildingArchetype].buildCost;
				// 	showCostTooltip(renderer, &gameState->uiState, buildCost, gameState->city.funds);
				// } break;

				// case ActionMode_Demolish: {
				// 	if (mouseButtonJustPressed(inputState, SDL_BUTTON_LEFT)) {
				// 		mouseDragStartPos = mouseWorldPos;
				// 		dragRect = irectXYWH(mouseTilePos.x, mouseTilePos.y, 1, 1);
				// 	} else if (mouseButtonPressed(inputState, SDL_BUTTON_LEFT)) {
				// 		dragRect = irectCovering(mouseDragStartPos, mouseWorldPos);
				// 		int32 demolitionCost = calculateDemolitionCost(&gameState->city, dragRect);
				// 		showCostTooltip(renderer, &gameState->uiState, demolitionCost, gameState->city.funds);
				// 	}	

				// 	if (mouseButtonJustReleased(inputState, SDL_BUTTON_LEFT)) {
				// 		// Demolish everything within dragRect!
				// 		demolishRect(&gameState->uiState, &gameState->city, dragRect);
				// 		dragRect = irectXYWH(-1,-1,0,0);
				// 	}
				// } break;

				// case ActionMode_None: {
				// 	if (mouseButtonJustPressed(inputState, SDL_BUTTON_LEFT)) {
				// 		int tileI = tileIndex(&gameState->city, mouseTilePos.x, mouseTilePos.y);
				// 		int buildingID = gameState->city.tileBuildings[tileI];

				// 		SDL_Log("Building ID at position (%d,%d) = %d", mouseTilePos.x, mouseTilePos.y, buildingID);
				// 	}
				// } break;
			}
		}

		if (mouseButtonJustPressed(inputState, SDL_BUTTON_RIGHT)) {
			// Unselect current thing
			// setActiveButton(&actionButtonGroup, null);
			gameState->uiState.actionMode = ActionMode_None;
			setCursor(renderer, Cursor_Main);
		}
	}

// RENDERING

	// Draw terrain
	drawTextureAtlasItem(renderer, false, TextureAtlasItem_Map1, v2(0,0), v2(2000.0f / TILE_SIZE, 1517.0f / TILE_SIZE), 0);
// 	for (int32 y = (cameraBounds.y < 0) ? 0 : (int32)cameraBounds.y;
// 		(y < gameState->city.height) && (y < cameraBounds.y + cameraBounds.h);
// 		y++)
// 	{
// 		for (int32 x = (cameraBounds.x < 0) ? 0 : (int32)cameraBounds.x;
// 			(x < gameState->city.width) && (x < cameraBounds.x + cameraBounds.w);
// 			x++)
// 		{
// 			Terrain t = terrainAt(&gameState->city,x,y);
// 			TextureAtlasItem textureAtlasItem;
// 			switch (t) {
// 				case Terrain_Forest: {
// 					textureAtlasItem = TextureAtlasItem_ForestTile;
// 				} break;
// 				case Terrain_Water: {
// 					textureAtlasItem = TextureAtlasItem_WaterTile;
// 				} break;
// 				case Terrain_Ground:
// 				default: {
// 					textureAtlasItem = TextureAtlasItem_GroundTile;
// 				} break;
// 			}

// 			drawTextureAtlasItem(renderer, false, textureAtlasItem,
// 				v2(x+0.5f,y+0.5f), v2(1.0f, 1.0f), -1000);

// #if 0
// 			// Data layer
// 			int32 pathGroup = pathGroupAt(&gameState->city, x, y);
// 			if (pathGroup > 0)
// 			{
// 				Color color = {};
// 				switch (pathGroup)
// 				{
// 					case 1: color = {0, 0, 255, 63}; break;
// 					case 2: color = {0, 255, 0, 63}; break;
// 					case 3: color = {255, 0, 0, 63}; break;
// 					case 4: color = {0, 255, 255, 63}; break;
// 					case 5: color = {255, 255, 0, 63}; break;
// 					case 6: color = {255, 0, 255, 63}; break;

// 					default: color = {255, 255, 255, 63}; break;
// 				}

// 				drawRect(renderer, false, rectXYWH(x, y, 1, 1), depthFromY(y) + 100.0f, &color);
// 			}
// #endif
// 		}
// 	}

	// Draw buildings
	for (uint32 i=1; i<gameState->city.buildingCount; i++)
	{
		Building building = gameState->city.buildings[i];

		BuildingDefinition *def = buildingDefinitions + building.archetype;

		V4 drawColor = makeWhite();

		// if (gameState->uiState.actionMode == ActionMode_Demolish
		// 	&& rectsOverlap(building.footprint, dragRect)) {
		// 	// Draw building red to preview demolition
		// 	drawColor = color255(255,128,128,255);
		// }

		switch (building.archetype) {

			default: {
				V2 drawPos = centre(building.footprint);
				drawTextureAtlasItem(renderer, false, def->textureAtlasItem,
				 	drawPos, v2(building.footprint.dim), depthFromY(drawPos.y), drawColor);
			} break;
		}
	}

	// Building preview
	// if (gameState->uiState.actionMode == ActionMode_Build
	// 	&& gameState->uiState.selectedBuildingArchetype != BA_None) {

	// 	V4 ghostColor = color255(128,255,128,255);
	// 	if (!canPlaceBuilding(&gameState->uiState, &gameState->city, gameState->uiState.selectedBuildingArchetype, mouseTilePos)) {
	// 		ghostColor = color255(255,0,0,128);
	// 	}
	// 	Rect footprint = irectCentreDim(mouseTilePos, buildingDefinitions[gameState->uiState.selectedBuildingArchetype].size);
	// 	drawTextureAtlasItem(
	// 		renderer,
	// 		false,
	// 		buildingDefinitions[gameState->uiState.selectedBuildingArchetype].textureAtlasItem,
	// 		centre(footprint),
	// 		v2(footprint.dim),
	// 		depthFromY(mouseTilePos.y) + 100,
	// 		ghostColor
	// 	);
	// } else if (gameState->uiState.actionMode == ActionMode_Demolish
	// 	&& mouseButtonPressed(inputState, SDL_BUTTON_LEFT)) {
	// 	// Demolition outline
	// 	drawRect(renderer, false, realRect(dragRect), 0, color255(128, 0, 0, 128));
	// }

	// Draw the UI!
	switch (gameState->status)
	{
		case GameStatus_Setup:
		{
			gameState->status = updateAndRenderMainMenuUI(renderer, &gameState->uiState, inputState, gameState->status);
		}
		break;

		case GameStatus_Playing:
		{
			updateAndRenderGameUI(renderer, &gameState->uiState, gameState, inputState);
		}
		break;

		case GameStatus_Won:
		case GameStatus_Lost:
		{
			updateAndRenderGameUI(renderer, &gameState->uiState, gameState, inputState);

			if (updateAndRenderGameOverUI(renderer, &gameState->uiState, gameState, inputState, gameState->status == GameStatus_Won))
			{
				gameState = startGame(gameState->arena);

				gameState->status = GameStatus_Setup;
			}
		}
		break;
	}

	drawTooltip(renderer, inputState, &gameState->uiState);
	drawUiMessage(renderer, &gameState->uiState);
}