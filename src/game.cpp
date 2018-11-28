#pragma once

#include "game_mainmenu.cpp"
#include "credits.cpp"
#include "settings.cpp"

const f32 uiPadding = 4; // TODO: Move this somewhere sensible!

GameState *initialiseGameState()
{
	GameState *result;
	bootstrapArena(GameState, result, gameArena);

	initCity(&result->gameArena, &result->city, 2000 / ITILE_SIZE, 1517 / ITILE_SIZE, LocalString("City Name Here"), gameStartFunds);

	result->status = GameStatus_Playing;

	return result;
}

void inputMoveCamera(Camera *camera, InputState *inputState, s32 cityWidth, s32 cityHeight)
{ 
	// Zooming
	if (canZoom && inputState->wheelY) {
		// round()ing the zoom so it doesn't gradually drift due to float imprecision
		camera->zoom = clamp(round(10 * camera->zoom - inputState->wheelY) * 0.1f, 0.1f, 10.0f);
	}

	// Panning
	f32 scrollSpeed = (CAMERA_PAN_SPEED * sqrt(camera->zoom)) * SECONDS_PER_FRAME;
	if (mouseButtonPressed(inputState, SDL_BUTTON_MIDDLE))
	{
		// Click-panning!
		float scale = scrollSpeed * 5.0f;
		V2 clickStartPos = inputState->clickStartPosition[mouseButtonIndex(SDL_BUTTON_MIDDLE)];
		camera->pos.x += (inputState->mousePosNormalised.x - clickStartPos.x) * scale;
		camera->pos.y += (inputState->mousePosNormalised.y - clickStartPos.y) * -scale;
	}
	else
	{
		if (keyIsPressed(inputState, SDLK_LEFT)
			|| keyIsPressed(inputState, SDLK_a)
			|| (inputState->mousePosNormalised.x < (-1.0f + CAMERA_EDGE_SCROLL_MARGIN)))
		{
			camera->pos.x -= scrollSpeed;
		}
		else if (keyIsPressed(inputState, SDLK_RIGHT)
			|| keyIsPressed(inputState, SDLK_d)
			|| (inputState->mousePosNormalised.x > (1.0f - CAMERA_EDGE_SCROLL_MARGIN)))
		{
			camera->pos.x += scrollSpeed;
		}

		if (keyIsPressed(inputState, SDLK_UP)
			|| keyIsPressed(inputState, SDLK_w)
			|| (inputState->mousePosNormalised.y > (1.0f - CAMERA_EDGE_SCROLL_MARGIN)))
		{
			camera->pos.y -= scrollSpeed;
		}
		else if (keyIsPressed(inputState, SDLK_DOWN)
			|| keyIsPressed(inputState, SDLK_s)
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
		f32 minX = (cameraSize.x * 0.5f) - CAMERA_MARGIN;
		camera->pos.x = clamp( camera->pos.x, minX, (f32)cityWidth - minX );
	}

	if (cityHeight < camera->size.y / camera->zoom) {
		// City smaller than camera, so centre on it
		camera->pos.y = cityHeight * 0.5f;
	} else {
		f32 minY = (cameraSize.y * 0.5f) - CAMERA_MARGIN;
		camera->pos.y = clamp( camera->pos.y, minY, (f32)cityHeight - minY );
	}
}

void updateAndRenderGameUI(RenderBuffer *uiBuffer, AssetManager *assets, UIState *uiState, GameState *gameState,
	                       InputState *inputState)
{
	DEBUG_FUNCTION();
	
	f32 windowWidth = (f32) uiBuffer->camera.size.x;
	V2 centre = uiBuffer->camera.pos;
	UITheme *theme = &assets->theme;
	BitmapFont *font = getFont(assets, theme->labelStyle.font);

	uiState->uiRectCount = 0;

	f32 left = uiPadding;

	Rect2 uiRect = uiState->uiRects[uiState->uiRectCount++] = rectXYWH(0,0, windowWidth, 64);
	drawRect(uiBuffer, uiRect, 0, theme->overlayColor);

	uiText(uiState, uiBuffer, font, gameState->city.name,
	       v2(left, uiPadding), ALIGN_LEFT, 1, theme->labelStyle.textColor);

	uiText(uiState, uiBuffer, font, myprintf("£{0}", {formatInt(gameState->city.funds)}),
	       v2(centre.x, uiPadding), ALIGN_RIGHT, 1, theme->labelStyle.textColor);

	uiText(uiState, uiBuffer, font, myprintf("(-£{0}/month)", {formatInt(gameState->city.monthlyExpenditure)}),
	       v2(centre.x, uiPadding), ALIGN_LEFT, 1, theme->labelStyle.textColor);

	// Build UI
	{
		Rect2 buttonRect = rectXYWH(uiPadding, 28 + uiPadding, 80, 24);

		if (uiMenuButton(uiState, uiBuffer, assets, inputState, LocalString("Build..."), buttonRect, 1, UIMenu_Build))
		{
			Rect2 menuButtonRect = buttonRect;
			menuButtonRect.y += menuButtonRect.h + uiPadding;

			Rect2 menuRect = expand(menuButtonRect, uiPadding);

			if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Build HQ"), menuButtonRect, 1,
					(uiState->actionMode == ActionMode_Build) && (uiState->selectedBuildingArchetype == BA_Farmhouse),
					SDLK_q, LocalString("(Q)")))
			{
				uiState->openMenu = UIMenu_None;
				uiState->selectedBuildingArchetype = BA_Farmhouse;
				uiState->actionMode = ActionMode_Build;
				setCursor(uiState, assets, Cursor_Build);
			}
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			menuRect.h += menuButtonRect.h + uiPadding;

			if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Build Field"), menuButtonRect, 1,
						(uiState->actionMode == ActionMode_Build) && (uiState->selectedBuildingArchetype == BA_Field),
						SDLK_f, LocalString("(F)")))
			{
				uiState->openMenu = UIMenu_None;
				uiState->selectedBuildingArchetype = BA_Field;
				uiState->actionMode = ActionMode_Build;
				setCursor(uiState, assets, Cursor_Build);
			}
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			menuRect.h += menuButtonRect.h + uiPadding;

			if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Build Barn"), menuButtonRect, 1,
						(uiState->actionMode == ActionMode_Build) && (uiState->selectedBuildingArchetype == BA_Barn),
						SDLK_b, LocalString("(B)")))
			{
				uiState->openMenu = UIMenu_None;
				uiState->selectedBuildingArchetype = BA_Barn;
				uiState->actionMode = ActionMode_Build;
				setCursor(uiState, assets, Cursor_Build);
			}
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			menuRect.h += menuButtonRect.h + uiPadding;

			if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Build Road"), menuButtonRect, 1,
						(uiState->actionMode == ActionMode_Build) && (uiState->selectedBuildingArchetype == BA_Path),
						SDLK_r, LocalString("(R)")))
			{
				uiState->openMenu = UIMenu_None;
				uiState->selectedBuildingArchetype = BA_Path;
				uiState->actionMode = ActionMode_Build;
				setCursor(uiState, assets, Cursor_Build);
			}

			uiState->uiRects[uiState->uiRectCount++] = menuRect;
			drawRect(uiBuffer, menuRect, 0, theme->overlayColor);
		}

		buttonRect.x += buttonRect.w + uiPadding;
		if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Demolish"), buttonRect, 1,
					(uiState->actionMode == ActionMode_Demolish),
					SDLK_x, LocalString("(X)")))
		{
			uiState->actionMode = ActionMode_Demolish;
			setCursor(uiState, assets, Cursor_Demolish);
		}
		buttonRect.x += buttonRect.w + uiPadding;
		if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Plant"), buttonRect, 1,
					(uiState->actionMode == ActionMode_Plant),
					SDLK_p, LocalString("(P)")))
		{
			uiState->actionMode = ActionMode_Plant;
			setCursor(uiState, assets, Cursor_Plant);
		}
		buttonRect.x += buttonRect.w + uiPadding;
		if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Harvest"), buttonRect, 1,
					(uiState->actionMode == ActionMode_Harvest),
					SDLK_h, LocalString("(H)")))
		{
			uiState->actionMode = ActionMode_Harvest;
			setCursor(uiState, assets, Cursor_Harvest);
		}
		buttonRect.x += buttonRect.w + uiPadding;
		if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Hire Worker"), buttonRect, 1,
					(uiState->actionMode == ActionMode_Hire),
					SDLK_g, LocalString("(G)")))
		{
			uiState->actionMode = ActionMode_Hire;
			setCursor(uiState, assets, Cursor_Hire);
		}

		// Main menu button
		buttonRect.x = windowWidth - (buttonRect.w + uiPadding);
		if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Menu"), buttonRect, 1))
		{
			// quit game somehow.
			gameState->status = GameStatus_Quit;
		}
	}
}

