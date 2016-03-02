#pragma once

void updateCamera(Camera *camera, InputState *inputState, int32 cityWidth, int32 cityHeight) // City size in tiles
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
		Coord clickStartPos = inputState->clickStartPosition[mouseButtonIndex(SDL_BUTTON_MIDDLE)];
		camera->pos += (v2(inputState->mousePos) - v2(clickStartPos)) * scale;
	}
	else
	{
		if (inputState->keyDown[SDL_SCANCODE_LEFT]
			|| inputState->keyDown[SDL_SCANCODE_A]
			|| (inputState->mousePos.x < CAMERA_EDGE_SCROLL_MARGIN))
		{
			camera->pos.x -= scrollSpeed;
		}
		else if (inputState->keyDown[SDL_SCANCODE_RIGHT]
			|| inputState->keyDown[SDL_SCANCODE_D]
			|| (inputState->mousePos.x > (camera->windowWidth - CAMERA_EDGE_SCROLL_MARGIN)))
		{
			camera->pos.x += scrollSpeed;
		}

		if (inputState->keyDown[SDL_SCANCODE_UP]
			|| inputState->keyDown[SDL_SCANCODE_W]
			|| (inputState->mousePos.y < CAMERA_EDGE_SCROLL_MARGIN))
		{
			camera->pos.y -= scrollSpeed;
		}
		else if (inputState->keyDown[SDL_SCANCODE_DOWN]
			|| inputState->keyDown[SDL_SCANCODE_S]
			|| (inputState->mousePos.y > (camera->windowHeight - CAMERA_EDGE_SCROLL_MARGIN)))
		{
			camera->pos.y += scrollSpeed;
		}
	}

	// Clamp camera
	real32 scale = TILE_SIZE / camera->zoom;
	real32 cityPixelWidth = cityWidth * scale,
			cityPixelHeight = cityHeight * scale;

	if (cityPixelWidth < camera->windowWidth) {
		// City smaller than camera, so centre on it
		camera->pos.x = cityWidth * 0.5f;
	} else {
		real32 minX = ((real32)camera->windowWidth/(2.0f * scale)) - CAMERA_MARGIN;
		camera->pos.x = clamp( camera->pos.x, minX, cityWidth - minX );
	}

	if (cityPixelHeight < camera->windowHeight) {
		// City smaller than camera, so centre on it
		camera->pos.y = cityHeight * 0.5f;
	} else {
		real32 minY = ((real32)camera->windowHeight/(2.0f * scale)) - CAMERA_MARGIN;
		camera->pos.y = clamp( camera->pos.y, minY, cityHeight - minY );
	}
}

void showCostTooltip(GLRenderer *renderer, UIState *uiState, int32 cost, int32 cityFunds) {
	if (cost > cityFunds) {
		uiState->tooltip.color = renderer->theme.tooltipColorBad;
	} else {
		uiState->tooltip.color = renderer->theme.tooltipColorNormal;
	}
	sprintf(uiState->tooltip.text, "-£%d", cost);
	uiState->tooltip.show = true;
}

const real32 uiPadding = 4;

bool drawCalendarUI(GLRenderer *renderer, Calendar *calendar, InputState *inputState, UIState *uiState)
{
	bool buttonAteMouseEvent = false;

	getDateString(calendar, calendar->dateStringBuffer);

	uiLabel(renderer, renderer->theme.font, calendar->dateStringBuffer,
			v2((real32)renderer->worldCamera.windowWidth - uiPadding, uiPadding), ALIGN_RIGHT,
			1, renderer->theme.labelColor);

	const real32 buttonSize = 24;
	RealRect buttonRect = rectXYWH(renderer->worldCamera.windowWidth - uiPadding - buttonSize, 32,
								buttonSize, buttonSize);
	if (uiButton(renderer, inputState, uiState, ">>>", buttonRect, 1, (calendar->speed == Speed3)))
	{
		calendar->paused = false;
		calendar->speed = Speed3;
		buttonAteMouseEvent = true;
	}
	buttonRect.x -= buttonSize + uiPadding;
	if (uiButton(renderer, inputState, uiState, ">>", buttonRect, 1, (calendar->speed == Speed2)))
	{
		calendar->paused = false;
		calendar->speed = Speed2;
		buttonAteMouseEvent = true;
	}
	buttonRect.x -= buttonSize + uiPadding;
	if (uiButton(renderer, inputState, uiState, ">", buttonRect, 1, (calendar->speed == Speed1)))
	{
		calendar->paused = false;
		calendar->speed = Speed1;
		buttonAteMouseEvent = true;
	}
	buttonRect.x -= buttonSize + uiPadding;
	if (uiButton(renderer, inputState, uiState, "||", buttonRect, 1, calendar->paused, SDL_SCANCODE_SPACE, "(Space)"))
	{
		calendar->paused = !calendar->paused;
		buttonAteMouseEvent = true;
	}

	return buttonAteMouseEvent;
}

