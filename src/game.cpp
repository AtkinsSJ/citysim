#pragma once

GameState *initialiseGameState()
{
	GameState *result;
	bootstrapArena(GameState, result, gameArena);
	initRandom(&result->gameRandom, Random_MT, 12345);

	s32 gameStartFunds = 1000000;
	initCity(&result->gameArena, &result->gameRandom, &result->city, 133, 117, LOCAL("city_default_name"), gameStartFunds);
	generateTerrain(&result->city);

	result->status = GameStatus_Playing;

	result->actionMode = ActionMode_None;

	result->worldDragState.citySize = v2i(result->city.width, result->city.height);

	return result;
}

void inputMoveCamera(Camera *camera, InputState *inputState, V2 windowSize, s32 cityWidth, s32 cityHeight)
{ 
	DEBUG_FUNCTION();
	
	const f32 CAMERA_MARGIN = 1; // How many tiles beyond the map can the camera scroll to show?
	const f32 CAMERA_PAN_SPEED = 10.0f; // Measured in world units per second

	// Zooming
	s32 zoomDelta = inputState->wheelY;

	// Turns out that having the zoom bound to the same key I use for navigating debug frames is REALLY ANNOYING
	// if (keyJustPressed(inputState, SDLK_PAGEUP))
	// {
	// 	zoomDelta++;
	// }
	// else if (keyJustPressed(inputState, SDLK_PAGEDOWN))
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
	f32 cameraEdgeScrollMarginX = cameraEdgeScrollPixelMargin / windowSize.x;
	f32 cameraEdgeScrollMarginY = cameraEdgeScrollPixelMargin / windowSize.y;

	if (mouseButtonPressed(inputState, MouseButton_Middle))
	{
		// Click-panning!
		float scale = scrollSpeed * 1.0f;
		V2 clickStartPos = getClickStartPos(inputState, MouseButton_Middle, camera);
		camera->pos += (camera->mousePos - clickStartPos) * scale;
	}
	else
	{
		if (keyIsPressed(inputState, SDLK_LEFT)
			|| keyIsPressed(inputState, SDLK_a)
			|| (inputState->mousePosNormalised.x < (-1.0f + cameraEdgeScrollMarginX)))
		{
			camera->pos.x -= scrollSpeed;
		}
		else if (keyIsPressed(inputState, SDLK_RIGHT)
			|| keyIsPressed(inputState, SDLK_d)
			|| (inputState->mousePosNormalised.x > (1.0f - cameraEdgeScrollMarginX)))
		{
			camera->pos.x += scrollSpeed;
		}

		if (keyIsPressed(inputState, SDLK_UP)
			|| keyIsPressed(inputState, SDLK_w)
			|| (inputState->mousePosNormalised.y > (1.0f - cameraEdgeScrollMarginY)))
		{
			camera->pos.y -= scrollSpeed;
		}
		else if (keyIsPressed(inputState, SDLK_DOWN)
			|| keyIsPressed(inputState, SDLK_s)
			|| (inputState->mousePosNormalised.y < (-1.0f + cameraEdgeScrollMarginY)))
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

DragResult updateDragState(DragState *dragState, InputState *inputState, V2I mouseTilePos, bool mouseIsOverUI, DragType dragType, V2I itemSize = {1,1})
{
	DEBUG_FUNCTION();

	DragResult result = {};

	if (dragState->isDragging && mouseButtonJustReleased(inputState, MouseButton_Left))
	{
		result.operation = DragResult_DoAction;
		result.dragRect = getDragArea(dragState, dragType, itemSize);

		dragState->isDragging = false;
	}
	else
	{
		// Update the dragging state
		if (!mouseIsOverUI && mouseButtonJustPressed(inputState, MouseButton_Left))
		{
			dragState->isDragging = true;
			dragState->mouseDragStartWorldPos = dragState->mouseDragEndWorldPos = mouseTilePos;
		}

		if (mouseButtonPressed(inputState, MouseButton_Left) && dragState->isDragging)
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
	Terrain *terrain = getTerrainAt(city, tilePos.x, tilePos.y);
	String terrainName = get(&terrainDefs, terrain->type)->name;
	window_label(context, myprintf("Terrain: {0}", {terrainName}));

	// Zone
	ZoneType zone = getZoneAt(city, tilePos.x, tilePos.y);
	window_label(context, myprintf("Zone: {0}", {zone ? zoneDefs[zone].name : makeString("None")}));

	// Building
	Building *building = getBuildingAtPosition(city, tilePos.x, tilePos.y);
	if (building != null)
	{
		BuildingDef *def = getBuildingDef(building->typeID);
		window_label(context, myprintf("Building: {0} (ID {1})", {def->name, formatInt(building->id)}));
		window_label(context, myprintf("- Residents: {0} / {1}", {formatInt(building->currentResidents), formatInt(def->residents)}));
		window_label(context, myprintf("- Jobs: {0} / {1}", {formatInt(building->currentJobs), formatInt(def->jobs)}));
		window_label(context, myprintf("- Power: {0}", {formatInt(def->power)}));
	}
	else
	{
		window_label(context, makeString("Building: None"));
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
}

void pauseMenuWindowProc(WindowContext *context, void *userData)
{
	DEBUG_FUNCTION();

	userData = userData; // Prevent the dumb warning

	// Centred, with equal button sizes
	context->alignment = ALIGN_H_CENTRE;

	UIButtonStyle *buttonStyle = findButtonStyle(&context->uiState->assets->theme, context->windowStyle->buttonStyleName);
	BitmapFont *buttonFont = getFont(context->uiState->assets, buttonStyle->fontName);
	f32 availableButtonTextWidth = context->contentArea.w - (2.0f * buttonStyle->padding);
	s32 maxButtonTextWidth = 0;

	String resume = LOCAL("button_resume");
	maxButtonTextWidth = max(maxButtonTextWidth, round_s32(calculateTextSize(buttonFont, resume, availableButtonTextWidth).x));
	String save   = LOCAL("button_save");
	maxButtonTextWidth = max(maxButtonTextWidth, round_s32(calculateTextSize(buttonFont, save, availableButtonTextWidth).x));
	String load   = LOCAL("button_load");
	maxButtonTextWidth = max(maxButtonTextWidth, round_s32(calculateTextSize(buttonFont, load, availableButtonTextWidth).x));
	String about  = LOCAL("button_about");
	maxButtonTextWidth = max(maxButtonTextWidth, round_s32(calculateTextSize(buttonFont, about, availableButtonTextWidth).x));
	String exit   = LOCAL("button_exit");
	maxButtonTextWidth = max(maxButtonTextWidth, round_s32(calculateTextSize(buttonFont, exit, availableButtonTextWidth).x));

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
		context->closeRequested = true;
	}
}

void updateAndRenderGameUI(Renderer *renderer, AssetManager *assets, UIState *uiState, GameState *gameState)
{
	DEBUG_FUNCTION();
	
	RenderBuffer *uiBuffer = &renderer->uiBuffer;
	f32 windowWidth = (f32) renderer->uiCamera.size.x;
	V2 centre = renderer->uiCamera.pos;
	UITheme *theme = &assets->theme;
	UILabelStyle *labelStyle = findLabelStyle(theme, makeString("title"));
	BitmapFont *font = getFont(assets, labelStyle->fontName);
	City *city = &gameState->city;

	const f32 uiPadding = 4; // TODO: Move this somewhere sensible!
	f32 left = uiPadding;
	f32 right = windowWidth - uiPadding;

	Rect2 uiRect = rectXYWH(0,0, windowWidth, 64);
	append(&uiState->uiRects, uiRect);
	drawSingleRect(uiBuffer, uiRect, renderer->shaderIds.untextured, theme->overlayColor);

	uiText(renderer, uiState->uiBuffer, font, city->name,
	       v2(left, uiPadding), ALIGN_LEFT, labelStyle->textColor);

	uiText(renderer, uiState->uiBuffer, font, myprintf("£{0} (-£{1}/month)", {formatInt(city->funds), formatInt(city->monthlyExpenditure)}), v2(centre.x, uiPadding), ALIGN_H_CENTRE, labelStyle->textColor);

	uiText(renderer, uiState->uiBuffer, font, myprintf("Pop: {0}, Jobs: {1}", {formatInt(city->totalResidents), formatInt(city->totalJobs)}), v2(centre.x, uiPadding+30), ALIGN_H_CENTRE, labelStyle->textColor);

	uiText(renderer, uiState->uiBuffer, font, myprintf("Power: {0}/{1}", {formatInt(city->powerLayer.cachedCombinedConsumption), formatInt(city->powerLayer.cachedCombinedProduction)}),
	       v2(right, uiPadding), ALIGN_RIGHT, labelStyle->textColor);

	uiText(renderer, uiState->uiBuffer, font, myprintf("R: {0}\nC: {1}\nI: {2}", {formatInt(city->residentialDemand), formatInt(city->commercialDemand), formatInt(city->industrialDemand)}),
	       v2(windowWidth * 0.75f, uiPadding), ALIGN_RIGHT, labelStyle->textColor);


	// Build UI
	{
		Rect2 buttonRect = rectXYWH(uiPadding, 28 + uiPadding, 80, 24);

		// The "ZONE" menu
		if (uiMenuButton(uiState, renderer, LOCAL("button_zone"), buttonRect, Menu_Zone))
		{
			RenderItem_DrawSingleRect *background = appendDrawRectPlaceholder(uiBuffer, renderer->shaderIds.untextured);
			Rect2 menuButtonRect = buttonRect;
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			
			Rect2 menuRect = rectXYWH(menuButtonRect.x - uiPadding, menuButtonRect.y - uiPadding, menuButtonRect.w + (uiPadding * 2), uiPadding);

			for (s32 zoneIndex=0; zoneIndex < ZoneCount; zoneIndex++)
			{
				if (uiButton(uiState, renderer, zoneDefs[zoneIndex].name, menuButtonRect,
						(gameState->actionMode == ActionMode_Zone) && (gameState->selectedZoneID == zoneIndex)))
				{
					uiCloseMenus(uiState);
					gameState->selectedZoneID = (ZoneType) zoneIndex;
					gameState->actionMode = ActionMode_Zone;
					setCursor(renderer, "build.png");
				}

				menuButtonRect.y += menuButtonRect.h + uiPadding;
				menuRect.h += menuButtonRect.h + uiPadding;
			}

			append(&uiState->uiRects, menuRect);
			fillDrawRectPlaceholder(background, menuRect, theme->overlayColor);
		}
		buttonRect.x += buttonRect.w + uiPadding;

		// The "BUILD" menu
		if (uiMenuButton(uiState, renderer, LOCAL("button_build"), buttonRect, Menu_Build))
		{
			RenderItem_DrawSingleRect *background = appendDrawRectPlaceholder(uiBuffer, renderer->shaderIds.untextured);
			Rect2 menuButtonRect = buttonRect;
			menuButtonRect.y += menuButtonRect.h + uiPadding;

			Rect2 menuRect = rectXYWH(menuButtonRect.x - uiPadding, menuButtonRect.y - uiPadding, menuButtonRect.w + (uiPadding * 2), uiPadding);

			for (auto it = iterate(getConstructibleBuildings());
				!it.isDone;
				next(&it))
			{
				BuildingDef *buildingDef = getValue(it);
				ASSERT(buildingDef->buildMethod != BuildMethod_None); //We somehow got an un-constructible building in our constructible buildings list!

				if (uiButton(uiState, renderer, buildingDef->name, menuButtonRect,
						(gameState->actionMode == ActionMode_Build) && (gameState->selectedBuildingTypeID == buildingDef->typeID)))
				{
					uiCloseMenus(uiState);
					gameState->selectedBuildingTypeID = buildingDef->typeID;
					gameState->actionMode = ActionMode_Build;
					setCursor(renderer, "build.png");
				}

				menuButtonRect.y += menuButtonRect.h + uiPadding;
				menuRect.h += menuButtonRect.h + uiPadding;
			}

			append(&uiState->uiRects, menuRect);
			fillDrawRectPlaceholder(background, menuRect, theme->overlayColor);
		}
		buttonRect.x += buttonRect.w + uiPadding;

		if (uiButton(uiState, renderer, LOCAL("button_demolish"), buttonRect,
					(gameState->actionMode == ActionMode_Demolish),
					SDLK_x, makeString("(X)")))
		{
			gameState->actionMode = ActionMode_Demolish;
			setCursor(renderer, "demolish.png");
		}
		buttonRect.x += buttonRect.w + uiPadding;

		// The, um, "MENU" menu. Hmmm.
		buttonRect.x = windowWidth - (buttonRect.w + uiPadding);
		if (uiButton(uiState, renderer, LOCAL("button_menu"), buttonRect))
		{
			showWindow(uiState, LOCAL("title_menu"), 200, 200, v2i(0,0), makeString("general"), WinFlag_Unique|WinFlag_Modal|WinFlag_AutomaticHeight, pauseMenuWindowProc, null);
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

void updateAndRenderGame(AppState *appState, InputState *inputState, Renderer *renderer, AssetManager *assets)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);
	
	if (appState->gameState == null)
	{
		appState->gameState = initialiseGameState();
		renderer->worldCamera.pos = v2(appState->gameState->city.width/2, appState->gameState->city.height/2);

		refreshBuildingSpriteCache(&buildingCatalogue, assets);
		refreshTerrainSpriteCache(&terrainDefs, assets);
	}

	GameState *gameState = appState->gameState;
	City *city = &gameState->city;

	if (assets->assetReloadHasJustHappened)
	{
		refreshBuildingSpriteCache(&buildingCatalogue, assets);
		refreshTerrainSpriteCache(&terrainDefs, assets);
	}


	// Update the simulation... need a smarter way of doing this!
	{
		DEBUG_BLOCK_T("Update simulation", DCDT_GameUpdate);

		calculateDemand(city);
		growSomeZoneBuildings(city);

		updateZoneLayer(city, &city->zoneLayer);

		updatePowerLayer(city, &city->powerLayer);
	}


	// UI!
	UIState *uiState = &globalAppState.uiState;
	updateAndRenderGameUI(renderer, assets, uiState, gameState);

	V4 ghostColorValid    = color255(128,255,128,255);
	V4 ghostColorInvalid  = color255(255,0,0,128);

	// CAMERA!
	Camera *worldCamera = &renderer->worldCamera;
	Camera *uiCamera    = &renderer->uiCamera;
	if (gameState->status == GameStatus_Playing)
	{
		inputMoveCamera(worldCamera, inputState, uiCamera->size, gameState->city.width, gameState->city.height);
	}

	V2I mouseTilePos = v2i(worldCamera->mousePos);
	bool mouseIsOverUI = uiState->mouseInputHandled;
	if (!mouseIsOverUI)
	{
		for (int i=0; i < uiState->uiRects.count; i++)
		{
			if (contains(uiState->uiRects.items[i], uiCamera->mousePos))
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

							if ((buildingDef->buildMethod == BuildMethod_Plop && mouseButtonJustReleased(inputState, MouseButton_Left))
							|| (buildingDef->buildMethod == BuildMethod_Paint && mouseButtonPressed(inputState, MouseButton_Left)))
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

						DragResult dragResult = updateDragState(&gameState->worldDragState, inputState, mouseTilePos, mouseIsOverUI, dragType, buildingDef->size);
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
									drawSingleRect(&renderer->worldOverlayBuffer, rect2(dragResult.dragRect), renderer->shaderIds.untextured, color255(255, 64, 64, 128));
								}
							} break;
						}
					} break;
				}
			} break;

			case ActionMode_Zone:
			{
				DragResult dragResult = updateDragState(&gameState->worldDragState, inputState, mouseTilePos, mouseIsOverUI, DragRect);

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
								zoneDefs[gameState->selectedZoneID].color
							};
							drawGrid(&renderer->worldOverlayBuffer, rect2(canZoneQuery->bounds), renderer->shaderIds.untextured, canZoneQuery->bounds.w, canZoneQuery->bounds.h, canZoneQuery->tileCanBeZoned, 2, palette);
						}
						else
						{
							drawSingleRect(&renderer->worldOverlayBuffer, rect2(dragResult.dragRect), renderer->shaderIds.untextured, color255(255, 64, 64, 128));
						}
					} break;
				}
			} break;

			case ActionMode_Demolish:
			{
				DragResult dragResult = updateDragState(&gameState->worldDragState, inputState, mouseTilePos, mouseIsOverUI, DragRect);
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
							drawSingleRect(&renderer->worldOverlayBuffer, rect2(dragResult.dragRect), renderer->shaderIds.untextured, color255(128, 0, 0, 128));
						}
						else
						{
							drawSingleRect(&renderer->worldOverlayBuffer, rect2(dragResult.dragRect), renderer->shaderIds.untextured, color255(255, 64, 64, 128));
						}
					} break;
				}
			} break;

			case ActionMode_None:
			{
				if (!mouseIsOverUI && mouseButtonJustPressed(inputState, MouseButton_Left))
				{
					if (tileExists(city, mouseTilePos.x, mouseTilePos.y))
					{
						gameState->inspectedTilePosition = mouseTilePos;
						showWindow(uiState, makeString("Inspect tile"), 250, 200, v2i(0,0), makeString("general"), WinFlag_AutomaticHeight | WinFlag_Unique, inspectTileWindowProc, gameState);
					}
				}
			} break;
		}
	}

	if (gameState->worldDragState.isDragging && mouseIsOverUI && mouseButtonJustReleased(inputState, MouseButton_Left))
	{
		// Not sure if this is the best idea, but it's the best I can come up with.
		gameState->worldDragState.isDragging = false;
	}

	if (mouseButtonJustPressed(inputState, MouseButton_Right))
	{
		// Unselect current thing
		gameState->actionMode = ActionMode_None;
		setCursor(renderer, "default.png");
	}

	// RENDERING
	// Pre-calculate the tile area that's visible to the player.
	// We err on the side of drawing too much, rather than risking having holes in the world.
	Rect2I visibleTileBounds = irectCentreSize(
		v2i(worldCamera->pos), v2i(worldCamera->size / worldCamera->zoom) + v2i(3, 3)
	);
	visibleTileBounds = intersect(visibleTileBounds, irectXYWH(0, 0, city->width, city->height));

	drawCity(city, renderer, visibleTileBounds, demolitionRect);

	// Data layer rendering
	if (gameState->dataLayerToDraw)
	{
		DEBUG_BLOCK_T("Draw data layer", DCDT_GameUpdate);

		//
		// TODO: @Speed: Render in a batch - but I think we want to treat whole-map per-tile grids as a special
		// case for rendering. Instead of allocating a batch for possibly the whole map, then filling in part,
		// we could just upload the data grid as a texture. Or maybe embed a 2d array inside the RenderItem.
		// Lots of possibilities! But right now this is temporary code really, so I'll avoid optimising it.
		//
		// - Sam, 22/07/2019
		//

		for (s32 y = visibleTileBounds.y;
			y < visibleTileBounds.y + visibleTileBounds.h;
			y++)
		{
			for (s32 x = visibleTileBounds.x;
				x < visibleTileBounds.x + visibleTileBounds.w;
				x++)
			{
				bool tileHasData = false;
				V4 color = {};

				switch (gameState->dataLayerToDraw)
				{
					case DataLayer_Paths:
					{
						s32 pathGroup = getPathGroupAt(city, x, y);
						if (pathGroup > 0)
						{
							color = genericDataLayerColors[pathGroup % genericDataLayerColorCount];
							tileHasData = true;
						}
					} break;

					case DataLayer_Power:
					{
						PowerNetwork *powerNetwork = getPowerNetworkAt(city, x, y);
						if (powerNetwork != null)
						{
							color = genericDataLayerColors[powerNetwork->id % genericDataLayerColorCount];
							tileHasData = true;
						}
					} break;
				}

				if (tileHasData)
				{
					drawSingleRect(&renderer->worldBuffer, rectXYWH((f32)x, (f32)y, 1.0f, 1.0f), renderer->shaderIds.untextured, color);
				}
			}
		}
	}

	if (gameState->status == GameStatus_Quit)
	{
		freeMemoryArena(&gameState->gameArena);
		appState->gameState = null;
		appState->appStatus = AppStatus_MainMenu;
	}
}

void updateAndRender(AppState *appState, InputState *inputState, Renderer *renderer, AssetManager *assets)
{
	DEBUG_FUNCTION();

	AppStatus oldAppStatus = appState->appStatus;

	UIState *uiState = &appState->uiState;
	uiState->uiRects.count = 0;
	uiState->mouseInputHandled = false;
	updateWindows(uiState);
	
	switch (appState->appStatus)
	{
		case AppStatus_MainMenu:
		{
			updateAndRenderMainMenu(appState, renderer, assets);
		} break;

		case AppStatus_Credits:
		{
			updateAndRenderCredits(appState, renderer, assets);
		} break;

		case AppStatus_SettingsMenu:
		{
			updateAndRenderSettingsMenu(appState, renderer, assets);
		} break;

		case AppStatus_Game:
		{
			updateAndRenderGame(appState, inputState, renderer, assets);
		} break;

		case AppStatus_Quit: break;
		
		default:
		{
			ASSERT(false); //Not implemented this AppStatus yet!
		} break;
	}

	renderWindows(renderer, uiState);
	
	if (appState->appStatus != oldAppStatus)
	{
		clear(&uiState->openWindows);
	}

	drawUiMessage(uiState, renderer);
}