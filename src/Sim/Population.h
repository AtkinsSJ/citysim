/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/EnumMap.h>
#include <Util/StringView.h>
#include <flecs.h>

struct ProvidesResidents {
    u32 count;
};

struct HasResidents {
    u32 count;
};

enum class ResidentType : u8 {
    People, // TODO: Wealth classes?
    COUNT,
};

constexpr StringView to_string(ResidentType const type)
{
    switch (type) {
    case ResidentType::People:
        return "Residents"_sv;
    case ResidentType::COUNT:
        break;
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
    COUNT,
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
    case JobType::COUNT:
        break;
    }
    VERIFY_NOT_REACHED();
}

class PopulationCache {
public:
    void update(flecs::world const&);

    u32 resident_capacity(Optional<ResidentType> = {}) const;
    u32 resident_count(Optional<ResidentType> = {}) const;

    u32 job_capacity(Optional<JobType> = {}) const;
    u32 job_count(Optional<JobType> = {}) const;

private:
    EnumMap<ResidentType, u32> m_resident_capacity;
    EnumMap<ResidentType, u32> m_resident_count;
    EnumMap<JobType, u32> m_job_capacity;
    EnumMap<JobType, u32> m_job_count;
};
