/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Settings.h"
#include <Assets/AssetManager.h>
#include <SDL2/SDL_filesystem.h>
#include <UI/Window.h>
#include <Util/StringBuilder.h>

static Settings* s_settings;

Settings& Settings::the()
{
    return *s_settings;
}

void Settings::initialize()
{
    s_settings = MemoryArena::bootstrap<Settings>("Settings"_s);
    s_settings->userDataPath = makeString(SDL_GetPrefPath("Baffled Badger Games", "CitySim"));
    s_settings->userSettingsFilename = "settings.cnf"_s;
    s_settings->settings = s_settings->arena.allocate<SettingsState>(s_settings->arena);
    s_settings->workingState = s_settings->arena.allocate<SettingsState>(s_settings->arena);
    initChunkedArray(&s_settings->m_listeners, &s_settings->arena, 32);
    s_settings->arena.mark_reset_position();

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

void Settings::load()
{
    arena.reset();
    settings->restore_default_values();

    File userSettingsFile = readFile(&temp_arena(), getUserSettingsPath());
    // User settings might not exist
    if (userSettingsFile.isLoaded) {
        settings->load_from_file(userSettingsFile.name, userSettingsFile.data);
    }

    logInfo("Settings loaded: windowed={0}, resolution={1}x{2}, locale={3}"_s,
        {
            formatBool(settings->windowed.value()),
            formatInt(settings->resolution.value().x),
            formatInt(settings->resolution.value().y),
            settings->locale.enum_data().id,
        });
}

void Settings::apply()
{
    for (auto it = m_listeners.iterate(); it.hasNext(); it.next()) {
        it.getValue()->on_settings_changed();
    }
}

bool Settings::save()
{
    return settings->save_to_file(getUserSettingsPath());
}

void Settings::register_listener(SettingsChangeListener& listener)
{
    m_listeners.append(listener);
}

void Settings::unregister_listener(SettingsChangeListener& listener)
{
    m_listeners.findAndRemove(Ref { listener });
}

WindowSettings getWindowSettings()
{
    WindowSettings result = {};
    result.width = s_settings->settings->resolution.value().x;
    result.height = s_settings->settings->resolution.value().y;
    result.isWindowed = s_settings->settings->windowed.value();

    return result;
}

Locale get_locale()
{
    return s_settings->settings->locale.enum_value();
}

void showSettingsWindow()
{
    s_settings->workingState->copy_from(*s_settings->settings);

    UI::showWindow(UI::WindowTitle::fromTextAsset("title_settings"_s), 480, 200, v2i(0, 0), "default"_s, WindowFlags::Unique | WindowFlags::AutomaticHeight | WindowFlags::Modal, settingsWindowProc);
}

void settingsWindowProc(UI::WindowContext* context, void*)
{
    auto& settings = Settings::the();
    UI::Panel& ui = context->windowPanel;

    settings.workingState->for_each_setting([&ui](auto& setting) {
        setting.add_ui_line(ui);
    });

    ui.startNewLine(HAlign::Left);
    if (ui.addTextButton(getText("button_cancel"_s))) {
        context->closeRequested = true;
    }
    if (ui.addTextButton(getText("button_restore_defaults"_s))) {
        settings.workingState->restore_default_values();
    }
    if (ui.addTextButton(getText("button_save"_s))) {
        settings.settings->copy_from(*settings.workingState);
        settings.save();
        settings.apply();
        context->closeRequested = true;
    }
}
