/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <UI/Forward.h>
#include <Util/Array.h>
#include <Util/ChunkedArray.h>
#include <Util/HashTable.h>
#include <Util/String.h>

enum class SettingType : u8 {
    Bool,
    Enum,
    Percent,
    S32,
    S32_Range,
    String,
    V2I,
};

struct SettingEnumData {
    String id;
    String displayName;
};
String getEnumDisplayName(SettingEnumData* data);

struct SettingDef {
    String name;
    String textAssetName;
    smm offsetWithinSettingsState;
    SettingType type;
    union {
        struct {
            void* dataA;
            void* dataB;
        };

        Array<SettingEnumData>* enumData;
        struct {
            s32 min;
            s32 max;
        } intRange;
    };
};

enum Locale {
    Locale_en,
    Locale_es,
    Locale_pl,
    /*	Locale_pl1,
            Locale_pl2,
            Locale_pl3,
            Locale_pl4,
            Locale_pl5,
            Locale_pl6,
            Locale_pl7,
            Locale_pl8,
            Locale_pl9,
            Locale_pl11,
            Locale_pl12,
            Locale_pl13,
            Locale_pl14,
            Locale_pl15,
            Locale_pl16,
            Locale_pl17,
            Locale_pl18,
            Locale_pl19,
            Locale_pl21,
            Locale_pl22,
            Locale_pl23,
            Locale_pl24,
            Locale_pl25,
            Locale_pl26,
            Locale_pl27,
            Locale_pl28,
            Locale_pl29,
            Locale_pl31,
            Locale_pl32,
            Locale_pl33,
            Locale_pl34,
            Locale_pl35,
            Locale_pl36,
            Locale_pl37,
            Locale_pl38,
            Locale_pl39,
            Locale_pl41,
            Locale_pl42,
            Locale_pl43,
            Locale_pl44,
            Locale_pl45,
            Locale_pl46,
            Locale_pl47,
            Locale_pl48,
            Locale_pl49,
            Locale_pl51,
            Locale_pl52,
            Locale_pl53,
            Locale_pl54,
            Locale_pl55,
            Locale_pl56,
            Locale_pl57,
            Locale_pl58,
            Locale_pl59,*/
    LocaleCount
};

inline SettingEnumData _localeData[LocaleCount] = {
    { "en"_s, "locale_en"_s },
    { "es"_s, "locale_es"_s },
    { "pl"_s, "locale_pl"_s },
    /*	{"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},
            {"pl"_s, "locale_pl"_s},*/
};
inline Array<SettingEnumData> localeData = makeArray<SettingEnumData>(LocaleCount, _localeData, LocaleCount);

struct SettingsState {
    bool windowed;
    V2I resolution;
    s32 locale; // Locale
    f32 musicVolume;
    f32 soundVolume;
    s32 widgetCount;
};

struct Settings {
    MemoryArena arena;
    HashTable<SettingDef> defs;
    ChunkedArray<String> defsOrder;

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
Settings& settings();
void initSettings();
void loadSettings();
void applySettings();
void saveSettings();

// Settings access
struct WindowSettings {
    s32 width;
    s32 height;
    bool isWindowed;
};
WindowSettings getWindowSettings();
String getLocale();

// Menu
void showSettingsWindow();
void settingsWindowProc(UI::WindowContext*, void*);

//
// INTERNAL
//
void registerSetting(String settingName, smm offset, SettingType type, String textAssetName, void* dataA = nullptr, void* dataB = nullptr);
SettingsState makeDefaultSettings();
void loadSettingsFile(String name, Blob settingsData);

// Grab a setting. index is for multi-value settings, to specify the array index
template<typename T>
T* getSettingDataRaw(SettingsState* state, SettingDef* def)
{
    // Make sure the requested type it the right one
    switch (def->type) {
    case SettingType::Bool:
        ASSERT(typeid(T*) == typeid(bool*));
        break;
    case SettingType::Enum:
        ASSERT(typeid(T*) == typeid(s32*));
        break;
    case SettingType::Percent:
        ASSERT(typeid(T*) == typeid(f32*));
        break;
    case SettingType::S32:
        ASSERT(typeid(T*) == typeid(s32*));
        break;
    case SettingType::S32_Range:
        ASSERT(typeid(T*) == typeid(s32*));
        break;
    case SettingType::String:
        ASSERT(typeid(T*) == typeid(String*));
        break;
    case SettingType::V2I:
        ASSERT(typeid(T*) == typeid(V2I*));
        break;
        INVALID_DEFAULT_CASE;
    }

    T* firstItem = (T*)((u8*)(state) + def->offsetWithinSettingsState);
    return firstItem;
}

template<typename T>
T getSettingData(SettingsState* state, SettingDef* def)
{
    return *getSettingDataRaw<T>(state, def);
}

template<typename T>
void setSettingData(SettingsState* state, SettingDef* def, T value)
{
    *getSettingDataRaw<T>(state, def) = value;
}

String getUserDataPath();
String getUserSettingsPath();
// Essentially a "serialiser". Writes strings for use in the conf file; not for human consumption!
String settingToString(SettingsState* state, SettingDef* def);
