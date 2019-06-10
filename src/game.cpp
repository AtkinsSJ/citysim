#pragma once

const f32 uiPadding = 4; // TODO: Move this somewhere sensible!

GameState *initialiseGameState()
{
	GameState *result;
	bootstrapArena(GameState, result, gameArena);
	randomSeed(&result->gameRandom, 12345);

	initCity(&result->gameArena, &result->gameRandom, &result->city, 133, 117, LOCAL("city_default_name"), gameStartFunds);
	generateTerrain(&result->city);

	result->status = GameStatus_Playing;

	result->actionMode = ActionMode_None;

	initChunkedArray(&result->overlayRenderItems, &result->gameArena, 512);

	return result;
}

void inputMoveCamera(Camera *camera, InputState *inputState, V2 windowSize, s32 cityWidth, s32 cityHeight)
{ 
	DEBUG_FUNCTION();

	// Zooming
	if (canZoom)
	{
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
	}

	// Panning
	f32 scrollSpeed = (CAMERA_PAN_SPEED * (f32) sqrt(camera->zoom)) * SECONDS_PER_FRAME;
	f32 cameraEdgeScrollPixelMargin = 8.0f;
	f32 cameraEdgeScrollMarginX = cameraEdgeScrollPixelMargin / windowSize.x;
	f32 cameraEdgeScrollMarginY = cameraEdgeScrollPixelMargin / windowSize.y;

	if (mouseButtonPressed(inputState, SDL_BUTTON_MIDDLE))
	{
		// Click-panning!
		float scale = scrollSpeed * 5.0f;
		V2 clickStartPos = inputState->clickStartPosNormalised[mouseButtonIndex(SDL_BUTTON_MIDDLE)];
		camera->pos.x += (inputState->mousePosNormalised.x - clickStartPos.x) * scale;
		camera->pos.y += (inputState->mousePosNormalised.y - clickStartPos.y) * -scale;
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
	}

	return result;
}

