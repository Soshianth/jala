// =============================================================================
// jalali_test.cpp — unit tests for the calendar-math layer
// SPDX-License-Identifier: MIT
//
// These tests exercise src/jalali.cpp and src/gregorian.cpp directly,
// without going through the CLI. They are fast (a few milliseconds)
// and cover the pure conversion, arithmetic, parsing, and validation
// functions.
//
// Reference values are drawn from two independent sources:
//
//   1. The official test suite of jalaali-js
//      (https://github.com/jalaali/jalaali-js, MIT), which is the
//      reference implementation of the Borkowski 1996 algorithm.
//   2. The official Iranian calendar for Nowruz dates in the
//      1394..1405 range.
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

// Assert that a Jalali date and a Gregorian date denote the same day.
// Both directions are checked, and the reverse conversions are
// verified as well.
#define CHECK_EQUIVALENT(jy, jm, jd, gy, gm, gd)                             \
    do {                                                                     \
        const GregorianDate _g =                                             \
            jdn_to_gregorian(persian_to_jdn(jy, jm, jd));                    \
        CHECK_EQ(_g.year,  gy);                                              \
        CHECK_EQ(_g.month, gm);                                              \
        CHECK_EQ(_g.day,   gd);                                              \
        const PersianDate _p = to_persian(GregorianDate{ gy, gm, gd });      \
        CHECK_EQ(_p.year,  jy);                                              \
        CHECK_EQ(_p.month, jm);                                              \
        CHECK_EQ(_p.day,   jd);                                              \
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
    // Known Jalali <-> Gregorian equivalences
    //
    // Every Nowruz in the range below is well-attested:
    //
    //   1394/01/01 = 2015-03-21   (Saturday)
    //   1395/01/01 = 2016-03-20   (Sunday)
    //   1403/01/01 = 2024-03-20   (Wednesday)
    //   1404/01/01 = 2025-03-21   (Friday)
    //
    // The remaining values exercise the days around leap-year
    // boundaries. 1394 is a common year (29 days in Esfand) and 1395
    // is a leap year (30 days), so the day before Nowruz 1395 is
    // 1394/12/29. Likewise 1403 is leap, so the day before Nowruz
    // 1404 is 1403/12/30.
    // =========================================================================
    {
        CHECK_EQUIVALENT(1394, 1, 1,   2015, 3, 21);  // Nowruz 1394
        CHECK_EQUIVALENT(1394, 12, 10, 2016, 2, 29);  // inside Esfand 1394
        CHECK_EQUIVALENT(1394, 12, 29, 2016, 3, 19);  // last day of common 1394
        CHECK_EQUIVALENT(1395, 1, 1,   2016, 3, 20);  // Nowruz 1395
        CHECK_EQUIVALENT(1395, 1, 22,  2016, 4, 10);  // 22nd day of 1395
        CHECK_EQUIVALENT(1403, 1, 1,   2024, 3, 20);  // Nowruz 1403
        CHECK_EQUIVALENT(1403, 12, 30, 2025, 3, 20);  // last day of leap 1403
        CHECK_EQUIVALENT(1404, 1, 1,   2025, 3, 21);  // Nowruz 1404
        CHECK_EQUIVALENT(1405, 7, 12,  2026, 10, 4);  // verified elsewhere
        CHECK_EQUIVALENT(1398, 10, 11, 2020, 1, 1);   // verified elsewhere
    }

    // =========================================================================
    // Jalali round trip over a range of dates
    // =========================================================================
    // Conversion must be its own inverse for every valid date in the
    // sample range. This catches drift or off-by-one errors in the JDN
    // arithmetic across month and year boundaries.
    {
        static constexpr int years[] = {
            1300, 1350, 1391, 1394, 1395, 1398, 1400,
            1403, 1404, 1405, 1420, 1450, 1500, 1600,
        };
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
    // Persian leap years — reference values
    // =========================================================================
    // These values are taken directly from the jalaali-js test suite
    // and the official Iranian calendar. They exercise the era table
    // across several centuries, including boundaries where the older
    // 33-year cycle would disagree with the official calendar.
    {
        // Leap years (jalaali-js: isLeapJalaaliYear → true)
        CHECK( persian_is_leap(1391));
        CHECK( persian_is_leap(1395));
        CHECK( persian_is_leap(1403));

        // Common years
        CHECK(!persian_is_leap(1394));
        CHECK(!persian_is_leap(1404));

        // Esfand length matches the leap state
        CHECK_EQ(persian_month_days(1395, 12), 30);
        CHECK_EQ(persian_month_days(1394, 12), 29);
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
    // parse_date: heuristic boundary between Jalali and Gregorian
    // =========================================================================
    // parse_date uses a simple heuristic: a year below 1700 is
    // treated as Jalali, anything else as Gregorian. This means the
    // algorithm upper bound (MAX_YEAR = 3177, the last year defined
    // by the jalaali-js era table) is *not* reachable through
    // parse_date — any year above 1699 is interpreted as Gregorian
    // before MAX_YEAR is ever consulted. The CLI enforces the Jalali
    // bound separately for numeric arguments (see main.cpp).
    //
    // We assert the actual heuristic boundary here.
    {
        const SimpleDate jalali    = parse_date("1699/12/29");
        const SimpleDate gregorian = parse_date("1700/01/01");

        CHECK( jalali.valid);
        CHECK( gregorian.valid);
        CHECK( jalali.jalali);
        CHECK(!gregorian.jalali);

        CHECK_EQ(jalali.year, 1699);
        CHECK_EQ(gregorian.year, 1700);
    }

    // =========================================================================
    // parse_date: explicit CalendarMode
    // =========================================================================
    // An explicit mode overrides the year-based heuristic. This is
    // what makes it possible to enter a Jalali year above 1699, or to
    // force a year below 1700 to be read as Gregorian.
    {
        // Same string, two interpretations.
        const SimpleDate as_jalali =
            parse_date("1405/07/12", CalendarMode::Jalali);
        const SimpleDate as_gregorian =
            parse_date("1405/07/12", CalendarMode::Gregorian);

        CHECK(as_jalali.valid);
        CHECK( as_jalali.jalali);

        CHECK(as_gregorian.valid);
        CHECK(!as_gregorian.jalali);

        // Auto reproduces the heuristic.
        CHECK( parse_date("1405/07/12", CalendarMode::Auto).jalali);
        CHECK(!parse_date("2026-10-04", CalendarMode::Auto).jalali);

        // 1404/12/30 is invalid in the Jalali calendar (1404 is a
        // common year) but perfectly fine as a Gregorian date.
        CHECK(!parse_date("1404/12/30", CalendarMode::Jalali).valid);
        CHECK( parse_date("1404/12/30", CalendarMode::Gregorian).valid);

        // A Jalali year above the heuristic boundary now works.
        const SimpleDate big = parse_date("3177/01/01", CalendarMode::Jalali);
        CHECK(big.valid);
        CHECK(big.jalali);
        CHECK_EQ(big.year, 3177);
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
    // Leap-year consistency across a wide range
    // =========================================================================
    // persian_month_days and persian_to_jdn are derived from the same
    // jal_cal() lookup, so they must agree on the length of every
    // month in every year of the supported range.
    //
    // This test walks a broad range of years to make sure that the era
    // table and the month-length logic never drift apart.
    {
        for (int y = 1200; y <= 2000; ++y) {
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