#pragma once

#include "game_mainmenu.cpp"
#include "credits.cpp"
#include "settings.cpp"

const f32 uiPadding = 4; // TODO: Move this somewhere sensible!

GameState *initialiseGameState()
{
	GameState *result;
	bootstrapArena(GameState, result, gameArena);
	randomSeed(&result->gameRandom, 12345);

	initCity(&result->gameArena, &result->gameRandom, &result->city, 128, 128, LocalString("City Name Here"), gameStartFunds);
	generateTerrain(&result->city);

	result->status = GameStatus_Playing;

	return result;
}

void inputMoveCamera(Camera *camera, InputState *inputState, V2 windowSize, s32 cityWidth, s32 cityHeight)
{ 
	// Zooming
	if (canZoom)
	{
		s32 zoomDelta = 0;

		if (canZoom && inputState->wheelY) {
			zoomDelta = inputState->wheelY;
		}

		if (keyJustPressed(inputState, SDLK_PAGEUP))
		{
			zoomDelta++;
		}
		else if (keyJustPressed(inputState, SDLK_PAGEDOWN))
		{
			zoomDelta--;
		}

		if (zoomDelta)
		{
			// round()ing the zoom so it doesn't gradually drift due to float imprecision
			camera->zoom = (f32) clamp((f32) round(10 * camera->zoom + zoomDelta) * 0.1f, 0.1f, 10.0f);
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
		V2 clickStartPos = inputState->clickStartPosition[mouseButtonIndex(SDL_BUTTON_MIDDLE)];
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

enum DragType
{
	DragLine,
	DragRect
};
Rect2I getDragArea(UIState *uiState, DragType dragType)
{
	Rect2I result = irectXYWH(0, 0, 0, 0);

	if (uiState->isDragging)
	{
		switch (dragType)
		{
			case DragRect:
			{
				result = irectCovering(uiState->mouseDragStartPos, uiState->mouseDragEndPos);
			} break;

			case DragLine:
			{
				// Axis-aligned straight line, in one dimension.
				// So, if you drag a diagonal line, it picks which direction has greater length and uses that.

				// determine orientation
				s32 xDiff = abs(uiState->mouseDragStartPos.x - uiState->mouseDragEndPos.x);
				s32 yDiff = abs(uiState->mouseDragStartPos.y - uiState->mouseDragEndPos.y);
				if (xDiff > yDiff)
				{
					// X
					result.w = xDiff + 1;
					result.h = 1;

					result.x = MIN(uiState->mouseDragStartPos.x, uiState->mouseDragEndPos.x);
					result.y = uiState->mouseDragStartPos.y;
				}
				else
				{
					// Y
					result.w = 1;
					result.h = yDiff + 1;

					result.x = uiState->mouseDragStartPos.x;
					result.y = MIN(uiState->mouseDragStartPos.y, uiState->mouseDragEndPos.y);
				}
			} break;

			INVALID_DEFAULT_CASE;
		}
	}

	return result;
}
enum DragResultOperation
{
	DragResult_Nothing,
	DragResult_DoAction,
	DragResult_ShowPreview,
};
struct DragResult
{
	DragResultOperation operation;
	Rect2I dragRect;
};
DragResult updateDragState(UIState *uiState, InputState *inputState, V2I mouseTilePos, bool mouseIsOverUI, DragType dragType, V2I minSize = {1,1})
{
	DragResult result = {};

	if (uiState->isDragging && mouseButtonJustReleased(inputState, SDL_BUTTON_LEFT))
	{
		if (!mouseIsOverUI)
		{
			result.operation = DragResult_DoAction;
			result.dragRect = getDragArea(uiState, dragType);
		}

		uiState->isDragging = false;
	}
	else
	{
		// Update the dragging state
		if (!mouseIsOverUI && mouseButtonJustPressed(inputState, SDL_BUTTON_LEFT))
		{
			uiState->isDragging = true;
			uiState->mouseDragStartPos = uiState->mouseDragEndPos = mouseTilePos;
		}

		if (mouseButtonPressed(inputState, SDL_BUTTON_LEFT) && uiState->isDragging)
		{
			uiState->mouseDragEndPos = mouseTilePos;
			result.dragRect = getDragArea(uiState, dragType);
		}
		else
		{
			result.dragRect = irectXYWH(mouseTilePos.x, mouseTilePos.y, 1, 1);
		}

		// Preview
		if (!mouseIsOverUI || uiState->isDragging)
		{
			result.operation = DragResult_ShowPreview;
		}
	}

	// minimum size
	if (result.dragRect.w < minSize.x) result.dragRect.w = minSize.x;
	if (result.dragRect.h < minSize.y) result.dragRect.h = minSize.y;

	return result;
}

void updateAndRenderGameUI(RenderBuffer *uiBuffer, AssetManager *assets, UIState *uiState, GameState *gameState,
	                       InputState *inputState)
{
	DEBUG_FUNCTION();
	
	f32 windowWidth = (f32) uiBuffer->camera.size.x;
	V2 centre = uiBuffer->camera.pos;
	UITheme *theme = &assets->theme;
	BitmapFont *font = getFont(assets, theme->labelStyle.font);
	City *city = &gameState->city;

	uiState->uiRects.count = 0;

	f32 left = uiPadding;
	f32 right = windowWidth - uiPadding;

	Rect2 uiRect = rectXYWH(0,0, windowWidth, 64);
	append(&uiState->uiRects, uiRect);
	drawRect(uiBuffer, uiRect, 0, theme->overlayColor);

	uiText(uiState, uiBuffer, font, city->name,
	       v2(left, uiPadding), ALIGN_LEFT, 1, theme->labelStyle.textColor);

	uiText(uiState, uiBuffer, font, myprintf("£{0} (-£{1}/month)", {formatInt(city->funds), formatInt(city->monthlyExpenditure)}), v2(centre.x, uiPadding), ALIGN_H_CENTRE, 1, theme->labelStyle.textColor);

	uiText(uiState, uiBuffer, font, myprintf("Pop: {0}, Jobs: {1}", {formatInt(city->totalResidents), formatInt(city->totalJobs)}), v2(centre.x, uiPadding+30), ALIGN_H_CENTRE, 1, theme->labelStyle.textColor);

	uiText(uiState, uiBuffer, font, myprintf("Power: {0}/{1}", {formatInt(city->powerLayer.combined.consumption), formatInt(city->powerLayer.combined.production)}),
	       v2(right, uiPadding), ALIGN_RIGHT, 1, theme->labelStyle.textColor);

	uiText(uiState, uiBuffer, font, myprintf("R: {0}\nC: {1}\nI: {2}", {formatInt(city->residentialDemand), formatInt(city->commercialDemand), formatInt(city->industrialDemand)}),
	       v2(windowWidth * 0.75f, uiPadding), ALIGN_RIGHT, 1, theme->labelStyle.textColor);


	// Build UI
	{
		Rect2 buttonRect = rectXYWH(uiPadding, 28 + uiPadding, 80, 24);

		// The "ZONE" menu
		if (uiMenuButton(uiState, uiBuffer, assets, inputState, LocalString("Zone..."), buttonRect, 1, UIMenu_Zone))
		{
			Rect2 menuButtonRect = buttonRect;
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			
			Rect2 menuRect = rectXYWH(menuButtonRect.x - uiPadding, menuButtonRect.y - uiPadding, menuButtonRect.w + (uiPadding * 2), uiPadding);

			for (s32 zoneIndex=0; zoneIndex < ZoneCount; zoneIndex++)
			{
				if (uiButton(uiState, uiBuffer, assets, inputState, zoneDefs[zoneIndex].name, menuButtonRect, 1,
						(uiState->actionMode == ActionMode_Zone) && (uiState->selectedZoneID == zoneIndex)))
				{
					uiState->openMenu = UIMenu_None;
					uiState->selectedZoneID = (ZoneType) zoneIndex;
					uiState->actionMode = ActionMode_Zone;
					setCursor(uiState, assets, Cursor_Build);
				}

				menuButtonRect.y += menuButtonRect.h + uiPadding;
				menuRect.h += menuButtonRect.h + uiPadding;
			}

			append(&uiState->uiRects, menuRect);
			drawRect(uiBuffer, menuRect, 0, theme->overlayColor);
		}
		buttonRect.x += buttonRect.w + uiPadding;

		// The "BUILD" menu
		if (uiMenuButton(uiState, uiBuffer, assets, inputState, LocalString("Build..."), buttonRect, 1, UIMenu_Build))
		{
			Rect2 menuButtonRect = buttonRect;
			menuButtonRect.y += menuButtonRect.h + uiPadding;

			Rect2 menuRect = rectXYWH(menuButtonRect.x - uiPadding, menuButtonRect.y - uiPadding, menuButtonRect.w + (uiPadding * 2), uiPadding);

			for (u32 i=0; i < buildingDefs.itemCount; i++)
			{
				BuildingDef *buildingDef = get(&buildingDefs, i);
				if (!buildingDef->buildMethod) continue;

				if (uiButton(uiState, uiBuffer, assets, inputState, buildingDef->name, menuButtonRect, 1,
						(uiState->actionMode == ActionMode_Build) && (uiState->selectedBuildingTypeID == i)))
				{
					uiState->openMenu = UIMenu_None;
					uiState->selectedBuildingTypeID = i;
					uiState->actionMode = ActionMode_Build;
					setCursor(uiState, assets, Cursor_Build);
				}

				menuButtonRect.y += menuButtonRect.h + uiPadding;
				menuRect.h += menuButtonRect.h + uiPadding;
			}

			append(&uiState->uiRects, menuRect);
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

		// The, um, "MENU" menu. Hmmm.
		buttonRect.x = windowWidth - (buttonRect.w + uiPadding);
		if (uiMenuButton(uiState, uiBuffer, assets, inputState, LocalString("Menu"), buttonRect, 1, UIMenu_System))
		{
			Rect2 menuButtonRect = buttonRect;
			menuButtonRect.y += menuButtonRect.h + uiPadding;

			Rect2 menuRect = rectXYWH(menuButtonRect.x - uiPadding, menuButtonRect.y - uiPadding, menuButtonRect.w + (uiPadding * 2), uiPadding);

			if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Save"), menuButtonRect, 1))
			{
				pushUiMessage(uiState, LocalString("Saving isn't implemented yet!"));
				uiState->openMenu = UIMenu_None;
			}
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			menuRect.h += menuButtonRect.h + uiPadding;

			if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Save"), menuButtonRect, 1))
			{
				pushUiMessage(uiState, LocalString("Saving isn't implemented yet!"));
				uiState->openMenu = UIMenu_None;
			}
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			menuRect.h += menuButtonRect.h + uiPadding;

			if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Save"), menuButtonRect, 1))
			{
				pushUiMessage(uiState, LocalString("Saving isn't implemented yet!"));
				uiState->openMenu = UIMenu_None;
			}
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			menuRect.h += menuButtonRect.h + uiPadding;

			if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Save"), menuButtonRect, 1))
			{
				pushUiMessage(uiState, LocalString("Saving isn't implemented yet!"));
				uiState->openMenu = UIMenu_None;
			}
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			menuRect.h += menuButtonRect.h + uiPadding;

			if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Save"), menuButtonRect, 1))
			{
				pushUiMessage(uiState, LocalString("Saving isn't implemented yet!"));
				uiState->openMenu = UIMenu_None;
			}
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			menuRect.h += menuButtonRect.h + uiPadding;

			if (uiButton(uiState, uiBuffer, assets, inputState, LocalString("Exit"), menuButtonRect, 1))
			{
				gameState->status = GameStatus_Quit;
				uiState->openMenu = UIMenu_None;
			}
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			menuRect.h += menuButtonRect.h + uiPadding;



			append(&uiState->uiRects, menuRect);
			drawRect(uiBuffer, menuRect, 0, theme->overlayColor);
		}
	}
}

void showCostTooltip(UIState *uiState, City *city, s32 buildCost)
{
	V4 color = canAfford(city, buildCost)
				? uiState->theme->tooltipStyle.textColorNormal
				: uiState->theme->tooltipStyle.textColorBad;

	String text = myprintf("£{0}", {formatInt(buildCost)});

	setTooltip(uiState, text, color);
}

void updateAndRenderGame(AppState *appState, InputState *inputState, Renderer *renderer, AssetManager *assets)
{
	DEBUG_FUNCTION();
	
	if (appState->gameState == null)
	{
		appState->gameState = initialiseGameState();
		renderer->worldBuffer.camera.pos = v2(appState->gameState->city.width/2, appState->gameState->city.height/2);
	}

	GameState *gameState = appState->gameState;
	City *city = &gameState->city;
	UIState *uiState = &globalAppState.uiState;

	if (assets->assetReloadHasJustHappened)
	{
		uiState->theme = &assets->theme;
		
		refreshZoneGrowableBuildingLists(&city->zoneLayer);

		assets->assetReloadHasJustHappened = false;
	}


	// Update the simulation... need a smarter way of doing this!
	calculateDemand(city);
	growSomeZoneBuildings(city);


	// CAMERA!
	Camera *worldCamera = &renderer->worldBuffer.camera;
	Camera *uiCamera    = &renderer->uiBuffer.camera;
	if (gameState->status == GameStatus_Playing)
	{
		inputMoveCamera(worldCamera, inputState, uiCamera->size, gameState->city.width, gameState->city.height);
	}

	V2I mouseTilePos = tilePosition(worldCamera->mousePos);
	bool mouseIsOverUI = inRects(uiState->uiRects.items, uiState->uiRects.count, uiCamera->mousePos);
	Rect2I demolitionRect = {0,0,-1,-1};

	switch (uiState->actionMode)
	{
		case ActionMode_Build:
		{
			BuildingDef *buildingDef = get(&buildingDefs, uiState->selectedBuildingTypeID);

			switch (buildingDef->buildMethod)
			{
				case BuildMethod_Paint:
				{
					if (!mouseIsOverUI)
					{
						Rect2I footprint = irectCentreDim(mouseTilePos, buildingDef->size);
						if (mouseButtonPressed(inputState, SDL_BUTTON_LEFT))
						{
							placeBuilding(uiState, city, uiState->selectedBuildingTypeID, footprint.x, footprint.y, true);
						}

						s32 buildCost = buildingDef->buildCost;
						showCostTooltip(uiState, city, buildCost);

						V4 ghostColor = color255(128,255,128,255);
						if (!canPlaceBuilding(uiState, &gameState->city, uiState->selectedBuildingTypeID, footprint.x, footprint.y))
						{
							ghostColor = color255(255,0,0,128);
						}
						drawTextureRegion(&renderer->worldBuffer, getTextureRegionID(assets, buildingDef->textureAssetType, 0),
										  rect2(footprint), depthFromY(mouseTilePos.y) + 100, ghostColor);
					}
				} break;

				case BuildMethod_Plop:
				{
					if (!mouseIsOverUI)
					{
						Rect2I footprint = irectCentreDim(mouseTilePos, buildingDef->size);
						if (mouseButtonJustReleased(inputState, SDL_BUTTON_LEFT))
						{
							placeBuilding(uiState, city, uiState->selectedBuildingTypeID, footprint.x, footprint.y, true);
						}

						s32 buildCost = buildingDef->buildCost;
						showCostTooltip(uiState, city, buildCost);

						V4 ghostColor = color255(128,255,128,255);
						if (!canPlaceBuilding(uiState, &gameState->city, uiState->selectedBuildingTypeID, footprint.x, footprint.y))
						{
							ghostColor = color255(255,0,0,128);
						}
						drawTextureRegion(&renderer->worldBuffer, getTextureRegionID(assets, buildingDef->textureAssetType, 0),
										  rect2(footprint), depthFromY(mouseTilePos.y) + 100, ghostColor);
					}
				} break;

				case BuildMethod_DragLine:
				{
					DragResult dragResult = updateDragState(uiState, inputState, mouseTilePos, mouseIsOverUI, DragLine);
					s32 buildCost = calculateBuildCost(city, uiState->selectedBuildingTypeID, dragResult.dragRect);

					switch (dragResult.operation)
					{
						case DragResult_DoAction:
						{
							if (canAfford(city, buildCost))
							{
								placeBuildingRect(uiState, city, uiState->selectedBuildingTypeID, dragResult.dragRect);
							}
						} break;

						case DragResult_ShowPreview:
						{
							if (!mouseIsOverUI) showCostTooltip(uiState, city, buildCost);

							if (canAfford(city, buildCost))
							{
								for (s32 y=0; y + buildingDef->height <= dragResult.dragRect.h; y += buildingDef->height)
								{
									for (s32 x=0; x + buildingDef->width <= dragResult.dragRect.w; x += buildingDef->width)
									{
										V4 ghostColor = color255(128,255,128,255);
										if (!canPlaceBuilding(uiState, city, uiState->selectedBuildingTypeID, dragResult.dragRect.x + x, dragResult.dragRect.y + y))
										{
											ghostColor = color255(255,0,0,128);
										}
										drawTextureRegion(&renderer->worldBuffer, getTextureRegionID(assets, buildingDef->textureAssetType, 0),
										                  rectXYWHi(dragResult.dragRect.x + x, dragResult.dragRect.y + y, buildingDef->width, buildingDef->height), depthFromY(dragResult.dragRect.y + y) + 100, ghostColor);
									}
								}
							}
							else
							{
								drawRect(&renderer->worldBuffer, rect2(dragResult.dragRect), 0, color255(255, 64, 64, 128));
							}
						} break;
					}
				} break;

				case BuildMethod_DragRect:
				{
					DragResult dragResult = updateDragState(uiState, inputState, mouseTilePos, mouseIsOverUI, DragRect, buildingDef->size);
					s32 buildCost = calculateBuildCost(city, uiState->selectedBuildingTypeID, dragResult.dragRect);

					switch (dragResult.operation)
					{
						case DragResult_DoAction:
						{
							if (canAfford(city, buildCost))
							{
								placeBuildingRect(uiState, city, uiState->selectedBuildingTypeID, dragResult.dragRect);
							}
						} break;

						case DragResult_ShowPreview:
						{
							if (!mouseIsOverUI) showCostTooltip(uiState, city, buildCost);

							if (canAfford(city, buildCost))
							{
								for (s32 y=0; y + buildingDef->height <= dragResult.dragRect.h; y += buildingDef->height)
								{
									for (s32 x=0; x + buildingDef->width <= dragResult.dragRect.w; x += buildingDef->width)
									{
										V4 ghostColor = color255(128,255,128,255);
										if (!canPlaceBuilding(uiState, city, uiState->selectedBuildingTypeID, dragResult.dragRect.x + x, dragResult.dragRect.y + y))
										{
											ghostColor = color255(255,0,0,128);
										}
										drawTextureRegion(&renderer->worldBuffer, getTextureRegionID(assets, buildingDef->textureAssetType, 0),
										                  rectXYWHi(dragResult.dragRect.x + x, dragResult.dragRect.y + y, buildingDef->width, buildingDef->height), depthFromY(dragResult.dragRect.y + y) + 100, ghostColor);
									}
								}
							}
							else
							{
								drawRect(&renderer->worldBuffer, rect2(dragResult.dragRect), 0, color255(255, 64, 64, 128));
							}
						} break;
					}
				} break;
			}
		} break;

		case ActionMode_Zone:
		{
			DragResult dragResult = updateDragState(uiState, inputState, mouseTilePos, mouseIsOverUI, DragRect);
			s32 zoneCost = calculateZoneCost(city, uiState->selectedZoneID, dragResult.dragRect);

			switch (dragResult.operation)
			{
				case DragResult_DoAction:
				{
					if (canAfford(city, zoneCost))
					{
						placeZone(uiState, city, uiState->selectedZoneID, dragResult.dragRect);
					}
				} break;

				case DragResult_ShowPreview:
				{
					if (!mouseIsOverUI) showCostTooltip(uiState, city, zoneCost);

					if (canAfford(city, zoneCost))
					{
						for (s32 y = dragResult.dragRect.y; y < dragResult.dragRect.y+dragResult.dragRect.h; y++)
						{
							for (s32 x = dragResult.dragRect.x; x < dragResult.dragRect.x+dragResult.dragRect.w; x++)
							{
								if (canZoneTile(city, uiState->selectedZoneID, x, y))
								{
									drawRect(&renderer->worldBuffer, rectXYWHi(x, y, 1, 1), 0, zoneDefs[uiState->selectedZoneID].color);
								}
							}
						}
					}
					else
					{
						drawRect(&renderer->worldBuffer, rect2(dragResult.dragRect), 0, color255(255, 64, 64, 128));
					}
				} break;
			}
		} break;

		case ActionMode_Demolish:
		{
			DragResult dragResult = updateDragState(uiState, inputState, mouseTilePos, mouseIsOverUI, DragRect);
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
				} break;

				case DragResult_ShowPreview:
				{
					if (!mouseIsOverUI) showCostTooltip(uiState, city, demolishCost);

					if (canAfford(city, demolishCost))
					{
						// Demolition outline
						drawRect(&renderer->worldBuffer, rect2(dragResult.dragRect), 0, color255(128, 0, 0, 128));
					}
					else
					{
						drawRect(&renderer->worldBuffer, rect2(dragResult.dragRect), 0, color255(255, 64, 64, 128));
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
					int tileI = tileIndex(city, mouseTilePos.x, mouseTilePos.y);

					String buildingName;
					int buildingID = city->tileBuildings[tileI];
					if (buildingID != 0)
					{
						buildingName = get(&buildingDefs, city->buildings[buildingID].typeID)->name;
					}
					else
					{
						buildingName = stringFromChars("None");
					}

					String terrainName = get(&terrainDefs, city->terrain[tileI].type)->name;

					String zoneName;
					ZoneType zone = city->zoneLayer.tiles[tileI];
					if (zone)
					{
						zoneName = zoneDefs[zone].name;
					}
					else
					{
						zoneName = stringFromChars("None");
					}

					logInfo("Clicked at ({0},{1}), terrain: {2}, building: {3}, zone: {4}", 
						{formatInt(mouseTilePos.x), formatInt(mouseTilePos.y), terrainName, buildingName, zoneName});
				}
			}
		} break;
	}

	if (uiState->isDragging && mouseIsOverUI && mouseButtonJustReleased(inputState, SDL_BUTTON_LEFT))
	{
		// Not sure if this is the best idea, but it's the best I can come up with.
		uiState->isDragging = false;
	}

	if (mouseButtonJustPressed(inputState, SDL_BUTTON_RIGHT))
	{
		// Unselect current thing
		uiState->actionMode = ActionMode_None;
		setCursor(uiState, assets, Cursor_Main);
	}

	// RENDERING
	// Pre-calculate the tile area that's visible to the player.
	// We err on the side of drawing too much, rather than risking having holes in the world.
	Rect2I visibleTileBounds = irectCentreWH(
		v2i((s32)worldCamera->pos.x - 1, (s32)worldCamera->pos.y - 1),
		(s32) (worldCamera->size.x / worldCamera->zoom + 5),
		(s32) (worldCamera->size.y / worldCamera->zoom + 5)
	);
	if (visibleTileBounds.x < 0)
	{
		visibleTileBounds.w += visibleTileBounds.x;
		visibleTileBounds.x = 0;
	}
	if (visibleTileBounds.y < 0)
	{
		visibleTileBounds.h += visibleTileBounds.y;
		visibleTileBounds.y = 0;
	}
	if (visibleTileBounds.w > gameState->city.width)
	{
		visibleTileBounds.w = gameState->city.width;
	}
	if (visibleTileBounds.h > gameState->city.height)
	{
		visibleTileBounds.h = gameState->city.height;
	}

	// Draw terrain
	for (s32 y = visibleTileBounds.y;
		y < visibleTileBounds.y + visibleTileBounds.h;
		y++)
	{
		for (s32 x = visibleTileBounds.x;
			x < visibleTileBounds.x + visibleTileBounds.w;
			x++)
		{
			Terrain t = terrainAt(city,x,y);
			if (t.type != 0)
			{
				TerrainDef *tDef = get(&terrainDefs, t.type);

				u32 textureRegionID = getTextureRegionID(assets, tDef->textureAssetType, t.textureRegionOffset);

				drawTextureRegion(&renderer->worldBuffer, textureRegionID, rectXYWH((f32)x, (f32)y, 1.0f, 1.0f), -1000.0f);
			}
		}
	}

	// Draw zones
	for (s32 y = visibleTileBounds.y;
		y < visibleTileBounds.y + visibleTileBounds.h;
		y++)
	{
		for (s32 x = visibleTileBounds.x;
			x < visibleTileBounds.x + visibleTileBounds.w;
			x++)
		{
			ZoneType zoneType = getZoneAt(city, x, y);
			if (zoneType != Zone_None)
			{
				drawRect(&renderer->worldBuffer, rectXYWH((f32)x, (f32)y, 1.0f, 1.0f), depthFromY(y) - 10.0f, zoneDefs[zoneType].color);
			}
		}
	}
	
	for (u32 i=1; i<gameState->city.buildings.count; i++)
	{
		Building building = gameState->city.buildings[i];

		if (rectsOverlap(building.footprint, visibleTileBounds))
		{
			BuildingDef *def = get(&buildingDefs, building.typeID);

			V4 drawColor = makeWhite();

			if (uiState->actionMode == ActionMode_Demolish
				&& uiState->isDragging
				&& rectsOverlap(building.footprint, demolitionRect)) {
				// Draw building red to preview demolition
				drawColor = color255(255,128,128,255);
			}

			V2 drawPos = centre(building.footprint);
			drawTextureRegion(&renderer->worldBuffer, getTextureRegionID(assets, def->textureAssetType, building.textureRegionOffset),
							  rect2(building.footprint), depthFromY(drawPos.y), drawColor);
		}
	}

	// Data layer rendering
	if (gameState->dataLayerToDraw)
	{
		for (s32 y = visibleTileBounds.y;
			y < visibleTileBounds.y + visibleTileBounds.h;
			y++)
		{
			for (s32 x = visibleTileBounds.x;
				x < visibleTileBounds.x + visibleTileBounds.w;
				x++)
			{
				V4 color = {};

				switch (gameState->dataLayerToDraw)
				{
					case DataLayer_Paths:
					{
						s32 pathGroup = pathGroupAt(city, x, y);
						if (pathGroup > 0)
						{
							switch (pathGroup)
							{
								case 1:  color = color255(  0,   0, 255, 63); break;
								case 2:  color = color255(  0, 255,   0, 63); break;
								case 3:  color = color255(255,   0,   0, 63); break;
								case 4:  color = color255(  0, 255, 255, 63); break;
								case 5:  color = color255(255, 255,   0, 63); break;
								case 6:  color = color255(255,   0, 255, 63); break;

								default: color = color255(255, 255, 255, 63); break;
							}
						}
					} break;

					case DataLayer_Power:
					{
						s32 powerGroup = powerGroupAt(city, x, y);
						if (powerGroup > 0)
						{
							switch (powerGroup)
							{
								case 1:  color = color255(  0,   0, 255, 63); break;
								case 2:  color = color255(  0, 255,   0, 63); break;
								case 3:  color = color255(255,   0,   0, 63); break;
								case 4:  color = color255(  0, 255, 255, 63); break;
								case 5:  color = color255(255, 255,   0, 63); break;
								case 6:  color = color255(255,   0, 255, 63); break;

								default: color = color255(255, 255, 255, 63); break;
							}
						}
					} break;
				}

				if (color.a > 0.01f)
				{
					drawRect(&renderer->worldBuffer, rectXYWH((f32)x, (f32)y, 1.0f, 1.0f), 9999.0f, color);
				}
			}
		}
	}

	// Draw the UI!
	updateAndRenderGameUI(&renderer->uiBuffer, assets, uiState, gameState, inputState);

	if (gameState->status == GameStatus_Quit)
	{
		freeMemoryArena(&gameState->gameArena);
		appState->gameState = null;
		appState->appStatus = AppStatus_MainMenu;
	}

	if (appState->appStatus == AppStatus_Game)
	{
		drawTooltip(uiState, &renderer->uiBuffer, assets);
		drawUiMessage(uiState, &renderer->uiBuffer, assets);
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