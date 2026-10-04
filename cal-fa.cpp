// cal_fa.cpp — نسخه 0.5.0
// تکمیل هم‌ارزی با jcal/jdate: -e، فرمت سفارشی -t، اعتبارسنجی ورودی
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <utility>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <getopt.h>
#include <unistd.h>

namespace bg = boost::gregorian;

const std::string VERSION = "0.5.0";

// ==================== رنگ‌ها ====================
const std::string RESET   = "\033[0m";
const std::string REVERSE = "\033[7m";
const std::string BOLD    = "\033[1m";
const std::string RED     = "\033[31m";
const std::string GREEN   = "\033[32m";
const std::string YELLOW  = "\033[33m";
const std::string BLUE    = "\033[34m";
const std::string CYAN    = "\033[36m";

// کاراکترهای جهت‌دهی یونیکد
const std::string LRI = "\u2066";
const std::string PDI = "\u2069";

// ==================== نام‌ها ====================
const std::vector<std::string> MONTH_NAMES = {
    "Farvardin", "Ordibehesht", "Khordad",
    "Tir", "Mordad", "Shahrivar",
    "Mehr", "Aban", "Azar",
    "Dey", "Bahman", "Esfand"
};
const std::vector<std::string> MONTH_NAMES_FA = {
    "فروردین", "اردیبهشت", "خرداد",
    "تیر", "مرداد", "شهریور",
    "مهر", "آبان", "آذر",
    "دی", "بهمن", "اسفند"
};

const std::vector<std::string> WEEKDAY_SHORT = {
    "Sh", "Ye", "Do", "Se", "Ch", "Pa", "Jo"
};
const std::vector<std::string> WEEKDAY_ABBR_EN = {
    "Sat", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri"
};
const std::vector<std::string> WEEKDAY_FA = {
    "ش", "ی", "د", "س", "چ", "پ", "ج"
};
const std::vector<std::string> WEEKDAY_FULL_EN = {
    "Saturday", "Sunday", "Monday", "Tuesday",
    "Wednesday", "Thursday", "Friday"
};
const std::vector<std::string> WEEKDAY_FULL_FA = {
    "شنبه", "یکشنبه", "دوشنبه", "سه‌شنبه",
    "چهارشنبه", "پنجشنبه", "جمعه"
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
    bool persian = false;
    bool julian_day = false;
    bool pahlavi = false;
    bool english_names = false;   // -e
};

// ==================== ابزار ====================
std::string to_persian_digits(const std::string& s) {
    static const char* fa[] = {"۰","۱","۲","۳","۴","۵","۶","۷","۸","۹"};
    std::string out;
    for (char c : s) {
        if (c >= '0' && c <= '9') out += fa[c - '0'];
        else                       out += c;
    }
    return out;
}

