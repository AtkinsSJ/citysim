#pragma once

GameState *newGameState()
{
	GameState *gameState;
	bootstrapArena(GameState, gameState, gameArena);
	initRandom(&gameState->gameRandom, Random_MT, 12345);

	gameState->status = GameStatus_Playing;
	gameState->actionMode = ActionMode_None;
	initFlags(&gameState->inspectTileDebugFlags, InspectTileDebugFlagCount);
	initDataViewUI(gameState);

	return gameState;
}

void beginNewGame()
{
	if (globalAppState.gameState != null)
	{
		freeGameState(globalAppState.gameState);
	}

	globalAppState.gameState = newGameState();
	GameState *gameState = globalAppState.gameState;

	s32 gameStartFunds = 1000000;
	initCity(&gameState->gameArena, &gameState->city, 128, 128, getText("city_default_name"_s), getText("player_default_name"_s), gameStartFunds);
	generateTerrain(&gameState->city, &gameState->gameRandom);

	initGameClock(&gameState->gameClock);
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

	//
	// I've experimented with snapping the camera to whole pixels, to try and reduce flickeriness
	// of sprite edges... it does keep the graphics looking crisper, and the camera can still
	// move freely, but you can feel it. It's very slightly uncomfortable. I know old games had
	// to snap to pixel coordinates, and they feel natural, but their camera movement is a lot
	// more constrained so you don't notice. Our "drag the mouse and the camera moves smoothly
	// at any angle and any speed" system makes any snapping more obvious.
	// I don't know if that is actually how we want the camera to move anyway! A more "click and
	// drag the map around" approach might be preferable, AND that would work nicely with snapping.
	// So yeah, not sure, but that's why I'm leaving this as code that's #define'd out.
	//
	// - Sam, 23/10/2019
	//
#define SNAP_CAMERA_TO_WHOLE_PIXELS 0
#if SNAP_CAMERA_TO_WHOLE_PIXELS
	camera->pos = camera->realPos;
#endif

	// Zooming
	s32 zoomDelta = inputState->wheelY;

	// Turns out that having the zoom bound to the same key I use for navigating debug frames is REALLY ANNOYING
	if (!isInputCaptured())
	{
		if (keyJustPressed(SDLK_EQUALS) || keyJustPressed(SDLK_KP_PLUS))
		{
			zoomDelta++;
		}
		else if (keyJustPressed(SDLK_MINUS) || keyJustPressed(SDLK_KP_MINUS))
		{
			zoomDelta--;
		}
	}

	if (zoomDelta)
	{
		camera->zoom = snapZoomLevel(camera->zoom + (zoomDelta * 0.1f));
	}

	// Panning
	f32 scrollSpeed = (CAMERA_PAN_SPEED * (f32) sqrt(camera->zoom)) * globalAppState.deltaTime;
	f32 cameraEdgeScrollPixelMargin = 8.0f;

	if (mouseButtonPressed(MouseButton_Middle))
	{
		// Click-panning!
		float scale = scrollSpeed * 1.0f;
		V2 clickStartPos = getClickStartPos(MouseButton_Middle, camera);
		camera->pos += (camera->mousePos - clickStartPos) * scale;
	}
	else if (!isInputCaptured())
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

#if SNAP_CAMERA_TO_WHOLE_PIXELS
	// Snap to whole-pixel coordinates
	// Really, this should work without having to hard-code 16 as the tile size... but IDK.
	// When zoomed in (zoom > 1) there are > 16 pixels per tile, so we snap to that instead.
	// When zoomed out, (zoom < 1) we'd rather snap to the nearest 1/16 because less is noticeably jerky.
	f32 snap = (camera->zoom < 1.0f) ? 16.0f : (16.0f * camera->zoom);
	camera->realPos = camera->pos;
	camera->pos.x = round_f32(camera->pos.x * snap) / snap;
	camera->pos.y = round_f32(camera->pos.y * snap) / snap;
#endif
}

Rect2I getDragArea(DragState *dragState, Rect2I cityBounds, DragType dragType, V2I itemSize)
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

		result = intersect(result, cityBounds);
	}

	return result;
}