GameStatus updateAndRenderMainMenuUI(GLRenderer *renderer, UIState *uiState, InputState *inputState,
									GameStatus gameStatus, char *cityName, int32 cityNameMaxLength)
{
	GameStatus result = gameStatus;

	drawRect(renderer, true, rectXYWH(0, 0, (real32)renderer->worldCamera.windowWidth, (real32)renderer->worldCamera.windowHeight), 0, renderer->theme.overlayColor);

	V2 position = v2((real32)renderer->worldCamera.windowWidth * 0.5f, 157.0f);
	real32 maxLabelWidth = (real32)renderer->worldCamera.windowWidth - 256;

	drawTextureAtlasItem(renderer, true, TextureAtlasItem_Menu_Logo, position, v2(499.0f, 154.0f), 0);

	position.y += 154.0f;

	position.y += (uiLabel(renderer, renderer->theme.font, "Type a name for your farm, then click on 'Play'. This is some text. As I add it, it moves left? MOOOOVE! I like to move it move it, I like to...\n...\n\nMOVE IT! Supercalifragalisticexpialidocioustryingtomakethissolongthatitteststhelinesplittingcodemaybemeybemaybe?",
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, renderer->theme.labelColor, maxLabelWidth)).h;

	uiTextInput(renderer, inputState, true, cityName, cityNameMaxLength, position, 1);
	position.y += 32;

	position.y += (uiLabel(renderer, renderer->theme.font, "Win by having £30,000 on hand, and lose by running out of money.",
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, renderer->theme.labelColor, maxLabelWidth)).h;

	position.y += (uiLabel(renderer, renderer->theme.font, "Workers are paid £50 at the start of each month.",
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, renderer->theme.labelColor, maxLabelWidth)).h;

	RealRect buttonRect = rectXYWH(uiPadding, renderer->worldCamera.windowHeight - uiPadding - 24, 80, 24);
	if (uiButton(renderer, inputState, uiState, "Exit", buttonRect, 1))
	{
		result = GameStatus_Quit;
	}
	buttonRect.x = ((real32)renderer->worldCamera.windowWidth * 0.5f) - buttonRect.w/2;
	if (uiButton(renderer, inputState, uiState, "Website", buttonRect, 1))
	{
		openUrlUnsafe("http://samatkins.co.uk");
	}
	buttonRect.x = renderer->worldCamera.windowWidth - uiPadding - buttonRect.w;
	if (uiButton(renderer, inputState, uiState, "Play", buttonRect, 1)) // , SDL_SCANCODE_RETURN
	{
		result = GameStatus_Playing;
	}

	return result;
}

