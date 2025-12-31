/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Setting.h"
#include <Settings/Settings.h>
#include <UI/Panel.h>
#include <Util/StringBuilder.h>

void Setting::add_ui_line(UI::Panel& ui)
{
    ui.startNewLine(HAlign::Left);
    ui.addLabel(getText(text_asset_name()));
    ui.alignWidgets(HAlign::Right);
    add_ui_widget(ui);
}

void Setting::add_ui_widget(UI::Panel& ui)
{
    ui.addLabel(serialize_value());
}

void BoolSetting::set_value_from(Setting const& other)
{
    set_value(dynamic_cast<BoolSetting const&>(other).value());
}

void BoolSetting::add_ui_widget(UI::Panel& ui)
{
    ui.addCheckbox(value_pointer());
}

bool BoolSetting::set_from_file(LineReader& reader)
{
    if (auto value = reader.read_bool(); value.has_value()) {
        set_value(value.release_value());
        return true;
    }
    return false;
}

String BoolSetting::serialize_value() const
{
    return formatBool(value());
}

void PercentSetting::set_value_from(Setting const& other)
{
    set_value(dynamic_cast<PercentSetting const&>(other).value());
}

void PercentSetting::add_ui_widget(UI::Panel& ui)
{
    float* percent = value_pointer();
    s32 intPercent = round_s32(*percent * 100.0f);
    ui.addLabel(myprintf("{0}%"_s, { formatInt(intPercent) }));
    ui.addSlider(percent, 0.0f, 1.0f);
}

bool PercentSetting::set_from_file(LineReader& reader)
{
    if (auto value = reader.read_float(); value.has_value()) {
        float clamped_value = clamp01(value.release_value());
        set_value(clamped_value);
        return true;
    }
    return false;
}

String PercentSetting::serialize_value() const
{
    return formatFloat(value(), 2);
}

void S32Setting::set_value_from(Setting const& other)
{
    set_value(dynamic_cast<S32Setting const&>(other).value());
}

bool S32Setting::set_from_file(LineReader& reader)
{
    if (auto value = reader.read_int<s32>(); value.has_value()) {
        set_value(value.release_value());
        return true;
    }
    return false;
}

String S32Setting::serialize_value() const
{
    return formatInt(value());
}

void S32RangeSetting::set_value_from(Setting const& other)
{
    set_value(dynamic_cast<S32RangeSetting const&>(other).value());
}

void S32RangeSetting::add_ui_widget(UI::Panel& ui)
{
    s32* intValue = value_pointer();
    ui.addLabel(formatInt(*intValue));
    ui.addSlider(intValue, m_min_value, m_max_value);
}

bool S32RangeSetting::set_from_file(LineReader& reader)
{
    if (auto value = reader.read_int<s32>(); value.has_value()) {
        s32 clampedValue = clamp(value.release_value(), m_min_value, m_max_value);
        set_value(clampedValue);
        return true;
    }
    return false;
}

String S32RangeSetting::serialize_value() const
{
    return formatInt(value());
}

void StringSetting::set_value_from(Setting const& other)
{
    set_value(dynamic_cast<StringSetting const&>(other).value());
}

bool StringSetting::set_from_file(LineReader& reader)
{
    String value = Settings::the().arena.allocate_string(reader.next_token());
    set_value(value);
    return true;
}

String StringSetting::serialize_value() const
{
    return value();
}

void V2ISetting::set_value_from(Setting const& other)
{
    set_value(dynamic_cast<V2ISetting const&>(other).value());
}

bool V2ISetting::set_from_file(LineReader& reader)
{
    if (auto value = V2I::read(reader); value.has_value()) {
        set_value(value.release_value());
        return true;
    }
    return false;
}

String V2ISetting::serialize_value() const
{
    StringBuilder stb;
    stb.append(formatInt(value().x));
    stb.append('x');
    stb.append(formatInt(value().y));
    return stb.deprecated_to_string();
}
