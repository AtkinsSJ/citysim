#pragma once

String getEnumDisplayName(SettingEnumData *data)
{
	return getText(data->displayName);
}

void registerSetting(String settingName, smm offset, SettingType type, String textAssetName, void *dataA, void *dataB)
{
	SettingDef def = {};
	def.name = settingName;
	def.textAssetName = textAssetName;
	def.offsetWithinSettingsState = offset;
	def.type = type;

	switch (type)
	{
		case SettingType::Enum: {
			def.enumData = (Array<SettingEnumData>*) dataA;
			ASSERT(dataB == null);
		} break;

		case SettingType::S32_Range: {
			def.intRange.min = truncate<s32>((s64)dataA);
			def.intRange.max = truncate<s32>((s64)dataB);
			ASSERT(dataA < dataB);
		} break;

		default: {
			def.dataA = dataA;
			def.dataB = dataB;
		} break;
	}

	settings->defs.put(settingName, def);
	settings->defsOrder.append(settingName);
}

SettingsState makeDefaultSettings()
{
	SettingsState result = {};

	result.windowed = true;
	result.resolution = v2i(1024, 600);
	result.locale = Locale_en;
	result.musicVolume = 0.5f;
	result.soundVolume = 0.5f;

	return result;
}

void initSettings()
{
	bootstrapArena(Settings, settings, settingsArena);
	initHashTable(&settings->defs);
	initChunkedArray(&settings->defsOrder, &settings->settingsArena, 32);
	markResetPosition(&settings->settingsArena);

	settings->userDataPath = makeString(SDL_GetPrefPath("Baffled Badger Games", "CitySim"));
	settings->userSettingsFilename = "settings.cnf"_s;

#define REGISTER_SETTING(settingName, type, ...) registerSetting(makeString(#settingName, true), offsetof(SettingsState, settingName), type, makeString("setting_" #settingName), __VA_ARGS__)

	// NB: The settings will appear in this order in the settings window
	REGISTER_SETTING(windowed,		SettingType::Bool);
	REGISTER_SETTING(resolution,	SettingType::V2I);
	REGISTER_SETTING(locale,		SettingType::Enum, &localeData);
	REGISTER_SETTING(musicVolume,	SettingType::Percent);
	REGISTER_SETTING(soundVolume,	SettingType::Percent);
	REGISTER_SETTING(widgetCount,	SettingType::S32_Range, (void*)5, (void*)15);

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

			switch (def->type)
			{
				case SettingType::Bool:
				{
					Maybe<bool> value = readBool(&reader);
					if (value.isValid)
					{
						setSettingData<bool>(&settings->settings, def, value.value);
					}
				} break;

				case SettingType::Enum:
				{
					String token = readToken(&reader);
					// Look it up in the enum data
					Array<SettingEnumData> enumData = *def->enumData;
					bool foundValue = false;
					for (s32 enumValueIndex = 0; enumValueIndex < enumData.count; enumValueIndex++)
					{
						if (equals(enumData[enumValueIndex].id, token))
						{
							setSettingData<s32>(&settings->settings, def, enumValueIndex);

							foundValue = true;
							break;
						}
					}

					if (!foundValue)
					{
						error(&reader, "Couldn't find '{0}' in the list of valid values for setting '{1}'."_s, {token, def->name});
					}

				} break;

				case SettingType::Percent:
				{
					Maybe<f64> value = readFloat(&reader);
					if (value.isValid)
					{
						f32 clampedValue = clamp01((f32)value.value);
						setSettingData<f32>(&settings->settings, def, clampedValue);
					}
				} break;

				case SettingType::S32:
				{
					Maybe<s32> value = readInt<s32>(&reader);
					if (value.isValid)
					{
						setSettingData<s32>(&settings->settings, def, value.value);
					}
				} break;

				case SettingType::S32_Range:
				{
					Maybe<s32> value = readInt<s32>(&reader);
					if (value.isValid)
					{
						s32 clampedValue = clamp(value.value, def->intRange.min, def->intRange.max);
						setSettingData<s32>(&settings->settings, def, clampedValue);
					}
				} break;

				case SettingType::String:
				{
					String value = pushString(&settings->settingsArena, readToken(&reader));
					setSettingData<String>(&settings->settings, def, value);
				} break;

				case SettingType::V2I:
				{
					Maybe<V2I> value = readV2I(&reader);
					if (value.isValid)
					{
						setSettingData<V2I>(&settings->settings, def, value.value);
					}
				} break;

				default: ASSERT(false); //Unhandled setting type!
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

	logInfo("Settings loaded: windowed={0}, resolution={1}x{2}, locale={3}"_s, {formatBool(settings->settings.windowed), formatInt(settings->settings.resolution.x), formatInt(settings->settings.resolution.y), localeData[settings->settings.locale].id});
}

void applySettings()
{
	resizeWindow(settings->settings.resolution.x, settings->settings.resolution.y, !settings->settings.windowed);

	reloadLocaleSpecificAssets();
}

String settingToString(SettingsState *state, SettingDef *def)
{
	StringBuilder stb = newStringBuilder(256);

	switch (def->type)
	{
		case SettingType::Bool:
		{
			append(&stb, formatBool(getSettingData<bool>(state, def)));
		} break;

		case SettingType::Enum:
		{
			s32 enumValueIndex = getSettingData<s32>(state, def);
			append(&stb, (*def->enumData)[enumValueIndex].id);
		} break;

		case SettingType::Percent:
		{
			append(&stb, formatFloat(getSettingData<f32>(state, def), 2));
		} break;

		case SettingType::S32:
		case SettingType::S32_Range:
		{
			append(&stb, formatInt(getSettingData<s32>(state, def)));
		} break;

		case SettingType::String:
		{
			append(&stb, getSettingData<String>(state, def));
		} break;

		case SettingType::V2I:
		{
			V2I value = getSettingData<V2I>(state, def);
			append(&stb, formatInt(value.x));
			append(&stb, 'x');
			append(&stb, formatInt(value.y));
		} break;

		default: ASSERT(false); //Unhandled setting type!
	}

	return getString(&stb);
}

void saveSettings()
{
	StringBuilder stb = newStringBuilder(2048);
	append(&stb, "# User-specific settings file.\n#\n"_s);
	append(&stb, "# I don't recommend fiddling with this manually, but it should work.\n"_s);
	append(&stb, "# If the game won't run, try deleting this file, and it should be re-generated with the default settings.\n\n"_s);

	for (auto it = settings->defsOrder.iterate();
		it.hasNext();
		it.next())
	{
		String name = it.getValue();
		SettingDef *def = settings->defs.find(name).value;

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
	return localeData[settings->settings.locale].id;
}

void showSettingsWindow()
{
	settings->workingState = settings->settings;

	UI::showWindow(getText("title_settings"_s), 480, 200, v2i(0,0), "default"_s, WindowFlags::Unique|WindowFlags::AutomaticHeight|WindowFlags::Modal, settingsWindowProc);
}

void settingsWindowProc(UI::WindowContext *context, void*)
{
	UI::Panel *ui = &context->windowPanel;

	for (auto it = settings->defsOrder.iterate();
		it.hasNext();
		it.next())
	{
		String name = it.getValue();
		SettingDef *def = settings->defs.find(name).value;

		ui->startNewLine(ALIGN_LEFT);
		ui->addText(getText(def->textAssetName));
		
		ui->alignWidgets(ALIGN_RIGHT);
		switch (def->type)
		{
			case SettingType::Bool: {
				ui->addCheckbox(getSettingDataRaw<bool>(&settings->workingState, def));
			} break;

			case SettingType::Enum: {
				// ui->addDropDownList(def->enumData, getSettingDataRaw<s32>(&settings->workingState, def), getEnumDisplayName);
				ui->alignWidgets(ALIGN_LEFT);
				ui->addRadioButtonGroup(def->enumData, getSettingDataRaw<s32>(&settings->workingState, def), getEnumDisplayName);
			} break;

			case SettingType::Percent: {
				ui->addSlider(getSettingDataRaw<f32>(&settings->workingState, def), 0.0f, 1.0f);
			} break;

			case SettingType::S32_Range: {
				ui->addSlider(getSettingDataRaw<s32>(&settings->workingState, def), def->intRange.min, def->intRange.max);
			} break;

			default: {
				ui->addText(settingToString(&settings->workingState, def));
			} break;
		}
	}

	ui->startNewLine(ALIGN_LEFT);
	if (ui->addTextButton(getText("button_cancel"_s)))
	{
		context->closeRequested = true;
	}
	if (ui->addTextButton(getText("button_restore_defaults"_s)))
	{
		settings->workingState = makeDefaultSettings();
	}
	if (ui->addTextButton(getText("button_save"_s)))
	{
		settings->settings = settings->workingState;
		saveSettings();
		applySettings();
		context->closeRequested = true;
	}
}

template <typename T>
T *getSettingDataRaw(SettingsState *state, SettingDef *def)
{
	// Make sure the requested type it the right one
	switch (def->type)
	{
		case SettingType::Bool: 		ASSERT(typeid(T*) == typeid(bool*));	break;
		case SettingType::Enum: 		ASSERT(typeid(T*) == typeid(s32*));		break;
		case SettingType::Percent: 		ASSERT(typeid(T*) == typeid(f32*));		break;
		case SettingType::S32: 			ASSERT(typeid(T*) == typeid(s32*));		break;
		case SettingType::S32_Range: 	ASSERT(typeid(T*) == typeid(s32*));		break;
		case SettingType::String: 		ASSERT(typeid(T*) == typeid(String*));	break;
		case SettingType::V2I: 			ASSERT(typeid(T*) == typeid(V2I*));		break;
		INVALID_DEFAULT_CASE;
	}

	T *firstItem = (T*)((u8*)(state) + def->offsetWithinSettingsState);
	return firstItem;
}

template <typename T>
inline T getSettingData(SettingsState *state, SettingDef *def)
{
	return *getSettingDataRaw<T>(state, def);
}

template <typename T>
inline void setSettingData(SettingsState *state, SettingDef *def, T value)
{
	*getSettingDataRaw<T>(state, def) = value;
}