DragResult updateDragState(DragState *dragState, Rect2I cityBounds, V2I mouseTilePos, bool mouseIsOverUI, DragType dragType, V2I itemSize)
{
	DEBUG_FUNCTION();

	DragResult result = {};

	if (dragState->isDragging && mouseButtonJustReleased(MouseButton_Left))
	{
		result.operation = DragResult_DoAction;
		result.dragRect = getDragArea(dragState, cityBounds, dragType, itemSize);

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
			result.dragRect = getDragArea(dragState, cityBounds, dragType, itemSize);
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

void inspectTileWindowProc(UI::WindowContext *context, void *userData)
{
	DEBUG_FUNCTION();

	UI::Panel *ui = &context->windowPanel;
	ui->alignWidgets(ALIGN_EXPAND_H);

	GameState *gameState = (GameState *) userData;
	City *city = &gameState->city;

	V2I tilePos = gameState->inspectedTilePosition;

	// CitySector
	CitySector *sector = getSectorAtTilePos(&city->sectors, tilePos.x, tilePos.y);
	ui->addLabel(myprintf("CitySector: x={0} y={1} w={2} h={3}"_s, {formatInt(sector->bounds.x), formatInt(sector->bounds.y), formatInt(sector->bounds.w), formatInt(sector->bounds.h)}));

	// Terrain
	TerrainDef *terrain = getTerrainAt(city, tilePos.x, tilePos.y);
	ui->addLabel(myprintf("Terrain: {0}, {1} tiles from water"_s, {getText(terrain->textAssetName), formatInt(getDistanceToWaterAt(city, tilePos.x, tilePos.y))}));

	// Zone
	ZoneType zone = getZoneAt(city, tilePos.x, tilePos.y);
	ui->addLabel(myprintf("Zone: {0}"_s, {zone ? getText(getZoneDef(zone).textAssetName) : "None"_s}));

	// Building
	Building *building = getBuildingAt(city, tilePos.x, tilePos.y);
	if (building != null)
	{
		s32 buildingIndex = city->tileBuildingIndex.get(tilePos.x, tilePos.y);
		BuildingDef *def = getBuildingDef(building->typeID);
		ui->addLabel(myprintf("Building: {0} (ID {1}, array index {2})"_s, {getText(def->textAssetName), formatInt(building->id), formatInt(buildingIndex)}));
		ui->addLabel(myprintf("Constructed: {0}"_s, {formatDateTime(dateTimeFromTimestamp(building->creationDate), DateTime_ShortDate)}));
		ui->addLabel(myprintf("Variant: {0}"_s, {formatInt(building->variantIndex)}));
		ui->addLabel(myprintf("- Residents: {0} / {1}"_s, {formatInt(building->currentResidents), formatInt(def->residents)}));
		ui->addLabel(myprintf("- Jobs: {0} / {1}"_s, {formatInt(building->currentJobs), formatInt(def->jobs)}));
		ui->addLabel(myprintf("- Power: {0}"_s, {formatInt(def->power)}));

		// Problems
		for (s32 problemIndex = 0; problemIndex < BuildingProblemCount; problemIndex++)
		{
			if (hasProblem(building, (BuildingProblemType) problemIndex))
			{
				ui->addLabel(myprintf("- PROBLEM: {0}"_s, {getText(buildingProblemNames[problemIndex])}));
			}
		}
	}
	else
	{
		ui->addLabel("Building: None"_s);
	}

	// Land value
	ui->addLabel(myprintf("Land value: {0}%"_s, {formatFloat(getLandValuePercentAt(city, tilePos.x, tilePos.y) * 100.0f, 0)}));

	// Debug info
	if (!isEmpty(&gameState->inspectTileDebugFlags))
	{
		if (gameState->inspectTileDebugFlags & DebugInspect_Fire)
		{
			debugInspectFire(ui, city, tilePos.x, tilePos.y);
		}
		if (gameState->inspectTileDebugFlags & DebugInspect_Power)
		{
			debugInspectPower(ui, city, tilePos.x, tilePos.y);
		}
		if (gameState->inspectTileDebugFlags & DebugInspect_Transport)
		{
			debugInspectTransport(ui, city, tilePos.x, tilePos.y);
		}
	}


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

void pauseMenuWindowProc(UI::WindowContext *context, void * /*userData*/)
{
	DEBUG_FUNCTION();
	UI::Panel *ui = &context->windowPanel;
	ui->alignWidgets(ALIGN_EXPAND_H);

	if (ui->addTextButton(getText("button_resume"_s)))
	{
		context->closeRequested = true;
	}

	if (ui->addTextButton(getText("button_save"_s)))
	{
		showSaveGameWindow();
	}

	if (ui->addTextButton(getText("button_load"_s)))
	{
		showLoadGameWindow();
	}

	if (ui->addTextButton(getText("button_about"_s)))
	{
		showAboutWindow();
	}

	if (ui->addTextButton(getText("button_settings"_s)))
	{
		showSettingsWindow();
	}

	if (ui->addTextButton(getText("button_exit"_s)))
	{
		globalAppState.gameState->status = GameStatus_Quit;
		// NB: We don't close the window here, because doing so makes the window disappear one frame
		// before the main menu appears, so we get a flash of the game world.
		// All windows are closed when switching GameStatus so it's fine.
		// - Sam, 07/08/2019
		// context->closeRequested = true;
	}
}

void updateAndRenderGameUI(GameState *gameState)
{
	DEBUG_FUNCTION();
	
	RenderBuffer *uiBuffer = &renderer->uiBuffer;
	UI::LabelStyle *labelStyle = getStyle<UI::LabelStyle>("title"_s);
	BitmapFont *font = getFont(&labelStyle->font);
	City *city = &gameState->city;

	const s32 uiPadding = 4; // TODO: Move this somewhere sensible!
	s32 left = uiPadding;
	s32 right = UI::windowSize.x - uiPadding;
	s32 rowHeight = 30;
	s32 toolbarHeight = uiPadding * 2 + rowHeight * 2;
	s32 width3 = (UI::windowSize.x - (uiPadding * 2)) / 3;

	Rect2I uiRect = irectXYWH(0,0, UI::windowSize.x, toolbarHeight);
	UI::addUIRect(uiRect);
	drawSingleRect(uiBuffer, uiRect, renderer->shaderIds.untextured, color255(0, 0, 0, 128));

	UI::putLabel(city->name, irectXYWH(left, uiPadding, width3, rowHeight), labelStyle);

	UI::putLabel(myprintf("£{0} (-£{1}/month)"_s, {formatInt(city->funds), formatInt(city->monthlyExpenditure)}), irectXYWH(width3, uiPadding, width3, rowHeight), labelStyle);

	UI::putLabel(myprintf("Pop: {0}, Jobs: {1}"_s, {formatInt(getTotalResidents(city)), formatInt(getTotalJobs(city))}), irectXYWH(width3, uiPadding + rowHeight, width3, rowHeight), labelStyle);

	// Game clock
	Rect2I clockBounds = {};
	{
		GameClock *clock = &gameState->gameClock;

		// We're sizing the clock area based on the speed control buttons.
		// The >>> button is the largest, so they're all set to that size.
		// For the total area, we just add their widths and padding together.
		UI::ButtonStyle *buttonStyle = getStyle<UI::ButtonStyle>("default"_s);
		V2I speedButtonSize = UI::calculateButtonSize(">>>"_s, buttonStyle);
		s32 clockWidth = (speedButtonSize.x * 4) + (uiPadding * 3);
		clockBounds = irectXYWH(right - clockWidth, uiPadding, clockWidth, toolbarHeight);

		String dateString = formatDateTime(clock->cosmeticDate, DateTime_ShortDate);
		V2I dateStringSize = calculateTextSize(font, dateString, clockWidth);

		// Draw a progress bar for the current day
		Rect2I dateRect = irectXYWH(right - clockWidth, uiPadding, clockWidth, dateStringSize.y);
		drawSingleRect(uiBuffer, dateRect, renderer->shaderIds.untextured, color255(0, 0, 0, 128));
		Rect2I dateProgressRect = dateRect;
		dateProgressRect.w = round_s32(dateProgressRect.w * clock->timeWithinDay);
		drawSingleRect(uiBuffer, dateProgressRect, renderer->shaderIds.untextured, color255(64, 255, 64, 128));

		UI::putLabel(dateString, dateRect, labelStyle);

		// Speed control buttons
		Rect2I speedButtonRect = irectXYWH(right - speedButtonSize.x, toolbarHeight - (uiPadding + speedButtonSize.y), speedButtonSize.x, speedButtonSize.y);

		if (UI::putTextButton(">>>"_s, speedButtonRect, buttonStyle, buttonIsActive(clock->speed == Speed_Fast)))
		{
			clock->speed = Speed_Fast;
			clock->isPaused = false;
		}
		speedButtonRect.x -= speedButtonRect.w + uiPadding;

		if (UI::putTextButton(">>"_s, speedButtonRect, buttonStyle, buttonIsActive(clock->speed == Speed_Medium)))
		{
			clock->speed = Speed_Medium;
			clock->isPaused = false;
		}
		speedButtonRect.x -= speedButtonRect.w + uiPadding;

		if (UI::putTextButton(">"_s, speedButtonRect, buttonStyle, buttonIsActive(clock->speed == Speed_Slow)))
		{
			clock->speed = Speed_Slow;
			clock->isPaused = false;
		}
		speedButtonRect.x -= speedButtonRect.w + uiPadding;

		if (UI::putTextButton("||"_s, speedButtonRect, buttonStyle, buttonIsActive(clock->isPaused)))
		{
			clock->isPaused = !clock->isPaused;
		}
	}

/*
	UI::putLabel(myprintf("Power: {0}/{1}"_s, {formatInt(city->powerLayer.cachedCombinedConsumption), formatInt(city->powerLayer.cachedCombinedProduction)}),
	       irectXYWH(right - width3, uiPadding, width3, rowHeight), labelStyle);
*/

	UI::putLabel(myprintf("R: {0}\nC: {1}\nI: {2}"_s, {formatInt(city->zoneLayer.demand[Zone_Residential]), formatInt(city->zoneLayer.demand[Zone_Commercial]), formatInt(city->zoneLayer.demand[Zone_Industrial])}),
		irectXYWH(clockBounds.x - 100, uiPadding, 100, toolbarHeight), labelStyle);

	UI::ButtonStyle *buttonStyle = getStyle<UI::ButtonStyle>("default"_s);
	UI::PanelStyle *popupMenuPanelStyle = getStyle<UI::PanelStyle>("popupMenu"_s);
	// Build UI
	{
		// The, um, "MENU" menu. Hmmm.
		String menuButtonText = getText("button_menu"_s);
		V2I buttonSize = UI::calculateButtonSize(menuButtonText, buttonStyle);
		Rect2I buttonRect = irectXYWH(uiPadding, toolbarHeight - (buttonSize.y + uiPadding), buttonSize.x, buttonSize.y);
		if (UI::putTextButton(menuButtonText, buttonRect, buttonStyle))
		{
			UI::showWindow(UI::WindowTitle::fromTextAsset("title_menu"_s), 200, 200, v2i(0,0), "default"_s,
					   WindowFlags::Unique | WindowFlags::Modal | WindowFlags::AutomaticHeight | WindowFlags::Pause,
					   pauseMenuWindowProc);
		}
		buttonRect.x += buttonRect.w + uiPadding;

		// The "ZONE" menu
		String zoneButtonText = getText("button_zone"_s);
		buttonRect.size = UI::calculateButtonSize(zoneButtonText, buttonStyle);
		if (UI::putTextButton(zoneButtonText, buttonRect, buttonStyle, buttonIsActive(UI::isMenuVisible(Menu_Zone))))
		{
			UI::toggleMenuVisible(Menu_Zone);
		}

		if (UI::isMenuVisible(Menu_Zone))
		{
			s32 popupMenuWidth = 150;
			s32 popupMenuMaxHeight = UI::windowSize.y - (buttonRect.y + buttonRect.h);

			UI::Panel menu = UI::Panel(irectXYWH(buttonRect.x - popupMenuPanelStyle->padding.left, buttonRect.y + buttonRect.h, popupMenuWidth, popupMenuMaxHeight), popupMenuPanelStyle);
			for (s32 zoneIndex=0; zoneIndex < ZoneTypeCount; zoneIndex++)
			{
				if (menu.addTextButton(getText(getZoneDef(zoneIndex).textAssetName),
					buttonIsActive((gameState->actionMode == ActionMode_Zone) && (gameState->selectedZoneID == zoneIndex))))
				{
					UI::hideMenus();
					gameState->selectedZoneID = (ZoneType) zoneIndex;
					gameState->actionMode = ActionMode_Zone;
					setCursor("build"_s);
				}
			}
			menu.end(true);
		}

		buttonRect.x += buttonRect.w + uiPadding;

		// The "BUILD" menu
		String buildButtonText = getText("button_build"_s);
		buttonRect.size = UI::calculateButtonSize(buildButtonText, buttonStyle);
		if (UI::putTextButton(buildButtonText, buttonRect, buttonStyle, buttonIsActive(UI::isMenuVisible(Menu_Build))))
		{
			UI::toggleMenuVisible(Menu_Build);
		}

		if (UI::isMenuVisible(Menu_Build))
		{
			ChunkedArray<BuildingDef *> *constructibleBuildings = getConstructibleBuildings();

			s32 popupMenuWidth = 150;
			s32 popupMenuMaxHeight = UI::windowSize.y - (buttonRect.y + buttonRect.h);

			UI::Panel menu = UI::Panel(irectXYWH(buttonRect.x - popupMenuPanelStyle->padding.left, buttonRect.y + buttonRect.h, popupMenuWidth, popupMenuMaxHeight), popupMenuPanelStyle);

			for (auto it = constructibleBuildings->iterate();
				it.hasNext();
				it.next())
			{
				BuildingDef *buildingDef = it.getValue();

				if (menu.addTextButton(getText(buildingDef->textAssetName),
						buttonIsActive((gameState->actionMode == ActionMode_Build) && (gameState->selectedBuildingTypeID == buildingDef->typeID))))
				{
					UI::hideMenus();
					gameState->selectedBuildingTypeID = buildingDef->typeID;
					gameState->actionMode = ActionMode_Build;
					setCursor("build"_s);
				}
			}

			menu.end(true);
		}
		buttonRect.x += buttonRect.w + uiPadding;

		// The Terrain button
		String terrainButtonText = getText("button_terrain"_s);
		buttonRect.size = UI::calculateButtonSize(terrainButtonText, buttonStyle);
		bool isTerrainWindowOpen = UI::isWindowOpen(modifyTerrainWindowProc);
		if (UI::putTextButton(terrainButtonText, buttonRect, buttonStyle, buttonIsActive(isTerrainWindowOpen)))
		{
			if (isTerrainWindowOpen)
			{
				UI::closeWindow(modifyTerrainWindowProc);
			}
			else
			{
				showTerrainWindow();
			}
		}
		buttonRect.x += buttonRect.w + uiPadding;

		// Demolish button
		String demolishButtonText = getText("button_demolish"_s);
		buttonRect.size = UI::calculateButtonSize(demolishButtonText, buttonStyle);
		if (UI::putTextButton(demolishButtonText, buttonRect, buttonStyle,
					buttonIsActive(gameState->actionMode == ActionMode_Demolish)))
		{
			gameState->actionMode = ActionMode_Demolish;
			setCursor("demolish"_s);
		}
		buttonRect.x += buttonRect.w + uiPadding;
	}

	drawDataViewUI(gameState);
}

void costTooltipWindowProc(UI::WindowContext *context, void *userData)
{
	s32 buildCost = truncate32((smm)userData);
	City *city = &globalAppState.gameState->city;

	UI::Panel *ui = &context->windowPanel;

	String style = canAfford(city, buildCost)
				? "cost-affordable"_s
				: "cost-unaffordable"_s;

	String text = myprintf("£{0}"_s, {formatInt(buildCost)});
	ui->addLabel(text, style);
}

void showCostTooltip(s32 buildCost)
{
	UI::showTooltip(costTooltipWindowProc, (void*)(smm)buildCost);
}

void debugToolsWindowProc(UI::WindowContext *context, void *userData)
{
	GameState *gameState = (GameState *)userData;
	UI::Panel *ui = &context->windowPanel;
	ui->alignWidgets(ALIGN_EXPAND_H);

	if (ui->addTextButton("Inspect fire info"_s, buttonIsActive(gameState->inspectTileDebugFlags & DebugInspect_Fire)))
	{
		gameState->inspectTileDebugFlags ^= DebugInspect_Fire;
	}
	if (ui->addTextButton("Add Fire"_s, buttonIsActive(gameState->actionMode == ActionMode_Debug_AddFire)))
	{
		gameState->actionMode = ActionMode_Debug_AddFire;
	}
	if (ui->addTextButton("Remove Fire"_s, buttonIsActive(gameState->actionMode == ActionMode_Debug_RemoveFire)))
	{
		gameState->actionMode = ActionMode_Debug_RemoveFire;
	}

	if (ui->addTextButton("Inspect power info"_s, buttonIsActive(gameState->inspectTileDebugFlags & DebugInspect_Power)))
	{
		gameState->inspectTileDebugFlags ^= DebugInspect_Power;
	}
	
	if (ui->addTextButton("Inspect transport info"_s, buttonIsActive(gameState->inspectTileDebugFlags & DebugInspect_Transport)))
	{
		gameState->inspectTileDebugFlags ^= DebugInspect_Transport;
	}
}

AppStatus updateAndRenderGame(GameState *gameState, f32 deltaTime)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	AppStatus result = AppStatus_Game;
	City *city = &gameState->city;

	if (assets->assetReloadHasJustHappened)
	{
		remapTerrainTypes(city);
		remapBuildingTypes(city);
	}

	// Update the simulation... need a smarter way of doing this!
	if (!UI::hasPauseWindowOpen())
	{
		DEBUG_BLOCK_T("Update simulation", DCDT_Simulation);

		u8 clockEvents = incrementClock(&gameState->gameClock, deltaTime);
		if (clockEvents & ClockEvent_NewWeek)
		{
			logInfo("New week!"_s);
		}
		if (clockEvents & ClockEvent_NewMonth)
		{
			logInfo("New month, a budget tick should happen here!"_s);
		}
		if (clockEvents & ClockEvent_NewYear)
		{
			logInfo("New year!"_s);
		}

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
	updateAndRenderGameUI(gameState);

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
	bool mouseIsOverUI = UI::isMouseInputHandled() || UI::mouseIsWithinUIRects();

	city->demolitionRect = irectNegativeInfinity();

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

							if (!mouseIsOverUI) showCostTooltip(buildCost);

							Sprite *sprite = getSprite(buildingDef->spriteName, 0);
							V4 color = canPlace ? ghostColorValid : ghostColorInvalid;
							drawSingleSprite(&renderer->worldOverlayBuffer, sprite, rect2(footprint), renderer->shaderIds.pixelArt, color);
						}
					} break;

					case BuildMethod_DragLine: // Fallthrough
					case BuildMethod_DragRect:
					{
						DragType dragType = (buildingDef->buildMethod == BuildMethod_DragLine) ? DragLine : DragRect;

						DragResult dragResult = updateDragState(&gameState->worldDragState, city->bounds, mouseTilePos, mouseIsOverUI, dragType, buildingDef->size);
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
									UI::pushToast(getText("msg_cannot_afford_construction"_s));
								}
							} break;

							case DragResult_ShowPreview:
							{
								if (!mouseIsOverUI) showCostTooltip(buildCost);

								if (canAfford(city, buildCost))
								{
									Sprite *sprite = getSprite(buildingDef->spriteName, 0);
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

							default: break;
						}
					} break;

					INVALID_DEFAULT_CASE;
				}
			} break;

			case ActionMode_Zone:
			{
				DragResult dragResult = updateDragState(&gameState->worldDragState, city->bounds, mouseTilePos, mouseIsOverUI, DragRect);

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
						if (!mouseIsOverUI) showCostTooltip(zoneCost);
						if (canAfford(city, zoneCost))
						{
							V4 palette[] = {
								color255(255, 0, 0, 16),
								getZoneDef(gameState->selectedZoneID).color
							};
							drawGrid(&renderer->worldOverlayBuffer, rect2(canZoneQuery->bounds), canZoneQuery->bounds.w, canZoneQuery->bounds.h, canZoneQuery->tileCanBeZoned, 2, palette);
						}
						else
						{
							drawSingleRect(&renderer->worldOverlayBuffer, dragResult.dragRect, renderer->shaderIds.untextured, color255(255, 64, 64, 128));
						}
					} break;


					default: break;
				}
			} break;

			case ActionMode_Demolish:
			{
				DragResult dragResult = updateDragState(&gameState->worldDragState, city->bounds, mouseTilePos, mouseIsOverUI, DragRect);
				s32 demolishCost = calculateDemolitionCost(city, dragResult.dragRect);
				city->demolitionRect = dragResult.dragRect;

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
							UI::pushToast(getText("msg_cannot_afford_demolition"_s));
						}
					} break;

					case DragResult_ShowPreview:
					{
						if (!mouseIsOverUI) showCostTooltip(demolishCost);

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


					default: break;
				}
			} break;

			case ActionMode_SetTerrain:
			{
				// Temporary click-and-drag, no-cost terrain editing
				// We probably want to make this better in several ways, and add a cost to it, and such
				if (!mouseIsOverUI
				 && mouseButtonPressed(MouseButton_Left)
				 && tileExists(city, mouseTilePos.x, mouseTilePos.y))
				{
					setTerrainAt(city, mouseTilePos.x, mouseTilePos.y, gameState->selectedTerrainID);
				}
			} break;

			case ActionMode_Debug_AddFire:
			{
				if (!mouseIsOverUI
				 && mouseButtonJustPressed(MouseButton_Left)
				 && tileExists(city, mouseTilePos.x, mouseTilePos.y))
				{
					startFireAt(city, mouseTilePos.x, mouseTilePos.y);
				}
			} break;

			case ActionMode_Debug_RemoveFire:
			{
				if (!mouseIsOverUI
				 && mouseButtonJustPressed(MouseButton_Left)
				 && tileExists(city, mouseTilePos.x, mouseTilePos.y))
				{
					removeFireAt(city, mouseTilePos.x, mouseTilePos.y);
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
						UI::showWindow(UI::WindowTitle::fromLambda([]() {
								V2I tilePos = globalAppState.gameState->inspectedTilePosition;
								return getText("title_inspect"_s, {formatInt(tilePos.x), formatInt(tilePos.y)});
							}), 250, 200, windowPos, "default"_s, WindowFlags::AutomaticHeight | WindowFlags::Unique, inspectTileWindowProc, gameState);
					}
				}
			} break;

			INVALID_DEFAULT_CASE;
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
		setCursor("default"_s);
	}

	// RENDERING
	// Pre-calculate the tile area that's visible to the player.
	// We err on the side of drawing too much, rather than risking having holes in the world.
	Rect2I visibleTileBounds = irectCentreSize(
		v2i(worldCamera->pos), v2i(worldCamera->size / worldCamera->zoom) + v2i(3, 3)
	);
	visibleTileBounds = intersect(visibleTileBounds, city->bounds);

	// logInfo("visibleTileBounds = {0} {1} {2} {3}"_s, {formatInt(visibleTileBounds.x),formatInt(visibleTileBounds.y),formatInt(visibleTileBounds.w),formatInt(visibleTileBounds.h)});

	drawCity(city, visibleTileBounds);

	// Data layer rendering
	if (gameState->dataLayerToDraw)
	{
		drawDataViewOverlay(gameState, visibleTileBounds);
	}

	if (gameState->status == GameStatus_Quit)
	{
		result = AppStatus_MainMenu;
	}

	return result;
}

