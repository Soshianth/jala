// =============================================================================
// main.cpp — command-line interface, rendering, and dispatch for jala
// SPDX-License-Identifier: MIT
// =============================================================================

#include "jalali.hpp"
#include "holidays.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <getopt.h>
#include <unistd.h>

using namespace jala;

namespace bg = boost::gregorian;

namespace {

// =============================================================================
// Constants
// =============================================================================

constexpr std::string_view VERSION = "1.1.0";

// ANSI escape sequences for terminal colors.
namespace ansi {
constexpr std::string_view reset   = "\033[0m";
constexpr std::string_view reverse = "\033[7m";
constexpr std::string_view bold    = "\033[1m";
constexpr std::string_view green   = "\033[32m";
constexpr std::string_view yellow  = "\033[33m";
constexpr std::string_view blue    = "\033[34m";
constexpr std::string_view cyan    = "\033[36m";
constexpr std::string_view red     = "\033[31m";
} // namespace ansi

// Unicode Left-to-Right Mark (U+200E). Wrapping a Persian weekday
// cell in LRM forces bidi-aware terminals to render it left-to-right,
// keeping the header aligned with the numeric cells below. Users on
// terminals that mishandle LRM can disable this with --no-bidi.
constexpr std::string_view LRM = "\u200E";

// =============================================================================
// Runtime configuration
// =============================================================================

struct Options {
    int  year          = -1;
    int  month         = -1;
    bool full_year     = false;
    bool three_months  = false;
    bool stacked       = false;
    bool color         = true;
    bool persian       = false;
    bool julian_day    = false;
    bool imperial      = false;
    bool english_names = false;
    bool no_bidi       = false;
    bool show_holidays = true;
    bool show_events   = false;
};

// =============================================================================
// Terminal width helpers
// =============================================================================

// Compute the display width of `s`, ignoring ANSI escape sequences and
// Unicode zero-width control characters. This allows callers to
// right-pad or measure colored strings correctly.
[[nodiscard]] int visible_length(std::string_view s) {
    int  len    = 0;
    bool in_esc = false;

    for (size_t i = 0; i < s.size(); ) {
        const auto c = static_cast<unsigned char>(s[i]);

        if (in_esc) {
            if (c == 'm') in_esc = false;
            ++i;
            continue;
        }
        if (c == '\033') { in_esc = true; ++i; continue; }
        if ((c & 0xC0) == 0x80) { ++i; continue; }

        // Zero-width Unicode controls: U+200B..U+200F and U+2060..U+206F.
        if (c == 0xE2 && i + 2 < s.size()) {
            const auto b = static_cast<unsigned char>(s[i + 1]);
            const auto d = static_cast<unsigned char>(s[i + 2]);
            if (b == 0x80 && d >= 0x8B && d <= 0x8F) { i += 3; continue; }
            if (b == 0x81 && d >= 0xA0 && d <= 0xAF) { i += 3; continue; }
        }

        ++len;

        if      ((c & 0xE0) == 0xC0) i += 2;  // 2-byte UTF-8
        else if ((c & 0xF0) == 0xE0) i += 3;  // 3-byte UTF-8
        else if ((c & 0xF8) == 0xF0) i += 4;  // 4-byte UTF-8
        else                         i += 1;  // ASCII or invalid byte
    }
    return len;
}

[[nodiscard]] std::string pad_visible(std::string_view s, int width) {
    const int v = visible_length(s);
    std::string out(s);
    if (v < width) out.append(static_cast<size_t>(width - v), ' ');
    return out;
}

// =============================================================================
// Calendar layout
// =============================================================================

[[nodiscard]] int cell_width(const Options& opts) {
    const int day_w = opts.julian_day ? 3 : 2;
    const int wd_w  = (!opts.persian && opts.english_names) ? 3 : 2;
    return std::max(day_w, wd_w);
}

[[nodiscard]] int row_width(const Options& opts) {
    return 7 * cell_width(opts) + 6;
}

[[nodiscard]] std::vector<std::string> format_month(int year, int month,
                                                    const Options& opts) {
    std::vector<std::string> lines;
    const int cw = cell_width(opts);
    const int rw = row_width(opts);

    // ---- Month title ----
    const int display_year = opts.imperial ? year + IMPERIAL_OFFSET : year;
    std::string title = opts.persian
        ? std::string(MONTHS_FA[month - 1]) + " " +
              to_persian_digits(std::to_string(display_year))
        : std::string(MONTHS_EN[month - 1]) + " " +
              std::to_string(display_year);
    if (opts.imperial) title += "(pa)";

    const int pad = std::max(0, (rw - visible_length(title)) / 2);
    std::string title_line;
    if (opts.color) title_line += ansi::bold;
    title_line.append(static_cast<size_t>(pad), ' ');
    title_line += title;
    if (opts.color) title_line += ansi::reset;
    lines.push_back(std::move(title_line));

    // ---- Weekday header ----
    std::string header;
    for (int i = 0; i < 7; ++i) {
        const char* name = opts.persian       ? WEEKDAYS_FA[i]
                         : opts.english_names ? WEEKDAYS_ABBR_EN[i]
                                              : WEEKDAYS_SHORT[i];

        std::string cell(
            static_cast<size_t>(std::max(0, cw - visible_length(name))), ' ');
        cell += name;

        if (opts.persian && !opts.no_bidi) {
            cell.insert(0, LRM);
            cell.append(LRM);
        }

        if (opts.color) header += ansi::blue;
        header += cell;
        if (opts.color) header += ansi::reset;
        if (i < 6) header += ' ';
    }
    lines.push_back(std::move(header));

    // ---- Day numbers ----
    const int first_wd = persian_weekday(year, month, 1);
    const int days     = persian_month_days(year, month);
    const PersianDate today = to_persian(bg::day_clock::local_day());
    const bool is_current   = (today.year == year && today.month == month);

    // Build the holiday index once per program run. Since this function
    // may be called multiple times (e.g. for a full year), a function-
    // local static ensures we only construct it once.
    static const HolidayIndex holidays;

    std::string row;
    int col = 0;

    // Leading blank cells before the first day of the month.
    for (int i = 0; i < first_wd; ++i) {
        row.append(static_cast<size_t>(cw), ' ');
        if (++col < 7) row += ' ';
    }

    for (int d = 1; d <= days; ++d) {
        const int num = opts.julian_day ? persian_day_of_year(year, month, d) : d;
        std::string s = std::to_string(num);
        std::string cell(static_cast<size_t>(cw - s.size()), ' ');
        cell += s;
        if (opts.persian) cell = to_persian_digits(cell);

        const int wd = persian_weekday(year, month, d);
        const bool is_holiday =
            opts.show_holidays &&
            holidays.contains(format_holiday_key(year, month, d));

        if (is_current && today.day == d) {
            // Today: reverse video (highest priority)
            if (opts.color) row += ansi::reverse;
            row += cell;
            if (opts.color) row += ansi::reset;
        } else if (is_holiday) {
            // Official holiday: red
            if (opts.color) row += ansi::red;
            row += cell;
            if (opts.color) row += ansi::reset;
        } else if (wd == 6) {
            // Friday: yellow
            if (opts.color) row += ansi::yellow;
            row += cell;
            if (opts.color) row += ansi::reset;
        } else {
            row += cell;
        }

        if (++col == 7) {
            lines.push_back(std::move(row));
            row.clear();
            col = 0;
        } else {
            row += ' ';
        }
    }
    if (col != 0) lines.push_back(std::move(row));
    return lines;
}

// Print a chronological list of all events (holidays and non-holidays)
// for the given months. Used by the -E/--events flag.
void print_events(const Options& opts,
                  const std::vector<std::pair<int,int>>& months) {
    static const EventIndex events;

    if (opts.color) std::cout << ansi::bold << "Events:" << ansi::reset << "\n";
    else            std::cout << "Events:\n";

    bool any = false;

    for (const auto& [year, month] : months) {
        const int days = persian_month_days(year, month);

        for (int d = 1; d <= days; ++d) {
            const std::string key = format_holiday_key(year, month, d);
            const auto& evs = events.events(key);
            if (evs.empty()) continue;

            any = true;

            char buf[24];
            std::snprintf(buf, sizeof(buf), "%04d/%02d/%02d", year, month, d);
            const std::string date_str =
                opts.persian ? to_persian_digits(buf) : std::string(buf);

            for (const auto& e : evs) {
                std::cout << "  " << date_str << "  ";
                if (opts.color) {
                    std::cout << (e.is_holiday ? ansi::red : ansi::cyan);
                }
                std::cout << e.description;
                if (opts.color) std::cout << ansi::reset;
                std::cout << "\n";
            }
        }
    }

    if (!any) std::cout << "  (no events)\n";
    std::cout << "\n";
}

// =============================================================================
// Output helpers
// =============================================================================

void print_single(const Options& opts, int year, int month) {
    for (const auto& line : format_month(year, month, opts))
        std::cout << line << '\n';
    std::cout << '\n';

    if (opts.show_events) {
        print_events(opts, {{year, month}});
    }
}

void print_columns(const Options& opts,
                   const std::vector<std::pair<int,int>>& months) {
    std::vector<std::vector<std::string>> rendered;
    rendered.reserve(months.size());
    for (const auto& [y, m] : months)
        rendered.push_back(format_month(y, m, opts));

    size_t max_lines = 0;
    for (const auto& v : rendered)
        max_lines = std::max(max_lines, v.size());

    const int col_w = row_width(opts);

    for (size_t i = 0; i < max_lines; ++i) {
        std::string line;
        for (size_t j = 0; j < rendered.size(); ++j) {
            constexpr int gap = 3;
            const std::string_view cell =
                i < rendered[j].size()
                    ? std::string_view(rendered[j][i])
                    : std::string_view{};
            line += pad_visible(cell, col_w);
            if (j + 1 < rendered.size())
                line.append(gap, ' ');
        }
        while (!line.empty() && line.back() == ' ') line.pop_back();
        std::cout << line << '\n';
    }
    if (opts.show_events) {
        print_events(opts, months);
    }
    std::cout << '\n';
}

void print_stacked(const Options& opts,
                   const std::vector<std::pair<int,int>>& months) {
    for (const auto& [y, m] : months) print_single(opts, y, m);
}

void print_year(int year, const Options& opts) {
    const int display_year = opts.imperial ? year + IMPERIAL_OFFSET : year;
    std::string ys = opts.persian
        ? to_persian_digits(std::to_string(display_year))
        : std::to_string(display_year);
    if (opts.imperial) ys += "(pa)";

    if (opts.stacked) {
        if (opts.color) std::cout << ansi::bold << ansi::green;
        std::cout << "Year " << ys << "\n\n";
        if (opts.color) std::cout << ansi::reset;
        for (int m = 1; m <= MAX_MONTH; ++m) print_single(opts, year, m);
        return;
    }

    const int col_w      = row_width(opts);
    const int grid_width = 3 * col_w + 2 * 3;
    const int pad        = std::max(0, (grid_width - visible_length(ys)) / 2);

    if (opts.color) std::cout << ansi::bold << ansi::green;
    std::cout << std::string(static_cast<size_t>(pad), ' ') << ys << "\n\n";
    if (opts.color) std::cout << ansi::reset;

    for (int m = 1; m <= MAX_MONTH; m += 3) {
        std::vector<std::pair<int,int>> batch;
        for (int k = 0; k < 3 && m + k <= MAX_MONTH; ++k)
            batch.emplace_back(year, m + k);
        print_columns(opts, batch);
    }
}

void print_three_months(int year, int month, const Options& opts) {
    int prev = month - 1, next = month + 1;
    int py = year, ny = year;
    if (prev < 1)  { prev = MAX_MONTH; --py; }
    if (next > MAX_MONTH) { next = 1; ++ny; }

    const std::vector<std::pair<int,int>> months = {
        {py, prev}, {year, month}, {ny, next}
    };
    if (opts.stacked) print_stacked(opts, months);
    else              print_columns(opts, months);
}

// =============================================================================
// Subcommands
// =============================================================================

int cmd_convert(std::string_view arg, const Options& opts) {
    const SimpleDate sd = parse_date(arg);
    if (!sd.valid) {
        std::cerr << "Error: cannot parse date '" << arg << "'\n";
        return 1;
    }

    const auto fmt2 = [&](int n) {
        char buf[4];
        std::snprintf(buf, sizeof(buf), "%02d", n);
        return opts.persian ? to_persian_digits(buf) : std::string(buf);
    };
    const auto fmt_year = [&](int y) {
        return opts.persian ? to_persian_digits(std::to_string(y))
                            : std::to_string(y);
    };

    if (sd.jalali) {
        const auto g = jdn_to_gregorian(
            persian_to_jdn(sd.year, sd.month, sd.day));
        const int wd = persian_weekday(sd.year, sd.month, sd.day);
        const char* wdn = opts.persian ? WEEKDAYS_FULL_FA[wd]
                                        : WEEKDAYS_FULL_EN[wd];

        std::cout << '\n';
        if (opts.color) std::cout << ansi::bold << ansi::cyan;
        std::cout << "Jalali:    ";
        if (opts.color) std::cout << ansi::reset;
        std::cout << fmt_year(sd.year) << '/'
                  << fmt2(sd.month) << '/' << fmt2(sd.day)
                  << "  (" << wdn << ")\n";

        if (opts.color) std::cout << ansi::bold << ansi::cyan;
        std::cout << "Gregorian: ";
        if (opts.color) std::cout << ansi::reset;
        std::cout << g.year() << '-'
                  << std::setw(2) << std::setfill('0')
                  << g.month().as_number() << '-'
                  << std::setw(2) << std::setfill('0') << g.day()
                  << std::setfill(' ') << "\n\n";
    } else {
        // Constructing a boost::gregorian::date can throw if any
        // component is out of range. parse_date() has already
        // validated the input, but we keep the guard for defense
        // in depth and to produce a clean error message.
        boost::gregorian::date g;
        try {
            g = boost::gregorian::date(sd.year, sd.month, sd.day);
        } catch (const std::exception& e) {
            std::cerr << "Error: invalid Gregorian date: "
                      << e.what() << "\n";
            return 1;
        }
        if (g.is_not_a_date()) {
            std::cerr << "Error: invalid Gregorian date.\n";
            return 1;
        }
        const PersianDate p = to_persian(g);
        const int wd = persian_weekday(p.year, p.month, p.day);
        const char* wdn = opts.persian ? WEEKDAYS_FULL_FA[wd]
                                        : WEEKDAYS_FULL_EN[wd];

        std::cout << '\n';
        if (opts.color) std::cout << ansi::bold << ansi::cyan;
        std::cout << "Gregorian: ";
        if (opts.color) std::cout << ansi::reset;
        std::cout << sd.year << '-'
                  << std::setw(2) << std::setfill('0') << sd.month << '-'
                  << std::setw(2) << std::setfill('0') << sd.day
                  << std::setfill(' ') << "\n";

        if (opts.color) std::cout << ansi::bold << ansi::cyan;
        std::cout << "Jalali:    ";
        if (opts.color) std::cout << ansi::reset;
        std::cout << fmt_year(p.year) << '/'
                  << fmt2(p.month) << '/' << fmt2(p.day)
                  << "  (" << wdn << ")\n\n";
    }
    return 0;
}

int cmd_diff(std::string_view a, std::string_view b, const Options& opts) {
    const SimpleDate da = parse_date(a);
    const SimpleDate db = parse_date(b);
    if (!da.valid) { std::cerr << "Error: invalid date '" << a << "'\n"; return 1; }
    if (!db.valid) { std::cerr << "Error: invalid date '" << b << "'\n"; return 1; }

    // Convert each date to a JDN. Gregorian dates may throw if the
    // input is out of range; parse_date() has already validated
    // them, but we catch exceptions anyway to produce a clean error.
    long j1 = 0;
    long j2 = 0;
    try {
        j1 = da.jalali
            ? persian_to_jdn(da.year, da.month, da.day)
            : boost::gregorian::date(da.year, da.month, da.day).julian_day();
        j2 = db.jalali
            ? persian_to_jdn(db.year, db.month, db.day)
            : boost::gregorian::date(db.year, db.month, db.day).julian_day();
    } catch (const std::exception& e) {
        std::cerr << "Error: could not convert date: "
                  << e.what() << "\n";
        return 1;
    }

    const long diff  = std::abs(j2 - j1);
    const long weeks = diff / 7;
    const long days  = diff % 7;

    const auto pf = [&](long n) {
        return opts.persian ? to_persian_digits(std::to_string(n))
                            : std::to_string(n);
    };

    std::cout << '\n';
    if (opts.color) std::cout << ansi::bold << ansi::cyan;
    std::cout << "Difference: ";
    if (opts.color) std::cout << ansi::reset;
    std::cout << pf(diff) << " day" << (diff == 1 ? "" : "s");
    if (diff >= 7) {
        std::cout << "  (" << pf(weeks) << " week" << (weeks == 1 ? "" : "s")
                  << " + " << pf(days) << " day" << (days == 1 ? "" : "s")
                  << ")";
    }
    std::cout << "\n\n";
    return 0;
}

// =============================================================================
// jdate-style formatting
// =============================================================================

[[nodiscard]] std::string format_jdate(std::string_view fmt, const Options& opts) {
    const auto g  = boost::gregorian::day_clock::local_day();
    const auto p  = to_persian(g);
    const int  wd = persian_weekday(p.year, p.month, p.day);
    const int  display_year = opts.imperial ? p.year + IMPERIAL_OFFSET : p.year;

    const std::time_t t  = std::time(nullptr);
    const std::tm*    lt = std::localtime(&t);

    std::string out;
    char buf[32];

    for (size_t i = 0; i < fmt.size(); ++i) {
        if (fmt[i] != '%' || i + 1 >= fmt.size()) {
            out += fmt[i];
            continue;
        }

        switch (fmt[++i]) {
            case 'Y': std::snprintf(buf, sizeof(buf), "%04d", display_year); out += buf; break;
            case 'y': std::snprintf(buf, sizeof(buf), "%02d", display_year % 100); out += buf; break;
            case 'm': std::snprintf(buf, sizeof(buf), "%02d", p.month); out += buf; break;
            case 'd': std::snprintf(buf, sizeof(buf), "%02d", p.day);   out += buf; break;
            case 'B': out += opts.persian ? MONTHS_FA[p.month - 1]
                                          : MONTHS_EN[p.month - 1]; break;
            case 'b':
                // Persian month names are UTF-8 multibyte; truncating to a
                // fixed byte count would split a code point. Fall back to
                // the full name in that case.
                if (opts.persian) out += MONTHS_FA[p.month - 1];
                else              out.append(MONTHS_EN[p.month - 1], 3);
                break;
            case 'A': out += opts.persian ? WEEKDAYS_FULL_FA[wd]
                                          : WEEKDAYS_FULL_EN[wd]; break;
            case 'a': out += opts.persian ? WEEKDAYS_FA[wd]
                                          : WEEKDAYS_ABBR_EN[wd]; break;
            case 'H': std::snprintf(buf, sizeof(buf), "%02d", lt->tm_hour); out += buf; break;
            case 'M': std::snprintf(buf, sizeof(buf), "%02d", lt->tm_min);  out += buf; break;
            case 'S': std::snprintf(buf, sizeof(buf), "%02d", lt->tm_sec);  out += buf; break;
            case '%': out += '%';  break;
            case 'n': out += '\n'; break;
            case 't': out += '\t'; break;
            default:  out += '%'; out += fmt[i]; break;
        }
    }
    if (opts.persian) out = to_persian_digits(out);
    return out;
}

int cmd_today(std::string_view fmt_arg, const Options& opts) {
    std::string fmt = fmt_arg.empty()
        ? std::string("%A %d %B %Y  %H:%M:%S")
        : std::string(fmt_arg);
    if (!fmt.empty() && fmt.front() == '+') fmt.erase(fmt.begin());

    const std::string out = format_jdate(fmt, opts);
    if (opts.color) std::cout << ansi::bold << ansi::cyan;
    std::cout << out;
    if (opts.color) std::cout << ansi::reset;
    std::cout << '\n';
    return 0;
}

// =============================================================================
// Help text
// =============================================================================

void print_help(const char* prog) {
    using std::cout;
    cout << '\n';
    cout << ansi::bold << "jala " << VERSION << ansi::reset
         << " — Persian calendar in the terminal\n\n";
    cout << ansi::bold << "Usage:" << ansi::reset << "\n";
    cout << "  " << prog << " [options] [month] [year]\n";
    cout << "  " << prog << " -c <date>          # convert date\n";
    cout << "  " << prog << " -d <date1> <date2> # diff two dates\n";
    cout << "  " << prog << " -t [+FORMAT]       # current date & time\n\n";
    cout << ansi::bold << "Options:" << ansi::reset << "\n";
    cout << "  -y            Show full year (3-column grid)\n";
    cout << "  -3            Show three months side by side\n";
    cout << "  -s            Stacked mode (single column)\n";
    cout << "  -p            Persian digits and month/weekday names\n";
    cout << "  -e            English weekday abbreviations (Sat, Sun, ...)\n";
    cout << "  -j            Show day-of-year instead of day-of-month\n";
    cout << "  -P            Imperial (Shahanshahi) year = Jalali + 1180\n";
    cout << "  -t, --today   Print current date and time\n";
    cout << "  -c, --convert Convert Jalali <-> Gregorian\n";
    cout << "  -d, --diff    Difference in days between two dates\n";
    cout << "  -B, --no-bidi Do not wrap Persian weekday cells in LRM\n";
    cout << "  -H, --no-holidays  Do not highlight Iranian holidays\n";
    cout << "  -E, --events       List all events after the calendar\n";
    cout << "  -n            No color\n";
    cout << "  -h            Show this help\n";
    cout << "  -v            Show version\n\n";
    cout << ansi::bold << "Format specifiers for -t:" << ansi::reset << "\n";
    cout << "  %Y (year)  %y (2-digit year)  %m (month)  %d (day)\n";
    cout << "  %B (full month)  %b (short month)\n";
    cout << "  %A (full weekday)  %a (short weekday)\n";
    cout << "  %H:%M:%S (time)  %% (literal %)\n\n";
    cout << ansi::bold << "Examples:" << ansi::reset << "\n";
    cout << "  " << prog << "                     # current month\n";
    cout << "  " << prog << " -e 7 1405             # English weekday names\n";
    cout << "  " << prog << " -t                    # date & time now\n";
    cout << "  " << prog << " -t '+%Y/%m/%d'        # formatted Jalali date\n";
    cout << "  " << prog << " -tp '+%A %d %B %Y'    # fully Persian\n";
    cout << "  " << prog << " -c 1405/07/12         # Jalali -> Gregorian\n\n";
}

} // namespace

