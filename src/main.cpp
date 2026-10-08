// =============================================================================
// main.cpp — command-line interface, rendering, and dispatch for jala
// SPDX-License-Identifier: MIT
//
// This is the presentation layer: it parses command-line arguments,
// lays out the calendar grid, and dispatches to the subcommands.
// The calendar math lives in jalali.cpp and the Gregorian helpers in
// gregorian.cpp; this file only formats their output.
//
// There is no dependency on Boost or any external library.
// =============================================================================

#include "jalali.hpp"
#include "holidays.hpp"

#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <getopt.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

using namespace jala;

namespace {

// =============================================================================
// Constants
// =============================================================================

constexpr std::string_view VERSION = "1.4.0";

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
}  // namespace ansi

// Controls when ANSI colors are emitted.
//   Auto   — colors only when stdout is a TTY (default).
//   Always — colors are always emitted, even when piped or redirected.
//   Never  — colors are never emitted.
enum class ColorMode {
    Auto,
    Always,
    Never,
};

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
    ColorMode color_mode = ColorMode::Auto;
    CalendarMode calendar_mode = CalendarMode::Auto;
};

// =============================================================================
// Terminal width helpers
// =============================================================================

// Compute the display width of `s` in terminal columns.
//
// This is more accurate than counting code points: it uses wcwidth()
// to ask the C library for the column width of each wide character,
// so CJK (width 2) and combining marks (width 0) are handled
// correctly. ANSI escape sequences are skipped.
//
// The conversion is done with mbrtowc() using the current locale,
// which the program is expected to have set up via setlocale() in
// main().
[[nodiscard]] int visible_length(std::string_view s) {
    int  width  = 0;
    bool in_esc = false;

    // Save and reset the conversion state for each call, since the
    // same buffer may be reused across calls.
    std::mbstate_t state{};

    for (size_t i = 0; i < s.size(); ) {
        const auto c = static_cast<unsigned char>(s[i]);

        // Skip ANSI escape sequences entirely.
        if (in_esc) {
            if (c == 'm') in_esc = false;
            ++i;
            continue;
        }
        if (c == '\033') { in_esc = true; ++i; continue; }

        // Decode one multibyte character.
        wchar_t wc = 0;
        const size_t len = std::mbrtowc(&wc, s.data() + i,
                                        s.size() - i, &state);
        if (len == static_cast<size_t>(-1) ||
            len == static_cast<size_t>(-2)) {
            // Invalid or incomplete sequence: skip one byte.
            std::memset(&state, 0, sizeof(state));
            ++i;
            continue;
        }
        if (len == 0) break;  // embedded NUL

        i += len;

        // Determine the cell width of this character.
        const int w = ::wcwidth(wc);
        if (w > 0) {
            width += w;
        }
        // w < 0 means a non-printable character; we count it as 0.
    }
    return width;
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
    // In Persian mode, the weekday cells are single RTL letters. A
    // bidi-aware terminal will render a run of such letters right-
    // to-left, which would reverse the visual order of the header
    // relative to the numeric cells below.
    //
    // With LRM (the default), each cell is wrapped in a pair of LRM
    // markers, forcing the terminal to treat each cell as LTR. With
    // --no-bidi, we do not insert LRM; to keep the visual order
    // correct we instead emit the cells in reverse order, so that
    // the terminal's own RTL rendering turns them back into the
    // intended left-to-right sequence.
    const bool reverse_header = opts.persian && opts.no_bidi;

    std::string header;
    for (int k = 0; k < 7; ++k) {
        const int i = reverse_header ? (6 - k) : k;

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
        if (k < 6) header += ' ';
    }
    lines.push_back(std::move(header));

    // ---- Day numbers ----
    const int first_wd = persian_weekday(year, month, 1);
    const int days     = persian_month_days(year, month);

    // The local variable is named `today_date` so that it does not
    // shadow the function `jala::today()` used to obtain it.
    const PersianDate today_date = to_persian(jala::today());
    const bool is_current =
        (today_date.year == year && today_date.month == month);

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
        const int num = opts.julian_day
            ? persian_day_of_year(year, month, d) : d;
        std::string s = std::to_string(num);
        std::string cell(static_cast<size_t>(cw - s.size()), ' ');
        cell += s;
        if (opts.persian) cell = to_persian_digits(cell);

        const int wd = persian_weekday(year, month, d);
        const bool is_holiday =
            opts.show_holidays &&
            holidays.contains(format_holiday_key(year, month, d));

        if (is_current && today_date.day == d) {
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
    const SimpleDate sd = parse_date(arg, opts.calendar_mode);
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
        const GregorianDate g = jdn_to_gregorian(
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
        std::cout << g.year << '-'
                  << std::setw(2) << std::setfill('0') << g.month << '-'
                  << std::setw(2) << std::setfill('0') << g.day
                  << std::setfill(' ') << "\n\n";
    } else {
        // parse_date() has already validated the Gregorian date,
        // so no exceptions are possible here.
        const GregorianDate g{ sd.year, sd.month, sd.day };
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
    const SimpleDate da = parse_date(a, opts.calendar_mode);
    const SimpleDate db = parse_date(b, opts.calendar_mode);
    if (!da.valid) { std::cerr << "Error: invalid date '" << a << "'\n"; return 1; }
    if (!db.valid) { std::cerr << "Error: invalid date '" << b << "'\n"; return 1; }

    // Both dates have been validated, so the conversions cannot fail.
    const long j1 = da.jalali
        ? persian_to_jdn(da.year, da.month, da.day)
        : to_jdn(da.year, da.month, da.day);
    const long j2 = db.jalali
        ? persian_to_jdn(db.year, db.month, db.day)
        : to_jdn(db.year, db.month, db.day);

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
    const PersianDate p  = to_persian(jala::today());
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

// Print the command-line help. When `use_color` is false, all ANSI
// escape sequences are omitted, so the output stays readable in a
// pipe or with NO_COLOR set.
void print_help(const char* prog, bool use_color) {
    using std::cout;

    const auto B = use_color ? ansi::bold  : std::string_view{};
    const auto R = use_color ? ansi::reset : std::string_view{};

    cout << '\n';
    cout << B << "jala " << VERSION << R
         << " — Persian calendar in the terminal\n\n";
    cout << B << "Usage:" << R << "\n";
    cout << "  " << prog << " [options] [month] [year]\n";
    cout << "  " << prog << " -c <date>          # convert date\n";
    cout << "  " << prog << " -d <date1> <date2> # diff two dates\n";
    cout << "  " << prog << " -t [+FORMAT]       # current date & time\n\n";
    cout << B << "Options:" << R << "\n";
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
    cout << "  --color=WHEN  Colorize output: always, never, or auto\n";
    cout << "                (default: auto; colors only when stdout is a TTY)\n";
    cout << "  --calendar=WHEN  Interpret -c and -d dates in: auto,\n";
    cout << "                   jalali, or gregorian (default: auto)\n";
    cout << "  -n            No color (same as --color=never)\n";
    cout << "  -h            Show this help\n";
    cout << "  -v            Show version\n\n";
    cout << B << "Format specifiers for -t:" << R << "\n";
    cout << "  %Y (year)  %y (2-digit year)  %m (month)  %d (day)\n";
    cout << "  %B (full month)  %b (short month)\n";
    cout << "  %A (full weekday)  %a (short weekday)\n";
    cout << "  %H:%M:%S (time)  %% (literal %)\n\n";
    cout << B << "Examples:" << R << "\n";
    cout << "  " << prog << "                     # current month\n";
    cout << "  " << prog << " -e 7 1405             # English weekday names\n";
    cout << "  " << prog << " -t                    # date & time now\n";
    cout << "  " << prog << " -t '+%Y/%m/%d'        # formatted Jalali date\n";
    cout << "  " << prog << " -tp '+%A %d %B %Y'    # fully Persian\n";
    cout << "  " << prog << " -c 1405/07/12         # Jalali -> Gregorian\n\n";
}

// =============================================================================
// Program entry point
// =============================================================================

[[nodiscard]] int run(int argc, char* argv[]) {
    Options opts;

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
        {"color",       required_argument, nullptr, 1000},
        {"calendar",    required_argument, nullptr, 1001},
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

    bool help_requested = false;

    int opt;
    while ((opt = getopt_long(argc, argv, "c:dty3spejPBNHnEhv",
                              long_opts, nullptr)) != -1) {
        switch (opt) {
            case 1000: {
                // --color=WHEN
                const std::string_view when(optarg);
                if      (when == "always") opts.color_mode = ColorMode::Always;
                else if (when == "never")  opts.color_mode = ColorMode::Never;
                else if (when == "auto")   opts.color_mode = ColorMode::Auto;
                else {
                    std::cerr << "Error: --color must be "
                                 "'always', 'never', or 'auto'\n";
                    return 1;
                }
                break;
            }
            case 1001: {
                // --calendar=auto|jalali|gregorian
                const std::string_view cal(optarg);
                if      (cal == "auto")      opts.calendar_mode = CalendarMode::Auto;
                else if (cal == "jalali")    opts.calendar_mode = CalendarMode::Jalali;
                else if (cal == "gregorian") opts.calendar_mode = CalendarMode::Gregorian;
                else {
                    std::cerr << "Error: --calendar must be "
                                 "'auto', 'jalali', or 'gregorian'\n";
                    return 1;
                }
                break;
            }
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
            case 'n': opts.color_mode    = ColorMode::Never; break;
            case 'h': help_requested     = true; break;
            case 'v': std::cout << "jala " << VERSION << '\n'; return 0;
            default:  print_help(argv[0], true); return 1;
        }
    }

    // Resolve the final color decision. The default (Auto) emits colors
    // only when stdout is a terminal and NO_COLOR is not set. Explicit
    // --color=always overrides NO_COLOR; explicit --color=never forces
    // color off regardless of the environment.
    switch (opts.color_mode) {
        case ColorMode::Always:
            opts.color = true;
            break;
        case ColorMode::Never:
            opts.color = false;
            break;
        case ColorMode::Auto:
            opts.color = (isatty(STDOUT_FILENO) != 0) &&
                         (std::getenv("NO_COLOR") == nullptr);
            break;
    }

    if (help_requested) {
        print_help(argv[0], opts.color);
        return 0;
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

    const PersianDate today_date = to_persian(jala::today());
    const int year  = opts.year  != -1 ? opts.year  : today_date.year;
    const int month = opts.month != -1 ? opts.month : today_date.month;

    if (opts.full_year)         print_year(year, opts);
    else if (opts.three_months) print_three_months(year, month, opts);
    else                        print_single(opts, year, month);

    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    // Set the locale for the current environment so that wcwidth()
    // and mbrtowc() interpret UTF-8 correctly. If the environment
    // does not specify a locale, fall back to the C.UTF-8 locale,
    // which is guaranteed to be UTF-8 on modern glibc systems.
    if (std::setlocale(LC_ALL, "") == nullptr) {
        std::setlocale(LC_ALL, "C.UTF-8");
    }

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