void initDataViewUI(GameState *gameState)
{
	DataViewUI *dataViewUI = gameState->dataViewUI;
	City *city = &gameState->city;

	dataViewUI[DataView_None].title = "data_view_none"_s;

	dataViewUI[DataView_Desirability_Residential].title = "data_view_desirability_residential"_s;
	setGradient(&dataViewUI[DataView_Desirability_Residential], "desirability"_s);
	setTileOverlay(&dataViewUI[DataView_Desirability_Residential], &city->zoneLayer.tileDesirability[Zone_Residential].items, "desirability"_s);

	dataViewUI[DataView_Desirability_Commercial].title = "data_view_desirability_commercial"_s;
	setGradient(&dataViewUI[DataView_Desirability_Commercial], "desirability"_s);
	setTileOverlay(&dataViewUI[DataView_Desirability_Commercial], &city->zoneLayer.tileDesirability[Zone_Commercial].items, "desirability"_s);

	dataViewUI[DataView_Desirability_Industrial].title = "data_view_desirability_industrial"_s;
	setGradient(&dataViewUI[DataView_Desirability_Industrial], "desirability"_s);
	setTileOverlay(&dataViewUI[DataView_Desirability_Industrial], &city->zoneLayer.tileDesirability[Zone_Industrial].items, "desirability"_s);

	dataViewUI[DataView_Crime].title = "data_view_crime"_s;
	setGradient(&dataViewUI[DataView_Crime], "service_coverage"_s);
	setFixedColors(&dataViewUI[DataView_Crime], "service_buildings"_s, {"data_view_buildings_powered"_s, "data_view_buildings_unpowered"_s});
	setHighlightedBuildings(&dataViewUI[DataView_Crime], &city->crimeLayer.policeBuildings, &BuildingDef::policeEffect);
	setTileOverlay(&dataViewUI[DataView_Crime], &city->crimeLayer.tilePoliceCoverage.items, "service_coverage"_s);

	dataViewUI[DataView_Fire].title = "data_view_fire"_s;
	setGradient(&dataViewUI[DataView_Fire], "risk"_s);
	setFixedColors(&dataViewUI[DataView_Fire], "service_buildings"_s, {"data_view_buildings_powered"_s, "data_view_buildings_unpowered"_s});
	setHighlightedBuildings(&dataViewUI[DataView_Fire], &city->fireLayer.fireProtectionBuildings, &BuildingDef::fireProtection);
	setTileOverlay(&dataViewUI[DataView_Fire], &city->fireLayer.tileOverallFireRisk.items, "risk"_s);

	dataViewUI[DataView_Health].title = "data_view_health"_s;
	setGradient(&dataViewUI[DataView_Health], "service_coverage"_s);
	setFixedColors(&dataViewUI[DataView_Health], "service_buildings"_s, {"data_view_buildings_powered"_s, "data_view_buildings_unpowered"_s});
	setHighlightedBuildings(&dataViewUI[DataView_Health], &city->healthLayer.healthBuildings, &BuildingDef::healthEffect);
	setTileOverlay(&dataViewUI[DataView_Health], &city->healthLayer.tileHealthCoverage.items, "service_coverage"_s);

	dataViewUI[DataView_LandValue].title = "data_view_landvalue"_s;
	setGradient(&dataViewUI[DataView_LandValue], "land_value"_s);
	setTileOverlay(&dataViewUI[DataView_LandValue], &city->landValueLayer.tileLandValue.items, "land_value"_s);

	dataViewUI[DataView_Pollution].title = "data_view_pollution"_s;
	setGradient(&dataViewUI[DataView_Pollution], "pollution"_s);
	setTileOverlay(&dataViewUI[DataView_Pollution], &city->pollutionLayer.tilePollution.items, "pollution"_s);

	dataViewUI[DataView_Power].title = "data_view_power"_s;
	setFixedColors(&dataViewUI[DataView_Power], "power"_s, {"data_view_power_powered"_s, "data_view_power_brownout"_s, "data_view_power_blackout"_s});
	setHighlightedBuildings(&dataViewUI[DataView_Power], &city->powerLayer.powerBuildings);
	setTileOverlayCallback(&dataViewUI[DataView_Power], calculatePowerOverlayForTile, "power"_s);
}