// =============================================================================
// Program entry point
// =============================================================================

[[nodiscard]] int run(int argc, char* argv[]) {
    Options opts;

    // Respect the NO_COLOR convention (https://no-color.org).
    if (const char* nc = std::getenv("NO_COLOR"); nc && *nc)
        opts.color = false;

    static const struct option long_opts[] = {
        {"convert",     required_argument, nullptr, 'c'},
        {"diff",        no_argument,       nullptr, 'd'},
        {"today",       no_argument,       nullptr, 't'},
        {"year",        no_argument,       nullptr, 'y'},
        {"three",       no_argument,       nullptr, '3'},
        {"stacked",     no_argument,       nullptr, 's'},
        {"persian",     no_argument,       nullptr, 'p'},
        {"english",     no_argument,       nullptr, 'e'},
        {"julian",      no_argument,       nullptr, 'j'},
        {"pahlavi",     no_argument,       nullptr, 'P'},
        {"no-bidi",     no_argument,       nullptr, 'B'},
        {"no-holidays", no_argument,       nullptr, 'H'},
        {"events",      no_argument,       nullptr, 'E'},
        {"nocolor",     no_argument,       nullptr, 'n'},
        {"help",        no_argument,       nullptr, 'h'},
        {"version",     no_argument,       nullptr, 'v'},
        {nullptr, 0, nullptr, 0}
    };

    bool        do_convert  = false;
    bool        do_diff     = false;
    bool        do_today    = false;
    std::string convert_arg;
    std::string today_fmt;

    int opt;
    while ((opt = getopt_long(argc, argv, "c:dty3spejPBNHnEhv",
                              long_opts, nullptr)) != -1) {
        switch (opt) {
            case 'c': do_convert = true; convert_arg = optarg; break;
            case 'd': do_diff    = true; break;
            case 't': do_today   = true; break;
            case 'y': opts.full_year     = true; break;
            case '3': opts.three_months  = true; break;
            case 's': opts.stacked       = true; break;
            case 'p': opts.persian       = true; break;
            case 'e': opts.english_names = true; break;
            case 'j': opts.julian_day    = true; break;
            case 'P': opts.imperial      = true; break;
            case 'B': opts.no_bidi       = true; break;
            case 'H': opts.show_holidays = false; break;
            case 'E': opts.show_events   = true;  break;
            case 'n': opts.color         = false; break;
            case 'h': print_help(argv[0]); return 0;
            case 'v': std::cout << "jala " << VERSION << '\n'; return 0;
            default:  print_help(argv[0]); return 1;
        }
    }

    if (do_convert) return cmd_convert(convert_arg, opts);

    if (do_today) {
        if (optind < argc) today_fmt = argv[optind];
        return cmd_today(today_fmt, opts);
    }

    if (do_diff) {
        if (argc - optind < 2) {
            std::cerr << "Error: -d requires two dates.\n";
            return 1;
        }
        return cmd_diff(argv[optind], argv[optind + 1], opts);
    }

    const int remaining = argc - optind;

    if (opts.full_year && remaining == 1) {
        // `-y <year>` — a single positional argument is the year.
        try {
            opts.year = std::stoi(argv[optind]);
        } catch (...) {
            std::cerr << "Error: year must be a number.\n";
            return 1;
        }
        if (opts.year < 1 || opts.year > MAX_YEAR) {
            std::cerr << "Error: year must be between 1 and "
                      << MAX_YEAR << ".\n";
            return 1;
        }
    } else {
        if (remaining >= 1) {
            try {
                opts.month = std::stoi(argv[optind]);
            } catch (...) {
                std::cerr << "Error: month must be a number.\n";
                return 1;
            }
            if (opts.month < 1 || opts.month > MAX_MONTH) {
                std::cerr << "Error: month must be between 1 and "
                          << MAX_MONTH << ".\n";
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
            if (opts.year < 1 || opts.year > MAX_YEAR) {
                std::cerr << "Error: year must be between 1 and "
                          << MAX_YEAR << ".\n";
                return 1;
            }
        }
    }

    const PersianDate today = to_persian(
        boost::gregorian::day_clock::local_day());
    const int year  = opts.year  != -1 ? opts.year  : today.year;
    const int month = opts.month != -1 ? opts.month : today.month;

    if (opts.full_year)         print_year(year, opts);
    else if (opts.three_months) print_three_months(year, month, opts);
    else                        print_single(opts, year, month);

    return 0;
}

int main(int argc, char* argv[]) {
    try {
        return run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Error: unknown exception\n";
        return 1;
    }
}