// ==================== تبدیل تاریخ ====================
long persian_to_jdn(int year, int month, int day) {
    long epbase = year - ((year >= 0) ? 474 : 473);
    long epyear = 474 + (epbase % 2820);
    long m = (month <= 7) ? (month - 1) * 31 : (month - 1) * 30 + 6;
    return day + m + ((epyear * 682 - 110) / 2816) +
           (epyear - 1) * 365 + (epbase / 2820) * 1029983 + 1948320;
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

int persian_day_of_year(int year, int month, int day) {
    int doy = 0;
    for (int m = 1; m < month; ++m)
        doy += persian_month_days(year, m);
    return doy + day;
}

// ==================== تجزیه‌ی تاریخ ====================
struct SimpleDate { int y, m, d; bool jalali; bool valid = false; };

SimpleDate parse_date(const std::string& s) {
    SimpleDate out;
    if (s == "today" || s == "now") {
        PersianDate p = to_persian(bg::day_clock::local_day());
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
        try { parts.push_back(std::stoi(tok)); } catch (...) { return out; }
    }
    if (parts.size() != 3) return out;
    out.y = parts[0]; out.m = parts[1]; out.d = parts[2];
    out.jalali = (out.y < 1700);
    out.valid = true;
    return out;
}

// ==================== عرض نمایشی ====================
int visible_length(const std::string& s) {
    int len = 0; bool in_esc = false;
    for (size_t i = 0; i < s.size(); ) {
        unsigned char c = s[i];
        if (in_esc) { if (c == 'm') in_esc = false; ++i; continue; }
        if (c == '\033') { in_esc = true; ++i; continue; }
        if ((c & 0xC0) == 0x80) { ++i; continue; }
        // نادیده گرفتن کاراکترهای صفر-عرض Unicode
        if (c == 0xE2 && i + 2 < s.size()) {
            unsigned char b = s[i+1], d = s[i+2];
            if (b == 0x80 && d >= 0x8B && d <= 0x8F) { i += 3; continue; }
            if (b == 0x81 && d >= 0xA0 && d <= 0xAF) { i += 3; continue; }
        }
        ++len;
        if      ((c & 0x80) == 0x00) i += 1;
        else if ((c & 0xE0) == 0xC0) i += 2;
        else if ((c & 0xF0) == 0xE0) i += 3;
        else if ((c & 0xF8) == 0xF0) i += 4;
        else                         i += 1;
    }
    return len;
}

std::string pad_visible(const std::string& s, int width) {
    int v = visible_length(s);
    if (v >= width) return s;
    return s + std::string(width - v, ' ');
}

// ==================== ابعاد تقویم ====================
int cell_width(const Options& opts) {
    int dw = opts.julian_day ? 3 : 2;
    int ww = (!opts.persian && opts.english_names) ? 3 : 2;
    return std::max(dw, ww);
}

int calendar_row_width(const Options& opts) {
    return 7 * cell_width(opts) + 6;
}

// ==================== تولید خطوط یک ماه ====================
std::vector<std::string> format_calendar(int year, int month, const Options& opts) {
    std::vector<std::string> lines;
    int cw = cell_width(opts);
    int rw = calendar_row_width(opts);

    int display_year = opts.pahlavi ? (year + 1180) : year;
    std::string title;
    if (opts.persian) {
        title = MONTH_NAMES_FA[month - 1] + " " +
                to_persian_digits(std::to_string(display_year));
    } else {
        title = MONTH_NAMES[month - 1] + " " + std::to_string(display_year);
    }
    if (opts.pahlavi) title += "(pa)";

    int pad = (rw - visible_length(title)) / 2;
    if (pad < 0) pad = 0;
    std::string tl;
    if (opts.color) tl += BOLD;
    tl += std::string(pad, ' ') + title;
    if (opts.color) tl += RESET;
    lines.push_back(tl);

    // سرستون روزهای هفته
    std::string wd;
    for (int i = 0; i < 7; ++i) {
        std::string name;
        if (opts.persian)              name = WEEKDAY_FA[i];
        else if (opts.english_names)   name = WEEKDAY_ABBR_EN[i];
        else                           name = WEEKDAY_SHORT[i];

        std::string padded = std::string(cw - visible_length(name), ' ') + name;
        if (opts.persian) padded = LRI + padded + PDI;
        if (opts.color) wd += BLUE;
        wd += padded;
        if (opts.color) wd += RESET;
        if (i < 6) wd += " ";
    }
    lines.push_back(wd);

    // اعداد روزها
    int first_wd = persian_weekday(year, month, 1);
    int days = persian_month_days(year, month);
    PersianDate today_p = to_persian(bg::day_clock::local_day());
    bool is_current = (today_p.year == year && today_p.month == month);

    std::string row;
    int col = 0;
    for (int i = 0; i < first_wd; ++i) {
        row += std::string(cw, ' ');
        ++col;
        if (col < 7) row += " ";
    }
    for (int d = 1; d <= days; ++d) {
        int num = opts.julian_day ? persian_day_of_year(year, month, d) : d;
        std::string s = std::to_string(num);
        std::string cell = std::string(cw - s.size(), ' ') + s;
        if (opts.persian) cell = to_persian_digits(cell);
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

// ==================== چاپ‌ها ====================
void print_single(const Options& opts, int year, int month) {
    for (const auto& line : format_calendar(year, month, opts))
        std::cout << line << "\n";
    std::cout << "\n";
}

void print_columns(const Options& opts,
                   const std::vector<std::pair<int,int>>& months) {
    std::vector<std::vector<std::string>> all;
    for (const auto& [y, m] : months)
        all.push_back(format_calendar(y, m, opts));

    int max_lines = 0;
    for (const auto& v : all) max_lines = std::max(max_lines, (int)v.size());

    const int col_width = calendar_row_width(opts);
    const int gap = 3;
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

void print_year(int year, const Options& opts) {
    int display_year = opts.pahlavi ? (year + 1180) : year;
    std::string ys = opts.persian
        ? to_persian_digits(std::to_string(display_year))
        : std::to_string(display_year);
    if (opts.pahlavi) ys += "(pa)";

    if (opts.stacked) {
        if (opts.color) std::cout << BOLD << GREEN;
        std::cout << "Year " << ys << "\n\n";
        if (opts.color) std::cout << RESET;
        for (int m = 1; m <= 12; ++m) print_single(opts, year, m);
        return;
    }

    const int cw = calendar_row_width(opts);
    const int grid_width = 3 * cw + 2 * 3;
    int pad = (grid_width - visible_length(ys)) / 2;
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
        return 1;
    }
    auto fmt_num2 = [&](int n) {
        char buf[4];
        std::snprintf(buf, sizeof(buf), "%02d", n);
        return opts.persian ? to_persian_digits(buf) : std::string(buf);
    };
    auto fmt_year = [&](int y) {
        return opts.persian ? to_persian_digits(std::to_string(y))
                            : std::to_string(y);
    };

    if (sd.jalali) {
        long jdn = persian_to_jdn(sd.y, sd.m, sd.d);
        bg::date g = jdn_to_gregorian(jdn);
        int wd = persian_weekday(sd.y, sd.m, sd.d);
        std::string wdn = opts.persian ? WEEKDAY_FULL_FA[wd] : WEEKDAY_FULL_EN[wd];
        std::cout << "\n";
        if (opts.color) std::cout << BOLD << CYAN;
        std::cout << "Jalali:    ";
        if (opts.color) std::cout << RESET;
        std::cout << fmt_year(sd.y) << "/" << fmt_num2(sd.m) << "/" << fmt_num2(sd.d)
                  << "  (" << wdn << ")\n";
        if (opts.color) std::cout << BOLD << CYAN;
        std::cout << "Gregorian: ";
        if (opts.color) std::cout << RESET;
        std::cout << g.year() << "-"
                  << std::setw(2) << std::setfill('0') << g.month().as_number() << "-"
                  << std::setw(2) << std::setfill('0') << g.day()
                  << std::setfill(' ') << "\n\n";
    } else {
        bg::date g(sd.y, sd.m, sd.d);
        if (g.is_not_a_date()) { std::cerr << "Error: invalid date.\n"; return 1; }
        PersianDate p = to_persian(g);
        int wd = persian_weekday(p.year, p.month, p.day);
        std::string wdn = opts.persian ? WEEKDAY_FULL_FA[wd] : WEEKDAY_FULL_EN[wd];
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
        std::cout << fmt_year(p.year) << "/" << fmt_num2(p.month) << "/" << fmt_num2(p.day)
                  << "  (" << wdn << ")\n\n";
    }
    return 0;
}

// ==================== اختلاف ====================
int cmd_diff(const std::string& a, const std::string& b, const Options& opts) {
    SimpleDate da = parse_date(a), db = parse_date(b);
    if (!da.valid) { std::cerr << "Error: invalid date '" << a << "'\n"; return 1; }
    if (!db.valid) { std::cerr << "Error: invalid date '" << b << "'\n"; return 1; }
    long j1 = da.jalali ? persian_to_jdn(da.y, da.m, da.d)
                        : bg::date(da.y, da.m, da.d).julian_day();
    long j2 = db.jalali ? persian_to_jdn(db.y, db.m, db.d)
                        : bg::date(db.y, db.m, db.d).julian_day();
    long diff = std::abs(j2 - j1);
    long weeks = diff / 7, days_rem = diff % 7;
    auto pf = [&](long n) {
        return opts.persian ? to_persian_digits(std::to_string(n))
                            : std::to_string(n);
    };
    std::cout << "\n";
    if (opts.color) std::cout << BOLD << CYAN;
    std::cout << "Difference: ";
    if (opts.color) std::cout << RESET;
    std::cout << pf(diff) << " day" << (diff == 1 ? "" : "s");
    if (diff >= 7)
        std::cout << "  (" << pf(weeks) << " week" << (weeks == 1 ? "" : "s")
                  << " + " << pf(days_rem) << " day" << (days_rem == 1 ? "" : "s") << ")";
    std::cout << "\n\n";
    return 0;
}

// ==================== jdate با فرمت سفارشی ====================
std::string format_jdate(const std::string& fmt, const Options& opts) {
    bg::date g = bg::day_clock::local_day();
    PersianDate p = to_persian(g);
    int wd = persian_weekday(p.year, p.month, p.day);
    int display_year = opts.pahlavi ? (p.year + 1180) : p.year;

    std::time_t t = std::time(nullptr);
    std::tm* lt = std::localtime(&t);

    std::string out;
    char buf[32];
    for (size_t i = 0; i < fmt.size(); ++i) {
        if (fmt[i] == '%' && i + 1 < fmt.size()) {
            char c = fmt[++i];
            switch (c) {
                case 'Y': std::snprintf(buf, sizeof(buf), "%04d", display_year); out += buf; break;
                case 'y': std::snprintf(buf, sizeof(buf), "%02d", display_year % 100); out += buf; break;
                case 'm': std::snprintf(buf, sizeof(buf), "%02d", p.month); out += buf; break;
                case 'd': std::snprintf(buf, sizeof(buf), "%02d", p.day);   out += buf; break;
                case 'B': out += opts.persian ? MONTH_NAMES_FA[p.month - 1]
                                              : MONTH_NAMES[p.month - 1]; break;
                case 'b': out += (opts.persian ? MONTH_NAMES_FA[p.month - 1]
                                                : MONTH_NAMES[p.month - 1]).substr(0, 3); break;
                case 'A': out += opts.persian ? WEEKDAY_FULL_FA[wd]
                                              : WEEKDAY_FULL_EN[wd]; break;
                case 'a': out += opts.persian ? WEEKDAY_FA[wd]
                                              : WEEKDAY_ABBR_EN[wd]; break;
                case 'H': std::snprintf(buf, sizeof(buf), "%02d", lt->tm_hour); out += buf; break;
                case 'M': std::snprintf(buf, sizeof(buf), "%02d", lt->tm_min);  out += buf; break;
                case 'S': std::snprintf(buf, sizeof(buf), "%02d", lt->tm_sec);  out += buf; break;
                case '%': out += '%'; break;
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                default: out += '%'; out += c; break;
            }
        } else {
            out += fmt[i];
        }
    }
    if (opts.persian) out = to_persian_digits(out);
    return out;
}

int cmd_today(const std::string& fmt_arg, const Options& opts) {
    std::string fmt = fmt_arg.empty()
        ? "%A %d %B %Y  %H:%M:%S"
        : fmt_arg;
    if (!fmt.empty() && fmt[0] == '+') fmt = fmt.substr(1);

    std::string out = format_jdate(fmt, opts);
    if (opts.color) std::cout << BOLD << CYAN;
    std::cout << out;
    if (opts.color) std::cout << RESET;
    std::cout << "\n";
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
    std::cout << "  " << prog << " -d <date1> <date2> # diff two dates\n";
    std::cout << "  " << prog << " -t [+FORMAT]       # current date & time\n\n";
    std::cout << BOLD << "Options:" << RESET << "\n";
    std::cout << "  -y            Show full year (3-column grid)\n";
    std::cout << "  -3            Show three months side by side\n";
    std::cout << "  -s            Stacked mode (single column)\n";
    std::cout << "  -p            Persian digits and month/weekday names\n";
    std::cout << "  -e            English full weekday abbreviations (Sat, Sun, ...)\n";
    std::cout << "  -j            Show day-of-year instead of day-of-month\n";
    std::cout << "  -P            Imperial (Shahanshahi) year = Jalali + 1180\n";
    std::cout << "  -t, --today   Print current date and time\n";
    std::cout << "  -c, --convert Convert Jalali <-> Gregorian\n";
    std::cout << "  -d, --diff    Difference in days between two dates\n";
    std::cout << "  -n            No color\n";
    std::cout << "  -h            Show this help\n";
    std::cout << "  -v            Show version\n\n";
    std::cout << BOLD << "Format specifiers for -t:" << RESET << "\n";
    std::cout << "  %Y (year)  %y (2-digit year)  %m (month)  %d (day)\n";
    std::cout << "  %B (full month)  %b (short month)\n";
    std::cout << "  %A (full weekday)  %a (short weekday)\n";
    std::cout << "  %H:%M:%S (time)  %% (literal %)\n\n";
    std::cout << BOLD << "Examples:" << RESET << "\n";
    std::cout << "  " << prog << "                     # current month\n";
    std::cout << "  " << prog << " -e 7 1405             # English weekday names\n";
    std::cout << "  " << prog << " -t                    # date & time now\n";
    std::cout << "  " << prog << " -t '+%Y/%m/%d'        # formatted Jalali date\n";
    std::cout << "  " << prog << " -tp '+%A %d %B %Y'    # fully Persian\n";
    std::cout << "  " << prog << " -c 1405/07/12         # Jalali -> Gregorian\n\n";
}

// ==================== تابع اصلی ====================
// ==================== تابع اصلی ====================
int main(int argc, char* argv[]) {
    Options opts;
    int opt;
    bool do_convert = false, do_diff = false, do_today = false;
    std::string convert_arg, today_fmt;

    // توجه: -t دیگر optional_argument نیست
    static struct option long_opts[] = {
        {"convert", required_argument, 0, 'c'},
        {"diff",    no_argument,       0, 'd'},
        {"today",   no_argument,       0, 't'},
        {"year",    no_argument,       0, 'y'},
        {"three",   no_argument,       0, '3'},
        {"stacked", no_argument,       0, 's'},
        {"persian", no_argument,       0, 'p'},
        {"english", no_argument,       0, 'e'},
        {"julian",  no_argument,       0, 'j'},
        {"pahlavi", no_argument,       0, 'P'},
        {"nocolor", no_argument,       0, 'n'},
        {"help",    no_argument,       0, 'h'},
        {"version", no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    // رشته‌ی گزینه‌ها: 't' بدون ':' (آرگومان اجباری ندارد)
    while ((opt = getopt_long(argc, argv, "c:dty3spejPnhv", long_opts, nullptr)) != -1) {
        switch (opt) {
            case 'c':
                do_convert = true;
                convert_arg = optarg;
                break;
            case 'd':
                do_diff = true;
                break;
            case 't':
                do_today = true;
                break;
            case 'y':
                opts.full_year = true;
                break;
            case '3':
                opts.three_months = true;
                break;
            case 's':
                opts.stacked = true;
                break;
            case 'p':
                opts.persian = true;
                break;
            case 'e':
                opts.english_names = true;
                break;
            case 'j':
                opts.julian_day = true;
                break;
            case 'P':
                opts.pahlavi = true;
                break;
            case 'n':
                opts.color = false;
                break;
            case 'h':
                print_help(argv[0]);
                return 0;
            case 'v':
                std::cout << "cal-fa " << VERSION << "\n";
                return 0;
            default:
                print_help(argv[0]);
                return 1;
        }
    }

    // ---- حالت تبدیل تاریخ ----
    if (do_convert) {
        return cmd_convert(convert_arg, opts);
    }

    // ---- حالت jdate (-t) ----
    // اگر آرگومان غیرگزینه‌ای بعد از -t مانده باشد، آن را فرمت در نظر می‌گیریم.
    if (do_today) {
        if (optind < argc) {
            today_fmt = argv[optind];
        }
        return cmd_today(today_fmt, opts);
    }

    // ---- حالت اختلاف دو تاریخ ----
    if (do_diff) {
        int rem = argc - optind;
        if (rem < 2) {
            std::cerr << "Error: -d requires two dates.\n";
            return 1;
        }
        return cmd_diff(argv[optind], argv[optind + 1], opts);
    }

    // ---- حالت تقویم: تجزیه‌ی [month] [year] ----
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
        if (opts.year < 1 || opts.year > 9999) {
            std::cerr << "Error: year must be between 1 and 9999.\n";
            return 1;
        }
    }

    // تعیین تاریخ پیش‌فرض (امروز)
    PersianDate today_p = to_persian(bg::day_clock::local_day());
    int year  = (opts.year  != -1) ? opts.year  : today_p.year;
    int month = (opts.month != -1) ? opts.month : today_p.month;

    // ---- اجرای حالت نهایی ----
    if (opts.full_year) {
        print_year(year, opts);
    } else if (opts.three_months) {
        print_three_months(year, month, opts);
    } else {
        print_single(opts, year, month);
    }

    return 0;
}
