/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "SettingsState.h"
#include <Settings/Settings.h>
#include <Util/StringBuilder.h>

BaseSettingsState::BaseSettingsState(MemoryArena& arena)
{
    initChunkedArray(&m_settings_order, &arena, 32);
}

void BaseSettingsState::restore_default_values()
{
    for (auto it = m_settings_by_name.iterate();
        it.hasNext();
        it.next()) {
        auto entry = *it.get();
        entry->restore_default_value();
    }
}

void BaseSettingsState::copy_from(BaseSettingsState& other)
{
    for (auto it = m_settings_by_name.iterate();
        it.hasNext();
        it.next()) {
        auto entry = *it.get();
        entry->set_value_from(other.setting_by_name(entry->name()).value());
    }
}

Optional<Ref<Setting>> BaseSettingsState::setting_by_name(String const& name)
{
    auto setting = m_settings_by_name.find(name);
    if (setting.has_value())
        return *setting.value();
    return {};
}

void BaseSettingsState::load_from_file(String filename, Blob data)
{
    LineReader reader { filename, data };

    while (reader.load_next_line()) {
        String settingName = reader.next_token('=');

        auto maybe_setting = m_settings_by_name.find(settingName);

        if (!maybe_setting.has_value()) {
            reader.warn("Unrecognized setting: {0}"_s, { settingName });
        } else {
            (*maybe_setting.value())->set_from_file(reader);
        }
    }
}

bool BaseSettingsState::save_to_file(String filename)
{
    StringBuilder stb { 2048 };
    stb.append("# User-specific settings file.\n#\n"_s);
    stb.append("# I don't recommend fiddling with this manually, but it should work.\n"_s);
    stb.append("# If the game won't run, try deleting this file, and it should be re-generated with the default settings.\n\n"_s);

    for (auto it = m_settings_order.iterate();
        it.hasNext();
        it.next()) {
        String name = it.getValue();
        auto& setting = *m_settings_by_name.find(name).value();

        stb.append(name);
        stb.append(" = "_s);
        stb.append(setting->serialize_value());
        stb.append('\n');
    }

    String userSettingsPath = getUserSettingsPath();
    String fileData = stb.deprecated_to_string();

    if (writeFile(filename, fileData)) {
        logInfo("Settings saved successfully."_s);
        return true;
    }

    // TODO: Really really really need to display an error window to the user!
    logError("Failed to save user settings to {0}."_s, { userSettingsPath });
    return false;
}

void BaseSettingsState::register_setting(Setting& setting)
{
    m_settings_by_name.put(setting.name(), setting);
    m_settings_order.append(setting.name());
}

SettingsState::SettingsState(MemoryArena& arena)
    : BaseSettingsState(arena)
{
    register_setting(windowed);
    register_setting(resolution);
    register_setting(locale);
    register_setting(music_volume);
    register_setting(sound_volume);
    register_setting(widget_count);
}
