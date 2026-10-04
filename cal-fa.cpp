// cal_fa.cpp — مرحله ۲.۵: نمایش سه‌ستونه
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <utility>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <unistd.h>

namespace bg = boost::gregorian;

const std::string VERSION = "0.3.0";

// ==================== رنگ‌ها ====================
const std::string RESET   = "\033[0m";
const std::string REVERSE = "\033[7m";
const std::string BOLD    = "\033[1m";
const std::string RED     = "\033[31m";
const std::string GREEN   = "\033[32m";
const std::string YELLOW  = "\033[33m";
const std::string BLUE    = "\033[34m";

// ==================== نام‌ها ====================
const std::vector<std::string> MONTH_NAMES = {
    "Farvardin", "Ordibehesht", "Khordad",
    "Tir", "Mordad", "Shahrivar",
    "Mehr", "Aban", "Azar",
    "Dey", "Bahman", "Esfand"
};

const std::vector<std::string> WEEKDAY_NAMES = {
    "Sh", "Ye", "Do", "Se", "Ch", "Pa", "Jo"
};

// ==================== ساختارها ====================
struct PersianDate { int year, month, day; };

struct Options {
    int year = -1;
    int month = -1;
    bool full_year = false;
    bool three_months = false;
    bool stacked = false;   // ← گزینه‌ی جدید
    bool color = true;
};

