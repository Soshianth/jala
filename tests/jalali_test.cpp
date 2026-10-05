// =============================================================================
// jalali_test.cpp — unit tests for the calendar-math layer
// SPDX-License-Identifier: MIT
//
// These tests exercise src/jalali.cpp directly, without going through
// the CLI. They are fast (a few milliseconds) and cover the pure
// conversion, arithmetic, and parsing functions.
//
// Build:  make test-unit
// =============================================================================

#include "jalali.hpp"

#include <cassert>
#include <iostream>

using namespace jala;

// Simple assertion helper: prints the expression when it fails so the
// error location is obvious.
#define CHECK(expr)                                                          \
    do {                                                                     \
        if (!(expr)) {                                                       \
            std::cerr << "FAIL: " << #expr                                   \
                      << " at " << __FILE__ << ":" << __LINE__ << "\n";      \
            return 1;                                                        \
        }                                                                    \
    } while (0)

int main() {
    // ---- Round-trip conversion ----
    {
        const long jdn = persian_to_jdn(1405, 7, 12);
        const PersianDate p = jdn_to_persian(jdn);
        CHECK(p.year == 1405);
        CHECK(p.month == 7);
        CHECK(p.day == 12);
    }

    // ---- Known date: 1405/07/12 == 2026-10-04 ----
    {
        const long jdn = persian_to_jdn(1405, 7, 12);
        const auto g = jdn_to_gregorian(jdn);
        CHECK(g.year() == 2026);
        CHECK(g.month().as_number() == 10);
        CHECK(g.day() == 4);
    }

    // ---- Reverse conversion: 2026-10-04 == 1405/07/12 ----
    {
        const auto g = boost::gregorian::date(2026, 10, 4);
        const PersianDate p = to_persian(g);
        CHECK(p.year == 1405);
        CHECK(p.month == 7);
        CHECK(p.day == 12);
    }

    // ---- Weekday: 1405/07/12 is a Sunday (index 1) ----
    {
        CHECK(persian_weekday(1405, 7, 12) == 1);
    }

    // ---- Month lengths ----
    {
        CHECK(persian_month_days(1405, 1)  == 31);  // Farvardin
        CHECK(persian_month_days(1405, 7)  == 30);  // Mehr
        CHECK(persian_month_days(1405, 11) == 30);  // Bahman
        CHECK(persian_month_days(1405, 12) == 29);  // Esfand, common year
    }

    // ---- Leap year: 1403 is a leap year, 1404 is not ----
    {
        CHECK(persian_month_days(1403, 12) == 30);
        CHECK(persian_month_days(1404, 12) == 29);
    }

    // ---- Day of year ----
    // Months 1..6 have 31 days each, so the last day of Shahrivar is
    // day 186 and the first day of Mehr is day 187.
    {
        CHECK(persian_day_of_year(1405, 1, 1)   == 1);
        CHECK(persian_day_of_year(1405, 1, 31)  == 31);
        CHECK(persian_day_of_year(1405, 6, 31)  == 186);
        CHECK(persian_day_of_year(1405, 7, 1)   == 187);
        CHECK(persian_day_of_year(1405, 7, 12)  == 198);
        CHECK(persian_day_of_year(1405, 12, 29) == 365);
    }

    // ---- parse_date: Jalali input ----
    {
        const SimpleDate d = parse_date("1405/07/12");
        CHECK(d.valid);
        CHECK(d.jalali);
        CHECK(d.year  == 1405);
        CHECK(d.month == 7);
        CHECK(d.day   == 12);
    }

    // ---- parse_date: Gregorian input ----
    {
        const SimpleDate d = parse_date("2026-10-04");
        CHECK(d.valid);
        CHECK(!d.jalali);
        CHECK(d.year  == 2026);
        CHECK(d.month == 10);
        CHECK(d.day   == 4);
    }

    // ---- parse_date: "today" keyword ----
    {
        const SimpleDate d = parse_date("today");
        CHECK(d.valid);
        CHECK(d.jalali);
    }

    // ---- parse_date: invalid input ----
    {
        CHECK(!parse_date("garbage").valid);
        CHECK(!parse_date("1405").valid);
        CHECK(!parse_date("1405/07").valid);
    }

    // ---- to_persian_digits ----
    {
        CHECK(to_persian_digits("1405") == "۱۴۰۵");
        CHECK(to_persian_digits("abc") == "abc");
        CHECK(to_persian_digits("")    == "");
    }

    std::cout << "All unit tests passed.\n";
    return 0;
}
