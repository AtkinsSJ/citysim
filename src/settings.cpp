#pragma once

void registerSetting(Settings *settings, String settingName, smm offset, Type type, s32 count)
{
	SettingDef def = {};
	def.name = settingName;
	def.offset = offset;
	def.type = type;
	def.count = count;

	put(&settings->defs, settingName, def);
}

void loadDefaultSettings(Settings *settings)
{
	settings->windowed = true;
	settings->resolution = v2i(1024, 768);
	settings->locale = makeString("en");
}

void initSettings(Settings *settings)
{
	*settings = {};
	initHashTable(&settings->defs);

	settings->userDataPath = makeString(SDL_GetPrefPath("Baffled Badger Games", "CitySim"));
	settings->userSettingsFilename = makeString("settings.cnf");
	settings->defaultSettingsFilename = makeString("default-settings.cnf");

#define REGISTER_SETTING(settingName, type, count) registerSetting(settings, makeString(#settingName), offsetof(Settings, settingName), Type_##type, count)

	REGISTER_SETTING(windowed,   bool,   1);
	REGISTER_SETTING(resolution, s32,    2);
	REGISTER_SETTING(locale,     String, 1);

#undef REGISTER_SETTING

	loadDefaultSettings(settings);
}

inline String getUserSettingsPath(Settings *settings)
{
	return myprintf("{0}{1}", {settings->userDataPath, settings->userSettingsFilename}, true);
}

void loadSettingsFile(Settings *settings, String name, Blob settingsData)
{
	LineReader reader = readLines(name, settingsData);

	while (!isDone(&reader))
	{
		String line = nextLine(&reader);
		if (isEmpty(line)) break; // Is this necessary? It shouldn't be!

		String settingName;
		String remainder;

		settingName = nextToken(line, &remainder, '=');

		SettingDef *def = find(&settings->defs, settingName);

		if (def == null)
		{
			error(&reader, "Unrecognized setting: {0}", {settingName});
		}
		else
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

					case Type_String:
					{
						((String *)firstItem)[i] = pushString(&globalAppState.systemArena, sValue);
					} break;

					default: ASSERT(false); //Unhandled setting type!
				}
			}
		}
	}
}

void loadSettings(Settings *settings)
{
	loadDefaultSettings(settings);

	File userSettingsFile = readFile(tempArena, getUserSettingsPath(settings));
	// User settings might not exist
	if (userSettingsFile.isLoaded)
	{
		loadSettingsFile(settings, userSettingsFile.name, userSettingsFile.data);
	}

	logInfo("Settings loaded: windowed={0}, resolution={1}x{2}, locale={3}", {formatBool(settings->windowed), formatInt(settings->resolution.x), formatInt(settings->resolution.y), settings->locale});
}

void applySettings(Settings *settings)
{
	resizeWindow(settings->resolution.x, settings->resolution.y, !settings->windowed);

	setLocale(settings->locale);
}

void saveSettings(Settings *settings)
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
		auto entry = getEntry(it);
		String name = entry->key;
		SettingDef *def = &entry->value;

		append(&stb, name);
		append(&stb, " = ");

		u8* firstItem = base + def->offset;

		for (s32 i=0; i < def->count; i++)
		{
			switch (def->type)
			{
				case Type_bool:
				{
					append(&stb, formatBool(((bool *)firstItem)[i]));
				} break;

				case Type_s32:
				{
					append(&stb, formatInt(((s32 *)firstItem)[i]));
				} break;

				case Type_String:
				{
					append(&stb, ((String *)firstItem)[i]);
				} break;

				default: ASSERT(false); //Unhandled setting type!
			}
		}

		append(&stb, '\n');
	}

	String userSettingsPath = getUserSettingsPath(settings);
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

AppStatus updateAndRenderSettingsMenu(UIState *uiState)
{
	AppStatus result = AppStatus_SettingsMenu;

	f32 windowWidth = (f32) renderer->uiCamera.size.x;
	f32 windowHeight = (f32) renderer->uiCamera.size.y;

	V2 position = v2(windowWidth * 0.5f, 157.0f);
	f32 maxLabelWidth = windowWidth - 256;

	UILabelStyle *labelStyle = findLabelStyle(&assets->theme, makeString("title"));
	BitmapFont *font = getFont(labelStyle->fontName);

	position.y += (uiText(&renderer->uiBuffer, font, LOCAL("title_settings"),
			position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;

	// position.y += (uiText(uiState, font, LocalString("There are no settings yet, soz."),
	// 		position, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth)).h;

	f32 uiBorderPadding = 4;
	Rect2 buttonRect = rectXYWH(uiBorderPadding, windowHeight - uiBorderPadding - 24, 80, 24);
	if (uiButton(uiState, LOCAL("button_back"), buttonRect, false, SDLK_ESCAPE))
	{
		result = AppStatus_MainMenu;
	}

	return result;
}