// ==================== تبدیل تاریخ ====================
long persian_to_jdn(int year, int month, int day) {
    long epbase = year - ((year >= 0) ? 474 : 473);
    long epyear = 474 + (epbase % 2820);
    long m = (month <= 7) ? (month - 1) * 31 : (month - 1) * 30 + 6;
    return day + m + ((epyear * 682 - 110) / 2816) +
           (epyear - 1) * 365 + (epbase / 2820) * 1029983 +
           1948320;
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
    return (persian_to_jdn(y, m, d) + 2) % 7;
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

// ==================== محاسبه‌ی عرض نمایشی (بدون احتساب کدهای رنگ) ====================
int visible_length(const std::string& s) {
    int len = 0;
    bool in_esc = false;
    for (unsigned char c : s) {
        if (in_esc) {
            if (c == 'm') in_esc = false;
            continue;
        }
        if (c == '\033') { in_esc = true; continue; }
        ++len;
    }
    return len;
}

std::string pad_visible(const std::string& s, int width) {
    int v = visible_length(s);
    if (v >= width) return s;
    return s + std::string(width - v, ' ');
}

// ==================== تولید خطوط یک ماه ====================
std::vector<std::string> format_calendar(int year, int month, const Options& opts) {
    std::vector<std::string> lines;
    std::string title = MONTH_NAMES[month - 1] + " " + std::to_string(year);
    const int width = 20;

    // عنوان (وسط‌چین)
    int pad = (width - (int)title.size()) / 2;
    if (pad < 0) pad = 0;
    std::string title_line;
    if (opts.color) title_line += BOLD;
    title_line += std::string(pad, ' ') + title;
    if (opts.color) title_line += RESET;
    lines.push_back(title_line);

    // سرستون روزهای هفته (آبی)
    std::string wd;
    for (int i = 0; i < 7; ++i) {
        if (opts.color) wd += BLUE;
        wd += WEEKDAY_NAMES[i];
        if (opts.color) wd += RESET;
        if (i < 6) wd += " ";
    }
    lines.push_back(wd);

    // روزها
    int first_wd = persian_weekday(year, month, 1);
    int days = persian_month_days(year, month);
    bg::date today_g = bg::day_clock::local_day();
    PersianDate today_p = to_persian(today_g);
    bool is_current = (today_p.year == year && today_p.month == month);

    std::string row;
    int col = 0;
    for (int i = 0; i < first_wd; ++i) {
        row += "   ";
        ++col;
    }
    for (int d = 1; d <= days; ++d) {
        std::string s = std::to_string(d);
        std::string cell = (s.size() < 2) ? (" " + s) : s;
        int wd_num = persian_weekday(year, month, d);

        if (is_current && today_p.day == d) {
            if (opts.color) row += REVERSE;
            row += cell;
            if (opts.color) row += RESET;
        } else if (wd_num == 6) {
            if (opts.color) row += YELLOW;
            row += cell;
            if (opts.color) row += RESET;
        } else {
            row += cell;
        }
        ++col;
        if (col == 7) {
            lines.push_back(row);
            row.clear();
            col = 0;
        } else {
            row += " ";
        }
    }
    if (col != 0) lines.push_back(row);

    return lines;
}

// ==================== چاپ تک‌ستونه ====================
void print_single(const Options& opts, int year, int month) {
    for (const auto& line : format_calendar(year, month, opts)) {
        std::cout << line << "\n";
    }
    std::cout << "\n";
}

// ==================== چاپ چند ماه کنار هم (ستونی) ====================
void print_columns(const Options& opts,
                   const std::vector<std::pair<int,int>>& months) {
    std::vector<std::vector<std::string>> all;
    for (const auto& [y, m] : months) {
        all.push_back(format_calendar(y, m, opts));
    }

    int max_lines = 0;
    for (const auto& v : all) max_lines = std::max(max_lines, (int)v.size());

    const int col_width = 20;
    const int gap = 3;

    for (int i = 0; i < max_lines; ++i) {
        std::string line;
        for (size_t j = 0; j < all.size(); ++j) {
            std::string content = (i < (int)all[j].size()) ? all[j][i] : "";
            line += pad_visible(content, col_width);
            if (j + 1 < all.size()) line += std::string(gap, ' ');
        }
        // حذف فاصله‌های انتهایی
        while (!line.empty() && line.back() == ' ') line.pop_back();
        std::cout << line << "\n";
    }
    std::cout << "\n";
}

// ==================== چاپ چند ماه پشت سر هم (stacked) ====================
void print_stacked(const Options& opts,
                   const std::vector<std::pair<int,int>>& months) {
    for (const auto& [y, m] : months) {
        print_single(opts, y, m);
    }
}

// ==================== چاپ کل سال ====================
void print_year(int year, const Options& opts) {
    if (opts.color) std::cout << BOLD << GREEN;
    //std::cout << "===============================================\n";
    std::cout << "                             Year " << year << "\n";
    //std::cout << "===============================================\n";
    if (opts.color) std::cout << RESET;

    if (opts.stacked) {
        for (int m = 1; m <= 12; ++m) print_single(opts, year, m);
    } else {
        // ۳ ستون × ۴ ردیف
        for (int m = 1; m <= 12; m += 3) {
            std::vector<std::pair<int,int>> batch;
            for (int k = 0; k < 3 && m + k <= 12; ++k) {
                batch.push_back({year, m + k});
            }
            print_columns(opts, batch);
        }
    }
}

// ==================== چاپ سه ماه ====================
void print_three_months(int year, int month, const Options& opts) {
    int prev = month - 1, next = month + 1;
    int prev_year = year, next_year = year;
    if (prev < 1)  { prev = 12; prev_year--; }
    if (next > 12) { next = 1;  next_year++; }

    std::vector<std::pair<int,int>> months = {
        {prev_year, prev}, {year, month}, {next_year, next}
    };

    if (opts.stacked) print_stacked(opts, months);
    else              print_columns(opts, months);
}

// ==================== راهنما ====================
void print_help(const char* prog) {
    std::cout << "\n";
    std::cout << BOLD << "cal-fa " << VERSION << RESET
              << " — Persian calendar in terminal\n\n";
    std::cout << BOLD << "Usage:" << RESET << "\n";
    std::cout << "  " << prog << " [options] [month] [year]\n\n";
    std::cout << BOLD << "Options:" << RESET << "\n";
    std::cout << "  -y          Show full year (3 columns x 4 rows)\n";
    std::cout << "  -3          Show three months side by side\n";
    std::cout << "  -s          Stacked mode: print months in single column\n";
    std::cout << "              (use with -y or -3)\n";
    std::cout << "  -n          No color\n";
    std::cout << "  -h          Show this help\n";
    std::cout << "  -v          Show version\n\n";
    std::cout << BOLD << "Examples:" << RESET << "\n";
    std::cout << "  " << prog << "              # Current month\n";
    std::cout << "  " << prog << " 7 1405       # Mehr 1405\n";
    std::cout << "  " << prog << " -3 7 1405    # Three months, side by side\n";
    std::cout << "  " << prog << " -3s 7 1405   # Three months, stacked\n";
    std::cout << "  " << prog << " -y 1405      # Full year in columns\n";
    std::cout << "  " << prog << " -ys 1405     # Full year, stacked\n\n";
}

// ==================== تابع اصلی ====================
int main(int argc, char* argv[]) {
    Options opts;
    int opt;

    while ((opt = getopt(argc, argv, "y3snhv")) != -1) {
        switch (opt) {
            case 'y': opts.full_year = true; break;
            case '3': opts.three_months = true; break;
            case 's': opts.stacked = true; break;
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

    int remaining = argc - optind;
    if (remaining >= 1) {
        try {
            opts.month = std::stoi(argv[optind]);
            if (opts.month < 1 || opts.month > 12) {
                std::cerr << "Error: month must be between 1 and 12.\n";
                return 1;
            }
        } catch (...) {
            std::cerr << "Error: month must be a number.\n";
            return 1;
        }
    }
    if (remaining >= 2) {
        try {
            opts.year = std::stoi(argv[optind + 1]);
        } catch (...) {
            std::cerr << "Error: year must be a number.\n";
            return 1;
        }
    }

    bg::date today_g = bg::day_clock::local_day();
    PersianDate today_p = to_persian(today_g);
    int year  = (opts.year  != -1) ? opts.year  : today_p.year;
    int month = (opts.month != -1) ? opts.month : today_p.month;

    if (opts.full_year)         print_year(year, opts);
    else if (opts.three_months) print_three_months(year, month, opts);
    else                        print_single(opts, year, month);

    return 0;
}
