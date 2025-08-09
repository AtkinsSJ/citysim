/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Setting.h"
#include <Settings/Settings.h>
#include <UI/Panel.h>
#include <Util/StringBuilder.h>

template<typename T>
void Setting<T>::add_ui_line(UI::Panel& ui)
{
    ui.startNewLine(HAlign::Left);
    ui.addLabel(getText(text_asset_name()));
    ui.alignWidgets(HAlign::Right);
    add_ui_widget(ui);
}

template<typename T>
void Setting<T>::add_ui_widget(UI::Panel& ui)
{
    ui.addLabel(serialize_value());
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

bool StringSetting::set_from_file(LineReader& reader)
{
    // FIXME: @Leak
    String value = pushString(&Settings::the().arena, reader.next_token());
    set_value(value);
    return true;
}

String StringSetting::serialize_value() const
{
    return value();
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
    StringBuilder stb = newStringBuilder(256);
    append(&stb, formatInt(value().x));
    append(&stb, 'x');
    append(&stb, formatInt(value().y));
    return getString(&stb);
}
