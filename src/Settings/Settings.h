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

enum class Locale : u8 {
    En,
    Es,
    Pl,
    COUNT
};

inline SettingEnumData _localeData[to_underlying(Locale::COUNT)] = {
    { "en"_s, "locale_en"_s },
    { "es"_s, "locale_es"_s },
    { "pl"_s, "locale_pl"_s },
};
inline Array<SettingEnumData> localeData = makeArray<SettingEnumData>(to_underlying(Locale::COUNT), _localeData, to_underlying(Locale::COUNT));

struct SettingsState {
    static SettingsState make_default();

    bool windowed;
    V2I resolution;
    Locale locale;
    float musicVolume;
    float soundVolume;
    s32 widgetCount;
};

struct Settings {
public:
    static Settings& the();
    static void initialize();

    void load();
    bool save();
    void apply();

private:
    void register_setting(String setting_name, smm offset, SettingType type, String text_asset_name, void* data_a = nullptr, void* data_b = nullptr);

    void load_settings_from_file(String filename, Blob data);

public:
    MemoryArena arena;
    HashTable<SettingDef> defs;
    ChunkedArray<String> defsOrder;

    String userDataPath;
    String userSettingsFilename;

    // The actual settings
    // You shouldn't access these directly! Use the getters below.
    SettingsState settings;

    // FIXME: Replace this with storage in the settings window
    SettingsState workingState; // Used in settings screen
};

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
        ASSERT(typeid(T*) == typeid(float*));
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
