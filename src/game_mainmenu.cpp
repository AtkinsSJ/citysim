#pragma once

void testWindowProc(WindowContext *context, Window *window, void *userData)
{
	s32 *ourNumber = (s32*) userData;

	// Stuff we'd like to do:
	// - print out the number
	// - increment button
	// - decrement button

	V4 textColor = color255(255, 255, 255, 255);

	window_text(context, LocalString("This is a window! Fun times."), textColor);
	window_text(context, myprintf("The number is {0}", {formatInt(*ourNumber)}), textColor);
	window_text(context, LocalString("Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged.\nIt was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum."), textColor);
	// if (window_button(context, LocalString("Increment")))
	// {
	// 	(*ourNumber)++;
	// }
	// if (window_button(context, LocalString("Decrement")))
	// {
	// 	(*ourNumber)--;
	// }
}

void updateAndRenderMainMenu(AppState *appState, InputState *inputState, Renderer *renderer, AssetManager *assets)
{
	DEBUG_FUNCTION();
	
	AppStatus result = appState->appStatus;

	RenderBuffer *uiBuffer = &renderer->uiBuffer;

	f32 windowWidth = (f32) uiBuffer->camera.size.x;
	f32 windowHeight = (f32) uiBuffer->camera.size.y;
	UITheme *theme = &assets->theme;
	UIState *uiState = &appState->uiState;

	drawRect(uiBuffer, rectXYWH(0, 0, windowWidth, windowHeight), 0, theme->overlayColor);

	V2 position = v2(windowWidth * 0.5f, 157.0f);
	f32 maxLabelWidth = windowWidth - 256;

	//drawGL_TextureAtlasItem(renderer, true, GL_TextureAtlasItem_Menu_Logo, position, v2(499.0f, 154.0f), 0);
	//position.y += 154.0f;

	UILabelStyle *labelStyle = &theme->labelStyle;
	BitmapFont *font = getFont(assets, labelStyle->font);

	position.y += (uiText(uiState, font, LocalString("City Builder Thing"),
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, labelStyle->textColor, maxLabelWidth)).h;

	position.y += (uiText(uiState, font, LocalString("Very much a work in progress!"),
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, labelStyle->textColor, maxLabelWidth)).h;

	Rect2 buttonRect = rectXYWH(position.x - (80/2), position.y + 32, 80, 24);
	if (uiButton(uiState, inputState, LocalString("Play"), buttonRect, 1)) // , SDLK_RETURN
	{
		result = AppStatus_Game;
	}
	buttonRect.y += 32;
	if (uiButton(uiState, inputState, LocalString("Credits"), buttonRect, 1))
	{
		result = AppStatus_Credits;
	}
	buttonRect.y += 32;
	if (uiButton(uiState, inputState, LocalString("Settings"), buttonRect, 1))
	{
		result = AppStatus_SettingsMenu;
	}
	buttonRect.y += 32;
	if (uiButton(uiState, inputState, LocalString("Create a window"), buttonRect, 1))
	{
		s32 *aNumber = new s32;
		*aNumber = randomInRange(&globalAppState.cosmeticRandom, INT32_MAX);
		showWindow(uiState, LocalString("Hello window!"), 200, -1, testWindowProc, aNumber);
	}
	buttonRect.y += 32;
	if (uiButton(uiState, inputState, LocalString("Website"), buttonRect, 1))
	{
		openUrlUnsafe("http://samatkins.co.uk");
	}
	buttonRect.y += 32;
	if (uiButton(uiState, inputState, LocalString("Exit"), buttonRect, 1))
	{
		result = AppStatus_Quit;
	}

	appState->appStatus = result;

	//
	//
	// Temporary window testing code!!!!
	//
	//

	updateAndRenderWindows(uiState, inputState);
}