void setGradient(DataViewUI *dataView, String paletteName)
{
	dataView->hasGradient = true;
	dataView->gradientPaletteName = paletteName;
}

void setFixedColors(DataViewUI *dataView, String paletteName, std::initializer_list<String> names)
{
	dataView->hasFixedColors = true;
	dataView->fixedPaletteName = paletteName;
	dataView->fixedColorNames = allocateArray<String>(&globalAppState.systemArena, truncate32(names.size()), false);
	for (auto it = names.begin(); it != names.end(); it++)
	{
		dataView->fixedColorNames.append( pushString(&globalAppState.systemArena, *it) );
	}
}

void setHighlightedBuildings(DataViewUI *dataView, ChunkedArray<BuildingRef> *highlightedBuildings, EffectRadius BuildingDef:: *effectRadiusMember)
{
	dataView->highlightedBuildings = highlightedBuildings;
	dataView->effectRadiusMember = effectRadiusMember;
}

void setTileOverlay(DataViewUI *dataView, u8 **tileData, String paletteName)
{
	dataView->overlayTileData = tileData;
	dataView->overlayPaletteName = paletteName;
}

void setTileOverlayCallback(DataViewUI *dataView, u8 (*calculateTileValue)(City *city, s32 x, s32 y), String paletteName)
{
	dataView->calculateTileValue = calculateTileValue;
	dataView->overlayPaletteName = paletteName;
}