DragResult updateDragState(DragState *dragState, InputState *inputState, V2I mouseTilePos, bool mouseIsOverUI, DragType dragType, V2I itemSize = {1,1})
{
	DEBUG_FUNCTION();

	DragResult result = {};

	if (dragState->isDragging && mouseButtonJustReleased(inputState, SDL_BUTTON_LEFT))
	{
		result.operation = DragResult_DoAction;
		result.dragRect = getDragArea(dragState, dragType, itemSize);

		dragState->isDragging = false;
	}
	else
	{
		// Update the dragging state
		if (!mouseIsOverUI && mouseButtonJustPressed(inputState, SDL_BUTTON_LEFT))
		{
			dragState->isDragging = true;
			dragState->mouseDragStartWorldPos = dragState->mouseDragEndWorldPos = mouseTilePos;
		}

		if (mouseButtonPressed(inputState, SDL_BUTTON_LEFT) && dragState->isDragging)
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

	// Sector
	Sector *sector = getSectorAtTilePos(city, tilePos.x, tilePos.y);
	s32 relX = tilePos.x - sector->bounds.x;
	s32 relY = tilePos.y - sector->bounds.y;

	window_label(context, myprintf("Sector: x={0} y={1} w={2} h={3}", {formatInt(sector->bounds.x), formatInt(sector->bounds.y), formatInt(sector->bounds.w), formatInt(sector->bounds.h)}));

	// Terrain
	String terrainName = get(&terrainDefs, sector->terrain[relY][relX].type)->name;
	window_label(context, myprintf("Terrain: {0}", {terrainName}));

	// Zone
	ZoneType zone = sector->tileZone[relY][relX];
	window_label(context, myprintf("Zone: {0}", {zone ? zoneDefs[zone].name : makeString("None")}));

	// Building
	Building *building = getBuildingAtPosition(city, tilePos.x, tilePos.y);
	if (building != null)
	{
		BuildingDef *def = get(&buildingDefs, building->typeID);
		window_label(context, myprintf("Building: {0} (ID {1})", {def->name, formatInt(building->id)}));
		window_label(context, myprintf("- Residents: {0} / {1}", {formatInt(building->currentResidents), formatInt(def->residents)}));
		window_label(context, myprintf("- Jobs: {0} / {1}", {formatInt(building->currentJobs), formatInt(def->jobs)}));
	}
	else
	{
		window_label(context, makeString("Building: None"));
	}

	// Power group
	s32 powerGroupIndex = powerGroupAt(city, tilePos.x, tilePos.y);
	if (powerGroupIndex != 0)
	{
		PowerGroup *powerGroup = get(&sector->powerGroups, powerGroupIndex - 1);
		window_label(context, myprintf("Power Group: #{0}/{1} within sector\n- Production: {2}\n- Consumption: {3}", {formatInt(powerGroupIndex), formatInt(sector->powerGroups.count), formatInt(powerGroup->production), formatInt(powerGroup->consumption)}));
	}
	else
	{
		window_label(context, makeString("Power Group: None"));
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
		pushUiMessage(context->uiState, makeString("Saving isn't implemented yet!"));
	}

	if (window_button(context, load, maxButtonTextWidth))
	{
		pushUiMessage(context->uiState, makeString("Loading isn't implemented yet!"));
	}

	if (window_button(context, about, maxButtonTextWidth))
	{
		showAboutWindow(context->uiState);
	}

	window_label(context, makeString("Test text"));

	if (window_button(context, exit, maxButtonTextWidth))
	{
		globalAppState.gameState->status = GameStatus_Quit;
		context->closeRequested = true;
	}
}

void updateAndRenderGameUI(RenderBuffer *uiBuffer, AssetManager *assets, UIState *uiState, GameState *gameState)
{
	DEBUG_FUNCTION();
	
	f32 windowWidth = (f32) uiBuffer->camera.size.x;
	V2 centre = uiBuffer->camera.pos;
	UITheme *theme = &assets->theme;
	UILabelStyle *labelStyle = findLabelStyle(theme, makeString("title"));
	BitmapFont *font = getFont(assets, labelStyle->fontName);
	City *city = &gameState->city;

	f32 left = uiPadding;
	f32 right = windowWidth - uiPadding;

	Rect2 uiRect = rectXYWH(0,0, windowWidth, 64);
	append(&uiState->uiRects, uiRect);
	drawRect(uiBuffer, uiRect, 0, uiState->untexturedShaderID, theme->overlayColor);

	uiText(uiState, font, city->name,
	       v2(left, uiPadding), ALIGN_LEFT, 1, labelStyle->textColor);

	uiText(uiState, font, myprintf("£{0} (-£{1}/month)", {formatInt(city->funds), formatInt(city->monthlyExpenditure)}), v2(centre.x, uiPadding), ALIGN_H_CENTRE, 1, labelStyle->textColor);

	uiText(uiState, font, myprintf("Pop: {0}, Jobs: {1}", {formatInt(city->totalResidents), formatInt(city->totalJobs)}), v2(centre.x, uiPadding+30), ALIGN_H_CENTRE, 1, labelStyle->textColor);

	uiText(uiState, font, myprintf("Power: {0}/{1}", {formatInt(city->powerLayer.combined.consumption), formatInt(city->powerLayer.combined.production)}),
	       v2(right, uiPadding), ALIGN_RIGHT, 1, labelStyle->textColor);

	uiText(uiState, font, myprintf("R: {0}\nC: {1}\nI: {2}", {formatInt(city->residentialDemand), formatInt(city->commercialDemand), formatInt(city->industrialDemand)}),
	       v2(windowWidth * 0.75f, uiPadding), ALIGN_RIGHT, 1, labelStyle->textColor);


	// Build UI
	{
		Rect2 buttonRect = rectXYWH(uiPadding, 28 + uiPadding, 80, 24);

		// The "ZONE" menu
		if (uiMenuButton(uiState, LOCAL("button_zone"), buttonRect, 1, Menu_Zone))
		{
			Rect2 menuButtonRect = buttonRect;
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			
			Rect2 menuRect = rectXYWH(menuButtonRect.x - uiPadding, menuButtonRect.y - uiPadding, menuButtonRect.w + (uiPadding * 2), uiPadding);

			for (s32 zoneIndex=0; zoneIndex < ZoneCount; zoneIndex++)
			{
				if (uiButton(uiState, zoneDefs[zoneIndex].name, menuButtonRect, 1,
						(gameState->actionMode == ActionMode_Zone) && (gameState->selectedZoneID == zoneIndex)))
				{
					uiCloseMenus(uiState);
					gameState->selectedZoneID = (ZoneType) zoneIndex;
					gameState->actionMode = ActionMode_Zone;
					setCursor(uiState, "build.png");
				}

				menuButtonRect.y += menuButtonRect.h + uiPadding;
				menuRect.h += menuButtonRect.h + uiPadding;
			}

			append(&uiState->uiRects, menuRect);
			drawRect(uiBuffer, menuRect, 0, uiState->untexturedShaderID, theme->overlayColor);
		}
		buttonRect.x += buttonRect.w + uiPadding;

		// The "BUILD" menu
		if (uiMenuButton(uiState, LOCAL("button_build"), buttonRect, 1, Menu_Build))
		{
			Rect2 menuButtonRect = buttonRect;
			menuButtonRect.y += menuButtonRect.h + uiPadding;

			Rect2 menuRect = rectXYWH(menuButtonRect.x - uiPadding, menuButtonRect.y - uiPadding, menuButtonRect.w + (uiPadding * 2), uiPadding);

			for (s32 i=0; i < buildingDefs.count; i++)
			{
				BuildingDef *buildingDef = get(&buildingDefs, i);
				if (!buildingDef->buildMethod) continue;

				if (uiButton(uiState, buildingDef->name, menuButtonRect, 1,
						(gameState->actionMode == ActionMode_Build) && (gameState->selectedBuildingTypeID == i)))
				{
					uiCloseMenus(uiState);
					gameState->selectedBuildingTypeID = i;
					gameState->actionMode = ActionMode_Build;
					setCursor(uiState, "build.png");
				}

				menuButtonRect.y += menuButtonRect.h + uiPadding;
				menuRect.h += menuButtonRect.h + uiPadding;
			}

			append(&uiState->uiRects, menuRect);
			drawRect(uiBuffer, menuRect, 0, uiState->untexturedShaderID, theme->overlayColor);
		}
		buttonRect.x += buttonRect.w + uiPadding;

		if (uiButton(uiState, LOCAL("button_demolish"), buttonRect, 1,
					(gameState->actionMode == ActionMode_Demolish),
					SDLK_x, makeString("(X)")))
		{
			gameState->actionMode = ActionMode_Demolish;
			setCursor(uiState, "demolish.png");
		}
		buttonRect.x += buttonRect.w + uiPadding;

		// The, um, "MENU" menu. Hmmm.
		buttonRect.x = windowWidth - (buttonRect.w + uiPadding);
		if (uiButton(uiState, LOCAL("button_menu"), buttonRect, 1))
		{
			showWindow(uiState, LOCAL("title_menu"), 200, 200, makeString("general"), WinFlag_Unique|WinFlag_Modal|WinFlag_AutomaticHeight, pauseMenuWindowProc, null);
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

void pushOverlayRenderItem(GameState *gameState, Rect2 rect, f32 depth, V4 color, s32 shaderID, Sprite *sprite = null)
{
	Asset *texture = null;
	Rect2 uv = {};
	if (sprite != null)
	{
		texture = sprite->texture;
		uv = sprite->uv;
	}
	makeRenderItem(appendBlank(&gameState->overlayRenderItems), rect, depth, texture, uv, shaderID, color);
}

void updateAndRenderGame(AppState *appState, InputState *inputState, Renderer *renderer, AssetManager *assets)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);
	
	if (appState->gameState == null)
	{
		appState->gameState = initialiseGameState();
		renderer->worldBuffer.camera.pos = v2(appState->gameState->city.width/2, appState->gameState->city.height/2);

		refreshBuildingSpriteCache(&buildingDefs, assets);
		refreshTerrainSpriteCache(&terrainDefs, assets);
	}

	GameState *gameState = appState->gameState;
	City *city = &gameState->city;

	if (assets->assetReloadHasJustHappened)
	{	
		refreshZoneGrowableBuildingLists(&city->zoneLayer);

		refreshBuildingSpriteCache(&buildingDefs, assets);
		refreshTerrainSpriteCache(&terrainDefs, assets);
	}


	// Update the simulation... need a smarter way of doing this!
	{
		DEBUG_BLOCK_T("Update simulation", DCDT_GameUpdate);
		calculateDemand(city);
		growSomeZoneBuildings(city);
	}


	// UI!
	UIState *uiState = &globalAppState.uiState;
	updateAndRenderGameUI(&renderer->uiBuffer, assets, uiState, gameState);

	s32 pixelArtShaderID  = getShader(assets, makeString("pixelart.glsl"  ))->rendererShaderID;
	s32 rectangleShaderID = getShader(assets, makeString("untextured.glsl"))->rendererShaderID;

	// CAMERA!
	Camera *worldCamera = &renderer->worldBuffer.camera;
	Camera *uiCamera    = &renderer->uiBuffer.camera;
	if (gameState->status == GameStatus_Playing)
	{
		inputMoveCamera(worldCamera, inputState, uiCamera->size, gameState->city.width, gameState->city.height);
	}

	V2I mouseTilePos = tilePosition(worldCamera->mousePos);
	bool mouseIsOverUI = uiState->mouseInputHandled || inRects(uiState->uiRects.items, uiState->uiRects.count, uiCamera->mousePos);
	Rect2I demolitionRect = {0,0,-1,-1};

	{
		DEBUG_BLOCK_T("ActionMode update", DCDT_GameUpdate);

		switch (gameState->actionMode)
		{
			case ActionMode_Build:
			{
				BuildingDef *buildingDef = get(&buildingDefs, gameState->selectedBuildingTypeID);

				switch (buildingDef->buildMethod)
				{
					case BuildMethod_Paint:
					{
						if (!mouseIsOverUI)
						{
							Rect2I footprint = irectCentreDim(mouseTilePos, buildingDef->size);
							if (mouseButtonPressed(inputState, SDL_BUTTON_LEFT))
							{
								placeBuilding(uiState, city, buildingDef, footprint.x, footprint.y, true);
							}

							s32 buildCost = buildingDef->buildCost;
							showCostTooltip(uiState, buildCost);

							V4 ghostColor = color255(128,255,128,255);
							if (!canPlaceBuilding(uiState, &gameState->city, buildingDef, footprint.x, footprint.y))
							{
								ghostColor = color255(255,0,0,128);
							}

							Sprite *sprite = getSprite(buildingDef->sprites, 0);
							pushOverlayRenderItem(gameState, rect2(footprint), depthFromY(mouseTilePos.y) + 100, ghostColor, pixelArtShaderID, sprite);
						}
					} break;

					case BuildMethod_Plop:
					{
						if (!mouseIsOverUI)
						{
							Rect2I footprint = irectCentreDim(mouseTilePos, buildingDef->size);
							if (mouseButtonJustReleased(inputState, SDL_BUTTON_LEFT))
							{
								placeBuilding(uiState, city, buildingDef, footprint.x, footprint.y, true);
							}

							s32 buildCost = buildingDef->buildCost;
							showCostTooltip(uiState, buildCost);

							V4 ghostColor = color255(128,255,128,255);
							if (!canPlaceBuilding(uiState, &gameState->city, buildingDef, footprint.x, footprint.y))
							{
								ghostColor = color255(255,0,0,128);
							}
							
							Sprite *sprite = getSprite(buildingDef->sprites, 0);
							pushOverlayRenderItem(gameState, rect2(footprint), depthFromY(mouseTilePos.y) + 100, ghostColor, pixelArtShaderID, sprite);
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
									placeBuildingRect(uiState, city, buildingDef, dragResult.dragRect);
								}
								else
								{
									pushUiMessage(uiState, makeString("Not enough money for construction."));
								}
							} break;

							case DragResult_ShowPreview:
							{
								if (!mouseIsOverUI) showCostTooltip(uiState, buildCost);

								if (canAfford(city, buildCost))
								{
									Sprite *sprite = getSprite(buildingDef->sprites, 0);
									for (s32 y=0; y + buildingDef->height <= dragResult.dragRect.h; y += buildingDef->height)
									{
										for (s32 x=0; x + buildingDef->width <= dragResult.dragRect.w; x += buildingDef->width)
										{
											V4 ghostColor = color255(128,255,128,255);
											if (!canPlaceBuilding(uiState, city, buildingDef, dragResult.dragRect.x + x, dragResult.dragRect.y + y))
											{
												ghostColor = color255(255,0,0,128);
											}

											Rect2 rect = rectXYWHi(dragResult.dragRect.x + x, dragResult.dragRect.y + y, buildingDef->width, buildingDef->height);
											pushOverlayRenderItem(gameState, rect, depthFromY(dragResult.dragRect.y + y) + 100, ghostColor, pixelArtShaderID, sprite);
										}
									}
								}
								else
								{
									pushOverlayRenderItem(gameState, rect2(dragResult.dragRect), 0, color255(255, 64, 64, 128), rectangleShaderID);
								}
							} break;
						}
					} break;
				}
			} break;

			case ActionMode_Zone:
			{
				DragResult dragResult = updateDragState(&gameState->worldDragState, inputState, mouseTilePos, mouseIsOverUI, DragRect);
				s32 zoneCost = calculateZoneCost(city, gameState->selectedZoneID, dragResult.dragRect);

				switch (dragResult.operation)
				{
					case DragResult_DoAction:
					{
						if (canAfford(city, zoneCost))
						{
							placeZone(uiState, city, gameState->selectedZoneID, dragResult.dragRect);
						}
					} break;

					case DragResult_ShowPreview:
					{
						if (!mouseIsOverUI) showCostTooltip(uiState, zoneCost);

						if (canAfford(city, zoneCost))
						{
							for (s32 y = dragResult.dragRect.y; y < dragResult.dragRect.y+dragResult.dragRect.h; y++)
							{
								for (s32 x = dragResult.dragRect.x; x < dragResult.dragRect.x+dragResult.dragRect.w; x++)
								{
									if (canZoneTile(city, gameState->selectedZoneID, x, y))
									{
										pushOverlayRenderItem(gameState, rectXYWHi(x, y, 1, 1), 0, zoneDefs[gameState->selectedZoneID].color, rectangleShaderID);
									}
								}
							}
						}
						else
						{
							pushOverlayRenderItem(gameState, rect2(dragResult.dragRect), 0, color255(255, 64, 64, 128), rectangleShaderID);
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
							demolishRect(uiState, city, dragResult.dragRect);
						}
						else
						{
							pushUiMessage(uiState, makeString("Not enough money for demolition."));
						}
					} break;

					case DragResult_ShowPreview:
					{
						if (!mouseIsOverUI) showCostTooltip(uiState, demolishCost);

						if (canAfford(city, demolishCost))
						{
							// Demolition outline
							pushOverlayRenderItem(gameState, rect2(dragResult.dragRect), 0, color255(128, 0, 0, 128), rectangleShaderID);
						}
						else
						{
							pushOverlayRenderItem(gameState, rect2(dragResult.dragRect), 0, color255(255, 64, 64, 128), rectangleShaderID);
						}
					} break;
				}
			} break;

			case ActionMode_None:
			{
				if (!mouseIsOverUI && mouseButtonJustPressed(inputState, SDL_BUTTON_LEFT))
				{
					if (tileExists(city, mouseTilePos.x, mouseTilePos.y))
					{
						gameState->inspectedTilePosition = mouseTilePos;
						showWindow(uiState, makeString("Inspect tile"), 250, 200, makeString("general"), WinFlag_AutomaticHeight | WinFlag_Unique, inspectTileWindowProc, gameState);
					}
				}
			} break;
		}
	}

	if (gameState->worldDragState.isDragging && mouseIsOverUI && mouseButtonJustReleased(inputState, SDL_BUTTON_LEFT))
	{
		// Not sure if this is the best idea, but it's the best I can come up with.
		gameState->worldDragState.isDragging = false;
	}

	if (mouseButtonJustPressed(inputState, SDL_BUTTON_RIGHT))
	{
		// Unselect current thing
		gameState->actionMode = ActionMode_None;
		setCursor(uiState, "default.png");
	}

	// RENDERING
	// Pre-calculate the tile area that's visible to the player.
	// We err on the side of drawing too much, rather than risking having holes in the world.
	Rect2I visibleTileBounds = irectCentreWH(
		v2i((s32)worldCamera->pos.x, (s32)worldCamera->pos.y),
		(s32) (worldCamera->size.x / worldCamera->zoom) + 3,
		(s32) (worldCamera->size.y / worldCamera->zoom) + 3
	);
	visibleTileBounds = cropRectangle(visibleTileBounds, irectXYWH(0, 0, city->width, city->height));
	Rect2I visibleSectors = irectXYWH(
		visibleTileBounds.x / SECTOR_SIZE,
		visibleTileBounds.y / SECTOR_SIZE,
		(visibleTileBounds.w / SECTOR_SIZE) + 2,
		(visibleTileBounds.h / SECTOR_SIZE) + 2
	);
	visibleSectors = cropRectangle(visibleSectors, irectXYWH(0, 0, city->sectorsX, city->sectorsY));

	// Draw terrain
	{
		DEBUG_BLOCK_T("Draw terrain", DCDT_GameUpdate);

		s32 terrainType = -1;
		SpriteGroup *terrainSprites = null;

		Rect2 spriteBounds = rectXYWH(0.0f, 0.0f, 1.0f, 1.0f);
		V4 terrainColor = makeWhite();

		for (s32 sY = visibleSectors.y;
			sY < visibleSectors.y + visibleSectors.h;
			sY++)
		{
			for (s32 sX = visibleSectors.x;
				sX < visibleSectors.x + visibleSectors.w;
				sX++)
			{
				Sector *sector = getSector(city, sX, sY);
				Rect2I relArea = cropRectangleToRelativeWithinSector(visibleTileBounds, sector);
				for (s32 relY=relArea.y;
					relY < relArea.y + relArea.h;
					relY++)
				{
					spriteBounds.y = (f32)(sector->bounds.y + relY);

					for (s32 relX=relArea.x;
						relX < relArea.x + relArea.w;
						relX++)
					{
						Terrain terrain = sector->terrain[relY][relX];

						if (terrain.type != terrainType)
						{
							terrainType = terrain.type;
							terrainSprites = get(&terrainDefs, terrainType)->sprites;
						}

						Sprite *sprite = getSprite(terrainSprites, terrain.spriteOffset);
						spriteBounds.x = (f32)(sector->bounds.x + relX);

						drawSprite(&renderer->worldBuffer, sprite, spriteBounds, -1000.0f, pixelArtShaderID, terrainColor);
					}
				}
			}
		}
	}

	// Draw zones
	{
		DEBUG_BLOCK_T("Draw zones", DCDT_GameUpdate);

		Rect2 spriteBounds = rectXYWH(0.0f, 0.0f, 1.0f, 1.0f);
		s32 zoneType = -1;
		V4 zoneColor = {};

		for (s32 sY = visibleSectors.y;
			sY < visibleSectors.y + visibleSectors.h;
			sY++)
		{
			for (s32 sX = visibleSectors.x;
				sX < visibleSectors.x + visibleSectors.w;
				sX++)
			{
				Sector *sector = getSector(city, sX, sY);

				Rect2I relArea = cropRectangleToRelativeWithinSector(visibleTileBounds, sector);
				for (s32 relY=relArea.y;
					relY < relArea.y + relArea.h;
					relY++)
				{
					spriteBounds.y = (f32)(sector->bounds.y + relY);

					for (s32 relX=relArea.x;
						relX < relArea.x + relArea.w;
						relX++)
					{
						ZoneType zone = sector->tileZone[relY][relX];
						if (zone != Zone_None)
						{
							if (zone != zoneType)
							{
								zoneType = zone;
								zoneColor = zoneDefs[zoneType].color;
							}

							spriteBounds.x = (f32)(sector->bounds.x + relX);

							drawRect(&renderer->worldBuffer, spriteBounds, -900.0f, rectangleShaderID, zoneColor);
						}
					}
				}
			}
		}
	}
	
	{
		DEBUG_BLOCK_T("Draw buildings", DCDT_GameUpdate);

		s32 typeID = -1;
		SpriteGroup *sprites = null;
		V4 drawColorNormal = makeWhite();
		V4 drawColorDemolish = color255(255,128,128,255);

		// TODO: If we knew the largest building size, we could speed this up a lot by only
		// iterating through Sectors that could possibly contain visible buildings, instead
		// of all the ones that are up or left from the camera.
		// Then again, buildings that visually extend up above their footprint would need
		// a similar thing for down/right directions probably.
		for (s32 sY = 0;
			sY < visibleSectors.y + visibleSectors.h;
			sY++)
		{
			for (s32 sX = 0;
				sX < visibleSectors.x + visibleSectors.w;
				sX++)
			{
				Sector *sector = getSector(city, sX, sY);

				for (auto it = iterate(&sector->buildings); !it.isDone; next(&it))
				{
					Building *building = get(it);
					if (rectsOverlap(building->footprint, visibleTileBounds))
					{
						if (typeID != building->typeID)
						{
							typeID = building->typeID;
							sprites = get(&buildingDefs, typeID)->sprites;
						}

						V4 drawColor = drawColorNormal;

						if (gameState->actionMode == ActionMode_Demolish
							&& gameState->worldDragState.isDragging
							&& rectsOverlap(building->footprint, demolitionRect))
						{
							// Draw building red to preview demolition
							drawColor = drawColorDemolish;
						}

						Sprite *sprite = getSprite(sprites, building->spriteOffset);
						drawSprite(&renderer->worldBuffer, sprite, rect2(building->footprint), 0, pixelArtShaderID, drawColor);
					}
				}
			}
		}
	}

	// Data layer rendering
	if (gameState->dataLayerToDraw)
	{
		DEBUG_BLOCK_T("Draw data layer", DCDT_GameUpdate);

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
						s32 pathGroup = pathGroupAt(city, x, y);
						if (pathGroup > 0)
						{
							color = genericDataLayerColors[pathGroup % genericDataLayerColorCount];
							tileHasData = true;
						}
					} break;

					case DataLayer_Power:
					{
						s32 powerGroup = powerGroupAt(city, x, y);
						if (powerGroup > 0)
						{
							color = genericDataLayerColors[powerGroup % genericDataLayerColorCount];
							tileHasData = true;
						}
					} break;
				}

				if (tileHasData)
				{
					drawRect(&renderer->worldBuffer, rectXYWH((f32)x, (f32)y, 1.0f, 1.0f), 9999.0f, rectangleShaderID, color);
				}
			}
		}
	}

	// Draw the things we prepared in overlayRenderItems earlier
	for (auto it = iterate(&gameState->overlayRenderItems);
		!it.isDone;
		next(&it))
	{
		drawRenderItem(&renderer->worldBuffer, get(it));
	}
	clear(&gameState->overlayRenderItems);

	{
		// DEBUG draw the sectors
		#if 0
		for (s32 y = 0; y < city->sectorsY; y++)
		{
			for (s32 x = 0; x < city->sectorsX; x++)
			{
				Sector *sector = city->sectors + (city->sectorsX * y) + x;

				V4 color;
				if ((x + y) % 2 == 1)
				{
					color = color255(  0,   0, 255, 63);
				}
				else
				{
					color = color255(255,   0,   0, 63);
				}

				drawRect(&renderer->worldBuffer, rect2(sector->bounds), 9999.0f, rectangleShaderID, color);
			}
		}
		#endif

		Sector *cursorSector = getSectorAtTilePos(city, mouseTilePos.x, mouseTilePos.y);
		if (cursorSector != null)
		{
			drawRect(&renderer->worldBuffer, rect2(cursorSector->bounds), 9999.0f, rectangleShaderID, color255(255, 255, 255, 63));
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
	updateAndRenderWindows(uiState);
	
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
			ASSERT(false, "Not implemented this AppStatus yet!");
		} break;
	}

	if (appState->appStatus != oldAppStatus)
	{
		clear(&uiState->openWindows);
	}

	drawUiMessage(uiState);
}