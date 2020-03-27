#pragma once

void initDataViewUI()
{
	dataViewUI[DataView_None].title = "data_view_none"_s;

	Array<V4> *desirabilityPalette     = getPalette("desirability"_s);
	Array<V4> *landValuePalette        = getPalette("land_value"_s);
	Array<V4> *pollutionPalette        = getPalette("pollution"_s);
	Array<V4> *powerPalette            = getPalette("power"_s);
	Array<V4> *riskPalette             = getPalette("risk"_s);
	Array<V4> *serviceBuildingsPalette = getPalette("service_buildings"_s);
	Array<V4> *serviceCoveragePalette  = getPalette("service_coverage"_s);

	dataViewUI[DataView_Desirability_Residential].title = "data_view_desirability_residential"_s;
	dataViewUI[DataView_Desirability_Residential].hasGradient = true;
	dataViewUI[DataView_Desirability_Residential].gradientColorMin = asOpaque(*first(desirabilityPalette));
	dataViewUI[DataView_Desirability_Residential].gradientColorMax = asOpaque(*last(desirabilityPalette));

	dataViewUI[DataView_Desirability_Commercial].title = "data_view_desirability_commercial"_s;
	dataViewUI[DataView_Desirability_Commercial].hasGradient = true;
	dataViewUI[DataView_Desirability_Commercial].gradientColorMin = asOpaque(*first(desirabilityPalette));
	dataViewUI[DataView_Desirability_Commercial].gradientColorMax = asOpaque(*last(desirabilityPalette));

	dataViewUI[DataView_Desirability_Industrial].title = "data_view_desirability_industrial"_s;
	dataViewUI[DataView_Desirability_Industrial].hasGradient = true;
	dataViewUI[DataView_Desirability_Industrial].gradientColorMin = asOpaque(*first(desirabilityPalette));
	dataViewUI[DataView_Desirability_Industrial].gradientColorMax = asOpaque(*last(desirabilityPalette));

	dataViewUI[DataView_Crime].title = "data_view_crime"_s;
	dataViewUI[DataView_Crime].hasGradient = true;
	dataViewUI[DataView_Crime].gradientColorMin = asOpaque(*first(serviceCoveragePalette));
	dataViewUI[DataView_Crime].gradientColorMax = asOpaque(*last(serviceCoveragePalette));
	dataViewUI[DataView_Crime].fixedColorCount = 2;
	dataViewUI[DataView_Crime].fixedColors[0] = asOpaque((*serviceBuildingsPalette)[0]);
	dataViewUI[DataView_Crime].fixedColors[1] = asOpaque((*serviceBuildingsPalette)[1]);
	dataViewUI[DataView_Crime].fixedColorNames[0] = "data_view_buildings_powered"_s;
	dataViewUI[DataView_Crime].fixedColorNames[1] = "data_view_buildings_unpowered"_s;

	dataViewUI[DataView_Fire].title = "data_view_fire"_s;
	dataViewUI[DataView_Fire].hasGradient = true;
	dataViewUI[DataView_Fire].gradientColorMin = asOpaque(*first(riskPalette));
	dataViewUI[DataView_Fire].gradientColorMax = asOpaque(*last(riskPalette));
	dataViewUI[DataView_Fire].fixedColorCount = 2;
	dataViewUI[DataView_Fire].fixedColors[0] = asOpaque((*serviceBuildingsPalette)[0]);
	dataViewUI[DataView_Fire].fixedColors[1] = asOpaque((*serviceBuildingsPalette)[1]);
	dataViewUI[DataView_Fire].fixedColorNames[0] = "data_view_buildings_powered"_s;
	dataViewUI[DataView_Fire].fixedColorNames[1] = "data_view_buildings_unpowered"_s;

	dataViewUI[DataView_Health].title = "data_view_health"_s;
	dataViewUI[DataView_Health].hasGradient = true;
	dataViewUI[DataView_Health].gradientColorMin = asOpaque(*first(serviceCoveragePalette));
	dataViewUI[DataView_Health].gradientColorMax = asOpaque(*last(serviceCoveragePalette));
	dataViewUI[DataView_Health].fixedColorCount = 2;
	dataViewUI[DataView_Health].fixedColors[0] = asOpaque((*serviceBuildingsPalette)[0]);
	dataViewUI[DataView_Health].fixedColors[1] = asOpaque((*serviceBuildingsPalette)[1]);
	dataViewUI[DataView_Health].fixedColorNames[0] = "data_view_buildings_powered"_s;
	dataViewUI[DataView_Health].fixedColorNames[1] = "data_view_buildings_unpowered"_s;

	dataViewUI[DataView_LandValue].title = "data_view_landvalue"_s;
	dataViewUI[DataView_LandValue].hasGradient = true;
	dataViewUI[DataView_LandValue].gradientColorMin = asOpaque(*first(landValuePalette));
	dataViewUI[DataView_LandValue].gradientColorMax = asOpaque(*last(landValuePalette));

	dataViewUI[DataView_Pollution].title = "data_view_pollution"_s;
	dataViewUI[DataView_Pollution].hasGradient = true;
	dataViewUI[DataView_Pollution].gradientColorMin = asOpaque(*first(pollutionPalette));
	dataViewUI[DataView_Pollution].gradientColorMax = asOpaque(*last(pollutionPalette));

	dataViewUI[DataView_Power].title = "data_view_power"_s;
	dataViewUI[DataView_Power].fixedColorCount = 3;
	dataViewUI[DataView_Power].fixedColors[0] = asOpaque((*powerPalette)[0]);
	dataViewUI[DataView_Power].fixedColors[1] = asOpaque((*powerPalette)[1]);
	dataViewUI[DataView_Power].fixedColors[2] = asOpaque((*powerPalette)[2]);
	dataViewUI[DataView_Power].fixedColorNames[0] = "data_view_power_powered"_s;
	dataViewUI[DataView_Power].fixedColorNames[1] = "data_view_power_brownout"_s;
	dataViewUI[DataView_Power].fixedColorNames[2] = "data_view_power_blackout"_s;
}