// Returns true if game should restart
bool updateAndRenderGameOverUI(RenderBuffer *uiBuffer, AssetManager *assets, UIState *uiState,
	                           InputState *inputState, bool won)
{
	DEBUG_FUNCTION();
	
	bool result = false;
	f32 windowWidth = (f32) uiBuffer->camera.size.x;
	f32 windowHeight = (f32) uiBuffer->camera.size.y;
	V2 cameraCentre = uiBuffer->camera.pos;

	UITheme *theme = &assets->theme;
	BitmapFont *font = getFont(assets, FontAssetType_Main);

	drawRect(uiBuffer, rectXYWH(0, 0, windowWidth, windowHeight), 10, theme->overlayColor);

	String gameOverText;
	if (won)
	{
		gameOverText = myprintf(LocalString("You won! You earned £{0} in ??? days"), {formatInt(gameWinFunds)});
	} else {
		gameOverText = LocalString("Game over! You ran out of money! :(");
	}

	uiText(uiState, uiBuffer, font, gameOverText, cameraCentre - v2(0, 32), ALIGN_CENTRE, 11, theme->labelStyle.textColor);

	if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Menu"), rectCentreSize(cameraCentre, v2(80, 24)), 11))
	{
		result = true;
	}

	return result;
}

void updateAndRenderGame(AppState *appState, InputState *inputState, Renderer *renderer, AssetManager *assets)
{
	DEBUG_FUNCTION();
	
	if (appState->gameState == 0)
	{
		appState->gameState = initialiseGameState();
		renderer->worldBuffer.camera.pos = v2(appState->gameState->city.width/2, appState->gameState->city.height/2);
	}

	GameState *gameState = appState->gameState;

	UIState *uiState = &globalAppState.uiState;

	// Win and Lose!
	if (gameState->city.funds >= gameWinFunds) {
		gameState->status = GameStatus_Won;
	} else if (gameState->city.funds < 0) {
		gameState->status = GameStatus_Lost;
	}

	// CAMERA!
	Camera *worldCamera = &renderer->worldBuffer.camera;
	Camera *uiCamera    = &renderer->uiBuffer.camera;
	V2I mouseTilePos = tilePosition(worldCamera->mousePos);

	Rect2I dragRect = irectNegativeInfinity();
	V2 mouseDragStartPos = v2(-1,-1);

	// UiButton/Mouse interaction
	if (gameState->status == GameStatus_Playing) {

		if (keyJustPressed(inputState, SDLK_INSERT)) {
			gameState->city.funds += 10000;
		} else if (keyJustPressed(inputState, SDLK_DELETE)) {
			gameState->city.funds -= 10000;
		}

		// Camera controls
		// HOME resets the camera and centres on the HQ
		if (keyJustPressed(inputState, SDLK_HOME)) {
			worldCamera->zoom = 1;
			// Jump to the farmhouse if we have one!
			if (gameState->city.firstBuildingOfType[BA_Farmhouse]) {
				worldCamera->pos = centre(gameState->city.firstBuildingOfType[BA_Farmhouse]->footprint);
			} else {
				pushUiMessage(uiState, LocalString("Build an HQ, then pressing [Home] will take you there."));
			}
		}

		// SDL_Log("Mouse world position: %f, %f", worldCamera->mousePos.x, worldCamera->mousePos.y);
		// This is a very basic check for "is the user clicking on the UI?"
		if (!inRects(uiState->uiRects, uiState->uiRectCount, uiCamera->mousePos)) {
			switch (uiState->actionMode) {
				case ActionMode_Build: {
					if (mouseButtonPressed(inputState, SDL_BUTTON_LEFT)) {
						placeBuilding(uiState, &gameState->city, uiState->selectedBuildingArchetype, mouseTilePos);
					}

					s32 buildCost = buildingDefinitions[uiState->selectedBuildingArchetype].buildCost;
					// showCostTooltip(renderer, uiState, buildCost, gameState->city.funds);
				} break;

				case ActionMode_Demolish: {
					if (mouseButtonJustPressed(inputState, SDL_BUTTON_LEFT)) {
						mouseDragStartPos = worldCamera->mousePos;
						dragRect = irectXYWH(mouseTilePos.x, mouseTilePos.y, 1, 1);
					} else if (mouseButtonPressed(inputState, SDL_BUTTON_LEFT)) {
						dragRect = irectCovering(mouseDragStartPos, worldCamera->mousePos);
						s32 demolitionCost = calculateDemolitionCost(&gameState->city, dragRect);
						// showCostTooltip(renderer, uiState, demolitionCost, gameState->city.funds);
					}	

					if (mouseButtonJustReleased(inputState, SDL_BUTTON_LEFT)) {
						// Demolish everything within dragRect!
						demolishRect(uiState, &gameState->city, dragRect);
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

	// RENDERING

	// Draw terrain

	Rect2 cameraBounds = rectCentreSize(worldCamera->pos, worldCamera->size);

	// Terrain, which we don't use
	for (s32 y = (cameraBounds.y < 0) ? 0 : (s32)cameraBounds.y;
		(y < gameState->city.height) && (y < cameraBounds.y + cameraBounds.h);
		y++)
	{
		for (s32 x = (cameraBounds.x < 0) ? 0 : (s32)cameraBounds.x;
			(x < gameState->city.width) && (x < cameraBounds.x + cameraBounds.w);
			x++)
		{
			Terrain t = terrainAt(&gameState->city,x,y);
			TextureAssetType textureAtlasItem;
			switch (t) {
				case Terrain_Forest: {
					textureAtlasItem = TextureAssetType_ForestTile;
				} break;
				case Terrain_Water: {
					textureAtlasItem = TextureAssetType_WaterTile;
				} break;
				case Terrain_Ground:
				default: {
					textureAtlasItem = TextureAssetType_GroundTile;
				} break;
			}

			drawTextureRegion(&renderer->worldBuffer, textureAtlasItem, rectXYWH(x, y, 1.0f, 1.0f), -1000.0f);

			#if 0 // Data layer rendering
			s32 pathGroup = pathGroupAt(&gameState->city, x, y);
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

				drawRect2I(renderer, false, rectXYWH(x, y, 1, 1), depthFromY(y) + 100.0f, &color);
			}
			#endif
		}
	}
	
	for (u32 i=1; i<gameState->city.buildingCount; i++)
	{
		Building building = gameState->city.buildings[i];

		BuildingDefinition *def = buildingDefinitions + building.archetype;

		V4 drawColor = makeWhite();

		if (uiState->actionMode == ActionMode_Demolish
			&& rectsOverlap(building.footprint, dragRect)) {
			// Draw building red to preview demolition
			drawColor = color255(255,128,128,255);
		}

		switch (building.archetype) {

			default: {
				V2 drawPos = centre(building.footprint);
				drawTextureRegion(&renderer->worldBuffer, def->textureAtlasItem,
								  rect2(building.footprint), depthFromY(drawPos.y), drawColor);
			} break;
		}
	}

	// Building preview
	if (uiState->actionMode == ActionMode_Build
		&& uiState->selectedBuildingArchetype != BA_None) {

		V4 ghostColor = color255(128,255,128,255);
		if (!canPlaceBuilding(uiState, &gameState->city, uiState->selectedBuildingArchetype, mouseTilePos)) {
			ghostColor = color255(255,0,0,128);
		}
		Rect2 footprint = rectCentreSize(v2(mouseTilePos), v2(buildingDefinitions[uiState->selectedBuildingArchetype].size));
		drawTextureRegion(&renderer->worldBuffer,
						  buildingDefinitions[uiState->selectedBuildingArchetype].textureAtlasItem,
						  footprint, depthFromY(mouseTilePos.y) + 100, ghostColor);
	} else if (uiState->actionMode == ActionMode_Demolish
		&& mouseButtonPressed(inputState, SDL_BUTTON_LEFT)) {
		// Demolition outline
		drawRect(&renderer->worldBuffer, rect2(dragRect), 0, color255(128, 0, 0, 128));
	}

	// Draw the UI!
	switch (gameState->status)
	{
		case GameStatus_Playing:
		{
			updateAndRenderGameUI(&renderer->uiBuffer, assets, uiState, gameState, inputState);
		}
		break;

		case GameStatus_Won:
		case GameStatus_Lost:
		{
			if (updateAndRenderGameOverUI(&renderer->uiBuffer, assets, uiState, inputState, gameState->status == GameStatus_Won))
			{
				gameState->status = GameStatus_Quit;
			}
		}
		break;
	}

	if (gameState->status == GameStatus_Quit)
	{
		freeMemoryArena(&gameState->gameArena);
		appState->gameState = 0;
		appState->appStatus = AppStatus_MainMenu;
	}

	if (appState->appStatus == AppStatus_Game)
	{
		drawTooltip(uiState, &renderer->uiBuffer, assets);
		drawUiMessage(uiState, &renderer->uiBuffer, assets);

		inputMoveCamera(worldCamera, inputState, gameState->city.width, gameState->city.height);
	}
}

void updateAndRender(AppState *appState, InputState *inputState, Renderer *renderer, AssetManager *assets)
{
	DEBUG_FUNCTION();
	
	switch (appState->appStatus)
	{
		case AppStatus_MainMenu:
		{
			updateAndRenderMainMenu(appState, inputState, renderer, assets);
		} break;

		case AppStatus_Credits:
		{
			updateAndRenderCredits(appState, inputState, renderer, assets);
		} break;

		case AppStatus_SettingsMenu:
		{
			updateAndRenderSettingsMenu(appState, inputState, renderer, assets);
		} break;

		case AppStatus_Game:
		{
			updateAndRenderGame(appState, inputState, renderer, assets);
		} break;

		case AppStatus_Quit: break;
		
		default:
		{
			ASSERT(false, "Not implemented this AppStatus yet!");
		} break;
	}
}