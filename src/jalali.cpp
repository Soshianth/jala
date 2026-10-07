// =============================================================================
// jalali.cpp — implementation of the calendar-math layer
// SPDX-License-Identifier: MIT
//
// This file implements the pure Jalali <-> Gregorian conversion layer.
// It has no dependency on I/O, terminal formatting, or command-line
// options. All algorithms use the 33-year leap-year cycle, which is
// the model that matches the official Iranian calendar for the
// modern era (roughly 1200-1600 Jalali).
// =============================================================================

#include "jalali.hpp"

#include <sstream>
#include <vector>

namespace jala {

// =============================================================================
// Internal helpers
// =============================================================================

namespace {

// The number of days in a complete 33-year Jalali cycle.
//
// A cycle contains 33 * 365 = 12045 common days plus 8 leap days,
// one for each leap-year position, for a total of 12053 days.
constexpr long DAYS_PER_CYCLE = 12053;

// The Julian Day Number of Farvardin 1, year 1 (the Jalali epoch).
//
// This constant anchors the entire conversion: given any Jalali
// date, we can compute an absolute day count relative to this
// number, and vice versa.
constexpr long JALALI_EPOCH_JDN = 1948320;

}  // namespace

// =============================================================================
// Calendar arithmetic
// =============================================================================

// Returns true if `year` is a leap year in the Jalali calendar.
//
// The Jalali calendar follows a 33-year cycle with eight leap
// years. Their positions within each cycle are:
//
//     1, 5, 9, 13, 17, 22, 26, 30
//
// The position of a given year inside its cycle is:
//
//     ((year - 1) mod 33) + 1
//
// In a leap year, Esfand (month 12) has 30 days instead of 29.
// Outside the modern era, the 33-year cycle can drift by one day
// from the official (observational) calendar; this limitation is
// documented in the README.
bool persian_is_leap(int year) {
    const int pos = ((year - 1) % 33) + 1;
    return (pos == 1  || pos == 5  || pos == 9  || pos == 13 ||
            pos == 17 || pos == 22 || pos == 26 || pos == 30);
}

// Number of days in a Jalali month.
//
// Months 1 through 6 have 31 days, months 7 through 11 have 30 days,
// and Esfand (month 12) has 30 days in leap years and 29 otherwise.
int persian_month_days(int year, int month) {
    if (month <= 6)  return 31;
    if (month <= 11) return 30;
    return persian_is_leap(year) ? 30 : 29;
}

// Day of the year (1..365 or 1..366) for a Jalali date.
int persian_day_of_year(int year, int month, int day) {
    int doy = 0;
    for (int m = 1; m < month; ++m) {
        doy += persian_month_days(year, m);
    }
    return doy + day;
}

// Weekday index for a Jalali date.
//
// Returns 0 for Shanbeh (Saturday) through 6 for Jomeh (Friday).
// The JDN offset +2 aligns the arithmetic so that JDN 1948320
// (Farvardin 1, year 1) maps to the correct Persian weekday.
int persian_weekday(int year, int month, int day) {
    return static_cast<int>((persian_to_jdn(year, month, day) + 2) % 7);
}

// =============================================================================
// Conversion
// =============================================================================

// Convert a Jalali date to a Julian Day Number.
//
// The conversion counts whole 33-year cycles before the target year
// and then walks the remaining years one by one. Each cycle has a
// fixed length of 12053 days. The final JDN is anchored at the
// Jalali epoch (Farvardin 1, year 1 = JDN 1948320).
long persian_to_jdn(int year, int month, int day) {
    // Number of complete years that come before `year`.
    const int years_before = year - 1;

    // Split those years into full 33-year cycles plus a remainder.
    const long full_cycles = years_before / 33;
    const int  partial     = years_before % 33;

    // Every complete cycle contributes a fixed number of days.
    long days = full_cycles * DAYS_PER_CYCLE;

    // Walk the remaining years one by one. The positions 1..32 map
    // directly to leap-year checks, since the cycle is defined with
    // position 1 as its first year.
    for (int i = 1; i <= partial; ++i) {
        days += persian_is_leap(i) ? 366 : 365;
    }

    // Add the days of the months that precede `month` in `year`.
    for (int m = 1; m < month; ++m) {
        days += persian_month_days(year, m);
    }

    // Add the days already elapsed within the current month
    // (day 1 is the first day, so we add day - 1).
    days += day - 1;

    return JALALI_EPOCH_JDN + days;
}

// Convert a Julian Day Number to a Jalali date.
//
// This is the exact inverse of persian_to_jdn(). It first locates
// the year by walking 33-year cycles and then individual years,
// then walks the months of that year, and finally reads off the
// day within the month.
PersianDate jdn_to_persian(long jdn) {
    // Days elapsed since Farvardin 1, year 1.
    long days = jdn - JALALI_EPOCH_JDN;

    // Locate the 33-year cycle that contains the target date.
    const long cycles = days / DAYS_PER_CYCLE;
    long       rem    = days % DAYS_PER_CYCLE;

    // Locate the year within the cycle by subtracting full years
    // until `rem` fits inside the current one.
    int year_in_cycle = 33;  // default if `rem` lands exactly at the end
    for (int i = 1; i <= 33; ++i) {
        const int len = persian_is_leap(i) ? 366 : 365;
        if (rem < len) {
            year_in_cycle = i;
            break;
        }
        rem -= len;
    }

    // Absolute year = completed cycles * 33 + position in cycle.
    const int year = static_cast<int>(cycles * 33 + year_in_cycle);

    // Locate the month within the year.
    int month = 1;
    while (month <= 12) {
        const int len = persian_month_days(year, month);
        if (rem < len) break;
        rem -= len;
        ++month;
    }

    // Whatever remains is the 0-based day index inside the month.
    const int day = static_cast<int>(rem) + 1;
    return { year, month, day };
}

// Convert a boost::gregorian::date to a Jalali date.
PersianDate to_persian(const boost::gregorian::date& g) {
    return jdn_to_persian(g.julian_day());
}

// Convert a Julian Day Number to a boost::gregorian::date.
//
// We compute the date as a difference from a fixed reference point
// (1970-01-01) to avoid recomputing the epoch on every call.
boost::gregorian::date jdn_to_gregorian(long jdn) {
    static const boost::gregorian::date ref(1970, 1, 1);
    static const long                  ref_jdn = ref.julian_day();
    return ref + boost::gregorian::days(jdn - ref_jdn);
}

// =============================================================================
// Parsing and formatting helpers
// =============================================================================

// Parse a date string in "YYYY/MM/DD", "YYYY-MM-DD", or "today".
//
// The separator is chosen by the first '/' or '-' found in the
// input. A year below 1700 is treated as Jalali; otherwise as
// Gregorian.
//
// The parser is strict: every numeric component must be a complete
// integer with no trailing characters, and the resulting date must
// exist in its respective calendar. On any failure, the returned
// SimpleDate has valid == false.
SimpleDate parse_date(std::string_view s) {
    SimpleDate out;

    // Special keyword: the current date.
    if (s == "today" || s == "now") {
        const PersianDate p = to_persian(
            boost::gregorian::day_clock::local_day());
        out = { p.year, p.month, p.day, true, true };
        return out;
    }

    // The separator is whichever of '/' or '-' appears first.
    const auto sep_pos = s.find_first_of("/-");
    if (sep_pos == std::string_view::npos) return out;
    const char sep = s[sep_pos];

    // Split the input into three components. An empty component or a
    // component that is not a full integer causes an early return
    // with valid == false.
    std::vector<int> parts;
    std::istringstream ss{ std::string(s) };
    std::string token;
    while (std::getline(ss, token, sep)) {
        if (token.empty()) return out;
        try {
            std::size_t pos = 0;
            const int value = std::stoi(token, &pos);
            // std::stoi accepts trailing garbage unless we check pos.
            if (pos != token.size()) return out;
            parts.push_back(value);
        } catch (...) {
            return out;
        }
    }
    if (parts.size() != 3) return out;

    out.year   = parts[0];
    out.month  = parts[1];
    out.day    = parts[2];
    out.jalali = (out.year < 1700);

    // Calendar-specific validation.
    if (out.jalali) {
        // Jalali: year >= 1, month in [1, 12], and day within the
        // length of that month (which itself depends on leap years).
        if (out.year < 1) return out;
        if (out.month < 1 || out.month > 12) return out;

        const int max_day = persian_month_days(out.year, out.month);
        if (out.day < 1 || out.day > max_day) return out;
    } else {
        // Gregorian: let Boost decide. The date constructor throws
        // on invalid input such as 2026-02-30, which we translate
        // into valid == false.
        try {
            boost::gregorian::date g(out.year, out.month, out.day);
            (void)g;
        } catch (const std::exception&) {
            return out;
        }
    }

    out.valid = true;
    return out;
}

// Replace ASCII digits in `s` with their Persian counterparts.
std::string to_persian_digits(std::string_view s) {
    static constexpr std::array<const char*, 10> digits = {
        "۰", "۱", "۲", "۳", "۴", "۵", "۶", "۷", "۸", "۹"
    };
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c >= '0' && c <= '9') out += digits[static_cast<size_t>(c - '0')];
        else                       out += c;
    }
    return out;
}

}  // namespace jala