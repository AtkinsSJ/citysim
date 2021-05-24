#pragma once

enum Type
{
	Type_bool,
	Type_enum,
	Type_s32,
	Type_String,
};

struct SettingEnumData
{
	String id;
	String displayName;
};
String getEnumDisplayName(SettingEnumData *data);

struct SettingDef
{
	String name;
	String textAssetName;
	smm offsetWithinSettingsState;
	Type type;
	s32 count;
	union {
		void *data;
		Array<SettingEnumData> *enumData;
	};
};

enum Locale
{
	Locale_en,
	Locale_es,
	Locale_pl,
	LocaleCount
};

SettingEnumData _localeData[LocaleCount] = {
	{"en"_s, "locale_en"_s},
	{"es"_s, "locale_es"_s},
	{"pl"_s, "locale_pl"_s},
};
Array<SettingEnumData> localeData = makeArray<SettingEnumData>(LocaleCount, _localeData, LocaleCount);

struct SettingsState
{
	bool windowed;
	V2I resolution;
	s32 locale; // Locale
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
void registerSetting(String settingName, smm offset, Type type, s32 count, String textAssetName, void *data = null);
SettingsState makeDefaultSettings();
void loadSettingsFile(String name, Blob settingsData);

// Grab a setting. index is for multi-value settings, to specify the array index
template <typename T>
T *getSettingDataRaw(SettingsState *state, SettingDef *def, s32 index = 0);
template <typename T>
T getSettingData(SettingsState *state, SettingDef *def, s32 index = 0);
template <typename T>
void setSettingData(SettingsState *state, SettingDef *def, T value, s32 index = 0);

String getUserDataPath();
String getUserSettingsPath();
// Essentially a "serialiser". Writes strings for use in the conf file; not for human consumption!
String settingToString(SettingsState *state, SettingDef *def);
