/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>

struct ProvidesResidents {
    u32 count;
};

struct HasResidents {
    u32 count;
};

enum class ResidentType : u8 {
    People, // TODO: Wealth classes?
};

constexpr StringView to_string(ResidentType const type)
{
    switch (type) {
    case ResidentType::People:
        return "Residents"_sv;
    }
    VERIFY_NOT_REACHED();
}

struct ProvidesJobs {
    u32 count;
};

struct HasJobs {
    u32 count;
};

enum class JobType : u8 {
    Civic,
    Commercial,
    Industrial,
};

constexpr StringView to_string(JobType const type)
{
    switch (type) {
    case JobType::Civic:
        return "Civic"_sv;
    case JobType::Commercial:
        return "Commercial"_sv;
    case JobType::Industrial:
        return "Industrial"_sv;
    }
    VERIFY_NOT_REACHED();
}
