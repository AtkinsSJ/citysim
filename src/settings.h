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
	ChunkedArray<SettingDef> defs;

	bool windowed;
	V2I resolution;
};

void loadSettings(Settings *settings, File defaultSettingsFile, File userSettingsFile);
void applySettings(Settings *settings);