void drawDataViewOverlay(GameState *gameState, Rect2I visibleTileBounds)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	if (gameState->dataLayerToDraw == DataView_None)  return;
	ASSERT(gameState->dataLayerToDraw < DataViewCount);

	City *city = &gameState->city;
	DataViewUI *dataView = &gameState->dataViewUI[gameState->dataLayerToDraw];

	if (dataView->overlayTileData)
	{
		// TODO: Use the visible tile bounds for rendering instead. We have two paths, one is just to output
		// an array in the layer as a texture, which we use for most things. The other is to generate an array
		// dynamically, which we only calculate for the visibleTileBounds to save unnecessary work.
		// The array-as-texture path currently returns the entire map, because that was simplest, but that
		// wastefully uploads the entire map's data to the GPU each frame anyway, so it's inefficient, even
		// though cropping it is non-trivial. But yeah, if they are consistent that would make things easier
		// to follow.
		// - Sam, 28/03/2020
		Rect2I bounds = city->bounds;

		Array<V4> *overlayPalette = getPalette(dataView->overlayPaletteName);
		drawGrid(&renderer->worldOverlayBuffer, rect2(bounds), bounds.w, bounds.h, *dataView->overlayTileData, (u16)overlayPalette->count, overlayPalette->items);
	}
	else if (dataView->calculateTileValue)
	{
		// The per-tile overlay data is generated
		Array2<u8> overlayTileData = allocateArray2<u8>(tempArena, visibleTileBounds.w, visibleTileBounds.h);

		for (s32 gridY = 0; gridY < visibleTileBounds.h; gridY++)
		{
			for (s32 gridX = 0; gridX < visibleTileBounds.w; gridX++)
			{
				u8 tileValue = dataView->calculateTileValue(city, visibleTileBounds.x + gridX, visibleTileBounds.y + gridY);
				overlayTileData.set(gridX, gridY, tileValue);
			}
		}

		Array<V4> *overlayPalette = getPalette(dataView->overlayPaletteName);
		drawGrid(&renderer->worldOverlayBuffer, rect2(visibleTileBounds), overlayTileData.w, overlayTileData.h, overlayTileData.items, (u16)overlayPalette->count, overlayPalette->items);
	}

	if (dataView->highlightedBuildings)
	{
		drawBuildingHighlights(city, dataView->highlightedBuildings);

		if (dataView->effectRadiusMember)
		{
			drawBuildingEffectRadii(city, dataView->highlightedBuildings, dataView->effectRadiusMember);
		}
	}
}

