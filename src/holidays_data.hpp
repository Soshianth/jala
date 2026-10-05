// =============================================================================
// holidays_data.hpp — auto-generated. DO NOT EDIT MANUALLY.
// Regenerate with: python3 scripts/embed_holidays.py
// Source of truth: data/holidays.json
// =============================================================================

#pragma once

namespace jala {

struct HolidayEntry {
    const char* date;  // YYYY/MM/DD (Jalali)
    const char* name;  // UTF-8 name
};

inline constexpr int HOLIDAYS_COUNT = 12;

inline constexpr HolidayEntry HOLIDAYS[] = {
    {"1405/01/01", "نوروز"},
    {"1405/01/02", "نوروز"},
    {"1405/01/03", "نوروز"},
    {"1405/01/04", "نوروز"},
    {"1405/01/12", "روز جمهوری اسلامی"},
    {"1405/01/13", "روز طبیعت"},
    {"1405/03/14", "رحلت امام خمینی"},
    {"1405/03/15", "قیام ۱۵ خرداد"},
    {"1405/05/05", "تاسوعای حسینی (نمونه - بررسی شود)"},
    {"1405/05/06", "عاشورای حسینی (نمونه - بررسی شود)"},
    {"1405/11/22", "پیروزی انقلاب اسلامی"},
    {"1405/12/29", "روز ملی شدن صنعت نفت"},
};

}  // namespace jala
