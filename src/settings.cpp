#pragma once

void registerSetting(String settingName, smm offset, Type type, s32 count, String textAssetName)
{
	SettingDef def = {};
	def.name = settingName;
	def.textAssetName = textAssetName;
	def.offsetWithinSettingsState = offset;
	def.type = type;
	def.count = count;

	settings->defs.put(settingName, def);
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

		Maybe<SettingDef *> maybeDef = settings->defs.find(settingName);

		if (!maybeDef.isValid)
		{
			error(&reader, "Unrecognized setting: {0}"_s, {settingName});
		}
		else
		{
			SettingDef *def = maybeDef.value;
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

	for (auto it = settings->defs.iterate();
		it.hasNext();
		it.next())
	{
		auto entry = it.getEntry();
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

void showSettingsWindow()
{
	settings->workingState = settings->settings;

	UI::showWindow(getText("title_settings"_s), 600, 200, v2i(0,0), "default"_s, WinFlag_Unique|WinFlag_AutomaticHeight|WinFlag_Modal, settingsWindowProc);
}

void settingsWindowProc(WindowContext *context, void*)
{
	UIPanel *ui = &context->windowPanel;

	for (auto it = settings->defs.iterate();
		it.hasNext();
		it.next())
	{
		auto entry = it.getEntry();
		String name = entry->key;
		SettingDef *def = &entry->value;

		ui->startNewLine(ALIGN_LEFT);
		ui->addText(name);
		ui->addText(settingToString(&settings->workingState, def));
	}

	ui->startNewLine(ALIGN_LEFT);
	if (ui->addButton(getText("button_cancel"_s)))
	{
		context->closeRequested = true;
	}
	if (ui->addButton(getText("button_restore_defaults"_s)))
	{
		settings->workingState = makeDefaultSettings();
	}
	if (ui->addButton(getText("button_save"_s)))
	{
		settings->settings = settings->workingState;
		saveSettings();
		applySettings();
		context->closeRequested = true;
	}
}
