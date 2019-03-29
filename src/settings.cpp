#pragma once

inline Settings defaultSettings()
{
	Settings result = {};
	result.windowed = true;
	result.resolution.x = 1024;
	result.resolution.y = 768;

	return result;
}

void registerSetting(Settings *settings, String settingName, smm offset, Type type, s32 count)
{
	SettingDef *def = appendBlank(&settings->defs);
	def->name = settingName;
	def->offset = offset;
	def->type = type;
	def->count = count;
}

void initSettings(Settings *settings)
{
	*settings = defaultSettings();
	initChunkedArray(&settings->defs, &globalAppState.systemArena, 64);

#define REGISTER_SETTING(settingName, type, count) registerSetting(settings, stringFromChars(#settingName), offsetof(Settings, settingName), Type_##type, count)

	REGISTER_SETTING(windowed,   bool, 1);
	REGISTER_SETTING(resolution, s32,  2);

#undef REGISTER_SETTING
}

void saveSettings(Settings *settings);

void loadSettings(Settings *settings, AssetManager *assets, File file)
{
	// todo: restore defaults here
	
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
				settings->resolution.x = (s32) w;
				settings->resolution.y = (s32) h;
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

	logInfo("Settings loaded: windowed={0}, resolution={1}x{2}", {formatBool(settings->windowed), formatInt(settings->resolution.x), formatInt(settings->resolution.y)});

	saveSettings(settings);
}

void applySettings(Settings *settings)
{
	resizeWindow(globalAppState.renderer, settings->resolution.x, settings->resolution.y, !settings->windowed);
}

void saveSettings(Settings *settings)
{
	// We have to decide at some point about how file-writing should work. Do we just pass a filename
	// into this function? Do we have a handle of some kind?
	// And then, how do we write out the settings in a way that's not 99 tons of boilerplate?
	// I guess it depends on what settings we even want, but if they're always a collection of N items
	// of the same type, we could do something like:
	//
	//     registerSetting("resolution", &settings->resolution, Type_S32, 2);
	//     registerSetting(...)
	//     registerSetting(...)
	//     registerSetting(...)
	//
	// ... with a table of some kind stored in the Settings struct (or elsewhere I guess) but in a way
	// that we run that code once on startup. Actually, we could use that for reading the settings too!
	// Might not even need to flatten things, if we use a pointer offset to access those fields instead
	// of doing so by name. OOOOOOH!
	// Another consideration: how do we store defaults in this system? Maybe a constant Settings struct
	// that has the default values, and we just copy from it? The offsets would be the same.

	logInfo("SAVING SETTINGS");

	u8* base = (u8*) settings;
	for (auto it = iterate(&settings->defs);
		!it.isDone;
		next(&it))
	{
		auto def = get(it);

		StringBuilder stb = newStringBuilder(1024);
		append(&stb, def->name);
		append(&stb, ' ');

		u8* firstItem = base + def->offset;

		for (s32 i=0; i < def->count; i++)
		{
			switch (def->type)
			{
				case Type_bool:
				{
					append(&stb, formatBool(((bool *)firstItem)[i]));
					append(&stb, ' ');
				} break;

				case Type_s32:
				{
					append(&stb, formatInt(((s32 *)firstItem)[i]));
					append(&stb, ' ');
				} break;

				default: ASSERT(false, "Unhandled setting type!");
			}
		}

		logInfo("Saving setting: {0}", {getString(&stb)});
	}
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
