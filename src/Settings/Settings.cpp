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

void registerSetting(String settingName, smm offset, SettingType type, String textAssetName, void* dataA, void* dataB)
{
    SettingDef def = {};
    def.name = settingName;
    def.textAssetName = textAssetName;
    def.offsetWithinSettingsState = offset;
    def.type = type;

    switch (type) {
    case SettingType::Enum: {
        def.enumData = (Array<SettingEnumData>*)dataA;
        ASSERT(dataB == nullptr);
    } break;

    case SettingType::S32_Range: {
        def.intRange.min = truncate<s32>((s64)dataA);
        def.intRange.max = truncate<s32>((s64)dataB);
        ASSERT(dataA < dataB);
    } break;

    default: {
        def.dataA = dataA;
        def.dataB = dataB;
    } break;
    }

    s_settings->defs.put(settingName, def);
    s_settings->defsOrder.append(settingName);
}

SettingsState makeDefaultSettings()
{
    SettingsState result = {};

    result.windowed = true;
    result.resolution = v2i(1024, 600);
    result.locale = Locale::En;
    result.musicVolume = 0.5f;
    result.soundVolume = 0.5f;

    return result;
}

Settings& settings()
{
    return *s_settings;
}

void initSettings()
{
    s_settings = MemoryArena::bootstrap<Settings>("Settings"_s);
    initHashTable(&s_settings->defs);
    initChunkedArray(&s_settings->defsOrder, &s_settings->arena, 32);
    s_settings->arena.mark_reset_position();

    s_settings->userDataPath = makeString(SDL_GetPrefPath("Baffled Badger Games", "CitySim"));
    s_settings->userSettingsFilename = "settings.cnf"_s;

#define REGISTER_SETTING(settingName, type, dataA, dataB) registerSetting(makeString(#settingName, true), offsetof(SettingsState, settingName), type, makeString("setting_" #settingName), dataA, dataB)

    // NB: The settings will appear in this order in the settings window
    REGISTER_SETTING(windowed, SettingType::Bool, nullptr, nullptr);
    REGISTER_SETTING(resolution, SettingType::V2I, nullptr, nullptr);
    REGISTER_SETTING(locale, SettingType::Enum, &localeData, nullptr);
    REGISTER_SETTING(musicVolume, SettingType::Percent, nullptr, nullptr);
    REGISTER_SETTING(soundVolume, SettingType::Percent, nullptr, nullptr);
    REGISTER_SETTING(widgetCount, SettingType::S32_Range, (void*)5, (void*)15);

#undef REGISTER_SETTING
}

String getUserDataPath()
{
    return s_settings->userDataPath;
}

String getUserSettingsPath()
{
    return myprintf("{0}{1}"_s, { s_settings->userDataPath, s_settings->userSettingsFilename }, true);
}

void loadSettingsFile(String name, Blob settingsData)
{
    LineReader reader { name, settingsData };

    while (reader.load_next_line()) {
        String settingName = reader.next_token('=');

        Maybe<SettingDef*> maybeDef = s_settings->defs.find(settingName);

        if (!maybeDef.isValid) {
            reader.error("Unrecognized setting: {0}"_s, { settingName });
        } else {
            SettingDef* def = maybeDef.value;

            switch (def->type) {
            case SettingType::Bool: {
                if (auto value = reader.read_bool(); value.has_value()) {
                    setSettingData<bool>(&s_settings->settings, def, value.release_value());
                }
            } break;

            case SettingType::Enum: {
                String token = reader.next_token();
                // Look it up in the enum data
                Array<SettingEnumData> enumData = *def->enumData;
                bool foundValue = false;
                for (s32 enumValueIndex = 0; enumValueIndex < enumData.count; enumValueIndex++) {
                    if (enumData[enumValueIndex].id == token) {
                        setSettingData<s32>(&s_settings->settings, def, enumValueIndex);

                        foundValue = true;
                        break;
                    }
                }

                if (!foundValue) {
                    reader.error("Couldn't find '{0}' in the list of valid values for setting '{1}'."_s, { token, def->name });
                }

            } break;

            case SettingType::Percent: {
                if (auto value = reader.read_float(); value.has_value()) {
                    float clamped_value = clamp01(value.release_value());
                    setSettingData<float>(&s_settings->settings, def, clamped_value);
                }
            } break;

            case SettingType::S32: {
                if (auto value = reader.read_int<s32>(); value.has_value()) {
                    setSettingData<s32>(&s_settings->settings, def, value.release_value());
                }
            } break;

            case SettingType::S32_Range: {
                if (auto value = reader.read_int<s32>(); value.has_value()) {
                    s32 clampedValue = clamp(value.release_value(), def->intRange.min, def->intRange.max);
                    setSettingData<s32>(&s_settings->settings, def, clampedValue);
                }
            } break;

            case SettingType::String: {
                String value = pushString(&s_settings->arena, reader.next_token());
                setSettingData<String>(&s_settings->settings, def, value);
            } break;

            case SettingType::V2I: {
                if (auto value = V2I::read(reader); value.has_value()) {
                    setSettingData<V2I>(&s_settings->settings, def, value.release_value());
                }
            } break;

            default:
                ASSERT(false); // Unhandled setting type!
            }
        }
    }
}

