/*
 * Copyright (c) 2025-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Settings/Setting.h>
#include <Util/ChunkedArray.h>
#include <Util/HashTable.h>
#include <Util/Optional.h>
#include <Util/Ref.h>
#include <Util/String.h>

class SettingsState {
public:
    virtual ~SettingsState() = default;

    void restore_default_values();
    void copy_from(SettingsState& other);

    Optional<Ref<Setting>> get_setting(String const& name);
    Optional<Ref<Setting>> get_setting(String const& name) const
    {
        return const_cast<SettingsState*>(this)->get_setting(name);
    }

    template<typename T>
    Optional<T> get_typed_setting(String const& name) const
    {
        auto maybe_setting = get_setting(name);
        if (!maybe_setting.has_value())
            return {};
        auto const& setting = *maybe_setting.value();
        if (auto* typed_setting = dynamic_cast<BaseSetting<T> const*>(&setting))
            return typed_setting->value();
        logError("Wrong type of setting requested for '{}'"_s, { name });
        return {};
    }

    template<Enum E>
    Optional<E> get_typed_setting(String const& name) const
    {
        if (auto underlying_int = get_typed_setting<u32>(name); underlying_int.has_value())
            return static_cast<E>(underlying_int.value());

        return {};
    }

    void load_from_file(String filename, Blob data);
    // FIXME: Should be const, but that's awkward right now.
    bool save_to_file(String filename);

    template<typename Callback>
    void for_each_setting(Callback callback)
    {
        for (auto it = m_settings_order.iterate();
            it.hasNext();
            it.next()) {
            String name = it.getValue();
            auto setting = *m_settings_by_name.find(name).value();
            callback(*setting);
        }
    }

protected:
    explicit SettingsState(MemoryArena& arena);
    void register_setting(Setting&);

private:
    HashTable<Ref<Setting>> m_settings_by_name;
    ChunkedArray<String> m_settings_order;
};
