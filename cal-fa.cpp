// cal_fa.cpp — مرحله ۳: تبدیل تاریخ و اختلاف، هدر سال وسط ستون دوم
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <utility>
#include <cstdlib>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <getopt.h>
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
const std::string CYAN    = "\033[36m";

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
    bool stacked = false;
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
    if (cyear == 1029982) ycycle = 2820;
    else {
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

bg::date jdn_to_gregorian(long jdn) {
    static const bg::date ref(1970, 1, 1);
    static const long ref_jdn = ref.julian_day();
    return ref + bg::days(jdn - ref_jdn);
}

int persian_weekday(int y, int m, int d) {
    return (persian_to_jdn(y, m, d) + 2) % 7;
}

int persian_month_days(int year, int month) {
    if (month <= 6) return 31;
    if (month <= 11) return 30;
    int r = year % 33;
    if (r == 1 || r == 5 || r == 9 || r == 13 ||
        r == 17 || r == 22 || r == 26 || r == 30) return 30;
    return 29;
}

// ==================== تجزیه‌ی تاریخ ====================
struct SimpleDate { int y, m, d; bool jalali; bool valid = false; };

SimpleDate parse_date(const std::string& s) {
    SimpleDate out;
    if (s == "today" || s == "now") {
        bg::date g = bg::day_clock::local_day();
        PersianDate p = to_persian(g);
        out = { p.year, p.month, p.day, true, true };
        return out;
    }
    char sep = 0;
    for (char c : s) if (c == '/' || c == '-') { sep = c; break; }
    if (!sep) return out;

    std::stringstream ss(s);
    std::string tok;
    std::vector<int> parts;
    while (std::getline(ss, tok, sep)) {
        try { parts.push_back(std::stoi(tok)); }
        catch (...) { return out; }
    }
    if (parts.size() != 3) return out;
    out.y = parts[0]; out.m = parts[1]; out.d = parts[2];
    out.jalali = (out.y < 1700);   // heuristic: <1700 → Jalali
    out.valid = true;
    return out;
}

// ==================== نمایش عرض (بدون کدهای ANSI) ====================
int visible_length(const std::string& s) {
    int len = 0; bool in_esc = false;
    for (unsigned char c : s) {
        if (in_esc) { if (c == 'm') in_esc = false; continue; }
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
    int pad = (width - (int)title.size()) / 2;
    if (pad < 0) pad = 0;

    std::string tl;
    if (opts.color) tl += BOLD;
    tl += std::string(pad, ' ') + title;
    if (opts.color) tl += RESET;
    lines.push_back(tl);

    // سرستون روزهای هفته (آبی)
    std::string wd;
    for (int i = 0; i < 7; ++i) {
        if (opts.color) wd += BLUE;
        wd += WEEKDAY_NAMES[i];
        if (opts.color) wd += RESET;
        if (i < 6) wd += " ";
    }
    lines.push_back(wd);

    int first_wd = persian_weekday(year, month, 1);
    int days = persian_month_days(year, month);
    bg::date today_g = bg::day_clock::local_day();
    PersianDate today_p = to_persian(today_g);
    bool is_current = (today_p.year == year && today_p.month == month);

    std::string row;
    int col = 0;
    for (int i = 0; i < first_wd; ++i) { row += "   "; ++col; }

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
        if (col == 7) { lines.push_back(row); row.clear(); col = 0; }
        else row += " ";
    }
    if (col != 0) lines.push_back(row);
    return lines;
}

// ==================== چاپ تک‌ستونه ====================
void print_single(const Options& opts, int year, int month) {
    for (const auto& line : format_calendar(year, month, opts))
        std::cout << line << "\n";
    std::cout << "\n";
}

// ==================== چاپ چند ماه ستونی ====================
void print_columns(const Options& opts,
                   const std::vector<std::pair<int,int>>& months) {
    std::vector<std::vector<std::string>> all;
    for (const auto& [y, m] : months)
        all.push_back(format_calendar(y, m, opts));

    int max_lines = 0;
    for (const auto& v : all) max_lines = std::max(max_lines, (int)v.size());

    const int col_width = 20, gap = 3;
    for (int i = 0; i < max_lines; ++i) {
        std::string line;
        for (size_t j = 0; j < all.size(); ++j) {
            std::string content = (i < (int)all[j].size()) ? all[j][i] : "";
            line += pad_visible(content, col_width);
            if (j + 1 < all.size()) line += std::string(gap, ' ');
        }
        while (!line.empty() && line.back() == ' ') line.pop_back();
        std::cout << line << "\n";
    }
    std::cout << "\n";
}

void print_stacked(const Options& opts,
                   const std::vector<std::pair<int,int>>& months) {
    for (const auto& [y, m] : months) print_single(opts, y, m);
}

// ==================== کل سال ====================
void print_year(int year, const Options& opts) {
    if (opts.stacked) {
        // حالت تک‌ستونه: هر ماه پشت سر هم
        if (opts.color) std::cout << BOLD << GREEN;
        std::cout << "Year " << year << "\n\n";
        if (opts.color) std::cout << RESET;
        for (int m = 1; m <= 12; ++m) print_single(opts, year, m);
        return;
    }

    // حالت سه‌ستونه: سال در مرکز ستون دوم (که با مرکز گرید یکی است)
    const int grid_width = 3 * 20 + 2 * 3;   // 66
    std::string ys = std::to_string(year);
    int pad = (grid_width - (int)ys.size()) / 2;
    if (pad < 0) pad = 0;
    if (opts.color) std::cout << BOLD << GREEN;
    std::cout << std::string(pad, ' ') << ys << "\n\n";
    if (opts.color) std::cout << RESET;

    for (int m = 1; m <= 12; m += 3) {
        std::vector<std::pair<int,int>> batch;
        for (int k = 0; k < 3 && m + k <= 12; ++k)
            batch.push_back({year, m + k});
        print_columns(opts, batch);
    }
}

// ==================== سه ماه ====================
void print_three_months(int year, int month, const Options& opts) {
    int prev = month - 1, next = month + 1;
    int py = year, ny = year;
    if (prev < 1)  { prev = 12; py--; }
    if (next > 12) { next = 1;  ny++; }
    std::vector<std::pair<int,int>> months = {{py, prev}, {year, month}, {ny, next}};
    if (opts.stacked) print_stacked(opts, months);
    else              print_columns(opts, months);
}

// ==================== تبدیل تاریخ ====================
int cmd_convert(const std::string& arg, const Options& opts) {
    SimpleDate sd = parse_date(arg);
    if (!sd.valid) {
        std::cerr << "Error: cannot parse date '" << arg << "'\n";
        std::cerr << "Use YYYY/MM/DD, YYYY-MM-DD, or 'today'.\n";
        return 1;
    }

    if (sd.jalali) {
        long jdn = persian_to_jdn(sd.y, sd.m, sd.d);
        bg::date g = jdn_to_gregorian(jdn);
        int wd = persian_weekday(sd.y, sd.m, sd.d);
        const std::vector<std::string> en_wd = {"Sat","Sun","Mon","Tue","Wed","Thu","Fri"};
        std::cout << "\n";
        if (opts.color) std::cout << BOLD << CYAN;
        std::cout << "Jalali:    ";
        if (opts.color) std::cout << RESET;
        std::cout << sd.y << "/"
                  << std::setw(2) << std::setfill('0') << sd.m << "/"
                  << std::setw(2) << std::setfill('0') << sd.d
                  << std::setfill(' ') << "  (" << en_wd[wd] << ")\n";

        if (opts.color) std::cout << BOLD << CYAN;
        std::cout << "Gregorian: ";
        if (opts.color) std::cout << RESET;
        std::cout << g.year() << "-"
                  << std::setw(2) << std::setfill('0') << g.month().as_number() << "-"
                  << std::setw(2) << std::setfill('0') << g.day()
                  << std::setfill(' ') << "\n\n";
    } else {
        // Gregorian → Jalali
        bg::date g(sd.y, sd.m, sd.d);
        if (g.is_not_a_date()) {
            std::cerr << "Error: invalid Gregorian date.\n";
            return 1;
        }
        PersianDate p = to_persian(g);
        int wd = persian_weekday(p.year, p.month, p.day);
        const std::vector<std::string> en_wd = {"Sat","Sun","Mon","Tue","Wed","Thu","Fri"};
        std::cout << "\n";
        if (opts.color) std::cout << BOLD << CYAN;
        std::cout << "Gregorian: ";
        if (opts.color) std::cout << RESET;
        std::cout << sd.y << "-"
                  << std::setw(2) << std::setfill('0') << sd.m << "-"
                  << std::setw(2) << std::setfill('0') << sd.d
                  << std::setfill(' ') << "\n";

        if (opts.color) std::cout << BOLD << CYAN;
        std::cout << "Jalali:    ";
        if (opts.color) std::cout << RESET;
        std::cout << p.year << "/"
                  << std::setw(2) << std::setfill('0') << p.month << "/"
                  << std::setw(2) << std::setfill('0') << p.day
                  << std::setfill(' ')
                  << "  (" << WEEKDAY_NAMES[wd] << ")\n\n";
    }
    return 0;
}

// ==================== اختلاف دو تاریخ ====================
int cmd_diff(const std::string& a, const std::string& b, const Options& opts) {
    SimpleDate da = parse_date(a), db = parse_date(b);
    if (!da.valid) { std::cerr << "Error: invalid date '" << a << "'\n"; return 1; }
    if (!db.valid) { std::cerr << "Error: invalid date '" << b << "'\n"; return 1; }

    long j1 = da.jalali
        ? persian_to_jdn(da.y, da.m, da.d)
        : bg::date(da.y, da.m, da.d).julian_day();
    long j2 = db.jalali
        ? persian_to_jdn(db.y, db.m, db.d)
        : bg::date(db.y, db.m, db.d).julian_day();

    long diff = std::abs(j2 - j1);
    long weeks = diff / 7;
    long days_rem = diff % 7;

    std::cout << "\n";
    if (opts.color) std::cout << BOLD << CYAN;
    std::cout << "Difference: ";
    if (opts.color) std::cout << RESET;
    std::cout << diff << " day" << (diff == 1 ? "" : "s");
    if (diff >= 7)
        std::cout << "  (" << weeks << " week" << (weeks == 1 ? "" : "s")
                  << " + " << days_rem << " day" << (days_rem == 1 ? "" : "s") << ")";
    std::cout << "\n\n";
    return 0;
}

// ==================== راهنما ====================
void print_help(const char* prog) {
    std::cout << "\n";
    std::cout << BOLD << "cal-fa " << VERSION << RESET
              << " — Persian calendar in terminal\n\n";
    std::cout << BOLD << "Usage:" << RESET << "\n";
    std::cout << "  " << prog << " [options] [month] [year]\n";
    std::cout << "  " << prog << " -c <date>          # convert date\n";
    std::cout << "  " << prog << " -d <date1> <date2> # diff two dates\n\n";
    std::cout << BOLD << "Options:" << RESET << "\n";
    std::cout << "  -y            Show full year (3-column grid)\n";
    std::cout << "  -3            Show three months side by side\n";
    std::cout << "  -s            Stacked mode (single column)\n";
    std::cout << "  -c, --convert Convert Jalali <-> Gregorian\n";
    std::cout << "  -d, --diff    Difference in days between two dates\n";
    std::cout << "  -n            No color\n";
    std::cout << "  -h            Show this help\n";
    std::cout << "  -v            Show version\n\n";
    std::cout << BOLD << "Date formats:" << RESET
              << " YYYY/MM/DD, YYYY-MM-DD, or 'today'\n";
    std::cout << BOLD << "Auto-detect:" << RESET
              << " year < 1700 -> Jalali, year >= 1700 -> Gregorian\n\n";
    std::cout << BOLD << "Examples:" << RESET << "\n";
    std::cout << "  " << prog << "                    # current month\n";
    std::cout << "  " << prog << " 7 1405               # Mehr 1405\n";
    std::cout << "  " << prog << " -3 7 1405            # three months side by side\n";
    std::cout << "  " << prog << " -ys 1405             # full year, stacked\n";
    std::cout << "  " << prog << " -c 1405/07/12        # Jalali -> Gregorian\n";
    std::cout << "  " << prog << " -c 2026-10-04        # Gregorian -> Jalali\n";
    std::cout << "  " << prog << " -d 1405/07/12 today  # days since then\n\n";
}

// ==================== تابع اصلی ====================
int main(int argc, char* argv[]) {
    Options opts;
    int opt;
    bool do_convert = false, do_diff = false;
    std::string convert_arg;

    static struct option long_opts[] = {
        {"convert", required_argument, 0, 'c'},
        {"diff",    no_argument,       0, 'd'},
        {"year",    no_argument,       0, 'y'},
        {"three",   no_argument,       0, '3'},
        {"stacked", no_argument,       0, 's'},
        {"nocolor", no_argument,       0, 'n'},
        {"help",    no_argument,       0, 'h'},
        {"version", no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "c:dy3snhv", long_opts, nullptr)) != -1) {
        switch (opt) {
            case 'c': do_convert = true; convert_arg = optarg; break;
            case 'd': do_diff = true; break;
            case 'y': opts.full_year = true; break;
            case '3': opts.three_months = true; break;
            case 's': opts.stacked = true; break;
            case 'n': opts.color = false; break;
            case 'h': print_help(argv[0]); return 0;
            case 'v': std::cout << "cal-fa " << VERSION << "\n"; return 0;
            default:  print_help(argv[0]); return 1;
        }
    }

    if (do_convert) return cmd_convert(convert_arg, opts);

    if (do_diff) {
        int rem = argc - optind;
        if (rem < 2) {
            std::cerr << "Error: -d requires two dates.\n";
            return 1;
        }
        return cmd_diff(argv[optind], argv[optind + 1], opts);
    }

    int remaining = argc - optind;
    if (remaining >= 1) {
        try {
            opts.month = std::stoi(argv[optind]);
            if (opts.month < 1 || opts.month > 12) {
                std::cerr << "Error: month must be 1..12\n";
                return 1;
            }
        } catch (...) {
            std::cerr << "Error: month must be a number\n";
            return 1;
        }
    }
    if (remaining >= 2) {
        try { opts.year = std::stoi(argv[optind + 1]); }
        catch (...) { std::cerr << "Error: year must be a number\n"; return 1; }
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