void loadSettings()
{
    s_settings->arena.reset();
    s_settings->settings = makeDefaultSettings();

    File userSettingsFile = readFile(&temp_arena(), getUserSettingsPath());
    // User settings might not exist
    if (userSettingsFile.isLoaded) {
        loadSettingsFile(userSettingsFile.name, userSettingsFile.data);
    }

    logInfo("Settings loaded: windowed={0}, resolution={1}x{2}, locale={3}"_s,
        {
            formatBool(s_settings->settings.windowed),
            formatInt(s_settings->settings.resolution.x),
            formatInt(s_settings->settings.resolution.y),
            localeData[to_underlying(s_settings->settings.locale)].id,
        });
}

void applySettings()
{
    the_renderer().resize_window(s_settings->settings.resolution.x, s_settings->settings.resolution.y, !s_settings->settings.windowed);

    reloadLocaleSpecificAssets();
}

String settingToString(SettingsState* state, SettingDef* def)
{
    StringBuilder stb = newStringBuilder(256);

    switch (def->type) {
    case SettingType::Bool: {
        append(&stb, formatBool(getSettingData<bool>(state, def)));
    } break;

    case SettingType::Enum: {
        s32 enumValueIndex = getSettingData<s32>(state, def);
        append(&stb, (*def->enumData)[enumValueIndex].id);
    } break;

    case SettingType::Percent: {
        append(&stb, formatFloat(getSettingData<float>(state, def), 2));
    } break;

    case SettingType::S32:
    case SettingType::S32_Range: {
        append(&stb, formatInt(getSettingData<s32>(state, def)));
    } break;

    case SettingType::String: {
        append(&stb, getSettingData<String>(state, def));
    } break;

    case SettingType::V2I: {
        V2I value = getSettingData<V2I>(state, def);
        append(&stb, formatInt(value.x));
        append(&stb, 'x');
        append(&stb, formatInt(value.y));
    } break;

    default:
        ASSERT(false); // Unhandled setting type!
    }

    return getString(&stb);
}

void saveSettings()
{
    StringBuilder stb = newStringBuilder(2048);
    append(&stb, "# User-specific settings file.\n#\n"_s);
    append(&stb, "# I don't recommend fiddling with this manually, but it should work.\n"_s);
    append(&stb, "# If the game won't run, try deleting this file, and it should be re-generated with the default settings.\n\n"_s);

    for (auto it = s_settings->defsOrder.iterate();
        it.hasNext();
        it.next()) {
        String name = it.getValue();
        SettingDef* def = s_settings->defs.find(name).value;

        append(&stb, name);
        append(&stb, " = "_s);
        append(&stb, settingToString(&s_settings->settings, def));
        append(&stb, '\n');
    }

    String userSettingsPath = getUserSettingsPath();
    String fileData = getString(&stb);

    if (writeFile(userSettingsPath, fileData)) {
        logInfo("Settings saved successfully."_s);
    } else {
        // TODO: Really really really need to display an error window to the user!
        logError("Failed to save user settings to {0}."_s, { userSettingsPath });
    }
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
    UI::Panel* ui = &context->windowPanel;

    for (auto it = s_settings->defsOrder.iterate();
        it.hasNext();
        it.next()) {
        String name = it.getValue();
        SettingDef* def = s_settings->defs.find(name).value;

        ui->startNewLine(HAlign::Left);
        ui->addLabel(getText(def->textAssetName));

        ui->alignWidgets(HAlign::Right);
        switch (def->type) {
        case SettingType::Bool: {
            ui->addCheckbox(getSettingDataRaw<bool>(&s_settings->workingState, def));
        } break;

        case SettingType::Enum: {
            // ui->addDropDownList(def->enumData, getSettingDataRaw<s32>(&settings->workingState, def), getEnumDisplayName);
            ui->addRadioButtonGroup(def->enumData, getSettingDataRaw<s32>(&s_settings->workingState, def), getEnumDisplayName);
        } break;

        case SettingType::Percent: {
            float* percent = getSettingDataRaw<float>(&s_settings->workingState, def);
            s32 intPercent = round_s32(*percent * 100.0f);
            ui->addLabel(myprintf("{0}%"_s, { formatInt(intPercent) }));
            ui->addSlider(percent, 0.0f, 1.0f);
        } break;

        case SettingType::S32_Range: {
            s32* intValue = getSettingDataRaw<s32>(&s_settings->workingState, def);
            ui->addLabel(formatInt(*intValue));
            ui->addSlider(intValue, def->intRange.min, def->intRange.max);
        } break;

        default: {
            ui->addLabel(settingToString(&s_settings->workingState, def));
        } break;
        }
    }

    ui->startNewLine(HAlign::Left);
    if (ui->addTextButton(getText("button_cancel"_s))) {
        context->closeRequested = true;
    }
    if (ui->addTextButton(getText("button_restore_defaults"_s))) {
        s_settings->workingState = makeDefaultSettings();
    }
    if (ui->addTextButton(getText("button_save"_s))) {
        s_settings->settings = s_settings->workingState;
        saveSettings();
        applySettings();
        context->closeRequested = true;
    }
}