void drawDataViewUI(GameState *gameState)
{
	DEBUG_FUNCTION();
	
	RenderBuffer *uiBuffer = &renderer->uiBuffer;
	UI::LabelStyle *labelStyle = getStyle<UI::LabelStyle>("title"_s);
	BitmapFont *font = getFont(&labelStyle->font);

	const s32 uiPadding = 4; // TODO: Move this somewhere sensible!
	UI::ButtonStyle *buttonStyle = getStyle<UI::ButtonStyle>("default"_s);
	UI::PanelStyle *popupMenuPanelStyle = getStyle<UI::PanelStyle>("popupMenu"_s);

	// Data-views menu
	String dataViewButtonText = getText("button_data_views"_s);
	V2I dataViewButtonSize = UI::calculateButtonSize(dataViewButtonText, buttonStyle);
	Rect2I dataViewButtonBounds = irectXYWH(uiPadding, UI::windowSize.y - uiPadding - dataViewButtonSize.y, dataViewButtonSize.x, dataViewButtonSize.y);

	Rect2I dataViewUIBounds = expand(dataViewButtonBounds, uiPadding);
	drawSingleRect(uiBuffer, dataViewUIBounds, renderer->shaderIds.untextured, color255(0, 0, 0, 128));
	UI::addUIRect(dataViewUIBounds);

	if (UI::putTextButton(dataViewButtonText, dataViewButtonBounds, buttonStyle))
	{
		UI::toggleMenuVisible(Menu_DataViews);
	}

	if (UI::isMenuVisible(Menu_DataViews))
	{
		// Measure the menu contents
		UI::ButtonStyle *popupButtonStyle = getStyle<UI::ButtonStyle>(&popupMenuPanelStyle->buttonStyle);
		s32 buttonMaxWidth = 0;
		s32 buttonMaxHeight = 0;
		for (DataView dataViewID = DataView_None; dataViewID < DataViewCount; dataViewID = (DataView)(dataViewID + 1))
		{
			String buttonText = getText(gameState->dataViewUI[dataViewID].title);
			V2I buttonSize = UI::calculateButtonSize(buttonText, popupButtonStyle);
			buttonMaxWidth = max(buttonMaxWidth, buttonSize.x);
			buttonMaxHeight = max(buttonMaxHeight, buttonSize.y);
		}
		s32 popupMenuWidth = buttonMaxWidth + popupMenuPanelStyle->padding.left + popupMenuPanelStyle->padding.right;
		s32 popupMenuMaxHeight = UI::windowSize.y - 128;
		s32 estimatedMenuHeight = (DataViewCount * buttonMaxHeight)
								+ ((DataViewCount - 1) * popupMenuPanelStyle->contentPadding)
								+ (popupMenuPanelStyle->padding.top + popupMenuPanelStyle->padding.bottom);

		UI::Panel menu = UI::Panel(irectAligned(dataViewButtonBounds.x - popupMenuPanelStyle->padding.left, dataViewButtonBounds.y, popupMenuWidth, popupMenuMaxHeight, ALIGN_BOTTOM | ALIGN_LEFT), popupMenuPanelStyle, UI::PanelFlags::LayoutBottomToTop);

		// Enable scrolling if there's too many items to fit
		if (estimatedMenuHeight > popupMenuMaxHeight)
		{
			menu.enableVerticalScrolling(UI::getMenuScrollbar(), true);
		}

		for (s32 dataViewID = DataViewCount - 1; dataViewID >= 0; dataViewID = dataViewID - 1)
		{
			String buttonText = getText(gameState->dataViewUI[dataViewID].title);

			if (menu.addTextButton(buttonText, buttonIsActive(gameState->dataLayerToDraw == dataViewID)))
			{
				UI::hideMenus();
				gameState->dataLayerToDraw = (DataView) dataViewID;
			}
		}

		menu.end(true);
	}

	// Data-view info
	if (!UI::isMenuVisible(Menu_DataViews)
	 && gameState->dataLayerToDraw != DataView_None)
	{
		V2I uiPos = dataViewButtonBounds.pos;
		uiPos.y -= uiPadding;

		DataViewUI *dataView = &gameState->dataViewUI[gameState->dataLayerToDraw];

		s32 paletteBlockSize = font->lineHeight;

		UI::Panel ui = UI::Panel(irectAligned(uiPos.x, uiPos.y, 240, 1000, ALIGN_BOTTOM | ALIGN_LEFT), null, UI::PanelFlags::LayoutBottomToTop);
		{
			// We're working from bottom to top, so we start at the end.

			// First, the named colors
			if (dataView->hasFixedColors)
			{
				Array<V4> *fixedPalette = getPalette(dataView->fixedPaletteName);
				ASSERT(fixedPalette->count >= dataView->fixedColorNames.count);

				for (s32 fixedColorIndex = dataView->fixedColorNames.count-1; fixedColorIndex >= 0; fixedColorIndex--)
				{
					Rect2I paletteBlockBounds = ui.addBlank(paletteBlockSize, paletteBlockSize);
					drawSingleRect(uiBuffer, paletteBlockBounds, renderer->shaderIds.untextured, asOpaque((*fixedPalette)[fixedColorIndex]));

					ui.addLabel(getText(dataView->fixedColorNames[fixedColorIndex]));
					ui.startNewLine();
				}
			}

			// Above that, the gradient
			if (dataView->hasGradient)
			{
				// Arbitrarily going to make the height 4x the width
				s32 gradientHeight = paletteBlockSize * 4;
				UI::Panel gradientPanel = ui.row(gradientHeight, ALIGN_BOTTOM, "plain"_s);

				UI::Panel gradientColumn = gradientPanel.column(paletteBlockSize, ALIGN_LEFT, "plain"_s);
				{
					Rect2I gradientBounds = gradientColumn.addBlank(paletteBlockSize, gradientHeight);

					Array<V4> *gradientPalette = getPalette(dataView->gradientPaletteName);
					V4 minColor = asOpaque(*gradientPalette->first());
					V4 maxColor = asOpaque(*gradientPalette->last());

					drawSingleRect(uiBuffer, rect2(gradientBounds), renderer->shaderIds.untextured, maxColor, maxColor, minColor, minColor);
				}
				gradientColumn.end();

				gradientPanel.addLabel(getText("data_view_minimum"_s));

				// @Hack! We read some internal Panel fields to manually move the "max" label up
				s32 spaceHeight = (gradientHeight - (2 * gradientPanel.largestItemHeightOnLine) - (2 * gradientPanel.style->contentPadding));
				gradientPanel.startNewLine();
				gradientPanel.addBlank(1, spaceHeight);
				gradientPanel.startNewLine();

				gradientPanel.addLabel(getText("data_view_maximum"_s));
				
				gradientPanel.end();
			}

			// Title and close button
			// TODO: Probably want to make this a Window that can't be moved?
			ui.addLabel(getText(dataView->title), "title"_s);

			ui.alignWidgets(ALIGN_RIGHT);
			if (ui.addTextButton("X"_s))
			{
				gameState->dataLayerToDraw = DataView_None;
			}

		}
		ui.end(true);
	}
}
