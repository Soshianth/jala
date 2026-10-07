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
#include "holidays.hpp"

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

    // ---- parse_date: valid Jalali inputs ----
    {
        CHECK(parse_date("1405/07/12").valid);
        CHECK(parse_date("1405-07-12").valid);
        CHECK(parse_date("1403/12/30").valid);  // leap year
        CHECK(parse_date("today").valid);
        CHECK(parse_date("now").valid);
    }

    // ---- parse_date: valid Gregorian inputs ----
    {
        CHECK(parse_date("2026-10-04").valid);
        CHECK(parse_date("2024-02-29").valid);  // Gregorian leap year
    }

    // ---- parse_date: malformed numbers ----
    {
        CHECK(!parse_date("garbage").valid);
        CHECK(!parse_date("").valid);
        CHECK(!parse_date("1405").valid);           // one component
        CHECK(!parse_date("1405/07").valid);        // two components
        CHECK(!parse_date("1405/07/12/13").valid);  // four components
        CHECK(!parse_date("1405//12").valid);       // empty component
        CHECK(!parse_date("1405/07/12xyz").valid);  // trailing chars
        CHECK(!parse_date("1405/0x7/12").valid);    // hex-like
        CHECK(!parse_date("14O5/07/12").valid);     // letter O
    }

    // ---- parse_date: out-of-range Jalali fields ----
    {
        CHECK(!parse_date("1405/00/12").valid);  // month 0
        CHECK(!parse_date("1405/13/12").valid);  // month 13
        CHECK(!parse_date("1405/07/00").valid);  // day 0
        CHECK(!parse_date("1405/07/32").valid);  // >31
        CHECK(!parse_date("1405/08/31").valid);  // 30-day month
        CHECK(!parse_date("1405/12/30").valid);  // non-leap Esfand
        CHECK(!parse_date("1403/12/31").valid);  // >30 in leap Esfand
        CHECK(!parse_date("0/01/01").valid);     // year 0
    }

    // ---- parse_date: out-of-range Gregorian fields ----
    {
        CHECK(!parse_date("2026-02-30").valid);  // Feb 30
        CHECK(!parse_date("2025-02-29").valid);  // non-leap Feb 29
        CHECK(!parse_date("2026-04-31").valid);  // April has 30 days
        CHECK(!parse_date("2026-13-01").valid);  // month 13
        CHECK(!parse_date("2026-00-01").valid);  // month 0
        CHECK(!parse_date("2026-01-00").valid);  // day 0
    }

    // ---- to_persian_digits ----
    {
        CHECK(to_persian_digits("1405") == "۱۴۰۵");
        CHECK(to_persian_digits("abc") == "abc");
        CHECK(to_persian_digits("")    == "");
    }

    // ---- HolidayIndex ----
    {
        const HolidayIndex idx;
        CHECK(idx.size() > 0);
        CHECK(idx.contains("1405/01/01"));
        CHECK(!idx.name("1405/01/01").empty());
        CHECK(!idx.contains("1404/01/01"));  // wrong year
    }

    // ---- EventIndex ----
    {
        const EventIndex idx;
        CHECK(idx.size() > 0);
        const auto& evs = idx.events("1405/01/01");
        CHECK(!evs.empty());
        // Every event has a non-empty description.
        for (const auto& e : evs) CHECK(!e.description.empty());
    }

    // ---- EventIndex: 1405/01/01 has at least one holiday event ----
    {
        const EventIndex idx;
        const auto& evs = idx.events("1405/01/01");
        CHECK(!evs.empty());
        bool found_holiday = false;
        for (const auto& e : evs) {
            if (e.is_holiday) { found_holiday = true; break; }
        }
        CHECK(found_holiday);
    }

    // ---- EventIndex: a date outside the dataset has no events ----
    {
        const EventIndex idx;
        const auto& evs = idx.events("1399/01/01");
        CHECK(evs.empty());
    }

    // ---- leap-year consistency: persian_month_days agrees with JDN ----
    // For every month of every year in a wide range, the day count
    // computed by persian_month_days must equal the JDN difference
    // between the first of the month and the first of the next month.
    {
        for (int y = 1300; y <= 1500; ++y) {
            for (int m = 1; m <= 12; ++m) {
                const long j1 = persian_to_jdn(y, m, 1);

                int nm = m + 1;
                int ny = y;
                if (nm > 12) { nm = 1; ++ny; }
                const long j2 = persian_to_jdn(ny, nm, 1);

                const int expected = static_cast<int>(j2 - j1);
                if (persian_month_days(y, m) != expected) {
                    std::cerr << "Mismatch at " << y << "/" << m
                              << ": month_days=" << persian_month_days(y, m)
                              << ", JDN diff=" << expected << "\n";
                    return 1;
                }
            }
        }
    }

    std::cout << "All unit tests passed.\n";
    return 0;
}
