#pragma once

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

void showCostTooltip(GL_Renderer *renderer, UIState *uiState, int32 cost, int32 cityFunds) {
	if (cost > cityFunds) {
		uiState->tooltip.color = renderer->theme.tooltipColorBad;
	} else {
		uiState->tooltip.color = renderer->theme.tooltipColorNormal;
	}
	sprintf(uiState->tooltip.text, "-£%d", cost);
	uiState->tooltip.show = true;
}

const real32 uiPadding = 4;

GameStatus updateAndRenderMainMenuUI(GL_Renderer *renderer, UIState *uiState, InputState *inputState,
									GameStatus gameStatus)
{
	GameStatus result = gameStatus;
	real32 windowWidth = (real32) renderer->uiBuffer.camera.windowWidth;
	real32 windowHeight = (real32) renderer->uiBuffer.camera.windowHeight;

	drawRect(renderer, true, rectXYWH(0, 0, windowWidth, windowHeight), 0, renderer->theme.overlayColor);

	V2 position = v2(windowWidth * 0.5f, 157.0f);
	real32 maxLabelWidth = windowWidth - 256;

	//drawGL_TextureAtlasItem(renderer, true, GL_TextureAtlasItem_Menu_Logo, position, v2(499.0f, 154.0f), 0);
	//position.y += 154.0f;

	position.y += (uiLabel(renderer, renderer->theme.font, "Under London",
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, renderer->theme.labelColor, maxLabelWidth)).h;

	position.y += (uiLabel(renderer, renderer->theme.font, "Very much a work in progress!",
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, renderer->theme.labelColor, maxLabelWidth)).h;

	RealRect buttonRect = rectXYWH(uiPadding, windowHeight - uiPadding - 24, 80, 24);
	if (uiButton(renderer, inputState, uiState, "Exit", buttonRect, 1))
	{
		result = GameStatus_Quit;
	}
	buttonRect.x = (windowWidth * 0.5f) - buttonRect.w/2;
	if (uiButton(renderer, inputState, uiState, "Website", buttonRect, 1))
	{
		openUrlUnsafe("http://samatkins.co.uk");
	}
	buttonRect.x = windowWidth - uiPadding - buttonRect.w;
	if (uiButton(renderer, inputState, uiState, "Play", buttonRect, 1)) // , SDL_SCANCODE_RETURN
	{
		result = GameStatus_Playing;
	}

	return result;
}

void updateAndRenderGameUI(GL_Renderer *renderer, UIState *uiState, GameState *gameState, InputState *inputState)
{
	real32 windowWidth = (real32) renderer->uiBuffer.camera.windowWidth;
	real32 windowHeight = (real32) renderer->uiBuffer.camera.windowHeight;

	uiState->uiRectCount = 0;

	real32 left = uiPadding;
	char stringBuffer[256];

	RealRect uiRect = uiState->uiRects[uiState->uiRectCount++] = rectXYWH(0,0, windowWidth, 64);
	drawRect(renderer, true, uiRect, 0, renderer->theme.overlayColor);

	uiLabel(renderer, renderer->theme.font, gameState->city.name, v2(left, uiPadding), ALIGN_LEFT, 1, renderer->theme.labelColor);

	const real32 centre = windowWidth * 0.5f;
	sprintf(stringBuffer, "£%d", gameState->city.funds);
	uiLabel(renderer, renderer->theme.font, stringBuffer, v2(centre, uiPadding), ALIGN_RIGHT, 1, renderer->theme.labelColor);
	sprintf(stringBuffer, "(-£%d/month)", gameState->city.monthlyExpenditure);
	uiLabel(renderer, renderer->theme.font, stringBuffer, v2(centre, uiPadding), ALIGN_LEFT, 1, renderer->theme.labelColor);

	// Build UI
	{
		V2 cameraCentre = v2(windowWidth/2.0f, windowHeight/2.0f);
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
				setCursor(&renderer->theme, Cursor_Build);
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
				setCursor(&renderer->theme, Cursor_Build);
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
				setCursor(&renderer->theme, Cursor_Build);
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
				setCursor(&renderer->theme, Cursor_Build);
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
			setCursor(&renderer->theme, Cursor_Demolish);
		}
		buttonRect.x += buttonRect.w + uiPadding;
		if (uiButton(renderer, inputState, uiState, "Plant", buttonRect, 1,
					(uiState->actionMode == ActionMode_Plant),
					SDL_SCANCODE_P, "(P)"))
		{
			uiState->actionMode = ActionMode_Plant;
			setCursor(&renderer->theme, Cursor_Plant);
		}
		buttonRect.x += buttonRect.w + uiPadding;
		if (uiButton(renderer, inputState, uiState, "Harvest", buttonRect, 1,
					(uiState->actionMode == ActionMode_Harvest),
					SDL_SCANCODE_H, "(H)"))
		{
			uiState->actionMode = ActionMode_Harvest;
			setCursor(&renderer->theme, Cursor_Harvest);
		}
		buttonRect.x += buttonRect.w + uiPadding;
		if (uiButton(renderer, inputState, uiState, "Hire Worker", buttonRect, 1,
					(uiState->actionMode == ActionMode_Hire),
					SDL_SCANCODE_G, "(G)"))
		{
			uiState->actionMode = ActionMode_Hire;
			setCursor(&renderer->theme, Cursor_Hire);
		}
	}
}

// Returns true if game should restart
bool updateAndRenderGameOverUI(GL_Renderer *renderer, UIState *uiState, InputState *inputState, bool won)
{
	bool result = false;
	real32 windowWidth = (real32) renderer->uiBuffer.camera.windowWidth;
	real32 windowHeight = (real32) renderer->uiBuffer.camera.windowHeight;

	V2 cameraCentre = v2(windowWidth/2.0f, windowHeight/2.0f);
	drawRect(renderer, true, rectXYWH(0, 0, windowWidth, windowHeight), 10, renderer->theme.overlayColor);

	char gameOverText[256];
	if (won)
	{
		sprintf(gameOverText, "You won! You earned £%d in ??? days", gameWinFunds);
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