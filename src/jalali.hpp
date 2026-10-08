// =============================================================================
// jalali.hpp — Persian (Jalali) <-> Gregorian calendar conversion
// SPDX-License-Identifier: MIT
//
// This header exposes the pure calendar-math layer of jala: conversion
// between the Jalali and Gregorian calendars, weekday and month-day
// arithmetic, and the canonical name tables. It has no dependency on
// I/O, terminal formatting, or command-line options, so it can be
// linked into other tools or unit-tested in isolation.
// =============================================================================

#pragma once

#include <array>
#include <string>
#include <string_view>

#include "gregorian.hpp"

namespace jala {

// =============================================================================
// Constants
// =============================================================================

inline constexpr int MAX_YEAR        = 9999;
inline constexpr int MAX_MONTH       = 12;
inline constexpr int IMPERIAL_OFFSET = 1180;  // Imperial = Jalali + 1180

// =============================================================================
// Data types
// =============================================================================

struct PersianDate {
    int year  = 0;
    int month = 0;
    int day   = 0;
};

// A date parsed from user input. `jalali` records whether the year was
// below 1700 and thus interpreted as a Jalali date (see parse_date).
struct SimpleDate {
    int  year   = 0;
    int  month  = 0;
    int  day    = 0;
    bool jalali = false;
    bool valid  = false;
};

// =============================================================================
// Name tables
// =============================================================================

// Latin transliterations of the twelve Persian months.
inline constexpr std::array<const char*, 12> MONTHS_EN = {
    "Farvardin", "Ordibehesht", "Khordad",
    "Tir",       "Mordad",      "Shahrivar",
    "Mehr",      "Aban",        "Azar",
    "Dey",       "Bahman",      "Esfand"
};

// Persian names of the twelve months.
inline constexpr std::array<const char*, 12> MONTHS_FA = {
    "فروردین", "اردیبهشت", "خرداد",
    "تیر",     "مرداد",    "شهریور",
    "مهر",     "آبان",     "آذر",
    "دی",      "بهمن",     "اسفند"
};

// Short Latin weekday headers (Shanbeh..Jomeh), modeled after jcal.
inline constexpr std::array<const char*, 7> WEEKDAYS_SHORT = {
    "Sh", "Ye", "Do", "Se", "Ch", "Pa", "Jo"
};

// Three-letter English weekday abbreviations (Saturday..Friday).
inline constexpr std::array<const char*, 7> WEEKDAYS_ABBR_EN = {
    "Sat", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri"
};

// Single-letter Persian weekday headers.
inline constexpr std::array<const char*, 7> WEEKDAYS_FA = {
    "ش", "ی", "د", "س", "چ", "پ", "ج"
};

// Full English weekday names (Saturday..Friday).
inline constexpr std::array<const char*, 7> WEEKDAYS_FULL_EN = {
    "Saturday", "Sunday",    "Monday",    "Tuesday",
    "Wednesday", "Thursday", "Friday"
};

// Full Persian weekday names.
inline constexpr std::array<const char*, 7> WEEKDAYS_FULL_FA = {
    "شنبه", "یکشنبه", "دوشنبه", "سه‌شنبه",
    "چهارشنبه", "پنجشنبه", "جمعه"
};

// =============================================================================
// Conversion
// =============================================================================

// Convert a Jalali date to a Julian Day Number.
[[nodiscard]] long persian_to_jdn(int year, int month, int day);

// Convert a Julian Day Number to a Jalali date.
[[nodiscard]] PersianDate jdn_to_persian(long jdn);

// Convert a Gregorian date to a Jalali date.
[[nodiscard]] PersianDate to_persian(const GregorianDate& g);

// Convert a Julian Day Number to a Gregorian date.
[[nodiscard]] GregorianDate jdn_to_gregorian(long jdn);

// =============================================================================
// Calendar arithmetic
// =============================================================================

// Weekday index for a Jalali date: 0 = Shanbeh (Saturday), 6 = Jomeh.
[[nodiscard]] int persian_weekday(int year, int month, int day);

// Returns true if `year` is a leap year in the Jalali calendar.
// A leap year has 366 days; Esfand has 30 days instead of 29.
[[nodiscard]] bool persian_is_leap(int year);

// Number of days in a Jalali month, accounting for leap years.
[[nodiscard]] int persian_month_days(int year, int month);

// Day of the year (1..365 or 1..366) for a Jalali date.
[[nodiscard]] int persian_day_of_year(int year, int month, int day);

// =============================================================================
// Parsing and formatting helpers
// =============================================================================

// Parse a date string. Accepts "YYYY/MM/DD", "YYYY-MM-DD", or "today".
//
// Years below 1700 are treated as Jalali; otherwise as Gregorian.
//
// A date is considered valid only if:
//   - each numeric component is a complete, well-formed integer
//     (no trailing characters),
//   - the month is in [1, 12],
//   - the day is in [1, days_in_month(year, month)],
//   - for Gregorian dates, the date exists (e.g. February 30 is
//     rejected).
//
// On any failure, the returned SimpleDate has valid == false.
[[nodiscard]] SimpleDate parse_date(std::string_view s);

// Replace ASCII digits in `s` with their Persian counterparts.
[[nodiscard]] std::string to_persian_digits(std::string_view s);

}  // namespace jala