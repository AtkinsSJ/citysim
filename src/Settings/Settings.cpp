/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Settings.h"
#include <Assets/AssetManager.h>
#include <IO/Paths.h>
#include <UI/Window.h>
#include <Util/StringBuilder.h>

static Settings* s_settings;

Settings& Settings::the()
{
    return *s_settings;
}

Settings& Settings::initialize_internal()
{
    s_settings = MemoryArena::bootstrap<Settings>("Settings"_s);
    return *s_settings;
}

void Settings::load()
{
    arena.reset();
    settings->restore_default_values();

    File userSettingsFile = readFile(&temp_arena(), Paths::user_settings_file());
    // User settings might not exist
    if (userSettingsFile.isLoaded) {
        settings->load_from_file(userSettingsFile.name, userSettingsFile.data);
    }

    logInfo("Settings loaded."_s);
}

void Settings::apply()
{
    for (auto it = m_listeners.iterate(); it.hasNext(); it.next()) {
        it.getValue()->on_settings_changed(*settings);
    }
}

bool Settings::save()
{
    return settings->save_to_file(Paths::user_settings_file());
}

void Settings::register_listener(SettingsChangeListener& listener)
{
    m_listeners.append(listener);
    listener.on_settings_changed(*settings);
}

void Settings::unregister_listener(SettingsChangeListener& listener)
{
    m_listeners.findAndRemove(Ref { listener });
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