void updateAndRenderGameUI(GLRenderer *renderer, UIState *uiState, GameState *gameState, InputState *inputState)
{
	uiState->uiRectCount = 0;

	real32 left = uiPadding;
	char stringBuffer[256];

	RealRect uiRect = uiState->uiRects[uiState->uiRectCount++] = rectXYWH(0,0, (real32) renderer->worldCamera.windowWidth, 64);
	drawRect(renderer, true, uiRect, 0, renderer->theme.overlayColor);

	uiLabel(renderer, renderer->theme.font, gameState->city.name, v2(left, uiPadding), ALIGN_LEFT, 1, renderer->theme.labelColor);

	const real32 centre = renderer->worldCamera.windowWidth * 0.5f;
	sprintf(stringBuffer, "£%d", gameState->city.funds);
	uiLabel(renderer, renderer->theme.font, stringBuffer, v2(centre, uiPadding), ALIGN_RIGHT, 1, renderer->theme.labelColor);
	sprintf(stringBuffer, "(-£%d/month)", gameState->city.monthlyExpenditure);
	uiLabel(renderer, renderer->theme.font, stringBuffer, v2(centre, uiPadding), ALIGN_LEFT, 1, renderer->theme.labelColor);

	drawCalendarUI(renderer, &gameState->calendar, inputState, uiState);

	// Build UI
	{
		V2 cameraCentre = v2(renderer->worldCamera.windowWidth/2.0f, renderer->worldCamera.windowHeight/2.0f);
		RealRect buttonRect = rectXYWH(uiPadding, 28 + uiPadding, 80, 24);

		if (uiMenuButton(renderer, inputState, uiState, "Build...", buttonRect, 1, UIMenu_Build))
		{
			RealRect menuButtonRect = buttonRect;
			menuButtonRect.y += menuButtonRect.h + uiPadding;

			RealRect menuRect = expandRect(menuButtonRect, uiPadding);

			if (uiButton(renderer, inputState, uiState, "Build HQ", menuButtonRect, 1,
					(uiState->actionMode == ActionMode_Build) && (uiState->selectedBuildingArchetype == BA_Farmhouse),
					SDL_SCANCODE_Q, "(Q)"))
			{
				uiState->openMenu = UIMenu_None;
				uiState->selectedBuildingArchetype = BA_Farmhouse;
				uiState->actionMode = ActionMode_Build;
				setCursor(renderer, Cursor_Build);
			}
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			menuRect.h += menuButtonRect.h + uiPadding;

			if (uiButton(renderer, inputState, uiState, "Build Field", menuButtonRect, 1,
						(uiState->actionMode == ActionMode_Build) && (uiState->selectedBuildingArchetype == BA_Field),
						SDL_SCANCODE_F, "(F)"))
			{
				uiState->openMenu = UIMenu_None;
				uiState->selectedBuildingArchetype = BA_Field;
				uiState->actionMode = ActionMode_Build;
				setCursor(renderer, Cursor_Build);
			}
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			menuRect.h += menuButtonRect.h + uiPadding;

			if (uiButton(renderer, inputState, uiState, "Build Barn", menuButtonRect, 1,
						(uiState->actionMode == ActionMode_Build) && (uiState->selectedBuildingArchetype == BA_Barn),
						SDL_SCANCODE_B, "(B)"))
			{
				uiState->openMenu = UIMenu_None;
				uiState->selectedBuildingArchetype = BA_Barn;
				uiState->actionMode = ActionMode_Build;
				setCursor(renderer, Cursor_Build);
			}
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			menuRect.h += menuButtonRect.h + uiPadding;

			if (uiButton(renderer, inputState, uiState, "Build Road", menuButtonRect, 1,
						(uiState->actionMode == ActionMode_Build) && (uiState->selectedBuildingArchetype == BA_Path),
						SDL_SCANCODE_R, "(R)"))
			{
				uiState->openMenu = UIMenu_None;
				uiState->selectedBuildingArchetype = BA_Path;
				uiState->actionMode = ActionMode_Build;
				setCursor(renderer, Cursor_Build);
			}

			uiState->uiRects[uiState->uiRectCount++] = menuRect;
			drawRect(renderer, true, menuRect, 0, renderer->theme.overlayColor);
		}

		buttonRect.x += buttonRect.w + uiPadding;
		if (uiButton(renderer, inputState, uiState, "Demolish", buttonRect, 1,
					(uiState->actionMode == ActionMode_Demolish),
					SDL_SCANCODE_X, "(X)"))
		{
			uiState->actionMode = ActionMode_Demolish;
			setCursor(renderer, Cursor_Demolish);
		}
		buttonRect.x += buttonRect.w + uiPadding;
		if (uiButton(renderer, inputState, uiState, "Plant", buttonRect, 1,
					(uiState->actionMode == ActionMode_Plant),
					SDL_SCANCODE_P, "(P)"))
		{
			uiState->actionMode = ActionMode_Plant;
			setCursor(renderer, Cursor_Plant);
		}
		buttonRect.x += buttonRect.w + uiPadding;
		if (uiButton(renderer, inputState, uiState, "Harvest", buttonRect, 1,
					(uiState->actionMode == ActionMode_Harvest),
					SDL_SCANCODE_H, "(H)"))
		{
			uiState->actionMode = ActionMode_Harvest;
			setCursor(renderer, Cursor_Harvest);
		}
		buttonRect.x += buttonRect.w + uiPadding;
		if (uiButton(renderer, inputState, uiState, "Hire Worker", buttonRect, 1,
					(uiState->actionMode == ActionMode_Hire),
					SDL_SCANCODE_G, "(G)"))
		{
			uiState->actionMode = ActionMode_Hire;
			setCursor(renderer, Cursor_Hire);
		}
	}
}

// Returns true if game should restart
bool updateAndRenderGameOverUI(GLRenderer *renderer, UIState *uiState, GameState *gameState, InputState *inputState, bool won)
{
	bool result = false;

	V2 cameraCentre = v2(renderer->worldCamera.windowWidth/2.0f, renderer->worldCamera.windowHeight/2.0f);
	drawRect(renderer, true,
			rectXYWH(0, 0, (real32)renderer->worldCamera.windowWidth, (real32)renderer->worldCamera.windowHeight),
			10, renderer->theme.overlayColor);

	char gameOverText[256];
	if (won)
	{
		sprintf(gameOverText, "You won! You earned £%d in %d days", gameWinFunds, gameState->calendar.totalDays);
	} else {
		sprintf(gameOverText, "Game over! You ran out of money! :(");
	}

	uiLabel(renderer, renderer->theme.font, gameOverText, cameraCentre - v2(0, 32), ALIGN_CENTRE, 11, renderer->theme.labelColor);

	if (uiButton(renderer, inputState, uiState, "Menu", rectCentreSize(cameraCentre, v2(80, 24)), 11))
	{
		result = true;
	}

	return result;
}