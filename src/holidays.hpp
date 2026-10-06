// =============================================================================
// holidays.hpp — query Iranian official holidays and events
// SPDX-License-Identifier: MIT
//
// The data is generated from data/holidays.json into holidays_data.hpp
// by scripts/embed_holidays.py. This header exposes two query
// interfaces:
//
//   HolidayIndex — fast lookup of whether a date is a holiday, and
//                  its (possibly multi-name) label. Used for colouring.
//
//   EventIndex   — all events on a given date, including non-holidays.
//                  Used by the -E/--events flag.
//
// Both indexes are cheap to build (a few hundred entries at most)
// and can be constructed once per program run.
// =============================================================================

#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace jala {

// =============================================================================
// Holiday lookup
// =============================================================================

class HolidayIndex {
public:
    HolidayIndex();

    // Returns true if `date` (formatted "YYYY/MM/DD") is a holiday.
    [[nodiscard]] bool contains(std::string_view date) const;

    // Returns the holiday name(s) for `date`, or an empty string if it
    // is not a holiday. Multiple names are joined with the Persian
    // comma "،".
    [[nodiscard]] std::string_view name(std::string_view date) const;

    [[nodiscard]] std::size_t size() const noexcept { return index_.size(); }

private:
    std::unordered_map<std::string, std::string_view> index_;
};

// =============================================================================
// Event lookup
// =============================================================================

// A single event on a given day.
struct Event {
    std::string_view description;  // UTF-8
    bool             is_holiday;   // true if the event is an official holiday
};

class EventIndex {
public:
    EventIndex();

    // Returns all events on `date`. The returned vector may be empty.
    // Events are returned in the order they appear in the source data,
    // which is alphabetical by description (sorted at generation time).
    [[nodiscard]] const std::vector<Event>& events(std::string_view date) const;

    [[nodiscard]] std::size_t size() const noexcept { return total_events_; }

private:
    std::unordered_map<std::string, std::vector<Event>> index_;
    std::size_t total_events_ = 0;

    static const std::vector<Event> empty_;
};

// =============================================================================
// Helpers
// =============================================================================

// Formats a Jalali date as "YYYY/MM/DD" with zero-padded month and day.
// This must match the format used in data/holidays.json.
[[nodiscard]] std::string format_holiday_key(int year, int month, int day);

}  // namespace jala