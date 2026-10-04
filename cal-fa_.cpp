// cal_fa.cpp
// تقویم شمسی در ترمینال، شبیه به ncal
// نسخه ۲: آرگومان‌های خط فرمان، هایلایت جمعه، نمایش تاریخ امروز

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>  // برای getopt
#include <boost/date_time/gregorian/gregorian.hpp>

namespace bg = boost::gregorian;

// ==================== ثابت‌ها و رنگ‌ها ====================
const std::string RESET   = "\033[0m";
const std::string REVERSE = "\033[7m";
const std::string BOLD    = "\033[1m";
const std::string RED     = "\033[31m";
const std::string GREEN   = "\033[32m";
const std::string YELLOW  = "\033[33m";
const std::string BLUE    = "\033[34m";
const std::string CYAN    = "\033[36m";

const std::string VERSION = "2.0.0";

// نام ماه‌های شمسی
const std::vector<std::string> MONTH_NAMES = {
    "فروردین", "اردیبهشت", "خرداد",
    "تیر", "مرداد", "شهریور",
    "مهر", "آبان", "آذر",
    "دی", "بهمن", "اسفند"
};

// نام ماه‌های میلادی
const std::vector<std::string> GREG_MONTH_NAMES = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

// نام روزهای هفته (شنبه تا جمعه)
const std::vector<std::string> WEEKDAY_NAMES = {
    "Sh", "Ye", "Do", "Se", "Ch", "Pa", "Jo"
};

// ==================== ساختارها ====================
struct PersianDate {
    int year;
    int month;
    int day;
};

struct Options {
    int year = -1;
    int month = -1;
    bool full_year = false;
    bool three_months = false;
    bool show_today = true;
    bool color = true;
};

// ==================== پیش‌اعلان توابع ====================
long persian_to_jdn(int year, int month, int day);
PersianDate to_persian(const bg::date& gdate);
int persian_weekday(int year, int month, int day);
int persian_month_days(int year, int month);
void print_calendar(int year, int month, const Options& opts);
void print_year(int year, const Options& opts);
void print_three_months(int year, int month, const Options& opts);
void print_today_info();
void print_help(const char* prog_name);

// ==================== تبدیل تاریخ ====================
long persian_to_jdn(int year, int month, int day) {
    long epbase = year - ((year >= 0) ? 474 : 473);
    long epyear = 474 + (epbase % 2820);
    long m = (month <= 7) ? (month - 1) * 31 : (month - 1) * 30 + 6;
    return day + m + ((epyear * 682 - 110) / 2816) +
           (epyear - 1) * 365 + (epbase / 2820) * 1029983 +
           1948320;  // اصلاح‌شده
}

PersianDate to_persian(const bg::date& gdate) {
    long jdn = gdate.julian_day();
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

    return { static_cast<int>(pyear), static_cast<int>(pmonth), static_cast<int>(pday) };
}

int persian_weekday(int year, int month, int day) {
    long jdn = persian_to_jdn(year, month, day);
    return (jdn + 2) % 7;  // اصلاح‌شده
}

int persian_month_days(int year, int month) {
    if (month <= 6) return 31;
    if (month <= 11) return 30;
    static const int leap_remainders[] = {1, 5, 9, 13, 17, 22, 26, 30};
    int r = year % 33;
    for (int lr : leap_remainders) {
        if (r == lr) return 30;
    }
    return 29;
}

// ==================== چاپ تقویم ====================
void print_calendar(int year, int month, const Options& opts) {
    std::string title = MONTH_NAMES[month - 1] + " " + std::to_string(year);
    const int width = 21;

    std::cout << "\n";

    // رنگ‌بندی عنوان
    if (opts.color) std::cout << BOLD << CYAN;
    int padding = (width - static_cast<int>(title.size())) / 2;
    if (padding < 0) padding = 0;
    std::cout << std::string(padding, ' ') << title << "\n";
    if (opts.color) std::cout << RESET;

    // خط جداکننده با کاراکتر یونیکد
    std::cout << "─────────────────────\n";

    // چاپ نام روزهای هفته
    // نکته: حروف فارسی چند بایتی هستند و std::setw(2) درست کار نمی‌کند.
    // بنابراین به صورت دستی یک فاصله قبل و بعد از هر حرف می‌گذاریم
    // تا هر سلول دقیقاً ۳ ستون عرض بگیرد (مثل اعداد).
    for (const auto& name : WEEKDAY_NAMES) {
        if (opts.color) std::cout << BLUE;
        std::cout << name << " ";
        if (opts.color) std::cout << RESET;
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
        std::string padded = (cell.size() < 2) ? (" " + cell) : cell;
        int wd = persian_weekday(year, month, d);

        // امروز: معکوس + پررنگ
        if (is_current && today.day == d) {
            std::cout << REVERSE << BOLD << padded << RESET;
        }
        // جمعه: زرد
        else if (wd == 6) {
            std::cout << YELLOW << padded << RESET;
        }
        else {
            std::cout << padded;
        }
        std::cout << " ";

        ++col;
        if (col == 7) {
            std::cout << "\n";
            col = 0;
        }
    }
    if (col != 0) std::cout << "\n";
    std::cout << "\n";
}

