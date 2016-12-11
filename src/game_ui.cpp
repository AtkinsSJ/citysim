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
	V2 cameraSize = camera->size / camera->zoom;
	if (cityWidth < cameraSize.x) {
		// City smaller than camera, so centre on it
		camera->pos.x = cityWidth * 0.5f;
	} else {
		real32 minX = (cameraSize.x * 0.5f) - CAMERA_MARGIN;
		camera->pos.x = clamp( camera->pos.x, minX, (real32)cityWidth - minX );
	}

	if (cityHeight < camera->size.y / camera->zoom) {
		// City smaller than camera, so centre on it
		camera->pos.y = cityHeight * 0.5f;
	} else {
		real32 minY = (cameraSize.y * 0.5f) - CAMERA_MARGIN;
		camera->pos.y = clamp( camera->pos.y, minY, (real32)cityHeight - minY );
	}
}

void showCostTooltip(UIState *uiState, AssetManager *assets, int32 cost, int32 cityFunds) {
	UITheme *theme = &assets->theme;

	if (cost > cityFunds) {
		uiState->tooltip.color = theme->tooltipColorBad;
	} else {
		uiState->tooltip.color = theme->tooltipColorNormal;
	}
	sprintf(uiState->tooltip.text, "-£%d", cost);
	uiState->tooltip.show = true;
}

const real32 uiPadding = 4;

GameStatus updateAndRenderMainMenuUI(Renderer *renderer, AssetManager *assets, UIState *uiState, InputState *inputState,
									GameStatus gameStatus)
{
	GameStatus result = gameStatus;
	real32 windowWidth = (real32) renderer->uiBuffer.camera.size.x;
	real32 windowHeight = (real32) renderer->uiBuffer.camera.size.y;
	UITheme *theme = &assets->theme;

	drawRect(&renderer->uiBuffer, rectXYWH(0, 0, windowWidth, windowHeight), 0, theme->overlayColor);

	V2 position = v2(windowWidth * 0.5f, 157.0f);
	real32 maxLabelWidth = windowWidth - 256;

	//drawGL_TextureAtlasItem(renderer, true, GL_TextureAtlasItem_Menu_Logo, position, v2(499.0f, 154.0f), 0);
	//position.y += 154.0f;

	UILabelStyle *labelStyle = &theme->labelStyle;
	BitmapFont *font = getFont(assets, labelStyle->font);

	position.y += (uiText(uiState, renderer, font, "Under London",
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, labelStyle->textColor, maxLabelWidth)).h;

	position.y += (uiText(uiState, renderer, font, "Very much a work in progress!",
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, labelStyle->textColor, maxLabelWidth)).h;

	RealRect buttonRect = rectXYWH(uiPadding, windowHeight - uiPadding - 24, 80, 24);
	if (uiButton(uiState, renderer, assets, inputState, "Exit", buttonRect, 1))
	{
		result = GameStatus_Quit;
	}
	buttonRect.x = (windowWidth * 0.5f) - buttonRect.w/2;
	if (uiButton(uiState, renderer, assets, inputState, "Website", buttonRect, 1))
	{
		openUrlUnsafe("http://samatkins.co.uk");
	}
	buttonRect.x = windowWidth - uiPadding - buttonRect.w;
	if (uiButton(uiState, renderer, assets, inputState, "Play", buttonRect, 1)) // , SDL_SCANCODE_RETURN
	{
		result = GameStatus_Playing;
	}

	return result;
}

