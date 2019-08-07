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
	// You shouldn't access these directly! Use the getters below.
	bool windowed;
	V2I resolution;
	String locale;
};

//
// PUBLIC
//
void initSettings(MemoryArena *arena);
void loadSettings();
void applySettings();
void saveSettings();

struct WindowSettings
{
	s32 width;
	s32 height;
	bool isWindowed;
};
WindowSettings getWindowSettings();

String getLocale();

AppStatus updateAndRenderSettingsMenu(struct UIState *uiState);

//
// INTERNAL
//
void registerSetting(String settingName, smm offset, Type type, s32 count);
void loadDefaultSettings();
void loadSettingsFile(String name, Blob settingsData);
String getUserSettingsPath();
