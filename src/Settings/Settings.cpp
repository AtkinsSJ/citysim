/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Settings.h"
#include <Assets/AssetManager.h>
#include <Gfx/Renderer.h>
#include <IO/LineReader.h>
#include <SDL2/SDL_filesystem.h>
#include <UI/Window.h>
#include <Util/StringBuilder.h>

static Settings* s_settings;

String getEnumDisplayName(SettingEnumData* data)
{
    return getText(data->displayName);
}

void Settings::register_setting(String setting_name, smm offset, SettingDef::Type type, String text_asset_name, void* data_a, void* data_b)
{
    SettingDef def {};
    def.name = setting_name;
    def.textAssetName = text_asset_name;
    def.offsetWithinSettingsState = offset;
    def.type = type;

    switch (type) {
    case SettingDef::Type::Enum: {
        ASSERT(data_b == nullptr);
        def.enumData = (Array<SettingEnumData>*)data_a;
    } break;

    case SettingDef::Type::S32_Range: {
        ASSERT(data_a < data_b);
        def.intRange.min = truncate<s32>((s64)data_a);
        def.intRange.max = truncate<s32>((s64)data_b);
    } break;

    default: {
        def.dataA = data_a;
        def.dataB = data_b;
    } break;
    }

    s_settings->defs.put(setting_name, def);
    s_settings->defsOrder.append(setting_name);
}

SettingsState SettingsState::make_default()
{
    SettingsState result = {};

    result.windowed = true;
    result.resolution = v2i(1024, 600);
    result.locale = Locale::En;
    result.musicVolume = 0.5f;
    result.soundVolume = 0.5f;

    return result;
}

Settings& Settings::the()
{
    return *s_settings;
}

void Settings::initialize()
{
    s_settings = MemoryArena::bootstrap<Settings>("Settings"_s);
    initHashTable(&s_settings->defs);
    initChunkedArray(&s_settings->defsOrder, &s_settings->arena, 32);
    s_settings->arena.mark_reset_position();

    s_settings->userDataPath = makeString(SDL_GetPrefPath("Baffled Badger Games", "CitySim"));
    s_settings->userSettingsFilename = "settings.cnf"_s;

#define REGISTER_SETTING(settingName, type, dataA, dataB) s_settings->register_setting(makeString(#settingName, true), offsetof(SettingsState, settingName), type, makeString("setting_" #settingName), dataA, dataB)

    // NB: The settings will appear in this order in the settings window
    REGISTER_SETTING(windowed, SettingDef::Type::Bool, nullptr, nullptr);
    REGISTER_SETTING(resolution, SettingDef::Type::V2I, nullptr, nullptr);
    REGISTER_SETTING(locale, SettingDef::Type::Enum, &localeData, nullptr);
    REGISTER_SETTING(musicVolume, SettingDef::Type::Percent, nullptr, nullptr);
    REGISTER_SETTING(soundVolume, SettingDef::Type::Percent, nullptr, nullptr);
    REGISTER_SETTING(widgetCount, SettingDef::Type::S32_Range, (void*)5, (void*)15);

#undef REGISTER_SETTING

    s_settings->load();
}

String getUserDataPath()
{
    return s_settings->userDataPath;
}

String getUserSettingsPath()
{
    return myprintf("{0}{1}"_s, { s_settings->userDataPath, s_settings->userSettingsFilename }, true);
}

void Settings::load_settings_from_file(String filename, Blob data)
{
    LineReader reader { filename, data };

    while (reader.load_next_line()) {
        String settingName = reader.next_token('=');

        Maybe<SettingDef*> maybeDef = defs.find(settingName);

        if (!maybeDef.isValid) {
            reader.error("Unrecognized setting: {0}"_s, { settingName });
        } else {
            maybeDef.value->set_from_file(settings, reader);
        }
    }
}

void Settings::load()
{
    arena.reset();
    settings = SettingsState::make_default();

    File userSettingsFile = readFile(&temp_arena(), getUserSettingsPath());
    // User settings might not exist
    if (userSettingsFile.isLoaded) {
        load_settings_from_file(userSettingsFile.name, userSettingsFile.data);
    }

    logInfo("Settings loaded: windowed={0}, resolution={1}x{2}, locale={3}"_s,
        {
            formatBool(settings.windowed),
            formatInt(settings.resolution.x),
            formatInt(settings.resolution.y),
            localeData[to_underlying(settings.locale)].id,
        });
}

void Settings::apply()
{
    // FIXME: Replace with callbacks

    the_renderer().resize_window(settings.resolution.x, settings.resolution.y, !settings.windowed);

    reloadLocaleSpecificAssets();
}

bool Settings::save()
{
    StringBuilder stb = newStringBuilder(2048);
    append(&stb, "# User-specific settings file.\n#\n"_s);
    append(&stb, "# I don't recommend fiddling with this manually, but it should work.\n"_s);
    append(&stb, "# If the game won't run, try deleting this file, and it should be re-generated with the default settings.\n\n"_s);

    for (auto it = defsOrder.iterate();
        it.hasNext();
        it.next()) {
        String name = it.getValue();
        SettingDef* def = defs.find(name).value;

        append(&stb, name);
        append(&stb, " = "_s);
        append(&stb, def->serialize(settings));
        append(&stb, '\n');
    }

    String userSettingsPath = getUserSettingsPath();
    String fileData = getString(&stb);

    if (writeFile(userSettingsPath, fileData)) {
        logInfo("Settings saved successfully."_s);
        return true;
    }

    // TODO: Really really really need to display an error window to the user!
    logError("Failed to save user settings to {0}."_s, { userSettingsPath });
    return false;
}

