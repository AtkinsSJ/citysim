/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Population.h"

PopulationCache::PopulationCache(flecs::world& world)
{
    m_query_provides_residents
        = world.query_builder<ProvidesResidents const>()
              .term_at(0)
              .second(flecs::Wildcard)
              .build();
    m_query_has_residents
        = world.query_builder<HasResidents const>()
              .term_at(0)
              .second(flecs::Wildcard)
              .build();
    m_query_provides_jobs
        = world.query_builder<ProvidesJobs const>()
              .term_at(0)
              .second(flecs::Wildcard)
              .build();
    m_query_has_jobs
        = world.query_builder<HasJobs const>()
              .term_at(0)
              .second(flecs::Wildcard)
              .build();
}

void PopulationCache::update()
{
    m_resident_capacity.clear();
    m_resident_count.clear();
    m_job_capacity.clear();
    m_job_count.clear();

    m_query_provides_residents.each([&](flecs::iter& it, size_t, ProvidesResidents const& residents) {
        auto type = it.pair(0).second().to_constant<ResidentType>();
        m_resident_capacity[type] += residents.count;
    });
    m_query_has_residents.each([&](flecs::iter& it, size_t, HasResidents const& residents) {
        auto type = it.pair(0).second().to_constant<ResidentType>();
        m_resident_count[type] += residents.count;
    });
    m_query_provides_jobs.each([&](flecs::iter& it, size_t, ProvidesJobs const& jobs) {
        auto type = it.pair(0).second().to_constant<JobType>();
        m_job_capacity[type] += jobs.count;
    });
    m_query_has_jobs.each([&](flecs::iter& it, size_t, HasJobs const& jobs) {
        auto type = it.pair(0).second().to_constant<JobType>();
        m_job_count[type] += jobs.count;
    });
}

u32 PopulationCache::resident_capacity(Optional<ResidentType> type) const
{
    if (type.has_value())
        return m_resident_capacity[type.value()];

    u32 total = 0;
    m_resident_capacity.for_each([&total](auto, u32 count) {
        total += count;
    });
    return total;
}

u32 PopulationCache::resident_count(Optional<ResidentType> type) const
{
    if (type.has_value())
        return m_resident_count[type.value()];

    u32 total = 0;
    m_resident_count.for_each([&total](auto, u32 count) {
        total += count;
    });
    return total;
}

u32 PopulationCache::job_capacity(Optional<JobType> type) const
{
    if (type.has_value())
        return m_job_capacity[type.value()];

    u32 total = 0;
    m_job_capacity.for_each([&total](auto, u32 count) {
        total += count;
    });
    return total;
}

u32 PopulationCache::job_count(Optional<JobType> type) const
{
    if (type.has_value())
        return m_job_count[type.value()];

    u32 total = 0;
    m_job_count.for_each([&total](auto, u32 count) {
        total += count;
    });
    return total;
}
