/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <IO/LineReader.h>
#include <UI/Panel.h>
#include <Util/Basic.h>
#include <Util/Enum.h>
#include <Util/EnumMap.h>
#include <Util/String.h>
#include <Util/Vector.h>

template<typename T>
class Setting {
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

    virtual ~Setting() = default;

    String const& name() const { return m_name; }
    Type type() const { return m_type; }
    T value() const { return m_value; }
    T* value_pointer() { return &m_value; }
    virtual void set_value(T value) { m_value = value; }
    String const& text_asset_name() const { return m_text_asset_name; }

    virtual void add_ui_line(UI::Panel&);
    virtual void add_ui_widget(UI::Panel& ui);

    virtual bool set_from_file(LineReader&) = 0;
    virtual String serialize_value() const = 0;

protected:
    Setting(String name, String text_asset_name, Type type, T default_value)
        : m_name(name)
        , m_text_asset_name(text_asset_name)
        , m_type(type)
        , m_value(default_value)
    {
    }

    String m_name;
    String m_text_asset_name;
    Type m_type;
    T m_value;
};

class BoolSetting final : public Setting<bool> {
public:
    BoolSetting(String name, String text_asset_name, bool default_value)
        : Setting(name, text_asset_name, Type::Bool, default_value)
    {
    }

    virtual ~BoolSetting() override = default;

private:
    virtual void add_ui_widget(UI::Panel&) override;
    virtual bool set_from_file(LineReader&) override;
    virtual String serialize_value() const override;
};

struct EnumSettingData {
    String id;
    String text_asset_name;
};

template<Enum E>
class EnumSetting final : public Setting<u64> {
public:
    EnumSetting(String name, String text_asset_name, EnumMap<E, EnumSettingData> enum_data, E default_value)
        : Setting(name, text_asset_name, Type::Enum, to_underlying(default_value))
        , m_enum_data(move(enum_data))
    {
    }

    E enum_value() const { return static_cast<E>(value()); }

    virtual ~EnumSetting() override = default;

private:
    virtual void add_ui_widget(UI::Panel&) override
    {
        // ui.addRadioButtonGroup(m_enum_data, value_pointer(), );
    }

    virtual bool set_from_file(LineReader& reader) override
    {
        String token = reader.next_token();
        // Look it up in the enum data
        for (auto it : enum_values<E>()) {
            if (m_enum_data[it].id == token) {
                set_value(to_underlying(it));
                return true;
            }
        }

        reader.error("Couldn't find '{0}' in the list of valid values for setting '{1}'."_s, { token, name() });
        return false;
    }

    virtual String serialize_value() const override
    {
        return m_enum_data[m_value].id;
    }

    EnumMap<E, EnumSettingData> m_enum_data;
};

class PercentSetting final : public Setting<float> {
public:
    PercentSetting(String name, String text_asset_name, float default_value)
        : Setting(name, text_asset_name, Type::Percent, default_value)
    {
    }

    virtual ~PercentSetting() override = default;

private:
    virtual void add_ui_widget(UI::Panel&) override;
    virtual bool set_from_file(LineReader&) override;
    virtual String serialize_value() const override;
};

class S32Setting final : public Setting<s32> {
public:
    S32Setting(String name, String text_asset_name, s32 default_value)
        : Setting(name, text_asset_name, Type::S32, default_value)
    {
    }

    virtual ~S32Setting() override = default;

private:
    virtual bool set_from_file(LineReader&) override;
    virtual String serialize_value() const override;
};

class S32RangeSetting final : public Setting<s32> {
public:
    S32RangeSetting(String name, String text_asset_name, s32 min_value, s32 max_value, s32 default_value)
        : Setting(name, text_asset_name, Type::S32_Range, default_value)
        , m_min_value(min_value)
        , m_max_value(max_value)
    {
    }

    virtual ~S32RangeSetting() override = default;

private:
    virtual void add_ui_widget(UI::Panel&) override;
    virtual bool set_from_file(LineReader&) override;
    virtual String serialize_value() const override;

    s32 m_min_value;
    s32 m_max_value;
};

class StringSetting final : public Setting<String> {
public:
    StringSetting(String name, String text_asset_name, String default_value)
        : Setting(name, text_asset_name, Type::String, default_value)
    {
    }

    virtual ~StringSetting() override = default;

private:
    virtual bool set_from_file(LineReader&) override;
    virtual String serialize_value() const override;
};

class V2ISetting final : public Setting<V2I> {
public:
    V2ISetting(String name, String text_asset_name, V2I default_value)
        : Setting(name, text_asset_name, Type::V2I, default_value)
    {
    }

    virtual ~V2ISetting() override = default;

private:
    virtual void add_ui_widget(UI::Panel&) override;
    virtual bool set_from_file(LineReader&) override;
    virtual String serialize_value() const override;
};
