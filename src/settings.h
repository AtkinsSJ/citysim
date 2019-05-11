#pragma once

enum Type
{
	Type_bool,
	Type_s32,
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

	bool windowed;
	V2I resolution;
};

struct AssetManager;

void loadSettings(Settings *settings, File defaultSettingsFile, File userSettingsFile);
void applySettings(Settings *settings);
void saveSettings(Settings *settings, AssetManager *assets);
