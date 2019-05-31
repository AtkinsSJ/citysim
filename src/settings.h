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

void loadSettings(Settings *settings);
void applySettings(Settings *settings);
void saveSettings(Settings *settings);
