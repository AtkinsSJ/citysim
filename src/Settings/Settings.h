/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "Setting.h"
#include <Settings/SettingsChangeListener.h>
#include <Settings/SettingsState.h>
#include <UI/Forward.h>
#include <Util/String.h>

struct Settings {
public:
    static Settings& the();
    static void initialize();

    void load();
    bool save();
    void apply();

    MemoryArena arena;

    String userDataPath;
    String userSettingsFilename;

    // FIXME: These are only pointers because of arena bootstrapping.
    SettingsState* settings;
    // FIXME: Replace this with storage in the settings window
    SettingsState* workingState;

    void register_listener(SettingsChangeListener&);
    void unregister_listener(SettingsChangeListener&);

private:
    ChunkedArray<Ref<SettingsChangeListener>> m_listeners {};
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
