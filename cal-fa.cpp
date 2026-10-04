// cal_fa.cpp — مرحله ۱: مطابقت با jcal
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace bg = boost::gregorian;

// ==================== رنگ‌ها ====================
const std::string RESET   = "\033[0m";
const std::string REVERSE = "\033[7m";
const std::string BOLD    = "\033[1m";
const std::string RED     = "\033[31m";
const std::string YELLOW  = "\033[33m";
const std::string BLUE    = "\033[34m";

// ==================== نام ماه‌ها (لاتین) ====================
const std::vector<std::string> MONTH_NAMES = {
    "Farvardin", "Ordibehesht", "Khordad",
    "Tir", "Mordad", "Shahrivar",
    "Mehr", "Aban", "Azar",
    "Dey", "Bahman", "Esfand"
};

// ==================== نام روزهای هفته (لاتین، مطابق jcal) ====================
const std::vector<std::string> WEEKDAY_NAMES = {
    "Sh", "Ye", "Do", "Se", "Ch", "Pa", "Jo"
};

// ==================== ساختار ====================
struct PersianDate { int year, month, day; };

// ==================== تبدیل تاریخ ====================
long persian_to_jdn(int year, int month, int day) {
    long epbase = year - ((year >= 0) ? 474 : 473);
    long epyear = 474 + (epbase % 2820);
    long m = (month <= 7) ? (month - 1) * 31 : (month - 1) * 30 + 6;
    return day + m + ((epyear * 682 - 110) / 2816) +
           (epyear - 1) * 365 + (epbase / 2820) * 1029983 +
           1948320;  // ← اصلاح شد (قبلاً ۱۹۴۸۳۲۰ - ۱ بود)
}

PersianDate to_persian(const bg::date& g) {
    long jdn = g.julian_day();
    long depoch = jdn - 2121446;
    long cycle = depoch / 1029983;
    long cyear = depoch % 1029983;
    long ycycle, aux1, aux2;
    if (cyear == 1029982) {
        ycycle = 2820;
    } else {
        aux1 = cyear / 366;
        aux2 = cyear % 366;
        ycycle = (2134 * aux1 + 2816 * aux2 + 2815) / 1028522 + aux1 + 1;
    }
    long pyear = ycycle + 2820 * cycle + 474;
    if (pyear <= 0) pyear--;
    long yday = jdn - persian_to_jdn(pyear, 1, 1) + 1;
    long pmonth = (yday <= 186) ? (yday - 1) / 31 + 1 : (yday - 7) / 30 + 1;
    long pday = jdn - persian_to_jdn(pyear, pmonth, 1) + 1;
    return { (int)pyear, (int)pmonth, (int)pday };
}

int persian_weekday(int y, int m, int d) {
    // 0 = Sh (شنبه) ... 6 = Jo (جمعه)
    return (persian_to_jdn(y, m, d) + 2) % 7;  // ← اصلاح شد
}

int persian_month_days(int year, int month) {
    if (month <= 6) return 31;
    if (month <= 11) return 30;
    int r = year % 33;
    if (r == 1 || r == 5 || r == 9 || r == 13 ||
        r == 17 || r == 22 || r == 26 || r == 30)
        return 30;
    return 29;
}

// ==================== چاپ تقویم ====================
void print_calendar(int year, int month) {
    std::string title = MONTH_NAMES[month - 1] + " " + std::to_string(year);
    const int width = 20;

    std::cout << "\n";

    // عنوان وسط‌چین
    int pad = (width - (int)title.size()) / 2;
    if (pad < 0) pad = 0;
    std::cout << std::string(pad, ' ') << title << "\n";

    // سرستون روزهای هفته (آبی)
    for (int i = 0; i < 7; ++i) {
        std::cout << BLUE << WEEKDAY_NAMES[i] << RESET;
        if (i < 6) std::cout << " ";
    }
    std::cout << "\n";

    int first_wd = persian_weekday(year, month, 1);
    int days = persian_month_days(year, month);

    // تاریخ امروز
    bg::date today_g = bg::day_clock::local_day();
    PersianDate today_p = to_persian(today_g);
    bool is_current = (today_p.year == year && today_p.month == month);

    // خانه‌های خالی ابتدای ماه
    int col = 0;
    for (int i = 0; i < first_wd; ++i) {
        std::cout << "   ";
        ++col;
    }

    // اعداد روزها
    for (int d = 1; d <= days; ++d) {
        std::string s = std::to_string(d);
        std::string cell = (s.size() < 2) ? (" " + s) : s;
        int wd = persian_weekday(year, month, d);

        if (is_current && today_p.day == d) {
            // امروز: فقط معکوس (پس‌زمینه سفید، متن سیاه)
            std::cout << REVERSE << cell << RESET;
        } else if (wd == 6) {
            // جمعه: زرد
            std::cout << YELLOW << cell << RESET;
        } else {
            std::cout << cell;
        }

        ++col;
        if (col == 7) {
            std::cout << "\n";
            col = 0;
        } else {
            std::cout << " ";
        }
    }
    if (col != 0) std::cout << "\n";
    std::cout << "\n";
}

// ==================== تابع اصلی ====================
int main(int argc, char* argv[]) {
    int year, month;
    bg::date today_g = bg::day_clock::local_day();
    PersianDate today_p = to_persian(today_g);

    if (argc >= 3) {
        month = std::stoi(argv[1]);
        year  = std::stoi(argv[2]);
    } else {
        year  = today_p.year;
        month = today_p.month;
    }

    print_calendar(year, month);
    return 0;
}
