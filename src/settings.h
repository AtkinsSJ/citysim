#pragma once

enum Type
{
	Type_bool,
	Type_s32,
	Type_String,
};

struct SettingDef
{
	String name;
	smm offset;
	Type type;
	s32 count;
};

struct Settings
{
	HashTable<SettingDef> defs;

	String userDataPath;
	String userSettingsFilename;
	String defaultSettingsFilename;

	// The actual settings
	bool windowed;
	V2I resolution;
	String locale;
};

//
// PUBLIC
//
void initSettings(Settings *settings);
void loadSettings(Settings *settings);
void applySettings(Settings *settings);
void saveSettings(Settings *settings);

void updateAndRenderSettingsMenu(struct AppState *appState, struct Renderer *renderer, struct AssetManager *assets);

//
// INTERNAL
//
void registerSetting(Settings *settings, String settingName, smm offset, Type type, s32 count);
void loadDefaultSettings(Settings *settings);
void loadSettingsFile(Settings *settings, String name, Blob settingsData);
String getUserSettingsPath(Settings *settings);