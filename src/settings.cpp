#pragma once

inline Settings defaultSettings()
{
	Settings result = {};
	result.windowed = true;
	result.windowWidth = 1280;
	result.windowHeight = 1024;

	return result;
}

void loadSettings(Settings *settings, AssetManager *assets, File file)
{
	*settings = defaultSettings();

	LineReader reader = startFile(file);

	while (reader.pos < reader.file.length)
	{
		String line = nextLine(&reader);
		String command;
		String remainder;

		command = nextToken(line, &remainder);

		if (equals(command, "resolution"))
		{
			s64 w;
			s64 h;

			if (asInt(nextToken(remainder, &remainder), &w)
				&& asInt(nextToken(remainder, &remainder), &h))
			{
				settings->windowWidth = (s32) w;
				settings->windowHeight = (s32) h;
			}
			else
			{
				error(&reader, "Couldn't parse window size. Expected format: \"{0} 1024 768\"", {command});
			}
		}
		else if (equals(command, "windowed"))
		{
			settings->windowed = readBool(&reader, command, remainder);
		}
		else
		{
			error(&reader, "Unrecognized command: {0}", {command});
		}
	}

	logInfo("Settings loaded: windowed={0}, resolution={1}x{2}", {formatBool(settings->windowed), formatInt(settings->windowWidth), formatInt(settings->windowHeight)});
}

void applySettings(Settings *settings)
{
	resizeWindow(globalAppState.renderer, settings->windowWidth, settings->windowHeight, !settings->windowed);
}

void updateAndRenderSettingsMenu(AppState *appState, InputState *inputState, Renderer *renderer, AssetManager *assets)
{
	AppStatus result = appState->appStatus;

	RenderBuffer *uiBuffer = &renderer->uiBuffer;
	f32 windowWidth = (f32) uiBuffer->camera.size.x;
	f32 windowHeight = (f32) uiBuffer->camera.size.y;
	UIState *uiState = &appState->uiState;
	uiState->mouseInputHandled = false;

	V2 position = v2(windowWidth * 0.5f, 157.0f);
	f32 maxLabelWidth = windowWidth - 256;

	UILabelStyle *labelStyle = findLabelStyle(assets, stringFromChars("title"));
	BitmapFont *font = getFont(assets, labelStyle->fontID);

	position.y += (uiText(uiState, font, LocalString("Settings"),
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, labelStyle->textColor, maxLabelWidth)).h;

	position.y += (uiText(uiState, font, LocalString("There are no settings yet, soz."),
			position, ALIGN_H_CENTRE | ALIGN_TOP, 1, labelStyle->textColor, maxLabelWidth)).h;

	f32 uiBorderPadding = 4;
	Rect2 buttonRect = rectXYWH(uiBorderPadding, windowHeight - uiBorderPadding - 24, 80, 24);
	if (uiButton(uiState, LocalString("Back"), buttonRect, 1, false, SDLK_ESCAPE))
	{
		result = AppStatus_MainMenu;
	}

	appState->appStatus = result;
}