WindowSettings getWindowSettings()
{
    WindowSettings result = {};
    result.width = s_settings->settings.resolution.x;
    result.height = s_settings->settings.resolution.y;
    result.isWindowed = s_settings->settings.windowed;

    return result;
}

String getLocale()
{
    return localeData[to_underlying(s_settings->settings.locale)].id;
}

void showSettingsWindow()
{
    s_settings->workingState = s_settings->settings;

    UI::showWindow(UI::WindowTitle::fromTextAsset("title_settings"_s), 480, 200, v2i(0, 0), "default"_s, WindowFlags::Unique | WindowFlags::AutomaticHeight | WindowFlags::Modal, settingsWindowProc);
}

void settingsWindowProc(UI::WindowContext* context, void*)
{
    auto& settings = Settings::the();
    UI::Panel* ui = &context->windowPanel;

    for (auto it = settings.defsOrder.iterate();
        it.hasNext();
        it.next()) {
        String name = it.getValue();
        SettingDef* def = settings.defs.find(name).value;
        def->add_ui_widget(settings.workingState, *ui);
    }

    ui->startNewLine(HAlign::Left);
    if (ui->addTextButton(getText("button_cancel"_s))) {
        context->closeRequested = true;
    }
    if (ui->addTextButton(getText("button_restore_defaults"_s))) {
        settings.workingState = SettingsState::make_default();
    }
    if (ui->addTextButton(getText("button_save"_s))) {
        settings.settings = settings.workingState;
        settings.save();
        settings.apply();
        context->closeRequested = true;
    }
}

void SettingDef::add_ui_widget(SettingsState& state, UI::Panel& ui)
{
    ui.startNewLine(HAlign::Left);
    ui.addLabel(getText(textAssetName));

    ui.alignWidgets(HAlign::Right);

    switch (type) {
    case Type::Bool: {
        ui.addCheckbox(get_raw<bool>(state));
    } break;

    case Type::Enum: {
        // ui.addDropDownList(enumData, get_raw<s32>(state), getEnumDisplayName);
        ui.addRadioButtonGroup(enumData, get_raw<s32>(state), getEnumDisplayName);
    } break;

    case Type::Percent: {
        float* percent = get_raw<float>(state);
        s32 intPercent = round_s32(*percent * 100.0f);
        ui.addLabel(myprintf("{0}%"_s, { formatInt(intPercent) }));
        ui.addSlider(percent, 0.0f, 1.0f);
    } break;

    case Type::S32_Range: {
        s32* intValue = get_raw<s32>(state);
        ui.addLabel(formatInt(*intValue));
        ui.addSlider(intValue, intRange.min, intRange.max);
    } break;

    default: {
        ui.addLabel(serialize(state));
    } break;
    }
}

bool SettingDef::set_from_file(SettingsState& state, LineReader& reader)
{
    switch (type) {
    case Type::Bool:
        if (auto value = reader.read_bool(); value.has_value()) {
            set<bool>(state, value.release_value());
            return true;
        }
        return false;

    case Type::Enum: {
        String token = reader.next_token();
        // Look it up in the enum data
        for (s32 enumValueIndex = 0; enumValueIndex < enumData->count; enumValueIndex++) {
            if ((*enumData)[enumValueIndex].id == token) {
                set<s32>(state, enumValueIndex);
                return true;
            }
        }

        reader.error("Couldn't find '{0}' in the list of valid values for setting '{1}'."_s, { token, name });
        return false;
    }

    case Type::Percent:
        if (auto value = reader.read_float(); value.has_value()) {
            float clamped_value = clamp01(value.release_value());
            set<float>(state, clamped_value);
            return true;
        }
        return false;

    case Type::S32:
        if (auto value = reader.read_int<s32>(); value.has_value()) {
            set<s32>(state, value.release_value());
            return true;
        }
        return false;

    case Type::S32_Range:
        if (auto value = reader.read_int<s32>(); value.has_value()) {
            s32 clampedValue = clamp(value.release_value(), intRange.min, intRange.max);
            set<s32>(state, clampedValue);
            return true;
        }
        return false;

    case Type::String: {
        // FIXME: @Leak
        String value = pushString(&Settings::the().arena, reader.next_token());
        set<String>(state, value);
        return true;
    }

    case Type::V2I:
        if (auto value = V2I::read(reader); value.has_value()) {
            set<V2I>(state, value.release_value());
            return true;
        }
        return false;
    }

    VERIFY_NOT_REACHED();
}

String SettingDef::serialize(SettingsState const& state) const
{
    StringBuilder stb = newStringBuilder(256);

    switch (type) {
    case Type::Bool: {
        append(&stb, formatBool(get<bool>(state)));
    } break;

    case Type::Enum: {
        s32 enumValueIndex = get<s32>(state);
        append(&stb, (*enumData)[enumValueIndex].id);
    } break;

    case Type::Percent: {
        append(&stb, formatFloat(get<float>(state), 2));
    } break;

    case Type::S32:
    case Type::S32_Range: {
        append(&stb, formatInt(get<s32>(state)));
    } break;

    case Type::String: {
        append(&stb, get<String>(state));
    } break;

    case Type::V2I: {
        V2I value = get<V2I>(state);
        append(&stb, formatInt(value.x));
        append(&stb, 'x');
        append(&stb, formatInt(value.y));
    } break;
    }

    return getString(&stb);
}
