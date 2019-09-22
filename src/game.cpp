#pragma once

GameState *beginNewGame()
{
	GameState *result;
	bootstrapArena(GameState, result, gameArena);
	initRandom(&result->gameRandom, Random_MT, 12345);

	s32 gameStartFunds = 1000000;
	initCity(&result->gameArena, &result->gameRandom, &result->city, 133, 117, LOCAL("city_default_name"), gameStartFunds);
	generateTerrain(&result->city);

	result->status = GameStatus_Playing;

	result->actionMode = ActionMode_None;

	result->worldDragState.citySize = result->city.bounds.size;

	renderer->worldCamera.pos = v2(result->city.bounds.size) / 2;

	return result;
}

void freeGameState(GameState *gameState)
{
	freeMemoryArena(&gameState->gameArena);
}

void inputMoveCamera(Camera *camera, V2 windowSize, V2 windowMousePos, s32 cityWidth, s32 cityHeight)
{ 
	DEBUG_FUNCTION();
	
	const f32 CAMERA_MARGIN = 1; // How many tiles beyond the map can the camera scroll to show?
	const f32 CAMERA_PAN_SPEED = 10.0f; // Measured in world units per second

	// Zooming
	s32 zoomDelta = inputState->wheelY;

	// Turns out that having the zoom bound to the same key I use for navigating debug frames is REALLY ANNOYING
	// if (keyJustPressed(SDLK_PAGEUP))
	// {
	// 	zoomDelta++;
	// }
	// else if (keyJustPressed(SDLK_PAGEDOWN))
	// {
	// 	zoomDelta--;
	// }

	if (zoomDelta)
	{
		// round()ing the zoom so it doesn't gradually drift due to float imprecision
		camera->zoom = (f32) clamp(round_f32(10 * camera->zoom + zoomDelta) * 0.1f, 0.1f, 10.0f);
	}

	// Panning
	f32 scrollSpeed = (CAMERA_PAN_SPEED * (f32) sqrt(camera->zoom)) * SECONDS_PER_FRAME;
	f32 cameraEdgeScrollPixelMargin = 8.0f;

	if (mouseButtonPressed(MouseButton_Middle))
	{
		// Click-panning!
		float scale = scrollSpeed * 1.0f;
		V2 clickStartPos = getClickStartPos(MouseButton_Middle, camera);
		camera->pos += (camera->mousePos - clickStartPos) * scale;
	}
	else
	{
		if (keyIsPressed(SDLK_LEFT)
			|| keyIsPressed(SDLK_a)
			|| (windowMousePos.x < cameraEdgeScrollPixelMargin))
		{
			camera->pos.x -= scrollSpeed;
		}
		else if (keyIsPressed(SDLK_RIGHT)
			|| keyIsPressed(SDLK_d)
			|| (windowMousePos.x > (windowSize.x - cameraEdgeScrollPixelMargin)))
		{
			camera->pos.x += scrollSpeed;
		}

		if (keyIsPressed(SDLK_UP)
			|| keyIsPressed(SDLK_w)
			|| (windowMousePos.y < cameraEdgeScrollPixelMargin))
		{
			camera->pos.y -= scrollSpeed;
		}
		else if (keyIsPressed(SDLK_DOWN)
			|| keyIsPressed(SDLK_s)
			|| (windowMousePos.y > (windowSize.y - cameraEdgeScrollPixelMargin)))
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

Rect2I getDragArea(DragState *dragState, DragType dragType, V2I itemSize)
{
	DEBUG_FUNCTION();

	Rect2I result = irectXYWH(0, 0, 0, 0);

	if (dragState->isDragging)
	{
		switch (dragType)
		{
			case DragRect:
			{
				// This is more complicated than a simple rectangle covering both points.
				// If the user is dragging a 3x3 building, then the drag area must cover that 3x3 footprint
				// even if they drag right-to-left, and in the case where the rectangle is not an even multiple
				// of 3, the building placements are aligned to match that initial footprint.
				// I'm struggling to find a good way of describing that... But basically, we want this to be
				// as intuitive to use as possible, and that means there MUST be a building placed where the
				// ghost was before the user pressed the mouse button, no matter which direction or size they
				// then drag it!

				V2I startP = dragState->mouseDragStartWorldPos;
				V2I endP = dragState->mouseDragEndWorldPos;

				if (startP.x < endP.x)
				{
					result.x = startP.x;
					result.w = max(endP.x - startP.x + 1, itemSize.x);

					s32 xRemainder = result.w % itemSize.x;
					result.w -= xRemainder;
				}
				else
				{
					result.x = endP.x;
					result.w = startP.x - endP.x + itemSize.x;

					s32 xRemainder = result.w % itemSize.x;
					result.x += xRemainder;
					result.w -= xRemainder;
				}

				if (startP.y < endP.y)
				{
					result.y = startP.y;
					result.h = max(endP.y - startP.y + 1, itemSize.y);

					s32 yRemainder = result.w % itemSize.y;
					result.h -= yRemainder;
				}
				else
				{
					result.y = endP.y;
					result.h = startP.y - endP.y + itemSize.y;

					s32 yRemainder = result.h % itemSize.y;
					result.y += yRemainder;
					result.h -= yRemainder;
				}

			} break;

			case DragLine:
			{
				// Axis-aligned straight line, in one dimension.
				// So, if you drag a diagonal line, it picks which direction has greater length and uses that.

				V2I startP = dragState->mouseDragStartWorldPos;
				V2I endP = dragState->mouseDragEndWorldPos;

				// determine orientation
				s32 xDiff = abs(startP.x - endP.x);
				s32 yDiff = abs(startP.y - endP.y);

				if (xDiff > yDiff)
				{
					// X
					if (startP.x < endP.x)
					{
						result.x = startP.x;
						result.w = max(xDiff + 1, itemSize.x);

						s32 xRemainder = result.w % itemSize.x;
						result.w -= xRemainder;
					}
					else
					{
						result.x = endP.x;
						result.w = xDiff + itemSize.x;

						s32 xRemainder = result.w % itemSize.x;
						result.x += xRemainder;
						result.w -= xRemainder;
					}

					result.y = startP.y;
					result.h = itemSize.y;
				}
				else
				{
					// Y
					if (startP.y < endP.y)
					{
						result.y = startP.y;
						result.h = max(yDiff + 1, itemSize.y);

						s32 yRemainder = result.h % itemSize.y;
						result.h -= yRemainder;
					}
					else
					{
						result.y = endP.y;
						result.h = yDiff + itemSize.y;

						s32 yRemainder = result.h % itemSize.y;
						result.y += yRemainder;
						result.h -= yRemainder;
					}

					result.x = startP.x;
					result.w = itemSize.x;
				}
			} break;

			INVALID_DEFAULT_CASE;
		}

		result = intersect(result, irectXYWH(0,0, dragState->citySize.x, dragState->citySize.y));
	}

	return result;
}

DragResult updateDragState(DragState *dragState, V2I mouseTilePos, bool mouseIsOverUI, DragType dragType, V2I itemSize)
{
	DEBUG_FUNCTION();

	DragResult result = {};

	if (dragState->isDragging && mouseButtonJustReleased(MouseButton_Left))
	{
		result.operation = DragResult_DoAction;
		result.dragRect = getDragArea(dragState, dragType, itemSize);

		dragState->isDragging = false;
	}
	else
	{
		// Update the dragging state
		if (!mouseIsOverUI && mouseButtonJustPressed(MouseButton_Left))
		{
			dragState->isDragging = true;
			dragState->mouseDragStartWorldPos = dragState->mouseDragEndWorldPos = mouseTilePos;
		}

		if (mouseButtonPressed(MouseButton_Left) && dragState->isDragging)
		{
			dragState->mouseDragEndWorldPos = mouseTilePos;
			result.dragRect = getDragArea(dragState, dragType, itemSize);
		}
		else
		{
			result.dragRect = irectXYWH(mouseTilePos.x, mouseTilePos.y, 1, 1);
		}

		// Preview
		if (!mouseIsOverUI || dragState->isDragging)
		{
			result.operation = DragResult_ShowPreview;
		}
	}

	// minimum size
	if (result.dragRect.w < itemSize.x) result.dragRect.w = itemSize.x;
	if (result.dragRect.h < itemSize.y) result.dragRect.h = itemSize.y;

	return result;
}

void inspectTileWindowProc(WindowContext *context, void *userData)
{
	DEBUG_FUNCTION();

	GameState *gameState = (GameState *) userData;
	City *city = &gameState->city;

	V2I tilePos = gameState->inspectedTilePosition;
	context->window->title = myprintf(LOCAL("title_inspect"), {formatInt(tilePos.x), formatInt(tilePos.y)});

	// CitySector
	CitySector *sector = getSectorAtTilePos(&city->sectors, tilePos.x, tilePos.y);
	window_label(context, myprintf("CitySector: x={0} y={1} w={2} h={3}", {formatInt(sector->bounds.x), formatInt(sector->bounds.y), formatInt(sector->bounds.w), formatInt(sector->bounds.h)}));

	// Terrain
	TerrainDef *terrain = getTerrainAt(city, tilePos.x, tilePos.y);
	window_label(context, myprintf("Terrain: {0}", {terrain->name}));

	// Zone
	ZoneType zone = getZoneAt(city, tilePos.x, tilePos.y);
	window_label(context, myprintf("Zone: {0}", {zone ? getText(getZoneDef(zone).nameID) : "None"s}));

	// Building
	Building *building = getBuildingAt(city, tilePos.x, tilePos.y);
	if (building != null)
	{
		s32 buildingIndex = getTileValue(city, city->tileBuildingIndex, tilePos.x, tilePos.y);
		BuildingDef *def = getBuildingDef(building->typeID);
		window_label(context, myprintf("Building: {0} (ID {1}, array index {2})", {def->name, formatInt(building->id), formatInt(buildingIndex)}));
		window_label(context, myprintf("- Residents: {0} / {1}", {formatInt(building->currentResidents), formatInt(def->residents)}));
		window_label(context, myprintf("- Jobs: {0} / {1}", {formatInt(building->currentJobs), formatInt(def->jobs)}));
		window_label(context, myprintf("- Power: {0}", {formatInt(def->power)}));

		// Problems
		for (s32 problemIndex = 0; problemIndex < BuildingProblemCount; problemIndex++)
		{
			if (building->problems & (BuildingProblem) problemIndex)
			{
				window_label(context, myprintf("- PROBLEM: {0}", {buildingProblemNames[problemIndex]}));
			}
		}
	}
	else
	{
		window_label(context, "Building: None"s);
	}

	// Power group
	PowerNetwork *powerNetwork = getPowerNetworkAt(city, tilePos.x, tilePos.y);
	if (powerNetwork != null)
	{
		window_label(context, myprintf("Power Network {0}:\n- Production: {1}\n- Consumption: {2}\n- Contained groups: {3}", {
			formatInt(powerNetwork->id),
			formatInt(powerNetwork->cachedProduction),
			formatInt(powerNetwork->cachedConsumption),
			formatInt(powerNetwork->groups.count)
		}));
	}

	window_label(context, myprintf("Distance to power: {0}", {formatInt(getDistanceToPower(city, tilePos.x, tilePos.y))}));

	// Transport
	for (s32 transportType = 0; transportType < TransportTypeCount; transportType++)
	{
		window_label(context, myprintf("Distance to transport #{0}: {1}", {formatInt(transportType), formatInt(getDistanceToTransport(city, tilePos.x, tilePos.y, (TransportType)transportType))}));
	}

	// Land value
	window_label(context, myprintf("Land value: {0}%", {formatFloat(getLandValuePercentAt(city, tilePos.x, tilePos.y) * 100.0f, 0)}));

	// Fire
	window_label(context, myprintf("Fire risk: {0}\nFire protection: {1}%", {formatInt(getFireRiskAt(city, tilePos.x, tilePos.y)), formatFloat(getFireProtectionPercentAt(city, tilePos.x, tilePos.y) * 100.0f, 0)}));

	// Highlight
	// Part of me wants this to happen outside of this windowproc, but we don't have a way of knowing when
	// the uiwindow is closed. Maybe at some point we'll want that functionality for other reasons, but
	// for now, it's cleaner and simpler to just do that drawing here.
	// Though, that does mean we can't control *when* the highlight is drawn, or make the building be drawn
	// as highlighted, so maybe this won't work and I'll have to delete this comment in 30 seconds' time!
	// - Sam, 28/08/2019

	V4 tileHighlightColor = color255(196, 196, 255, 64);
	drawSingleRect(&renderer->worldOverlayBuffer, rectXYWHi(tilePos.x, tilePos.y, 1, 1), renderer->shaderIds.untextured, tileHighlightColor);
}

void pauseMenuWindowProc(WindowContext *context, void * /*userData*/)
{
	DEBUG_FUNCTION();

	// Centred, with equal button sizes
	context->alignment = ALIGN_H_CENTRE;

	UIButtonStyle *buttonStyle = findButtonStyle(&assets->theme, context->windowStyle->buttonStyleName);
	BitmapFont *buttonFont = getFont(buttonStyle->fontName);
	s32 availableButtonTextWidth = context->contentArea.w - (2 * buttonStyle->padding);

	String resume = LOCAL("button_resume");
	String save   = LOCAL("button_save");
	String load   = LOCAL("button_load");
	String about  = LOCAL("button_about");
	String exit   = LOCAL("button_exit");
	s32 maxButtonTextWidth = calculateMaxTextWidth(buttonFont, {resume, save, load, about, exit}, availableButtonTextWidth);

	if (window_button(context, resume, maxButtonTextWidth))
	{
		context->closeRequested = true;
	}

	if (window_button(context, save, maxButtonTextWidth))
	{
		pushUiMessage(context->uiState, LOCAL("debug_msg_unimplemented"));
	}

	if (window_button(context, load, maxButtonTextWidth))
	{
		pushUiMessage(context->uiState, LOCAL("debug_msg_unimplemented"));
	}

	if (window_button(context, about, maxButtonTextWidth))
	{
		showAboutWindow(context->uiState);
	}

	window_label(context, LOCAL("button_resume"));

	if (window_button(context, exit, maxButtonTextWidth))
	{
		globalAppState.gameState->status = GameStatus_Quit;
		// NB: We don't close the window here, because doing so makes the window disappear one frame
		// before the main menu appears, so we get a flash of the game world.
		// All windows are closed when switching GameStatus so it's fine.
		// - Sam, 07/08/2019
		// context->closeRequested = true;
	}
}

void updateAndRenderGameUI(UIState *uiState, GameState *gameState)
{
	DEBUG_FUNCTION();
	
	RenderBuffer *uiBuffer = &renderer->uiBuffer;
	s32 windowWidth = round_s32(renderer->uiCamera.size.x);
	V2I centre = v2i(renderer->uiCamera.pos);
	UITheme *theme = &assets->theme;
	UILabelStyle *labelStyle = findLabelStyle(theme, "title"s);
	BitmapFont *font = getFont(labelStyle->fontName);
	City *city = &gameState->city;

	const s32 uiPadding = 4; // TODO: Move this somewhere sensible!
	s32 left = uiPadding;
	s32 right = windowWidth - uiPadding;

	Rect2I uiRect = irectXYWH(0,0, windowWidth, 64);
	append(&uiState->uiRects, uiRect);
	drawSingleRect(uiBuffer, uiRect, renderer->shaderIds.untextured, theme->overlayColor);

	uiText(&renderer->uiBuffer, font, city->name,
	       v2i(left, uiPadding), ALIGN_LEFT, labelStyle->textColor);

	uiText(&renderer->uiBuffer, font, myprintf("£{0} (-£{1}/month)", {formatInt(city->funds), formatInt(city->monthlyExpenditure)}), v2i(centre.x, uiPadding), ALIGN_H_CENTRE, labelStyle->textColor);

	uiText(&renderer->uiBuffer, font, myprintf("Pop: {0}, Jobs: {1}", {formatInt(getTotalResidents(city)), formatInt(getTotalJobs(city))}), v2i(centre.x, uiPadding+30), ALIGN_H_CENTRE, labelStyle->textColor);

	uiText(&renderer->uiBuffer, font, myprintf("Power: {0}/{1}", {formatInt(city->powerLayer.cachedCombinedConsumption), formatInt(city->powerLayer.cachedCombinedProduction)}),
	       v2i(right, uiPadding), ALIGN_RIGHT, labelStyle->textColor);

	uiText(&renderer->uiBuffer, font, myprintf("R: {0}\nC: {1}\nI: {2}", {formatInt(city->zoneLayer.demand[Zone_Residential]), formatInt(city->zoneLayer.demand[Zone_Commercial]), formatInt(city->zoneLayer.demand[Zone_Industrial])}),
	       v2i(round_s32(windowWidth * 0.75f), uiPadding), ALIGN_RIGHT, labelStyle->textColor);


	// Build UI
	{
		Rect2I buttonRect = irectXYWH(uiPadding, 28 + uiPadding, 80, 24);

		// The "ZONE" menu
		if (uiMenuButton(uiState, LOCAL("button_zone"), buttonRect, Menu_Zone))
		{
			//
			// UGH, all of this UI code is so hacky!
			// As I'm trying to use it, more and more of it is unraveling. I want to find the widest button width
			// beforehand, but that means having to get which font will be used, and that's not exposed nicely!
			// So I have to hackily write this buttonFont definition in the same way or it'll be wrong.
			// BitmapFont *buttonFont = getFont(findButtonStyle(&assets->theme, "general"s)->fontName);
			// So, that really wants to come out. Also, calculateTextSize() is the wrong call because we want
			// to know the BUTTON width, which will be the text size plus padding, depending on the style.
			//
			// So probably we want uiButton() to take a nullable Style parameter which defaults to something sensible?
			// Or maybe not nullable.
			//
			// I'm tempted to put random state in PopupMenu so we don't have to pass it to every call... but really,
			// that would force all buttons to be the same and we might not want that, so it's better to be a little
			// more verbose. That former kind of thinking is what's led to the UiState mess.
			//
			// Ramble ramble ramble.
			//
			// - Sam, 22/09/2019
			//

			// TODO: Get this style name from somewhere configurable? IDK
			UIButtonStyle *buttonStyle = findButtonStyle(&assets->theme, "general"s);

			s32 buttonMaxWidth = 0;
			for (s32 zoneIndex=0; zoneIndex < ZoneCount; zoneIndex++)
			{
				buttonMaxWidth = max(buttonMaxWidth, calculateButtonSize(getText(getZoneDef(zoneIndex).nameID), buttonStyle).x);
			}

			s32 popupMenuWidth = buttonMaxWidth + (uiPadding * 2);

			PopupMenu menu = beginPopupMenu(buttonRect.x - uiPadding, buttonRect.y + buttonRect.h, popupMenuWidth, theme->overlayColor);

			for (s32 zoneIndex=0; zoneIndex < ZoneCount; zoneIndex++)
			{
				if (popupMenuButton(uiState, &menu, getText(getZoneDef(zoneIndex).nameID),
						(gameState->actionMode == ActionMode_Zone) && (gameState->selectedZoneID == zoneIndex)))
				{
					uiCloseMenus(uiState);
					gameState->selectedZoneID = (ZoneType) zoneIndex;
					gameState->actionMode = ActionMode_Zone;
					setCursor("build");
				}
			}

			endPopupMenu(uiState, &menu);
		}
		buttonRect.x += buttonRect.w + uiPadding;

		// The "BUILD" menu
		if (uiMenuButton(uiState, LOCAL("button_build"), buttonRect, Menu_Build))
		{
			s32 popupMenuWidth = 200; // TODO: Actual width somehow! Use the button texts' size
			PopupMenu menu = beginPopupMenu(buttonRect.x - uiPadding, buttonRect.y + buttonRect.h, popupMenuWidth, theme->overlayColor);

			ChunkedArray<BuildingDef *> *constructibleBuildings = getConstructibleBuildings();

			for (auto it = iterate(constructibleBuildings);
				!it.isDone;
				next(&it))
			{
				BuildingDef *buildingDef = getValue(it);

				if (popupMenuButton(uiState, &menu, buildingDef->name,
						(gameState->actionMode == ActionMode_Build) && (gameState->selectedBuildingTypeID == buildingDef->typeID)))
				{
					uiCloseMenus(uiState);
					gameState->selectedBuildingTypeID = buildingDef->typeID;
					gameState->actionMode = ActionMode_Build;
					setCursor("build");
				}
			}

			endPopupMenu(uiState, &menu);
		}
		buttonRect.x += buttonRect.w + uiPadding;

		if (uiButton(uiState, LOCAL("button_demolish"), buttonRect,
					(gameState->actionMode == ActionMode_Demolish),
					SDLK_x, "(X)"s))
		{
			gameState->actionMode = ActionMode_Demolish;
			setCursor("demolish");
		}
		buttonRect.x += buttonRect.w + uiPadding;

		// Data layer menu
		if (uiMenuButton(uiState, LOCAL("button_data_views"), buttonRect, Menu_DataViews))
		{
			s32 popupMenuWidth = 200; // TODO: Actual width somehow! Use the button texts' size
			PopupMenu menu = beginPopupMenu(buttonRect.x - uiPadding, buttonRect.y + buttonRect.h, popupMenuWidth, theme->overlayColor);

			for (DataLayer dataViewID = DataLayer_None; dataViewID < DataLayerCount; dataViewID = (DataLayer)(dataViewID + 1))
			{
				String buttonText = getText(dataViewTitles[dataViewID]);

				if (popupMenuButton(uiState, &menu, buttonText, (gameState->dataLayerToDraw == dataViewID)))
				{
					uiCloseMenus(uiState);
					gameState->dataLayerToDraw = dataViewID;
				}
			}

			endPopupMenu(uiState, &menu);
		}
		buttonRect.x += buttonRect.w + uiPadding;

		// The, um, "MENU" menu. Hmmm.
		buttonRect.x = windowWidth - (buttonRect.w + uiPadding);
		if (uiButton(uiState, LOCAL("button_menu"), buttonRect))
		{
			showWindow(uiState, LOCAL("title_menu"), 200, 200, v2i(0,0), "general"s, WinFlag_Unique|WinFlag_Modal|WinFlag_AutomaticHeight, pauseMenuWindowProc, null);
		}
	}
}

void costTooltipWindowProc(WindowContext *context, void *userData)
{
	s32 buildCost = truncate32((smm)userData);
	City *city = &globalAppState.gameState->city;

	char *style = canAfford(city, buildCost)
				? "cost-affordable"
				: "cost-unaffordable";

	String text = myprintf("£{0}", {formatInt(buildCost)});
	window_label(context, text, style);
}

void showCostTooltip(UIState *uiState, s32 buildCost)
{
	showTooltip(uiState, costTooltipWindowProc, (void*)(smm)buildCost);
}

AppStatus updateAndRenderGame(GameState *gameState, UIState *uiState)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	AppStatus result = AppStatus_Game;

	City *city = &gameState->city;

	if (assets->assetReloadHasJustHappened)
	{
		refreshBuildingSpriteCache(&buildingCatalogue);
		refreshTerrainSpriteCache(&terrainDefs);
	}


	// Update the simulation... need a smarter way of doing this!
	{
		DEBUG_BLOCK_T("Update simulation", DCDT_Simulation);

		updateCrimeLayer    (city, &city->crimeLayer);
		updateFireLayer     (city, &city->fireLayer);
		updateHealthLayer   (city, &city->healthLayer);
		updateLandValueLayer(city, &city->landValueLayer);
		updatePollutionLayer(city, &city->pollutionLayer);
		updatePowerLayer    (city, &city->powerLayer);
		updateTransportLayer(city, &city->transportLayer);
		updateZoneLayer     (city, &city->zoneLayer);

		updateSomeBuildings(city);
	}


	// UI!
	updateAndRenderGameUI(uiState, gameState);

	V4 ghostColorValid    = color255(128,255,128,255);
	V4 ghostColorInvalid  = color255(255,0,0,128);

	// CAMERA!
	Camera *worldCamera = &renderer->worldCamera;
	Camera *uiCamera    = &renderer->uiCamera;
	if (gameState->status == GameStatus_Playing)
	{
		inputMoveCamera(worldCamera, uiCamera->size, uiCamera->mousePos, gameState->city.bounds.w, gameState->city.bounds.h);
	}

	V2I mouseTilePos = v2i(worldCamera->mousePos);
	bool mouseIsOverUI = uiState->mouseInputHandled;
	if (!mouseIsOverUI)
	{
		for (auto it = iterate(&uiState->uiRects); !it.isDone; next(&it))
		{
			if (contains(getValue(it), uiCamera->mousePos))
			{
				mouseIsOverUI = true;
				break;
			}
		}
	} 

	Rect2I demolitionRect = irectNegativeInfinity();

	{
		DEBUG_BLOCK_T("ActionMode update", DCDT_GameUpdate);

		switch (gameState->actionMode)
		{
			case ActionMode_Build:
			{
				BuildingDef *buildingDef = getBuildingDef(gameState->selectedBuildingTypeID);

				switch (buildingDef->buildMethod)
				{
					case BuildMethod_Paint: // Fallthrough
					case BuildMethod_Plop:
					{
						if (!mouseIsOverUI)
						{
							Rect2I footprint = irectCentreSize(mouseTilePos, buildingDef->size);
							s32 buildCost = buildingDef->buildCost;

							bool canPlace = canPlaceBuilding(&gameState->city, buildingDef, footprint.x, footprint.y);

							if ((buildingDef->buildMethod == BuildMethod_Plop && mouseButtonJustReleased(MouseButton_Left))
							|| (buildingDef->buildMethod == BuildMethod_Paint && mouseButtonPressed(MouseButton_Left)))
							{
								if (canPlace && canAfford(city, buildCost))
								{
									placeBuilding(city, buildingDef, footprint.x, footprint.y);
									spend(city, buildCost);
								}
							}

							if (!mouseIsOverUI) showCostTooltip(uiState, buildCost);

							Sprite *sprite = getSprite(buildingDef->sprites, 0);
							V4 color = canPlace ? ghostColorValid : ghostColorInvalid;
							drawSingleSprite(&renderer->worldOverlayBuffer, sprite, rect2(footprint), renderer->shaderIds.pixelArt, color);
						}
					} break;

					case BuildMethod_DragLine: // Fallthrough
					case BuildMethod_DragRect:
					{
						DragType dragType = (buildingDef->buildMethod == BuildMethod_DragLine) ? DragLine : DragRect;

						DragResult dragResult = updateDragState(&gameState->worldDragState, mouseTilePos, mouseIsOverUI, dragType, buildingDef->size);
						s32 buildCost = calculateBuildCost(city, buildingDef, dragResult.dragRect);

						switch (dragResult.operation)
						{
							case DragResult_DoAction:
							{
								if (canAfford(city, buildCost))
								{
									placeBuildingRect(city, buildingDef, dragResult.dragRect);
									spend(city, buildCost);
								}
								else
								{
									pushUiMessage(uiState, LOCAL("msg_cannot_afford_construction"));
								}
							} break;

							case DragResult_ShowPreview:
							{
								if (!mouseIsOverUI) showCostTooltip(uiState, buildCost);

								if (canAfford(city, buildCost))
								{
									Sprite *sprite = getSprite(buildingDef->sprites, 0);
									s32 maxGhosts = (dragResult.dragRect.w / buildingDef->width) * (dragResult.dragRect.h / buildingDef->height);
									// TODO: If maxGhosts is 1, just draw 1!
									DrawRectsGroup *rectsGroup = beginRectsGroupTextured(&renderer->worldOverlayBuffer, sprite->texture, renderer->shaderIds.pixelArt, maxGhosts);
									for (s32 y=0; y + buildingDef->height <= dragResult.dragRect.h; y += buildingDef->height)
									{
										for (s32 x=0; x + buildingDef->width <= dragResult.dragRect.w; x += buildingDef->width)
										{
											bool canPlace = canPlaceBuilding(city, buildingDef, dragResult.dragRect.x + x, dragResult.dragRect.y + y);

											Rect2 rect = rectXYWHi(dragResult.dragRect.x + x, dragResult.dragRect.y + y, buildingDef->width, buildingDef->height);

											V4 color = canPlace ? ghostColorValid : ghostColorInvalid;
											// TODO: All the sprites are the same, so we could optimise this!
											// Then again, eventually we might want ghosts to not be identical, eg ghost roads that visually connect.
											addSpriteRect(rectsGroup, sprite, rect, color);
										}
									}
									endRectsGroup(rectsGroup);
								}
								else
								{
									drawSingleRect(&renderer->worldOverlayBuffer, dragResult.dragRect, renderer->shaderIds.untextured, color255(255, 64, 64, 128));
								}
							} break;
						}
					} break;
				}
			} break;

			case ActionMode_Zone:
			{
				DragResult dragResult = updateDragState(&gameState->worldDragState, mouseTilePos, mouseIsOverUI, DragRect);

				CanZoneQuery *canZoneQuery = queryCanZoneTiles(city, gameState->selectedZoneID, dragResult.dragRect);
				s32 zoneCost = calculateZoneCost(canZoneQuery);

				switch (dragResult.operation)
				{
					case DragResult_DoAction:
					{
						if (canAfford(city, zoneCost))
						{
							placeZone(city, gameState->selectedZoneID, dragResult.dragRect);
							spend(city, zoneCost);
						}
					} break;

					case DragResult_ShowPreview:
					{
						if (!mouseIsOverUI) showCostTooltip(uiState, zoneCost);
						if (canAfford(city, zoneCost))
						{
							V4 palette[] = {
								color255(255, 0, 0, 16),
								getZoneDef(gameState->selectedZoneID).color
							};
							drawGrid(&renderer->worldOverlayBuffer, rect2(canZoneQuery->bounds), renderer->shaderIds.untextured, canZoneQuery->bounds.w, canZoneQuery->bounds.h, canZoneQuery->tileCanBeZoned, 2, palette);
						}
						else
						{
							drawSingleRect(&renderer->worldOverlayBuffer, dragResult.dragRect, renderer->shaderIds.untextured, color255(255, 64, 64, 128));
						}
					} break;
				}
			} break;

			case ActionMode_Demolish:
			{
				DragResult dragResult = updateDragState(&gameState->worldDragState, mouseTilePos, mouseIsOverUI, DragRect);
				s32 demolishCost = calculateDemolitionCost(city, dragResult.dragRect);
				demolitionRect = dragResult.dragRect;

				switch (dragResult.operation)
				{
					case DragResult_DoAction:
					{
						if (canAfford(city, demolishCost))
						{
							demolishRect(city, dragResult.dragRect);
							spend(city, demolishCost);
						}
						else
						{
							pushUiMessage(uiState, LOCAL("msg_cannot_afford_demolition"));
						}
					} break;

					case DragResult_ShowPreview:
					{
						if (!mouseIsOverUI) showCostTooltip(uiState, demolishCost);

						if (canAfford(city, demolishCost))
						{
							// Demolition outline
							drawSingleRect(&renderer->worldOverlayBuffer, dragResult.dragRect, renderer->shaderIds.untextured, color255(128, 0, 0, 128));
						}
						else
						{
							drawSingleRect(&renderer->worldOverlayBuffer, dragResult.dragRect, renderer->shaderIds.untextured, color255(255, 64, 64, 128));
						}
					} break;
				}
			} break;

			case ActionMode_None:
			{
				if (!mouseIsOverUI && mouseButtonJustPressed(MouseButton_Left))
				{
					if (tileExists(city, mouseTilePos.x, mouseTilePos.y))
					{
						gameState->inspectedTilePosition = mouseTilePos;
						V2I windowPos = v2i(renderer->uiCamera.mousePos) + v2i(16, 16);
						showWindow(uiState, "Inspect tile"s, 250, 200, windowPos, "general"s, WinFlag_AutomaticHeight | WinFlag_Unique, inspectTileWindowProc, gameState);
					}
				}
			} break;
		}
	}

	if (gameState->worldDragState.isDragging && mouseIsOverUI && mouseButtonJustReleased(MouseButton_Left))
	{
		// Not sure if this is the best idea, but it's the best I can come up with.
		gameState->worldDragState.isDragging = false;
	}

	if (mouseButtonJustPressed(MouseButton_Right))
	{
		// Unselect current thing
		gameState->actionMode = ActionMode_None;
		setCursor("default");
	}

	// RENDERING
	// Pre-calculate the tile area that's visible to the player.
	// We err on the side of drawing too much, rather than risking having holes in the world.
	Rect2I visibleTileBounds = irectCentreSize(
		v2i(worldCamera->pos), v2i(worldCamera->size / worldCamera->zoom) + v2i(3, 3)
	);
	visibleTileBounds = intersect(visibleTileBounds, city->bounds);

	drawCity(city, visibleTileBounds, demolitionRect);

	// Data layer rendering
	if (gameState->dataLayerToDraw)
	{
		DEBUG_BLOCK_T("Draw data layer", DCDT_GameUpdate);

		switch (gameState->dataLayerToDraw)
		{
			case DataLayer_Crime:                     drawCrimeDataLayer       (city, visibleTileBounds); break;
			case DataLayer_Desirability_Residential:  drawDesirabilityDataLayer(city, visibleTileBounds, Zone_Residential); break;
			case DataLayer_Desirability_Commercial:   drawDesirabilityDataLayer(city, visibleTileBounds, Zone_Commercial);  break;
			case DataLayer_Desirability_Industrial:   drawDesirabilityDataLayer(city, visibleTileBounds, Zone_Industrial);  break;
			case DataLayer_Fire:                      drawFireDataLayer        (city, visibleTileBounds); break;
			case DataLayer_Health:                    drawHealthDataLayer      (city, visibleTileBounds); break;
			case DataLayer_LandValue:                 drawLandValueDataLayer   (city, visibleTileBounds); break;
			case DataLayer_Pollution:                 drawPollutionDataLayer   (city, visibleTileBounds); break;
			case DataLayer_Power:                     drawPowerDataLayer       (city, visibleTileBounds); break;
		}
	}

	if (gameState->status == GameStatus_Quit)
	{
		result = AppStatus_MainMenu;
	}

	return result;
}
