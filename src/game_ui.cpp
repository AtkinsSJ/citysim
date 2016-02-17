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

void showCostTooltip(GLRenderer *renderer, int32 cost, int32 cityFunds) {
	if (cost > cityFunds) {
		renderer->tooltip.color = renderer->theme.tooltipColorBad;
	} else {
		renderer->tooltip.color = renderer->theme.tooltipColorNormal;
	}
	sprintf(renderer->tooltip.text, "-£%d", cost);
	renderer->tooltip.show = true;
}

const real32 uiPadding = 4;

bool drawCalendarUI(GLRenderer *renderer, Calendar *calendar, InputState *inputState, CalendarChange *change)
{
	bool buttonAteMouseEvent = false;

	if (change->isNewDay) {
		getDateString(calendar, calendar->dateStringBuffer);
	}

	uiLabel(renderer, renderer->theme.font, calendar->dateStringBuffer,
			v2((real32)renderer->worldCamera.windowWidth - uiPadding, uiPadding), ALIGN_RIGHT,
			1, renderer->theme.labelColor);

	const real32 buttonSize = 24;
	RealRect buttonRect = rectXYWH(renderer->worldCamera.windowWidth - uiPadding - buttonSize, 31,
								buttonSize, buttonSize);
	if (uiButton(renderer, inputState, ">>>", buttonRect, 1, (calendar->speed == Speed3)))
	{
		calendar->paused = false;
		calendar->speed = Speed3;
		buttonAteMouseEvent = true;
	}
	buttonRect.x -= buttonSize + uiPadding;
	if (uiButton(renderer, inputState, ">>", buttonRect, 1, (calendar->speed == Speed2)))
	{
		calendar->paused = false;
		calendar->speed = Speed2;
		buttonAteMouseEvent = true;
	}
	buttonRect.x -= buttonSize + uiPadding;
	if (uiButton(renderer, inputState, ">", buttonRect, 1, (calendar->speed == Speed1)))
	{
		calendar->paused = false;
		calendar->speed = Speed1;
		buttonAteMouseEvent = true;
	}
	buttonRect.x -= buttonSize + uiPadding;
	if (uiButton(renderer, inputState, "||", buttonRect, 1, calendar->paused, SDL_SCANCODE_SPACE, "(Space)"))
	{
		calendar->paused = !calendar->paused;
		buttonAteMouseEvent = true;
	}

	return buttonAteMouseEvent;
}