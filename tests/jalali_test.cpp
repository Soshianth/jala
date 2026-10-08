// =============================================================================
// jalali_test.cpp — unit tests for the calendar-math layer
// SPDX-License-Identifier: MIT
//
// These tests exercise src/jalali.cpp and src/gregorian.cpp directly,
// without going through the CLI. They are fast (a few milliseconds)
// and cover the pure conversion, arithmetic, parsing, and validation
// functions.
//
// Build:  make test-unit
// =============================================================================

#include "gregorian.hpp"
#include "holidays.hpp"
#include "jalali.hpp"

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

// Numeric comparison helper: prints both operands when they differ,
// which makes off-by-one bugs much easier to diagnose.
#define CHECK_EQ(a, b)                                                       \
    do {                                                                     \
        const auto _a = (a);                                                 \
        const auto _b = (b);                                                 \
        if (!(_a == _b)) {                                                   \
            std::cerr << "FAIL: " << #a << " == " << #b                      \
                      << "  (got " << _a << " and " << _b << ")"             \
                      << " at " << __FILE__ << ":" << __LINE__ << "\n";      \
            return 1;                                                        \
        }                                                                    \
    } while (0)

int main() {
    // =========================================================================
    // Gregorian core: JDN round trip and reference value
    // =========================================================================
    {
        // 1970-01-01 (Unix epoch) has JDN 2,440,588.
        CHECK_EQ(to_jdn(1970, 1, 1), 2440588L);

        const GregorianDate g = from_jdn(to_jdn(2026, 10, 4));
        CHECK_EQ(g.year,  2026);
        CHECK_EQ(g.month, 10);
        CHECK_EQ(g.day,   4);
    }

    // =========================================================================
    // Gregorian validity, including leap-year rules
    // =========================================================================
    {
        // Valid
        CHECK(is_valid(2026, 10, 4));
        CHECK(is_valid(2024, 2, 29));   // divisible by 4
        CHECK(is_valid(2000, 2, 29));   // divisible by 400

        // Invalid
        CHECK(!is_valid(2023, 2, 29));  // common year
        CHECK(!is_valid(1900, 2, 29));  // divisible by 100, not 400
        CHECK(!is_valid(2026, 13, 1));  // month out of range
        CHECK(!is_valid(2026, 0, 1));   // month below range
        CHECK(!is_valid(2026, 4, 31));  // April has 30 days
        CHECK(!is_valid(2026, 1, 0));   // day below range
        CHECK(!is_valid(0, 1, 1));      // year below range
    }

    // =========================================================================
    // Known Jalali -> Gregorian equivalences
    // =========================================================================
    // 1405/07/12 == 2026-10-04
    {
        const GregorianDate g = jdn_to_gregorian(persian_to_jdn(1405, 7, 12));
        CHECK_EQ(g.year,  2026);
        CHECK_EQ(g.month, 10);
        CHECK_EQ(g.day,   4);
    }
    // 1403/01/01 == 2024-03-20 (Nowruz 1403)
    {
        const GregorianDate g = jdn_to_gregorian(persian_to_jdn(1403, 1, 1));
        CHECK_EQ(g.year,  2024);
        CHECK_EQ(g.month, 3);
        CHECK_EQ(g.day,   20);
    }
    // 1398/10/11 == 2020-01-01
    {
        const GregorianDate g = jdn_to_gregorian(persian_to_jdn(1398, 10, 11));
        CHECK_EQ(g.year,  2020);
        CHECK_EQ(g.month, 1);
        CHECK_EQ(g.day,   1);
    }

    // =========================================================================
    // Known Gregorian -> Jalali equivalences (reverse of the above)
    // =========================================================================
    {
        const PersianDate p = to_persian(GregorianDate{ 2026, 10, 4 });
        CHECK_EQ(p.year,  1405);
        CHECK_EQ(p.month, 7);
        CHECK_EQ(p.day,   12);
    }
    {
        const PersianDate p = to_persian(GregorianDate{ 2024, 3, 20 });
        CHECK_EQ(p.year,  1403);
        CHECK_EQ(p.month, 1);
        CHECK_EQ(p.day,   1);
    }
    {
        const PersianDate p = to_persian(GregorianDate{ 2020, 1, 1 });
        CHECK_EQ(p.year,  1398);
        CHECK_EQ(p.month, 10);
        CHECK_EQ(p.day,   11);
    }

    // =========================================================================
    // Jalali round trip over a range of dates
    // =========================================================================
    // Conversion must be its own inverse for every valid date in the
    // sample range. This catches drift or off-by-one errors in the JDN
    // arithmetic across month and year boundaries.
    {
        static constexpr int years[] = { 1398, 1400, 1403, 1404, 1405 };
        for (int y : years) {
            for (int m = 1; m <= 12; ++m) {
                const int dim = persian_month_days(y, m);
                for (int d = 1; d <= dim; ++d) {
                    const PersianDate p =
                        jdn_to_persian(persian_to_jdn(y, m, d));
                    if (p.year != y || p.month != m || p.day != d) {
                        std::cerr << "FAIL: Jalali round trip "
                                  << y << "/" << m << "/" << d
                                  << " -> " << p.year << "/"
                                  << p.month << "/" << p.day << "\n";
                        return 1;
                    }
                }
            }
        }
    }

    // =========================================================================
    // Gregorian round trip over a range of dates, including leap years
    // =========================================================================
    {
        static constexpr int years[] = { 1970, 2000, 2024, 2026, 2100 };
        for (int y : years) {
            for (int m = 1; m <= 12; ++m) {
                const int dim = (m == 2)
                    ? (is_valid(y, 2, 29) ? 29 : 28)
                    : ((m == 4 || m == 6 || m == 9 || m == 11) ? 30 : 31);
                for (int d = 1; d <= dim; ++d) {
                    const GregorianDate g = from_jdn(to_jdn(y, m, d));
                    if (g.year != y || g.month != m || g.day != d) {
                        std::cerr << "FAIL: Gregorian round trip "
                                  << y << "-" << m << "-" << d
                                  << " -> " << g.year << "-"
                                  << g.month << "-" << g.day << "\n";
                        return 1;
                    }
                }
            }
        }
    }

    // =========================================================================
    // Weekday: 1405/07/12 is a Sunday (index 1, where 0 = Shanbeh)
    // =========================================================================
    {
        CHECK_EQ(persian_weekday(1405, 7, 12), 1);
    }

    // =========================================================================
    // Persian month lengths
    // =========================================================================
    {
        CHECK_EQ(persian_month_days(1405, 1),  31);  // Farvardin
        CHECK_EQ(persian_month_days(1405, 7),  30);  // Mehr
        CHECK_EQ(persian_month_days(1405, 11), 30);  // Bahman
        CHECK_EQ(persian_month_days(1405, 12), 29);  // Esfand, common year
    }

    // =========================================================================
    // Persian leap years: 1403 is a leap year, 1404 is not
    // =========================================================================
    {
        CHECK_EQ(persian_month_days(1403, 12), 30);
        CHECK_EQ(persian_month_days(1404, 12), 29);
    }

    // =========================================================================
    // Day of year
    // =========================================================================
    // Months 1..6 have 31 days each, so the last day of Shahrivar is
    // day 186 and the first day of Mehr is day 187.
    {
        CHECK_EQ(persian_day_of_year(1405, 1, 1),    1);
        CHECK_EQ(persian_day_of_year(1405, 1, 31),   31);
        CHECK_EQ(persian_day_of_year(1405, 6, 31),   186);
        CHECK_EQ(persian_day_of_year(1405, 7, 1),    187);
        CHECK_EQ(persian_day_of_year(1405, 7, 12),   198);
        CHECK_EQ(persian_day_of_year(1405, 12, 29),  365);
    }

    // =========================================================================
    // parse_date: Jalali input
    // =========================================================================
    {
        const SimpleDate d = parse_date("1405/07/12");
        CHECK(d.valid);
        CHECK(d.jalali);
        CHECK_EQ(d.year,  1405);
        CHECK_EQ(d.month, 7);
        CHECK_EQ(d.day,   12);
    }

    // =========================================================================
    // parse_date: Gregorian input
    // =========================================================================
    {
        const SimpleDate d = parse_date("2026-10-04");
        CHECK(d.valid);
        CHECK(!d.jalali);
        CHECK_EQ(d.year,  2026);
        CHECK_EQ(d.month, 10);
        CHECK_EQ(d.day,   4);
    }

    // =========================================================================
    // parse_date: "today" keyword
    // =========================================================================
    {
        const SimpleDate d = parse_date("today");
        CHECK(d.valid);
        CHECK(d.jalali);
    }

    // =========================================================================
    // parse_date: valid Jalali inputs
    // =========================================================================
    {
        CHECK(parse_date("1405/07/12").valid);
        CHECK(parse_date("1405-07-12").valid);
        CHECK(parse_date("1403/12/30").valid);  // leap year
        CHECK(parse_date("today").valid);
        CHECK(parse_date("now").valid);
    }

    // =========================================================================
    // parse_date: valid Gregorian inputs
    // =========================================================================
    {
        CHECK(parse_date("2026-10-04").valid);
        CHECK(parse_date("2024-02-29").valid);  // Gregorian leap year
    }

    // =========================================================================
    // parse_date: malformed numbers
    // =========================================================================
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

    // =========================================================================
    // parse_date: out-of-range Jalali fields
    // =========================================================================
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

    // =========================================================================
    // parse_date: out-of-range Gregorian fields
    // =========================================================================
    {
        CHECK(!parse_date("2026-02-30").valid);  // Feb 30
        CHECK(!parse_date("2025-02-29").valid);  // non-leap Feb 29
        CHECK(!parse_date("2026-04-31").valid);  // April has 30 days
        CHECK(!parse_date("2026-13-01").valid);  // month 13
        CHECK(!parse_date("2026-00-01").valid);  // month 0
        CHECK(!parse_date("2026-01-00").valid);  // day 0
    }

    // =========================================================================
    // to_persian_digits
    // =========================================================================
    {
        CHECK(to_persian_digits("1405") == "۱۴۰۵");
        CHECK(to_persian_digits("abc")  == "abc");
        CHECK(to_persian_digits("")     == "");
    }

    // =========================================================================
    // HolidayIndex
    // =========================================================================
    {
        const HolidayIndex idx;
        CHECK(idx.size() > 0);
        CHECK(idx.contains("1405/01/01"));
        CHECK(!idx.name("1405/01/01").empty());
        CHECK(!idx.contains("1404/01/01"));  // wrong year
    }

    // =========================================================================
    // EventIndex
    // =========================================================================
    {
        const EventIndex idx;
        CHECK(idx.size() > 0);
        const auto& evs = idx.events("1405/01/01");
        CHECK(!evs.empty());
        // Every event has a non-empty description.
        for (const auto& e : evs) CHECK(!e.description.empty());
    }

    // =========================================================================
    // EventIndex: 1405/01/01 has at least one holiday event
    // =========================================================================
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

    // =========================================================================
    // EventIndex: a date outside the dataset has no events
    // =========================================================================
    {
        const EventIndex idx;
        const auto& evs = idx.events("1399/01/01");
        CHECK(evs.empty());
    }

    // =========================================================================
    // leap-year consistency: persian_month_days agrees with JDN
    // =========================================================================
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