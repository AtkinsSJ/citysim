/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "Setting.h"
#include <Settings/SettingsChangeListener.h>
#include <Settings/SettingsState.h>
#include <UI/Forward.h>

class Settings {
public:
    static Settings& the();

    template<typename T>
    static Settings& initialize()
    {
        auto& settings = initialize_internal();
        settings.settings = settings.arena.allocate<T>(settings.arena);
        settings.workingState = settings.arena.allocate<T>(settings.arena);
        settings.m_listeners = { settings.arena, 32 };
        settings.arena.mark_reset_position();

        settings.load();

        return settings;
    }

    void load();
    bool save();
    void apply();

    template<typename T>
    Optional<T> get_typed_setting(String const& name) const
    {
        return settings->get_typed_setting<T>(name);
    }

    MemoryArena arena;

    // FIXME: These are only pointers because of arena bootstrapping.
    SettingsState* settings;
    // FIXME: Replace this with storage in the settings window
    SettingsState* workingState;

    void register_listener(SettingsChangeListener&);
    void unregister_listener(SettingsChangeListener&);

private:
    static Settings& initialize_internal();

    ChunkedArray<Ref<SettingsChangeListener>> m_listeners {};
};

// Menu
void showSettingsWindow();
void settingsWindowProc(UI::WindowContext*, void*);
