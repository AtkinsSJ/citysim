#pragma once

void updateCamera(Camera *camera, MouseState *mouseState, KeyboardState *keyboardState,
					int32 cityWidth, int32 cityHeight) // City size in tiles
{ 
	// Zooming
	if (canZoom && mouseState->wheelY) {
		// round()ing the zoom so it doesn't gradually drift due to float imprecision
		camera->zoom = clamp(round(10 * camera->zoom - mouseState->wheelY) * 0.1f, 0.1f, 10.0f);
	}

	// Panning
	real32 scrollSpeed = (CAMERA_PAN_SPEED * sqrt(camera->zoom)) * SECONDS_PER_FRAME;
	if (mouseButtonPressed(mouseState, SDL_BUTTON_MIDDLE))
	{
		// Click-panning!
		float scale = scrollSpeed * 0.01f;
		Coord clickStartPos = mouseState->clickStartPosition[mouseButtonIndex(SDL_BUTTON_MIDDLE)];
		camera->pos += (v2(mouseState->pos) - v2(clickStartPos)) * scale;
	}
	else
	{
		if (keyboardState->down[SDL_SCANCODE_LEFT]
			|| keyboardState->down[SDL_SCANCODE_A]
			|| (mouseState->pos.x < CAMERA_EDGE_SCROLL_MARGIN))
		{
			camera->pos.x -= scrollSpeed;
		}
		else if (keyboardState->down[SDL_SCANCODE_RIGHT]
			|| keyboardState->down[SDL_SCANCODE_D]
			|| (mouseState->pos.x > (camera->windowWidth - CAMERA_EDGE_SCROLL_MARGIN)))
		{
			camera->pos.x += scrollSpeed;
		}

		if (keyboardState->down[SDL_SCANCODE_UP]
			|| keyboardState->down[SDL_SCANCODE_W]
			|| (mouseState->pos.y < CAMERA_EDGE_SCROLL_MARGIN))
		{
			camera->pos.y -= scrollSpeed;
		}
		else if (keyboardState->down[SDL_SCANCODE_DOWN]
			|| keyboardState->down[SDL_SCANCODE_S]
			|| (mouseState->pos.y > (camera->windowHeight - CAMERA_EDGE_SCROLL_MARGIN)))
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

void showCostTooltip(Tooltip *tooltip, GLRenderer *renderer, int32 cost, int32 cityFunds) {
	if (cost > cityFunds) {
		tooltip->label.color = renderer->theme.tooltipColorBad;
	} else {
		tooltip->label.color = renderer->theme.tooltipColorNormal;
	}
	sprintf(tooltip->buffer, "-£%d", cost);
	setUiLabelText(&tooltip->label, tooltip->buffer);
	tooltip->show = true;
}

const real32 uiPadding = 4;

struct MainMenuUI {

	char *cityName;

	UiButton buttonStart,
			buttonExit,
			buttonWebsite;
};
void initMainMenuUI(MainMenuUI *menu, GLRenderer *renderer, char *cityName) {

	*menu = {};
	V2 screenCentre = v2(renderer->worldCamera.windowSize) / 2.0f;
	menu->cityName = cityName;

	RealRect buttonRect = rectXYWH(uiPadding, renderer->worldCamera.windowHeight - uiPadding - 24, 80, 24);
	initUiButton(&menu->buttonExit, renderer, buttonRect, "Exit");
	buttonRect.x = screenCentre.x - buttonRect.w/2;
	initUiButton(&menu->buttonWebsite, renderer, buttonRect, "Website");
	buttonRect.x = renderer->worldCamera.windowWidth - uiPadding - buttonRect.w;
	initUiButton(&menu->buttonStart, renderer, buttonRect, "Play", SDL_SCANCODE_RETURN);
}

void drawMainMenuUI(MainMenuUI *menu, GLRenderer *renderer) {
	drawRect(renderer, true, rectXYWH(0, 0, (real32)renderer->worldCamera.windowWidth, (real32)renderer->worldCamera.windowHeight),
			0, renderer->theme.overlayColor);

	V2 position = v2((real32)renderer->worldCamera.windowWidth * 0.5f, 157.0f);

	drawTextureAtlasItem(renderer, true, TextureAtlasItem_Menu_Logo, position, v2(499.0f, 154.0f), 0);

	position.y += 154.0f;

	uiLabel(renderer, renderer->theme.font, "Type a name for your farm, then click on 'Play'.",
			position, ALIGN_CENTER, renderer->theme.labelColor);
	position.y += 32;

	uiLabel(renderer, renderer->theme.font, menu->cityName,
			position, ALIGN_CENTER, renderer->theme.labelColor);
	position.y += 32;

	uiLabel(renderer, renderer->theme.font, "Win by having £30,000 on hand, and lose by running out of money.",
			position, ALIGN_CENTER, renderer->theme.labelColor);
	position.y += 32;

	uiLabel(renderer, renderer->theme.font, "Workers are paid £50 at the start of each month.",
			position, ALIGN_CENTER, renderer->theme.labelColor);
	position.y += 32;

	///

	drawUiButton(renderer, &menu->buttonExit);
	drawUiButton(renderer, &menu->buttonWebsite);
	drawUiButton(renderer, &menu->buttonStart);
}

struct CalendarUI {
	Calendar *calendar;
	UiButtonGroup buttonGroup;
	UiButton buttonPause,
			*buttonPlaySlow,
			*buttonPlayMedium,
			*buttonPlayFast;
	UiLabel labelDate;
	char dateStringBuffer[50];
};
void initCalendarUI(CalendarUI *ui, GLRenderer *renderer, Calendar *calendar) {

	*ui = {};
	ui->calendar = calendar;

	V2 textPosition = v2(renderer->worldCamera.windowWidth - uiPadding, uiPadding);
	getDateString(calendar, ui->dateStringBuffer);
	initUiLabel(&ui->labelDate, textPosition, ALIGN_RIGHT | ALIGN_TOP,
				ui->dateStringBuffer, renderer->theme.font, renderer->theme.labelColor);

	const real32 buttonSize = 24;
	RealRect buttonRect = rectXYWH(renderer->worldCamera.windowWidth - uiPadding - buttonSize, 31,
								buttonSize, buttonSize);
	ui->buttonPlayFast = addButtonToGroup(&ui->buttonGroup);
	initUiButton(ui->buttonPlayFast, renderer, buttonRect, ">>>");

	buttonRect.x -= buttonSize + uiPadding;
	ui->buttonPlayMedium = addButtonToGroup(&ui->buttonGroup);
	initUiButton(ui->buttonPlayMedium, renderer, buttonRect, ">>");

	buttonRect.x -= buttonSize + uiPadding;
	ui->buttonPlaySlow = addButtonToGroup(&ui->buttonGroup);
	initUiButton(ui->buttonPlaySlow, renderer, buttonRect, ">");

	buttonRect.x -= buttonSize + uiPadding;
	initUiButton(&ui->buttonPause, renderer, buttonRect, "||", SDL_SCANCODE_SPACE, "(Space)");

	switch (ui->calendar->speed) {
		case Speed1: {
			setActiveButton(&ui->buttonGroup, ui->buttonPlaySlow);
		} break;
		case Speed2: {
			setActiveButton(&ui->buttonGroup, ui->buttonPlayMedium);
		} break;
		case Speed3: {
			setActiveButton(&ui->buttonGroup, ui->buttonPlayFast);
		} break;
	}
}
bool updateCalendarUI(CalendarUI *ui, GLRenderer *renderer, Tooltip *tooltip,
					MouseState *mouseState, KeyboardState *keyboardState,
					CalendarChange *change) {

	bool buttonAteMouseEvent = false;

	if (updateUiButtonGroup(renderer, tooltip, &ui->buttonGroup, mouseState, keyboardState)
		|| updateUiButton(renderer, tooltip, &ui->buttonPause, mouseState, keyboardState) ) {
		buttonAteMouseEvent = true;
	}

	// Speed controls
#if 0
	int32 speedChange = 0;
	if (keyJustPressed(keyboardState, SDL_GetScancodeFromKey(SDLK_MINUS))) {
		if (ui->calendar->speed > Speed1) {
			speedChange = -1;
		}
	} else if (keyJustPressed(keyboardState, SDL_GetScancodeFromKey(SDLK_PLUS))) {
		if (ui->calendar->speed < Speed3) {
			speedChange = 1;
		}
	}
	if (speedChange) {
		ui->calendar->speed = (CalendarSpeed)(ui->calendar->speed + speedChange);
		switch (ui->calendar->speed) {
			case Speed1: {
				setActiveButton(&ui->buttonGroup, ui->buttonPlaySlow);
			} break;
			case Speed2: {
				setActiveButton(&ui->buttonGroup, ui->buttonPlayMedium);
			} break;
			case Speed3: {
				setActiveButton(&ui->buttonGroup, ui->buttonPlayFast);
			} break;
		}
	}
#endif
	if (ui->buttonPause.justClicked) {
		ui->calendar->paused = !ui->calendar->paused;
	} else if (ui->buttonPlaySlow->justClicked) {
		ui->calendar->paused = false;
		ui->calendar->speed = Speed1;
	} else if (ui->buttonPlayMedium->justClicked) {
		ui->calendar->paused = false;
		ui->calendar->speed = Speed2;
	} else if (ui->buttonPlayFast->justClicked) {
		ui->calendar->paused = false;
		ui->calendar->speed = Speed3;
	}
	ui->buttonPause.active = ui->calendar->paused;

	if (change->isNewDay) {
		getDateString(ui->calendar, ui->dateStringBuffer);
		setUiLabelText(&ui->labelDate, ui->dateStringBuffer);
	}

	return buttonAteMouseEvent;
}