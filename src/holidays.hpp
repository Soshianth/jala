// =============================================================================
// holidays.hpp — query Iranian official holidays from embedded data
// SPDX-License-Identifier: MIT
//
// The holiday data is generated from data/holidays.json into
// holidays_data.hpp by scripts/embed_holidays.py. This header exposes
// a minimal query interface: build an index once, then look up dates.
// =============================================================================

#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace jala {

// =============================================================================
// Holiday lookup
// =============================================================================

// An index over the embedded holiday table. Lookups are O(1) after
// construction. The index is cheap to build (only a few dozen entries
// at most) and can be constructed once per program run.
class HolidayIndex {
public:
    HolidayIndex();

    // Returns true if `date` (formatted "YYYY/MM/DD") is a holiday.
    [[nodiscard]] bool contains(std::string_view date) const;

    // Returns the holiday name for `date`, or an empty string if it is
    // not a holiday.
    [[nodiscard]] std::string_view name(std::string_view date) const;

    // Returns the number of holidays in the index.
    [[nodiscard]] std::size_t size() const noexcept { return index_.size(); }

private:
    std::unordered_map<std::string, std::string_view> index_;
};

// Formats a Jalali date as "YYYY/MM/DD" with zero-padded month and day.
// This must match the format used in data/holidays.json.
[[nodiscard]] std::string format_holiday_key(int year, int month, int day);

}  // namespace jala