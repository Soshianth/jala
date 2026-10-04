// cal_fa.cpp
// تقویم شمسی در ترمینال، شبیه به ncal

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <boost/date_time/gregorian/gregorian.hpp>
//#include <boost/date_time/julian_calendar.hpp>  // برای تقویم جلالی

namespace bg = boost::gregorian;

// رنگ‌های ANSI برای ترمینال
const std::string RESET   = "\033[0m";
const std::string REVERSE = "\033[7m";
const std::string BOLD    = "\033[1m";

// نام ماه‌های شمسی
const std::vector<std::string> MONTH_NAMES = {
    "فروردین", "اردیبهشت", "خرداد",
    "تیر", "مرداد", "شهریور",
    "مهر", "آبان", "آذر",
    "دی", "بهمن", "اسفند"
};

// نام روزهای هفته (شنبه تا جمعه)
const std::vector<std::string> WEEKDAY_NAMES = {
    "ش", "ی", "د", "س", "چ", "پ", "ج"
};

// ساختار ساده برای تاریخ شمسی
struct PersianDate {
    int year;
    int month;
    int day;
};

// پیش‌اعلان تابع تبدیل تاریخ شمسی به JDN
long persian_to_jdn(int year, int month, int day);

// تبدیل تاریخ میلادی به شمسی با استفاده از boost
PersianDate to_persian(const bg::date& gdate) {
    // boost::gregorian::date را به julian day تبدیل می‌کنیم
    // سپس با استفاده از الگوریتم تبدیل، تاریخ شمسی را به دست می‌آوریم.
    // برای سادگی از کتابخانه‌ی boost::date_time::julian_calendar استفاده نمی‌کنیم
    // چون در نسخه‌های جدید Boost ممکن است در دسترس نباشد.
    // در عوض از الگوریتم استاندارد تبدیل استفاده می‌کنیم.

    long jdn = gdate.julian_day();  // شماره روز ژولینی

    // الگوریتم تبدیل JDN به تقویم جلالی
    long depoch = jdn - 2121446;  // مبدأ تقویم جلالی (۲۱ مارس ۶۲۲ میلادی)
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

    return { static_cast<int>(pyear), static_cast<int>(pmonth), static_cast<int>(pday) };
}

// تبدیل تاریخ شمسی به JDN
long persian_to_jdn(int year, int month, int day) {
    long epbase = year - ((year >= 0) ? 474 : 473);
    long epyear = 474 + (epbase % 2820);
    long m = (month <= 7) ? (month - 1) * 31 : (month - 1) * 30 + 6;
    return day + m + ((epyear * 682 - 110) / 2816) +
           (epyear - 1) * 365 + (epbase / 2820) * 1029983 +
           (1948320 - 1);
}

// محاسبه روز هفته (۰ = شنبه، ...، ۶ = جمعه)
int persian_weekday(int year, int month, int day) {
    long jdn = persian_to_jdn(year, month, day);
    // JDN % 7: 0=دوشنبه، 1=سه‌شنبه، ...، 5=شنبه، 6=یکشنبه
    // می‌خواهیم شنبه=0 شود:
    int w = (jdn + 1) % 7;  // 0=شنبه، 1=یکشنبه، ...، 6=جمعه
    return w;
}

// تعداد روزهای یک ماه شمسی
int persian_month_days(int year, int month) {
    if (month <= 6) return 31;
    if (month <= 11) return 30;
    // اسفند
    // سال کبیسه: باقی‌مانده سال بر ۳۳ تقسیم بر ۳۳ و ...
    // الگوریتم ساده: اگر (year % 33) در لیست کبیسه‌ها باشد
    static const int leap_remainders[] = {
        1, 5, 9, 13, 17, 22, 26, 30
    };
    int r = year % 33;
    for (int lr : leap_remainders) {
        if (r == lr) return 30;
    }
    return 29;
}

// چاپ تقویم یک ماه شمسی
void print_calendar(int year, int month, bool highlight_today = true) {
    std::string title = MONTH_NAMES[month - 1] + " " + std::to_string(year);
    const int width = 20;

    std::cout << "\n";
    // وسط‌چین کردن عنوان (تقریبی، چون فارسی عرض متفاوتی دارد)
    int padding = (width - static_cast<int>(title.size())) / 2;
    if (padding < 0) padding = 0;
    std::cout << std::string(padding, ' ') << title << "\n";
    std::cout << std::string(width, '-') << "\n";

    // چاپ نام روزهای هفته
    for (const auto& name : WEEKDAY_NAMES) {
        std::cout << std::setw(2) << name << " ";
    }
    std::cout << "\n";

    int first_wd = persian_weekday(year, month, 1);
    int days = persian_month_days(year, month);

    // تاریخ امروز
    bg::date today_g = bg::day_clock::local_day();
    PersianDate today = to_persian(today_g);
    bool is_current = (today.year == year && today.month == month);

    // خانه‌های خالی ابتدای ماه
    int col = 0;
    for (int i = 0; i < first_wd; ++i) {
        std::cout << "   ";
        ++col;
    }

    for (int d = 1; d <= days; ++d) {
        std::string cell = std::to_string(d);
        // راست‌چین کردن در عرض ۲ کاراکتر
        std::string padded = (cell.size() < 2) ? (" " + cell) : cell;

        if (highlight_today && is_current && today.day == d) {
            std::cout << REVERSE << BOLD << padded << RESET << " ";
        } else {
            std::cout << padded << " ";
        }

        ++col;
        if (col == 7) {
            std::cout << "\n";
            col = 0;
        }
    }
    if (col != 0) std::cout << "\n";
    std::cout << "\n";
}

int main(int argc, char* argv[]) {
    // تاریخ امروز شمسی
    bg::date today_g = bg::day_clock::local_day();
    PersianDate today = to_persian(today_g);

    int year = today.year;
    int month = today.month;

    // آرگومان‌های خط فرمان: cal_fa [month] [year]
    if (argc >= 2) {
        try {
            month = std::stoi(argv[1]);
            if (month < 1 || month > 12) {
                std::cerr << "خطا: شماره ماه باید بین ۱ تا ۱۲ باشد.\n";
                return 1;
            }
        } catch (...) {
            std::cerr << "خطا: شماره ماه باید عدد باشد.\n";
            return 1;
        }
    }
    if (argc >= 3) {
        try {
            year = std::stoi(argv[2]);
        } catch (...) {
            std::cerr << "خطا: سال باید عدد باشد.\n";
            return 1;
        }
    }

    print_calendar(year, month);
    return 0;
}
