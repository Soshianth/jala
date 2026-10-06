// =============================================================================
// holidays.cpp — implementation of the holiday and event lookups
// SPDX-License-Identifier: MIT
// =============================================================================

#include "holidays.hpp"
#include "holidays_data.hpp"

#include <cstdio>

namespace jala {

// =============================================================================
// Helpers
// =============================================================================

std::string format_holiday_key(int year, int month, int day) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d/%02d/%02d", year, month, day);
    return std::string(buf);
}

// =============================================================================
// HolidayIndex
// =============================================================================

HolidayIndex::HolidayIndex() {
    index_.reserve(HOLIDAYS_COUNT);
    for (int i = 0; i < HOLIDAYS_COUNT; ++i) {
        index_.emplace(HOLIDAYS[i].date, HOLIDAYS[i].name);
    }
}

bool HolidayIndex::contains(std::string_view date) const {
    return index_.find(std::string(date)) != index_.end();
}

std::string_view HolidayIndex::name(std::string_view date) const {
    const auto it = index_.find(std::string(date));
    return (it != index_.end()) ? it->second : std::string_view{};
}

// =============================================================================
// EventIndex
// =============================================================================

// Static empty vector returned for dates with no events.
const std::vector<Event> EventIndex::empty_{};

EventIndex::EventIndex() {
    // We don't know how many distinct dates exist, so reserve based on
    // the total count (a reasonable upper bound).
    index_.reserve(EVENTS_COUNT);

    for (int i = 0; i < EVENTS_COUNT; ++i) {
        const EventEntry& e = EVENTS[i];
        index_[e.date].push_back({ e.description, e.is_holiday });
        ++total_events_;
    }
}

const std::vector<Event>& EventIndex::events(std::string_view date) const {
    const auto it = index_.find(std::string(date));
    return (it != index_.end()) ? it->second : empty_;
}

}  // namespace jala