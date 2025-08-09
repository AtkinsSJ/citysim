/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "Setting.h"

#include <UI/Forward.h>
#include <Util/Array.h>
#include <Util/ChunkedArray.h>
#include <Util/HashTable.h>
#include <Util/String.h>

struct SettingsState;

struct SettingEnumData {
    String id;
    String displayName;
};
String getEnumDisplayName(SettingEnumData* data);

class SettingDef {
public:
    enum class Type : u8 {
        Bool,
        Enum,
        Percent,
        S32,
        S32_Range,
        String,
        V2I,
    };

    template<typename T>
    T get(SettingsState const& state) const
    {
        return *const_cast<SettingDef&>(*this).get_raw<T>(const_cast<SettingsState&>(state));
    }

    template<typename T>
    void set(SettingsState& state, T value)
    {
        *get_raw<T>(state) = value;
    }

    void add_ui_widget(SettingsState&, UI::Panel&);

    bool set_from_file(SettingsState&, LineReader&);
    String serialize(SettingsState const&) const;

    String name;
    String textAssetName;
    smm offsetWithinSettingsState;
    Type type;
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

private:
    // Grab a setting. index is for multi-value settings, to specify the array index
    template<typename T>
    T* get_raw(SettingsState& state)
    {
        // Make sure the requested type it the right one
        switch (type) {
        case Type::Bool:
            ASSERT(typeid(T*) == typeid(bool*));
            break;
        case Type::Enum:
            ASSERT(typeid(T*) == typeid(s32*));
            break;
        case Type::Percent:
            ASSERT(typeid(T*) == typeid(float*));
            break;
        case Type::S32:
            ASSERT(typeid(T*) == typeid(s32*));
            break;
        case Type::S32_Range:
            ASSERT(typeid(T*) == typeid(s32*));
            break;
        case Type::String:
            ASSERT(typeid(T*) == typeid(String*));
            break;
        case Type::V2I:
            ASSERT(typeid(T*) == typeid(V2I*));
            break;
            INVALID_DEFAULT_CASE;
        }

        T* firstItem = (T*)((u8*)(&state) + offsetWithinSettingsState);
        return firstItem;
    }
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

struct SettingsAgain {
    BoolSetting windowed { "windowed"_s, "setting_windowed"_s, true };
    V2ISetting resolution { "resolution"_s, "setting_resolution"_s, v2i(1024, 600) };
    EnumSetting<Locale> locale { "locale"_s, "setting_locale"_s,
        EnumMap<Locale, EnumSettingData> {
            { "en"_s, "locale_en"_s },
            { "es"_s, "locale_es"_s },
            { "pl"_s, "locale_pl"_s },
        },
        Locale::En };
    PercentSetting music_volume { "music_volume"_s, "setting_music_volume"_s, 0.5f };
    PercentSetting sound_volume { "sound_volume"_s, "setting_sound_volume"_s, 0.5f };
    S32RangeSetting widget_count { "widget_count"_s, "setting_widget_count"_s, 5, 15, 10 };
};

struct Settings {
public:
    static Settings& the();
    static void initialize();

    void load();
    bool save();
    void apply();

private:
    void register_setting(String setting_name, smm offset, SettingDef::Type type, String text_asset_name, void* data_a = nullptr, void* data_b = nullptr);

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

String getUserDataPath();
String getUserSettingsPath();
