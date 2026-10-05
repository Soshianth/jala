// =============================================================================
// jalali.cpp — implementation of the calendar-math layer
// SPDX-License-Identifier: MIT
// =============================================================================

#include "jalali.hpp"

#include <sstream>
#include <vector>

namespace jala {

// =============================================================================
// Conversion
// =============================================================================

long persian_to_jdn(int year, int month, int day) {
    const long epbase = year - (year >= 0 ? 474 : 473);
    const long epyear = 474 + (epbase % 2820);
    const long m      = (month <= 7)
        ? static_cast<long>(month - 1) * 31
        : static_cast<long>(month - 1) * 30 + 6;
    return day + m
         + (epyear * 682 - 110) / 2816
         + (epyear - 1) * 365
         + (epbase / 2820) * 1029983
         + 1948320;
}

PersianDate jdn_to_persian(long jdn) {
    const long depoch = jdn - 2121446;
    const long cycle  = depoch / 1029983;
    const long cyear  = depoch % 1029983;

    long ycycle = 0;
    if (cyear == 1029982) {
        ycycle = 2820;
    } else {
        const long aux1 = cyear / 366;
        const long aux2 = cyear % 366;
        ycycle = (2134 * aux1 + 2816 * aux2 + 2815) / 1028522 + aux1 + 1;
    }

    long pyear = ycycle + 2820 * cycle + 474;
    if (pyear <= 0) --pyear;

    const long yday   = jdn - persian_to_jdn(static_cast<int>(pyear), 1, 1) + 1;
    const long pmonth = (yday <= 186)
        ? (yday - 1) / 31 + 1
        : (yday - 7) / 30 + 1;
    const long pday   = jdn - persian_to_jdn(static_cast<int>(pyear),
                                              static_cast<int>(pmonth), 1) + 1;

    return { static_cast<int>(pyear),
             static_cast<int>(pmonth),
             static_cast<int>(pday) };
}

PersianDate to_persian(const boost::gregorian::date& g) {
    return jdn_to_persian(g.julian_day());
}

boost::gregorian::date jdn_to_gregorian(long jdn) {
    static const boost::gregorian::date ref(1970, 1, 1);
    static const long                  ref_jdn = ref.julian_day();
    return ref + boost::gregorian::days(jdn - ref_jdn);
}

// =============================================================================
// Calendar arithmetic
// =============================================================================

int persian_weekday(int year, int month, int day) {
    return static_cast<int>((persian_to_jdn(year, month, day) + 2) % 7);
}

int persian_month_days(int year, int month) {
    if (month <= 6)  return 31;
    if (month <= 11) return 30;

    // Esfand has 29 days in common years and 30 in leap years. The
    // 33-year cycle contains eight leap years, identified by the
    // following remainders.
    const int r = year % 33;
    return (r == 1 || r == 5 || r == 9  || r == 13 ||
            r == 17 || r == 22 || r == 26 || r == 30) ? 30 : 29;
}

int persian_day_of_year(int year, int month, int day) {
    int doy = 0;
    for (int m = 1; m < month; ++m) doy += persian_month_days(year, m);
    return doy + day;
}

// =============================================================================
// Parsing and formatting helpers
// =============================================================================

SimpleDate parse_date(std::string_view s) {
    SimpleDate out;

    if (s == "today" || s == "now") {
        const PersianDate p = to_persian(
            boost::gregorian::day_clock::local_day());
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
        try {
            parts.push_back(std::stoi(token));
        } catch (...) {
            return out;
        }
    }
    if (parts.size() != 3) return out;

    out.year   = parts[0];
    out.month  = parts[1];
    out.day    = parts[2];
    out.jalali = (out.year < 1700);
    out.valid  = true;
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

} // namespace jala