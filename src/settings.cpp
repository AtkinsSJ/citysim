#pragma once

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
	*settings = {};
	initChunkedArray(&settings->defs, &globalAppState.systemArena, 64);

#define REGISTER_SETTING(settingName, type, count) registerSetting(settings, makeString(#settingName), offsetof(Settings, settingName), Type_##type, count)

	REGISTER_SETTING(windowed,   bool, 1);
	REGISTER_SETTING(resolution, s32,  2);

#undef REGISTER_SETTING
}

void loadSettingsFile(Settings *settings, File file)
{
	LineReader reader = startFile(file);

	while (!isDone(&reader))
	{
		String line = nextLine(&reader);
		if (line.length == 0) break;

		String settingName;
		String remainder;

		settingName = nextToken(line, &remainder, '=');

		bool foundIt = false;

		for (auto it = iterate(&settings->defs);
			!it.isDone;
			next(&it))
		{
			auto def = get(it);
			if (equals(def->name, settingName))
			{
				u8* firstItem = ((u8*)settings) + def->offset;

				for (s32 i=0; i < def->count; i++)
				{
					String sValue = nextToken(remainder, &remainder);
					switch (def->type)
					{
						case Type_bool:
						{
							bool value;
							if (asBool(sValue, &value))
							{
								((bool *)firstItem)[i] = value;
							}
							else
							{
								error(&reader, "Invalid value \"{0}\", expected true or false.", {sValue});
							}
						} break;

						case Type_s32:
						{
							s64 value;
							if (asInt(sValue, &value))
							{
								((s32 *)firstItem)[i] = (s32) value;
							}
							else
							{
								error(&reader, "Invalid value \"{0}\", expected integer.", {sValue});
							}
						} break;

						default: ASSERT(false, "Unhandled setting type!");
					}
				}

				foundIt = true;

				break;
			}
		}

		if (!foundIt)
		{
			error(&reader, "Unrecognized setting: {0}", {settingName});
		}
	}
}

void loadSettings(Settings *settings, File defaultSettingsFile, File userSettingsFile)
{
	loadSettingsFile(settings, defaultSettingsFile);

	// User settings might not exist
	if (userSettingsFile.isLoaded)
	{
		loadSettingsFile(settings, userSettingsFile);
	}

	logInfo("Settings loaded: windowed={0}, resolution={1}x{2}", {formatBool(settings->windowed), formatInt(settings->resolution.x), formatInt(settings->resolution.y)});
}

void applySettings(Settings *settings)
{
	resizeWindow(globalAppState.renderer, settings->resolution.x, settings->resolution.y, !settings->windowed);
}

void saveSettings(Settings *settings, AssetManager *assets)
{
	StringBuilder stb = newStringBuilder(2048);
	append(&stb, "# User-specific settings file.\n#\n");
	append(&stb, "# I don't recommend fiddling with this manually, but it should work.\n");
	append(&stb, "# If the game won't run, try deleting this file, and it should be re-generated with the default settings.\n\n");

	u8* base = (u8*) settings;
	for (auto it = iterate(&settings->defs);
		!it.isDone;
		next(&it))
	{
		auto def = get(it);

		append(&stb, def->name);
		append(&stb, " = ");

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

		append(&stb, '\n');
	}

	String userSettingsPath = getUserSettingsPath(assets);
	String fileData = getString(&stb);

	if (writeFile(userSettingsPath, fileData))
	{
		logInfo("Settings saved successfully.");
	}
	else
	{
		// TODO: Really really really need to display an error window to the user!
		logError("Failed to save user settings to {0}.", {userSettingsPath});
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

	UILabelStyle *labelStyle = findLabelStyle(assets, makeString("title"));
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