void updateAndRenderGameUI(Renderer *renderer, AssetManager *assets, UIState *uiState, GameState *gameState, InputState *inputState)
{
	real32 windowWidth = (real32) renderer->uiBuffer.camera.size.x;
	real32 windowHeight = (real32) renderer->uiBuffer.camera.size.y;
	UITheme *theme = &assets->theme;
	BitmapFont *font = getFont(assets, theme->labelStyle.font);

	uiState->uiRectCount = 0;

	real32 left = uiPadding;
	char stringBuffer[256];

	RealRect uiRect = uiState->uiRects[uiState->uiRectCount++] = rectXYWH(0,0, windowWidth, 64);
	drawRect(&renderer->uiBuffer, uiRect, 0, theme->overlayColor);

	uiText(uiState, renderer, font, gameState->city.name, v2(left, uiPadding), ALIGN_LEFT, 1, theme->labelStyle.textColor);

	const real32 centre = windowWidth * 0.5f;
	sprintf(stringBuffer, "£%d", gameState->city.funds);
	uiText(uiState, renderer, font, stringBuffer, v2(centre, uiPadding), ALIGN_RIGHT, 1, theme->labelStyle.textColor);
	sprintf(stringBuffer, "(-£%d/month)\0", gameState->city.monthlyExpenditure);
	uiText(uiState, renderer, font, stringBuffer, v2(centre, uiPadding), ALIGN_LEFT, 1, theme->labelStyle.textColor);

	// Build UI
	{
		V2 cameraCentre = v2(windowWidth/2.0f, windowHeight/2.0f);
		RealRect buttonRect = rectXYWH(uiPadding, 28 + uiPadding, 80, 24);

		if (uiMenuButton(uiState, renderer, assets, inputState, "Build...", buttonRect, 1, UIMenu_Build))
		{
			RealRect menuButtonRect = buttonRect;
			menuButtonRect.y += menuButtonRect.h + uiPadding;

			RealRect menuRect = expandRect(menuButtonRect, uiPadding);

			if (uiButton(uiState, renderer, assets, inputState, "Build HQ", menuButtonRect, 1,
					(uiState->actionMode == ActionMode_Build) && (uiState->selectedBuildingArchetype == BA_Farmhouse),
					SDL_SCANCODE_Q, "(Q)"))
			{
				uiState->openMenu = UIMenu_None;
				uiState->selectedBuildingArchetype = BA_Farmhouse;
				uiState->actionMode = ActionMode_Build;
				setCursor(uiState, assets, Cursor_Build);
			}
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			menuRect.h += menuButtonRect.h + uiPadding;

			if (uiButton(uiState, renderer, assets, inputState, "Build Field", menuButtonRect, 1,
						(uiState->actionMode == ActionMode_Build) && (uiState->selectedBuildingArchetype == BA_Field),
						SDL_SCANCODE_F, "(F)"))
			{
				uiState->openMenu = UIMenu_None;
				uiState->selectedBuildingArchetype = BA_Field;
				uiState->actionMode = ActionMode_Build;
				setCursor(uiState, assets, Cursor_Build);
			}
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			menuRect.h += menuButtonRect.h + uiPadding;

			if (uiButton(uiState, renderer, assets, inputState, "Build Barn", menuButtonRect, 1,
						(uiState->actionMode == ActionMode_Build) && (uiState->selectedBuildingArchetype == BA_Barn),
						SDL_SCANCODE_B, "(B)"))
			{
				uiState->openMenu = UIMenu_None;
				uiState->selectedBuildingArchetype = BA_Barn;
				uiState->actionMode = ActionMode_Build;
				setCursor(uiState, assets, Cursor_Build);
			}
			menuButtonRect.y += menuButtonRect.h + uiPadding;
			menuRect.h += menuButtonRect.h + uiPadding;

			if (uiButton(uiState, renderer, assets, inputState, "Build Road", menuButtonRect, 1,
						(uiState->actionMode == ActionMode_Build) && (uiState->selectedBuildingArchetype == BA_Path),
						SDL_SCANCODE_R, "(R)"))
			{
				uiState->openMenu = UIMenu_None;
				uiState->selectedBuildingArchetype = BA_Path;
				uiState->actionMode = ActionMode_Build;
				setCursor(uiState, assets, Cursor_Build);
			}

			uiState->uiRects[uiState->uiRectCount++] = menuRect;
			drawRect(&renderer->uiBuffer, menuRect, 0, theme->overlayColor);
		}

		buttonRect.x += buttonRect.w + uiPadding;
		if (uiButton(uiState, renderer, assets, inputState, "Demolish", buttonRect, 1,
					(uiState->actionMode == ActionMode_Demolish),
					SDL_SCANCODE_X, "(X)"))
		{
			uiState->actionMode = ActionMode_Demolish;
			setCursor(uiState, assets, Cursor_Demolish);
		}
		buttonRect.x += buttonRect.w + uiPadding;
		if (uiButton(uiState, renderer, assets, inputState, "Plant", buttonRect, 1,
					(uiState->actionMode == ActionMode_Plant),
					SDL_SCANCODE_P, "(P)"))
		{
			uiState->actionMode = ActionMode_Plant;
			setCursor(uiState, assets, Cursor_Plant);
		}
		buttonRect.x += buttonRect.w + uiPadding;
		if (uiButton(uiState, renderer, assets, inputState, "Harvest", buttonRect, 1,
					(uiState->actionMode == ActionMode_Harvest),
					SDL_SCANCODE_H, "(H)"))
		{
			uiState->actionMode = ActionMode_Harvest;
			setCursor(uiState, assets, Cursor_Harvest);
		}
		buttonRect.x += buttonRect.w + uiPadding;
		if (uiButton(uiState, renderer, assets, inputState, "Hire Worker", buttonRect, 1,
					(uiState->actionMode == ActionMode_Hire),
					SDL_SCANCODE_G, "(G)"))
		{
			uiState->actionMode = ActionMode_Hire;
			setCursor(uiState, assets, Cursor_Hire);
		}
	}
}

// Returns true if game should restart
bool updateAndRenderGameOverUI(Renderer *renderer, AssetManager *assets, UIState *uiState, InputState *inputState, bool won)
{
	bool result = false;
	real32 windowWidth = (real32) renderer->uiBuffer.camera.size.x;
	real32 windowHeight = (real32) renderer->uiBuffer.camera.size.y;

	UITheme *theme = &assets->theme;
	BitmapFont *font = getFont(assets, FontAssetType_Main);

	V2 cameraCentre = v2(windowWidth/2.0f, windowHeight/2.0f);
	drawRect(&renderer->uiBuffer, rectXYWH(0, 0, windowWidth, windowHeight), 10, theme->overlayColor);

	char gameOverText[256];
	if (won)
	{
		sprintf(gameOverText, "You won! You earned £%d in ??? days", gameWinFunds);
	} else {
		sprintf(gameOverText, "Game over! You ran out of money! :(");
	}

	uiText(uiState, renderer, font, gameOverText, cameraCentre - v2(0, 32), ALIGN_CENTRE, 11, theme->labelStyle.textColor);

	if (uiButton(uiState, renderer, assets, inputState, "Menu", rectCentreSize(cameraCentre, v2(80, 24)), 11))
	{
		result = true;
	}

	return result;
}