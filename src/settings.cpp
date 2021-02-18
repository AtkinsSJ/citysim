#pragma once

void registerSetting(String settingName, smm offset, Type type, s32 count, String textAssetName)
{
	SettingDef def = {};
	def.name = settingName;
	def.textAssetName = textAssetName;
	def.offsetWithinSettingsState = offset;
	def.type = type;
	def.count = count;

	put(&settings->defs, settingName, def);
}

SettingsState makeDefaultSettings()
{
	SettingsState result = {};

	result.windowed = true;
	result.resolution = v2i(1024, 600);
	result.locale = "en"_s;

	return result;
}

void initSettings()
{
	bootstrapArena(Settings, settings, settingsArena);
	initHashTable(&settings->defs);

	settings->userDataPath = makeString(SDL_GetPrefPath("Baffled Badger Games", "CitySim"));
	settings->userSettingsFilename = "settings.cnf"_s;

#define REGISTER_SETTING(settingName, type, count) registerSetting(makeString(#settingName), offsetof(SettingsState, settingName), Type_##type, count, makeString("setting_" #settingName))

	REGISTER_SETTING(windowed,   bool,   1);
	REGISTER_SETTING(resolution, s32,    2);
	REGISTER_SETTING(locale,     String, 1);

#undef REGISTER_SETTING
}

inline String getUserDataPath()
{
	return settings->userDataPath;
}

inline String getUserSettingsPath()
{
	return myprintf("{0}{1}"_s, {settings->userDataPath, settings->userSettingsFilename}, true);
}

void loadSettingsFile(String name, Blob settingsData)
{
	LineReader reader = readLines(name, settingsData);

	while (loadNextLine(&reader))
	{
		String settingName = readToken(&reader, '=');

		SettingDef *def = find(&settings->defs, settingName);

		if (def == null)
		{
			error(&reader, "Unrecognized setting: {0}"_s, {settingName});
		}
		else
		{
			u8* firstItem = ((u8*)(&settings->settings)) + def->offsetWithinSettingsState;

			for (s32 i=0; i < def->count; i++)
			{
				switch (def->type)
				{
					case Type_bool:
					{
						Maybe<bool> value = readBool(&reader);
						if (value.isValid)
						{
							((bool *)firstItem)[i] = value.value;
						}
					} break;

					case Type_s32:
					{
						Maybe<s32> value = readInt<s32>(&reader);
						if (value.isValid)
						{
							((s32 *)firstItem)[i] = value.value;
						}
					} break;

					case Type_String:
					{
						((String *)firstItem)[i] = pushString(&settings->settingsArena, readToken(&reader));
					} break;

					default: ASSERT(false); //Unhandled setting type!
				}
			}
		}
	}
}

void loadSettings()
{
	resetMemoryArena(&settings->settingsArena);
	settings->settings = makeDefaultSettings();

	File userSettingsFile = readFile(tempArena, getUserSettingsPath());
	// User settings might not exist
	if (userSettingsFile.isLoaded)
	{
		loadSettingsFile(userSettingsFile.name, userSettingsFile.data);
	}

	logInfo("Settings loaded: windowed={0}, resolution={1}x{2}, locale={3}"_s, {formatBool(settings->settings.windowed), formatInt(settings->settings.resolution.x), formatInt(settings->settings.resolution.y), settings->settings.locale});
}

void applySettings()
{
	resizeWindow(settings->settings.resolution.x, settings->settings.resolution.y, !settings->settings.windowed);

	reloadLocaleSpecificAssets();
}

String settingToString(SettingsState *state, SettingDef *def)
{
	StringBuilder stb = newStringBuilder(256);

	u8* firstItem = ((u8*) state) + def->offsetWithinSettingsState;

	for (s32 i=0; i < def->count; i++)
	{
		if (i > 0) append(&stb, ' ');

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

	return getString(&stb);
}

void saveSettings()
{
	StringBuilder stb = newStringBuilder(2048);
	append(&stb, "# User-specific settings file.\n#\n"_s);
	append(&stb, "# I don't recommend fiddling with this manually, but it should work.\n"_s);
	append(&stb, "# If the game won't run, try deleting this file, and it should be re-generated with the default settings.\n\n"_s);

	for (auto it = iterate(&settings->defs);
		hasNext(&it);
		next(&it))
	{
		auto entry = getEntry(&it);
		String name = entry->key;
		SettingDef *def = &entry->value;

		append(&stb, name);
		append(&stb, " = "_s);
		append(&stb, settingToString(&settings->settings, def));
		append(&stb, '\n');
	}

	String userSettingsPath = getUserSettingsPath();
	String fileData = getString(&stb);

	if (writeFile(userSettingsPath, fileData))
	{
		logInfo("Settings saved successfully."_s);
	}
	else
	{
		// TODO: Really really really need to display an error window to the user!
		logError("Failed to save user settings to {0}."_s, {userSettingsPath});
	}
}

WindowSettings getWindowSettings()
{
	WindowSettings result = {};
	result.width      = settings->settings.resolution.x;
	result.height     = settings->settings.resolution.y;
	result.isWindowed = settings->settings.windowed;

	return result;
}

String getLocale()
{
	return settings->settings.locale;
}

void initSettingsMenu()
{
	settings->workingState = settings->settings;
}

AppStatus updateAndRenderSettingsMenu(UIState *uiState, f32 /*deltaTime*/)
{
	AppStatus result = AppStatus_SettingsMenu;

	s32 windowWidth  = round_s32(renderer->uiCamera.size.x);
	s32 windowHeight = round_s32(renderer->uiCamera.size.y);

	s32 maxLabelWidth = windowWidth - 256;

	UILabelStyle *labelStyle = findLabelStyle(assets->theme, "title"_s);
	BitmapFont *font = getFont(&labelStyle->font);

	V2I titlePos = v2i(windowWidth / 2, 157);
	uiText(&renderer->uiBuffer, font, getText("title_settings"_s),
			titlePos, ALIGN_H_CENTRE | ALIGN_TOP, labelStyle->textColor, maxLabelWidth);

	// Settings go here!!!
	s32 windowCentre = windowWidth / 2;
	s32 settingsAreaWidth = 600;
	V2I labelPos   = v2i(windowCentre - (settingsAreaWidth / 2), 300);
	V2I settingPos = v2i(windowCentre + (settingsAreaWidth / 2), 300);
	for (auto it = iterate(&settings->defs);
		hasNext(&it);
		next(&it))
	{
		SettingDef *def = get(&it);

		uiText(&renderer->uiBuffer, font, getText(def->textAssetName), labelPos, ALIGN_LEFT | ALIGN_TOP, labelStyle->textColor, settingsAreaWidth);
		uiText(&renderer->uiBuffer, font, settingToString(&settings->workingState, def), settingPos, ALIGN_RIGHT | ALIGN_TOP, labelStyle->textColor, settingsAreaWidth);

		labelPos.y += 60;
		settingPos.y += 60;
	}

	UIButtonStyle *style = findButtonStyle(assets->theme, "default"_s);
	s32 uiBorderPadding = 8;
	String backText = getText("button_back"_s);
	V2I backSize = calculateButtonSize(backText, style);
	Rect2I buttonRect = irectXYWH(uiBorderPadding, windowHeight - uiBorderPadding - backSize.y, backSize.x, backSize.y);
	if (uiButton(uiState, backText, buttonRect, style, Button_Normal, SDLK_ESCAPE))
	{
		result = AppStatus_MainMenu;
	}

	return result;
}