// ==================== نمایش کل سال ====================
void print_year(int year, const Options& opts) {
    std::cout << "\n";
    if (opts.color) std::cout << BOLD << GREEN;
    std::cout << "═══════════════════════════════════════\n";
    std::cout << "           سال " << year << "\n";
    std::cout << "═══════════════════════════════════════\n";
    if (opts.color) std::cout << RESET;

    for (int m = 1; m <= 12; ++m) {
        print_calendar(year, m, opts);
    }
}

// ==================== نمایش سه ماه ====================
void print_three_months(int year, int month, const Options& opts) {
    int prev = month - 1;
    int next = month + 1;
    int prev_year = year, next_year = year;
    if (prev < 1)  { prev = 12; prev_year--; }
    if (next > 12) { next = 1;  next_year++; }

    print_calendar(prev_year, prev, opts);
    print_calendar(year, month, opts);
    print_calendar(next_year, next, opts);
}

// ==================== نمایش تاریخ امروز ====================
void print_today_info() {
    bg::date today_g = bg::day_clock::local_day();
    PersianDate today_p = to_persian(today_g);
    int wd = persian_weekday(today_p.year, today_p.month, today_p.day);

    const std::vector<std::string> GREG_WD = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    const std::vector<std::string> PERSIAN_WD = {"شنبه","یکشنبه","دوشنبه","سه‌شنبه","چهارشنبه","پنجشنبه","جمعه"};

    std::cout << "\n" << BOLD;

    // میلادی اول (LTR خالص)
    std::cout << today_g.year() << "-"
              << std::setw(2) << std::setfill('0') << today_g.month().as_number() << "-"
              << std::setw(2) << std::setfill('0') << today_g.day()
              << std::setfill(' ')
              << " (" << GREG_WD[today_g.day_of_week().as_number()] << ")";

    std::cout << "  |  ";

    // شمسی
    std::cout << PERSIAN_WD[wd] << " "
              << today_p.day << " " << MONTH_NAMES[today_p.month - 1]
              << " " << today_p.year;

    std::cout << RESET << "\n\n";
}

// ==================== راهنما ====================
void print_help(const char* prog_name) {
    std::cout << "\n";
    std::cout << BOLD << "cal-fa " << VERSION << RESET << " — تقویم شمسی در ترمینال\n\n";
    std::cout << BOLD << "استفاده:" << RESET << "\n";
    std::cout << "  " << prog_name << " [گزینه‌ها] [ماه] [سال]\n\n";
    std::cout << BOLD << "گزینه‌ها:" << RESET << "\n";
    std::cout << "  -y            نمایش کل سال\n";
    std::cout << "  -3            نمایش سه ماه (قبل، جاری، بعد)\n";
    std::cout << "  -n            بدون رنگ\n";
    std::cout << "  -h            نمایش این راهنما\n";
    std::cout << "  -v            نمایش نسخه\n\n";
    std::cout << BOLD << "مثال‌ها:" << RESET << "\n";
    std::cout << "  " << prog_name << "              # تقویم ماه جاری\n";
    std::cout << "  " << prog_name << " 7 1405       # مهر ۱۴۰۵\n";
    std::cout << "  " << prog_name << " -y 1405      # کل سال ۱۴۰۵\n";
    std::cout << "  " << prog_name << " -3 7 1405    # سه ماه حول مهر ۱۴۰۵\n\n";
}

// ==================== تابع اصلی ====================
int main(int argc, char* argv[]) {
    Options opts;
    int opt;

    // تجزیه‌ی آرگومان‌ها با getopt
    while ((opt = getopt(argc, argv, "y3nhv")) != -1) {
        switch (opt) {
            case 'y': opts.full_year = true; break;
            case '3': opts.three_months = true; break;
            case 'n': opts.color = false; break;
            case 'h': print_help(argv[0]); return 0;
            case 'v':
                std::cout << "cal-fa " << VERSION << "\n";
                return 0;
            default:
                print_help(argv[0]);
                return 1;
        }
    }

    // آرگومان‌های باقیمانده: ماه و سال
    int remaining = argc - optind;
    if (remaining >= 1) {
        try {
            opts.month = std::stoi(argv[optind]);
            if (opts.month < 1 || opts.month > 12) {
                std::cerr << "خطا: شماره ماه باید بین ۱ تا ۱۲ باشد.\n";
                return 1;
            }
        } catch (...) {
            std::cerr << "خطا: شماره ماه باید عدد باشد.\n";
            return 1;
        }
    }
    if (remaining >= 2) {
        try {
            opts.year = std::stoi(argv[optind + 1]);
        } catch (...) {
            std::cerr << "خطا: سال باید عدد باشد.\n";
            return 1;
        }
    }

    // تعیین تاریخ پیش‌فرض
    bg::date today_g = bg::day_clock::local_day();
    PersianDate today_p = to_persian(today_g);

    int year = (opts.year != -1) ? opts.year : today_p.year;
    int month = (opts.month != -1) ? opts.month : today_p.month;

    // نمایش اطلاعات امروز (اگر ماه/سال پیش‌فرض باشد)
    if (opts.show_today && opts.year == -1 && opts.month == -1 && opts.color) {
        print_today_info();
    }

    // اجرای حالت مناسب
    if (opts.full_year) {
        print_year(year, opts);
    } else if (opts.three_months) {
        print_three_months(year, month, opts);
    } else {
        print_calendar(year, month, opts);
    }

    return 0;
}
