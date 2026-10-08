// =============================================================================
// jalali.cpp — implementation of the calendar-math layer
// SPDX-License-Identifier: MIT
//
// The Jalali arithmetic in this file follows the algorithm used by
// the jalaali-js library [1], which in turn derives from the work of
// Kazimierz M. Borkowski ("The Persian calendar for 3000 years",
// Earth, Moon, and Planets 74: 223-230, 1996).
//
// This is the same algorithm used by Google Calendar, moment-jalaali,
// and most other modern implementations. It correctly models the
// irregular leap years around the year 1400.
//
// The Gregorian calendar is provided by the local gregorian.hpp
// header. There is no dependency on Boost or any other external
// library.
//
// [1] https://github.com/jalaali/jalaali-js (MIT)
// =============================================================================

#include "jalali.hpp"
#include "gregorian.hpp"

#include <sstream>
#include <string>
#include <vector>

namespace jala {

namespace {

// Era boundaries of the Jalali calendar. Every pair of consecutive
// values defines an era within which the leap-year pattern is
// deterministic. Values are chosen so that Farvardin 1 of every year
// in the era falls on the expected Gregorian date.
//
// Source: jalaali-js (MIT), itself derived from Borkowski 1996.
constexpr int BREAKS[] = {
    -61,   9,   38,   199,  426,  686,  756,  818, 1111,
    1181, 1210, 1635, 2060, 2097, 2192, 2262, 2324, 2394,
    2456, 3178,
};
constexpr int BREAKS_COUNT =
    static_cast<int>(sizeof(BREAKS) / sizeof(BREAKS[0]));

// Result of the era computation for a single Jalali year.
struct JalCal {
    int leap;   // 0 if this year is leap; 1..4 otherwise (years since
                // the most recent leap year, counted downward).
    int gy;     // Gregorian year that contains Farvardin 1.
    int march;  // Day in March on which Farvardin 1 falls.
};

// Determine the leap state and Gregorian anchor for Jalali year jy.
//
// The algorithm is only defined for jy in [BREAKS[0], BREAKS[last]),
// i.e. [-61, 3177]. Callers are expected to enforce this range; the
// result is undefined outside it.
//
// Note on division: the jalaali-js reference implementation uses
// truncating integer division (~~(a / b) in JavaScript). C++'s built-in
// / and % operators on int have exactly the same behaviour, so we use
// them directly instead of introducing floor/trunc helper functions.
constexpr JalCal jal_cal(int jy) noexcept {
    const int gy = jy + 621;

    int leapJ = -14;
    int jp    = BREAKS[0];
    int jm    = 0;
    int jump  = 0;

    // Locate the era that contains jy. Each iteration advances jp to
    // the start of the next era and accumulates the leap-year count.
    for (int i = 1; i < BREAKS_COUNT; ++i) {
        jm   = BREAKS[i];
        jump = jm - jp;
        if (jy < jm) break;
        leapJ += (jump / 33) * 8 + (jump % 33) / 4;
        jp     = jm;
    }
    const int n = jy - jp;

    // Accumulate the leap years from the start of the era to jy.
    leapJ += (n / 33) * 8 + ((n % 33) + 3) / 4;
    if (jump % 33 == 4 && jump - n == 4) leapJ += 1;

    // Count leap years in the Gregorian calendar from year 1 to gy.
    const int leapG = gy / 4 - ((gy / 100 + 1) * 3) / 4 - 150;

    // Day in March on which Farvardin 1 falls.
    const int march = 20 + leapJ - leapG;

    // Determine how many years have passed since the last leap year.
    // A value of 0 means jy itself is a leap year.
    int m = n;
    if (jump - m < 6) m = m - jump + ((jump + 4) / 33) * 33;
    int leap = ((m + 1) % 33 - 1) % 4;
    if (leap == -1) leap = 4;

    return { leap, gy, march };
}

}  // namespace

// =============================================================================
// Calendar arithmetic
// =============================================================================

bool persian_is_leap(int year) {
    return jal_cal(year).leap == 0;
}

int persian_month_days(int year, int month) {
    if (month <= 6)  return 31;
    if (month <= 11) return 30;
    return persian_is_leap(year) ? 30 : 29;
}

int persian_day_of_year(int year, int month, int day) {
    int doy = 0;
    for (int m = 1; m < month; ++m) {
        doy += persian_month_days(year, m);
    }
    return doy + day;
}

int persian_weekday(int year, int month, int day) {
    // JDN 0 falls on a Monday, so (JDN + 2) % 7 gives 0 = Shanbeh.
    return static_cast<int>((persian_to_jdn(year, month, day) + 2) % 7);
}

// =============================================================================
// Conversion
// =============================================================================

long persian_to_jdn(int year, int month, int day) {
    const JalCal r = jal_cal(year);

    // JDN of Farvardin 1 of `year`, then offset within the year.
    // The closed form below accumulates the month offset without
    // branching: months 1..6 have 31 days, 7..11 have 30, and month
    // 12 has 29 or 30 depending on the leap state.
    return to_jdn(r.gy, 3, r.march)
         + static_cast<long>(month - 1) * 31
         - (month / 7) * (month - 7)
         + (day - 1);
}

PersianDate jdn_to_persian(long jdn) {
    const GregorianDate g = from_jdn(jdn);
    int jy = g.year - 621;

    const JalCal r = jal_cal(jy);
    const long jdn1f = to_jdn(g.year, 3, r.march);
    long k = jdn - jdn1f;

    if (k >= 0) {
        if (k <= 185) {
            // First six months: 31 days each.
            return { jy,
                     1 + static_cast<int>(k / 31),
                     static_cast<int>(k % 31) + 1 };
        }
        k -= 186;
    } else {
        // The date belongs to the previous Jalali year.
        jy -= 1;
        k += 179;
        if (r.leap == 1) k += 1;
    }

    // Remaining months: 30 days each.
    return { jy,
             7 + static_cast<int>(k / 30),
             static_cast<int>(k % 30) + 1 };
}

PersianDate to_persian(const GregorianDate& g) {
    return jdn_to_persian(to_jdn(g.year, g.month, g.day));
}

GregorianDate jdn_to_gregorian(long jdn) {
    return from_jdn(jdn);
}

// =============================================================================
// Parsing and formatting helpers
// =============================================================================

SimpleDate parse_date(std::string_view s, CalendarMode mode) {
    SimpleDate out;

    if (s == "today" || s == "now") {
        const PersianDate p = to_persian(today());
        out = { p.year, p.month, p.day, true, true };
        return out;
    }

    const auto sep_pos = s.find_first_of("/-");
    if (sep_pos == std::string_view::npos) return out;
    const char sep = s[sep_pos];

    std::istringstream ss{ std::string(s) };
    std::string token;
    std::vector<int> parts;

    while (std::getline(ss, token, sep)) {
        if (token.empty()) return out;
        try {
            std::size_t pos = 0;
            const int value = std::stoi(token, &pos);
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

    // Decide which calendar to interpret the year in. An explicit
    // --calendar=jalali|gregorian overrides the heuristic.
    switch (mode) {
        case CalendarMode::Jalali:
            out.jalali = true;
            break;
        case CalendarMode::Gregorian:
            out.jalali = false;
            break;
        case CalendarMode::Auto:
            out.jalali = (out.year < 1700);
            break;
    }

    if (out.jalali) {
        if (out.year < 1 || out.year > MAX_YEAR) return out;
        if (out.month < 1 || out.month > 12) return out;
        const int max_day = persian_month_days(out.year, out.month);
        if (out.day < 1 || out.day > max_day) return out;
    } else {
        if (!is_valid(out.year, out.month, out.day)) return out;
    }

    out.valid = true;
    return out;
}

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