GameState *beginNewGame()
{
	GameState *result;
	bootstrapArena(GameState, result, gameArena);
	initRandom(&result->gameRandom, Random_MT, 12345);

	result->status = GameStatus_Playing;

	result->actionMode = ActionMode_None;

	initFlags(&result->inspectTileDebugFlags, InspectTileDebugFlagCount);

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
	// TODO: Wrap with !isInputCaptured()
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
	context->window->title = myprintf(getText("title_inspect"_s), {formatInt(tilePos.x), formatInt(tilePos.y)});

	// CitySector
	CitySector *sector = getSectorAtTilePos(&city->sectors, tilePos.x, tilePos.y);
	window_label(context, myprintf("CitySector: x={0} y={1} w={2} h={3}"_s, {formatInt(sector->bounds.x), formatInt(sector->bounds.y), formatInt(sector->bounds.w), formatInt(sector->bounds.h)}));

	// Terrain
	TerrainDef *terrain = getTerrainAt(city, tilePos.x, tilePos.y);
	window_label(context, myprintf("Terrain: {0}"_s, {getText(terrain->textAssetName)}));

	// Zone
	ZoneType zone = getZoneAt(city, tilePos.x, tilePos.y);
	window_label(context, myprintf("Zone: {0}"_s, {zone ? getText(getZoneDef(zone).textAssetName) : "None"_s}));

	// Building
	Building *building = getBuildingAt(city, tilePos.x, tilePos.y);
	if (building != null)
	{
		s32 buildingIndex = getTileValue(city, city->tileBuildingIndex, tilePos.x, tilePos.y);
		BuildingDef *def = getBuildingDef(building->typeID);
		window_label(context, myprintf("Building: {0} (ID {1}, array index {2})"_s, {getText(def->textAssetName), formatInt(building->id), formatInt(buildingIndex)}));
		window_label(context, myprintf("Variant: {0}"_s, {formatInt(building->variantIndex)}));
		window_label(context, myprintf("- Residents: {0} / {1}"_s, {formatInt(building->currentResidents), formatInt(def->residents)}));
		window_label(context, myprintf("- Jobs: {0} / {1}"_s, {formatInt(building->currentJobs), formatInt(def->jobs)}));
		window_label(context, myprintf("- Power: {0}"_s, {formatInt(def->power)}));

		// Problems
		for (s32 problemIndex = 0; problemIndex < BuildingProblemCount; problemIndex++)
		{
			if (hasProblem(building, (BuildingProblem) problemIndex))
			{
				window_label(context, myprintf("- PROBLEM: {0}"_s, {getText(buildingProblemNames[problemIndex])}));
			}
		}
	}
	else
	{
		window_label(context, "Building: None"_s);
	}

	// Land value
	window_label(context, myprintf("Land value: {0}%"_s, {formatFloat(getLandValuePercentAt(city, tilePos.x, tilePos.y) * 100.0f, 0)}));

	// Debug info
	if (!isEmpty(&gameState->inspectTileDebugFlags))
	{
		if (gameState->inspectTileDebugFlags & DebugInspect_Fire)
		{
			debugInspectFire(context, city, tilePos.x, tilePos.y);
		}
		if (gameState->inspectTileDebugFlags & DebugInspect_Power)
		{
			debugInspectPower(context, city, tilePos.x, tilePos.y);
		}
		if (gameState->inspectTileDebugFlags & DebugInspect_Transport)
		{
			debugInspectTransport(context, city, tilePos.x, tilePos.y);
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

void pauseMenuWindowProc(WindowContext *context, void * /*userData*/)
{
	DEBUG_FUNCTION();

	UIButtonStyle *buttonStyle = findButtonStyle(&assets->theme, context->windowStyle->buttonStyleName);
	s32 availableButtonTextWidth = context->contentArea.w - (2 * buttonStyle->padding);

	String resume = getText("button_resume"_s);
	String save   = getText("button_save"_s);
	String load   = getText("button_load"_s);
	String about  = getText("button_about"_s);
	String exit   = getText("button_exit"_s);
	s32 maxButtonTextWidth = availableButtonTextWidth;//calculateMaxTextWidth(buttonFont, {resume, save, load, about, exit}, availableButtonTextWidth);

	if (window_button(context, resume, maxButtonTextWidth))
	{
		context->closeRequested = true;
	}

	if (window_button(context, save, maxButtonTextWidth))
	{
		showSaveGameWindow(context->uiState);
	}

	if (window_button(context, load, maxButtonTextWidth))
	{
		showLoadGameWindow(context->uiState);
	}

	if (window_button(context, about, maxButtonTextWidth))
	{
		showAboutWindow(context->uiState);
	}

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
	s32 windowWidth  = round_s32(renderer->uiCamera.size.x);
	s32 windowHeight = round_s32(renderer->uiCamera.size.y);
	V2I centre = v2i(renderer->uiCamera.pos);
	UITheme *theme = &assets->theme;
	UILabelStyle *labelStyle = findLabelStyle(theme, "title"_s);
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

	uiText(&renderer->uiBuffer, font, myprintf("£{0} (-£{1}/month)"_s, {formatInt(city->funds), formatInt(city->monthlyExpenditure)}), v2i(centre.x, uiPadding), ALIGN_H_CENTRE, labelStyle->textColor);

	uiText(&renderer->uiBuffer, font, myprintf("Pop: {0}, Jobs: {1}"_s, {formatInt(getTotalResidents(city)), formatInt(getTotalJobs(city))}), v2i(centre.x, uiPadding+30), ALIGN_H_CENTRE, labelStyle->textColor);

	uiText(&renderer->uiBuffer, font, myprintf("Power: {0}/{1}"_s, {formatInt(city->powerLayer.cachedCombinedConsumption), formatInt(city->powerLayer.cachedCombinedProduction)}),
	       v2i(right, uiPadding), ALIGN_RIGHT, labelStyle->textColor);

	uiText(&renderer->uiBuffer, font, myprintf("R: {0}\nC: {1}\nI: {2}"_s, {formatInt(city->zoneLayer.demand[Zone_Residential]), formatInt(city->zoneLayer.demand[Zone_Commercial]), formatInt(city->zoneLayer.demand[Zone_Industrial])}),
	       v2i(round_s32(windowWidth * 0.75f), uiPadding), ALIGN_RIGHT, labelStyle->textColor);

	UIButtonStyle *buttonStyle = findButtonStyle(&assets->theme, "default"_s);
	UIPopupMenuStyle *popupMenuStyle = findPopupMenuStyle(&assets->theme, "default"_s);
	UIButtonStyle *popupButtonStyle = findButtonStyle(&assets->theme, popupMenuStyle->buttonStyleName);
	// Build UI
	{
		// The "ZONE" menu
		String zoneButtonText = getText("button_zone"_s);
		V2I buttonSize = calculateButtonSize(zoneButtonText, buttonStyle);
		Rect2I buttonRect = irectXYWH(uiPadding, buttonSize.y + uiPadding, buttonSize.x, buttonSize.y);
		if (uiMenuButton(uiState, zoneButtonText, buttonRect, Menu_Zone, buttonStyle))
		{
			//
			// UGH, all of this UI code is so hacky!
			// As I'm trying to use it, more and more of it is unraveling. I want to find the widest button width
			// beforehand, but that means having to get which font will be used, and that's not exposed nicely!
			// So I have to hackily write this buttonFont definition in the same way or it'll be wrong.
			// BitmapFont *buttonFont = getFont(findButtonStyle(&assets->theme, "default"_s)->fontName);
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

			s32 buttonMaxWidth = 0;
			for (s32 zoneIndex=0; zoneIndex < ZoneCount; zoneIndex++)
			{
				buttonMaxWidth = max(buttonMaxWidth, calculateButtonSize(getText(getZoneDef(zoneIndex).textAssetName), popupButtonStyle).x);
			}

			s32 popupMenuWidth = buttonMaxWidth + (popupMenuStyle->margin * 2);
			s32 popupMenuMaxHeight = windowHeight - (buttonRect.y + buttonRect.h);

			PopupMenu menu = beginPopupMenu(uiState, buttonRect.x - popupMenuStyle->margin, buttonRect.y + buttonRect.h, popupMenuWidth, popupMenuMaxHeight, popupMenuStyle);

			for (s32 zoneIndex=0; zoneIndex < ZoneCount; zoneIndex++)
			{
				if (popupMenuButton(uiState, &menu, getText(getZoneDef(zoneIndex).textAssetName), popupButtonStyle,
						(gameState->actionMode == ActionMode_Zone) && (gameState->selectedZoneID == zoneIndex)))
				{
					uiCloseMenus(uiState);
					gameState->selectedZoneID = (ZoneType) zoneIndex;
					gameState->actionMode = ActionMode_Zone;
					setCursor("build"_s);
				}
			}

			endPopupMenu(uiState, &menu);
		}
		buttonRect.x += buttonRect.w + uiPadding;

		// The "BUILD" menu
		String buildButtonText = getText("button_build"_s);
		buttonRect.size = calculateButtonSize(buildButtonText, buttonStyle);
		if (uiMenuButton(uiState, buildButtonText, buttonRect, Menu_Build, buttonStyle))
		{
			ChunkedArray<BuildingDef *> *constructibleBuildings = getConstructibleBuildings();

			s32 buttonMaxWidth = 0;
			for (auto it = iterate(constructibleBuildings);
				hasNext(&it);
				next(&it))
			{
				BuildingDef *buildingDef = getValue(&it);
				buttonMaxWidth = max(buttonMaxWidth, calculateButtonSize(getText(buildingDef->textAssetName), popupButtonStyle).x);
			}

			s32 popupMenuWidth = buttonMaxWidth + (popupMenuStyle->margin * 2);
			s32 popupMenuMaxHeight = windowHeight - (buttonRect.y + buttonRect.h);

			PopupMenu menu = beginPopupMenu(uiState, buttonRect.x - popupMenuStyle->margin, buttonRect.y + buttonRect.h, popupMenuWidth, popupMenuMaxHeight, popupMenuStyle);

			for (auto it = iterate(constructibleBuildings);
				hasNext(&it);
				next(&it))
			{
				BuildingDef *buildingDef = getValue(&it);

				if (popupMenuButton(uiState, &menu, getText(buildingDef->textAssetName), popupButtonStyle,
						(gameState->actionMode == ActionMode_Build) && (gameState->selectedBuildingTypeID == buildingDef->typeID)))
				{
					uiCloseMenus(uiState);
					gameState->selectedBuildingTypeID = buildingDef->typeID;
					gameState->actionMode = ActionMode_Build;
					setCursor("build"_s);
				}
			}

			endPopupMenu(uiState, &menu);
		}
		buttonRect.x += buttonRect.w + uiPadding;

		String demolishButtonText = getText("button_demolish"_s);
		buttonRect.size = calculateButtonSize(demolishButtonText, buttonStyle);
		if (uiButton(uiState, demolishButtonText, buttonRect, buttonStyle,
					(gameState->actionMode == ActionMode_Demolish),
					SDLK_x, "(X)"_s))
		{
			gameState->actionMode = ActionMode_Demolish;
			setCursor("demolish"_s);
		}
		buttonRect.x += buttonRect.w + uiPadding;

		// The, um, "MENU" menu. Hmmm.
		String menuButtonText = getText("button_menu"_s);
		buttonRect.size = calculateButtonSize(menuButtonText, buttonStyle);
		buttonRect.x = windowWidth - (buttonRect.w + uiPadding);
		if (uiButton(uiState, menuButtonText, buttonRect, buttonStyle))
		{
			showWindow(uiState, getText("title_menu"_s), 200, 200, v2i(0,0), "default"_s, WinFlag_Unique|WinFlag_Modal|WinFlag_AutomaticHeight, pauseMenuWindowProc, null);
		}
	}

	// Data-views menu
	RenderItem_DrawSingleRect *dataViewUIBackground = appendDrawRectPlaceholder(&renderer->uiBuffer, renderer->shaderIds.untextured);
	String dataViewButtonText = getText("button_data_views"_s);
	V2I dataViewButtonSize = calculateButtonSize(dataViewButtonText, buttonStyle);
	Rect2I dataViewButtonBounds = irectXYWH(uiPadding, windowHeight - uiPadding - dataViewButtonSize.y, dataViewButtonSize.x, dataViewButtonSize.y);
	if (uiMenuButton(uiState, dataViewButtonText, dataViewButtonBounds, Menu_DataViews, buttonStyle))
	{
		s32 buttonMaxWidth = 0;
		s32 menuContentHeight = (DataViewCount - 1) * popupMenuStyle->contentPadding;
		for (DataView dataViewID = DataView_None; dataViewID < DataViewCount; dataViewID = (DataView)(dataViewID + 1))
		{
			String buttonText = getText(dataViewUI[dataViewID].title);
			V2I buttonSize = calculateButtonSize(buttonText, popupButtonStyle);
			buttonMaxWidth = max(buttonMaxWidth, buttonSize.x);
			menuContentHeight += buttonSize.y;
		}
		s32 popupMenuWidth = buttonMaxWidth + (popupMenuStyle->margin * 2);
		s32 popupMenuMaxHeight = windowHeight - 128;

		s32 menuY = dataViewButtonBounds.y - min(popupMenuMaxHeight, (menuContentHeight + 2*popupMenuStyle->margin));

		PopupMenu menu = beginPopupMenu(uiState, dataViewButtonBounds.x - popupMenuStyle->margin, menuY, popupMenuWidth, popupMenuMaxHeight, popupMenuStyle);

		for (DataView dataViewID = DataView_None; dataViewID < DataViewCount; dataViewID = (DataView)(dataViewID + 1))
		{
			String buttonText = getText(dataViewUI[dataViewID].title);

			if (popupMenuButton(uiState, &menu, buttonText, popupButtonStyle, (gameState->dataLayerToDraw == dataViewID)))
			{
				uiCloseMenus(uiState);
				gameState->dataLayerToDraw = dataViewID;
			}
		}

		endPopupMenu(uiState, &menu);
	}

	// Data-view info
	if (uiState->openMenu != Menu_DataViews && gameState->dataLayerToDraw != DataView_None)
	{
		s32 dataViewUIWidth = dataViewButtonSize.x;

		V2I uiPos = dataViewButtonBounds.pos;
		uiPos.y -= uiPadding;

		DataViewUI *dataView = &dataViewUI[gameState->dataLayerToDraw];

		s32 paletteBlockSize = font->lineHeight;

		// We're working from bottom to top, so we start at the end.

		// First, the named colors
		for (s32 fixedColorIndex = dataView->fixedColorCount-1; fixedColorIndex >= 0; fixedColorIndex--)
		{
			// Block is to the left, so we offset the label by that width and padding
			Rect2I colorLabelBounds = uiText(&renderer->uiBuffer, font, getText(dataView->fixedColorNames[fixedColorIndex]), uiPos + v2i(paletteBlockSize + uiPadding, 0), ALIGN_LEFT | ALIGN_BOTTOM, labelStyle->textColor);

			Rect2I paletteBlockBounds = irectXYWH(uiPos.x, uiPos.y - paletteBlockSize, paletteBlockSize, paletteBlockSize);
			drawSingleRect(&renderer->uiBuffer, paletteBlockBounds, renderer->shaderIds.untextured, dataView->fixedColors[fixedColorIndex]);

			uiPos.y -= colorLabelBounds.h + uiPadding;

			// Overall width of this line is both block & label
			dataViewUIWidth = max(dataViewUIWidth, colorLabelBounds.w + uiPadding + paletteBlockSize);
		}

		// Above that, the gradient
		if (dataView->hasGradient)
		{
			// Arbitrarily going to make the height 4x the width
			Rect2I gradientBounds = irectXYWH(uiPos.x, uiPos.y - (paletteBlockSize * 4), paletteBlockSize, paletteBlockSize * 4);

			drawSingleRect(&renderer->uiBuffer, rect2(gradientBounds), renderer->shaderIds.untextured, dataView->gradientColorMax, dataView->gradientColorMax, dataView->gradientColorMin, dataView->gradientColorMin);

			s32 labelLeft = gradientBounds.x + gradientBounds.w + uiPadding;

			// One label at the top
			Rect2I minLabelBounds = uiText(&renderer->uiBuffer, font, getText("data_view_minimum"_s), v2i(labelLeft, gradientBounds.y + gradientBounds.h), ALIGN_LEFT | ALIGN_BOTTOM, labelStyle->textColor);

			// The other at the bottom
			Rect2I maxLabelBounds = uiText(&renderer->uiBuffer, font, getText("data_view_maximum"_s), v2i(labelLeft, gradientBounds.y), ALIGN_LEFT | ALIGN_TOP, labelStyle->textColor);

			uiPos.y -= gradientBounds.h + uiPadding;
			dataViewUIWidth = max(dataViewUIWidth, gradientBounds.w + uiPadding + max(minLabelBounds.w, maxLabelBounds.w));
		}

		Rect2I labelRect = uiText(&renderer->uiBuffer, font, getText(dataView->title),
			uiPos, ALIGN_LEFT | ALIGN_BOTTOM, labelStyle->textColor);
		uiPos.y -= labelRect.h;
		dataViewUIWidth = max(dataViewUIWidth, labelRect.w);

		// Close button
		V2I dataViewCloseButtonSize = calculateButtonSize("X"_s, buttonStyle);
		Rect2I dataViewCloseButtonBounds = irectXYWH(uiPos.x + dataViewUIWidth + uiPadding, uiPos.y, dataViewCloseButtonSize.x, dataViewCloseButtonSize.y);
		if (uiButton(uiState, "X"_s, dataViewCloseButtonBounds, buttonStyle))
		{
			gameState->dataLayerToDraw = DataView_None;
		}
		dataViewUIWidth += dataViewCloseButtonBounds.w + uiPadding;

		Rect2 dataViewUIBounds = rectXYWHi(uiPos.x - uiPadding, uiPos.y - uiPadding, dataViewUIWidth + (uiPadding*2), dataViewButtonBounds.y + dataViewButtonBounds.h - uiPos.y + (uiPadding*2));
		fillDrawRectPlaceholder(dataViewUIBackground, dataViewUIBounds, theme->overlayColor);
	}
	else
	{
		Rect2I dataViewUIBounds = expand(dataViewButtonBounds, uiPadding);
		fillDrawRectPlaceholder(dataViewUIBackground, dataViewUIBounds, theme->overlayColor);
	}
}

void costTooltipWindowProc(WindowContext *context, void *userData)
{
	s32 buildCost = truncate32((smm)userData);
	City *city = &globalAppState.gameState->city;

	String style = canAfford(city, buildCost)
				? "cost-affordable"_s
				: "cost-unaffordable"_s;

	String text = myprintf("£{0}"_s, {formatInt(buildCost)});
	window_label(context, text, style);
}

void showCostTooltip(UIState *uiState, s32 buildCost)
{
	showTooltip(uiState, costTooltipWindowProc, (void*)(smm)buildCost);
}

void debugToolsWindowProc(WindowContext *context, void *userData)
{
	GameState *gameState = (GameState *)userData;

	if (window_button(context, "Inspect fire info"_s, -1, (gameState->inspectTileDebugFlags & DebugInspect_Fire)))
	{
		gameState->inspectTileDebugFlags ^= DebugInspect_Fire;
	}
	if (window_button(context, "Add Fire"_s, -1, (gameState->actionMode == ActionMode_Debug_AddFire)))
	{
		gameState->actionMode = ActionMode_Debug_AddFire;
	}
	if (window_button(context, "Remove Fire"_s, -1, (gameState->actionMode == ActionMode_Debug_RemoveFire)))
	{
		gameState->actionMode = ActionMode_Debug_RemoveFire;
	}

	if (window_button(context, "Inspect power info"_s, -1, (gameState->inspectTileDebugFlags & DebugInspect_Power)))
	{
		gameState->inspectTileDebugFlags ^= DebugInspect_Power;
	}
	
	if (window_button(context, "Inspect transport info"_s, -1, (gameState->inspectTileDebugFlags & DebugInspect_Transport)))
	{
		gameState->inspectTileDebugFlags ^= DebugInspect_Transport;
	}
}

AppStatus updateAndRenderGame(GameState *gameState, UIState *uiState)
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
		for (auto it = iterate(&uiState->uiRects); hasNext(&it); next(&it))
		{
			if (contains(getValue(&it), uiCamera->mousePos))
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
									pushUiMessage(uiState, getText("msg_cannot_afford_construction"_s));
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
							drawGrid(&renderer->worldOverlayBuffer, rect2(canZoneQuery->bounds), canZoneQuery->bounds.w, canZoneQuery->bounds.h, canZoneQuery->tileCanBeZoned, 2, palette);
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
							pushUiMessage(uiState, getText("msg_cannot_afford_demolition"_s));
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
						showWindow(uiState, "Inspect tile"_s, 250, 200, windowPos, "default"_s, WinFlag_AutomaticHeight | WinFlag_Unique, inspectTileWindowProc, gameState);
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

	drawCity(city, visibleTileBounds, demolitionRect);

	// Data layer rendering
	if (gameState->dataLayerToDraw)
	{
		DEBUG_BLOCK_T("Draw data layer", DCDT_GameUpdate);

		switch (gameState->dataLayerToDraw)
		{
			case DataView_Crime:                     drawCrimeDataView       (city, visibleTileBounds); break;
			case DataView_Desirability_Residential:  drawDesirabilityDataView(city, visibleTileBounds, Zone_Residential); break;
			case DataView_Desirability_Commercial:   drawDesirabilityDataView(city, visibleTileBounds, Zone_Commercial);  break;
			case DataView_Desirability_Industrial:   drawDesirabilityDataView(city, visibleTileBounds, Zone_Industrial);  break;
			case DataView_Fire:                      drawFireDataView        (city, visibleTileBounds); break;
			case DataView_Health:                    drawHealthDataView      (city, visibleTileBounds); break;
			case DataView_LandValue:                 drawLandValueDataView   (city, visibleTileBounds); break;
			case DataView_Pollution:                 drawPollutionDataView   (city, visibleTileBounds); break;
			case DataView_Power:                     drawPowerDataView       (city, visibleTileBounds); break;
		}
	}

	if (gameState->status == GameStatus_Quit)
	{
		result = AppStatus_MainMenu;
	}

	return result;
}
