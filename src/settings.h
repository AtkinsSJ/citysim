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
	String textAssetName;
	smm offsetWithinSettingsState;
	Type type;
	s32 count;
};

struct SettingsState
{
	bool windowed;
	V2I resolution;
	String locale;
};

struct Settings
{
	MemoryArena settingsArena;
	HashTable<SettingDef> defs;

	String userDataPath;
	String userSettingsFilename;

	// The actual settings
	// You shouldn't access these directly! Use the getters below.
	SettingsState settings;

	SettingsState workingState; // Used in settings screen
};

//
// PUBLIC
//
void initSettings();
void loadSettings();
void applySettings();
void saveSettings();

// Settings access
struct WindowSettings
{
	s32 width;
	s32 height;
	bool isWindowed;
};
WindowSettings getWindowSettings();
String getLocale();

// Menu
void showSettingsWindow();
void settingsWindowProc(WindowContext*, void*);

//
// INTERNAL
//
void registerSetting(String settingName, smm offset, Type type, s32 count, String textAssetName);
SettingsState makeDefaultSettings();
void loadSettingsFile(String name, Blob settingsData);

String getUserDataPath();
String getUserSettingsPath();
String settingToString(SettingsState *state, SettingDef *def);
