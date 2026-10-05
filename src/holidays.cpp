// =============================================================================
// holidays.cpp — implementation of the holiday lookup
// SPDX-License-Identifier: MIT
// =============================================================================

#include "holidays.hpp"
#include "holidays_data.hpp"

#include <cstdio>

namespace jala {

std::string format_holiday_key(int year, int month, int day) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d/%02d/%02d", year, month, day);
    return std::string(buf);
}

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

}  // namespace jala