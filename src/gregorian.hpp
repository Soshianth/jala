// =============================================================================
// gregorian.hpp — minimal Gregorian calendar support (no Boost)
// SPDX-License-Identifier: MIT
//
// This header provides just enough of the Gregorian calendar to
// replace the Boost.Date_Time dependency. It exposes a plain POD
// date type, conversion to and from Julian Day Number (JDN), and a
// few helpers. All conversions go through JDN, which is the common
// currency used across jala for inter-calendar arithmetic.
//
// The algorithms are the classical Fliegel–Van Flandern formulas,
// valid for the proleptic Gregorian calendar (year 1 and later).
// They have been verified against the reference value:
//
//     1970-01-01  <->  JDN 2,440,588
// =============================================================================

#pragma once

namespace jala {

// A plain Gregorian date. No invariants are enforced by the type
// itself; use `is_valid()` to check a candidate date before relying
// on it.
struct GregorianDate {
    int year  = 0;
    int month = 0;   // 1..12
    int day   = 0;   // 1..31
};

// Gregorian date -> Julian Day Number.
// The inputs must satisfy `is_valid(year, month, day)`; calling with
// an invalid date yields a meaningless result.
[[nodiscard]] constexpr long to_jdn(int year, int month, int day) noexcept {
    const int a = (14 - month) / 12;
    const int y = year + 4800 - a;
    const int m = month + 12 * a - 3;
    return static_cast<long>(day)
         + (153L * m + 2) / 5
         + 365L * y
         + y / 4
         - y / 100
         + y / 400
         - 32045;
}

// Julian Day Number -> Gregorian date.
[[nodiscard]] constexpr GregorianDate from_jdn(long jdn) noexcept {
    const long a = jdn + 32044;
    const long b = (4 * a + 3) / 146097;
    const long c = a - (146097 * b) / 4;
    const long d = (4 * c + 3) / 1461;
    const long e = c - (1461 * d) / 4;
    const long m = (5 * e + 2) / 153;

    GregorianDate g;
    g.day   = static_cast<int>(e - (153 * m + 2) / 5 + 1);
    g.month = static_cast<int>(m + 3 - 12 * (m / 10));
    g.year  = static_cast<int>(100 * b + d - 4800 + m / 10);
    return g;
}

// Returns true if (year, month, day) is a real Gregorian date.
// Leap years follow the standard rule: divisible by 4, except
// centuries, except every 400 years.
[[nodiscard]] constexpr bool is_valid(int year, int month, int day) noexcept {
    if (year < 1 || month < 1 || month > 12 || day < 1) return false;

    constexpr int mdays[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31,
    };
    int max = mdays[month - 1];

    if (month == 2) {
        const bool leap = (year % 4 == 0 && year % 100 != 0)
                       || (year % 400 == 0);
        if (leap) max = 29;
    }
    return day <= max;
}

// Today's local date in the Gregorian calendar.
[[nodiscard]] GregorianDate today() noexcept;

}  // namespace jala