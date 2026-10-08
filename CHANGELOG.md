# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.4.0] - 2026-10-08

### Added

- `--calendar=auto|jalali|gregorian` for the `-c` and `-d` subcommands.
  The default, `auto`, keeps the existing heuristic (year below 1700
  is Jalali, otherwise Gregorian). `jalali` and `gregorian` force the
  interpretation, which makes it possible to enter a Jalali year above
  1699 (for example `jala -c 3177/01/01 --calendar=jalali`) and to
  read a year below 1700 as Gregorian.
- Reference tests for jalaali-js and the official Iranian calendar,
  covering Nowruz in 1394, 1395, 1403, and 1404, and leap-year
  boundaries in 1394/12/29, 1403/12/30, and 1404/12/29.

### Changed

- Replaced the 33-year leap-year cycle with the jalaali-js algorithm
  (Borkowski 1996). The new algorithm matches the official Iranian
  calendar for the entire modern era and remains valid up to year
  3177, which is now enforced as `MAX_YEAR`. It shares the same
  public interface as before; only `src/jalali.cpp` and the
  arithmetic inside `src/jalali.hpp` changed.
- Verified that the `jal_cal()` era table and `persian_month_days()`
  agree on month lengths across the years 1200–2000 in the unit
  tests.

### Fixed

- `src/main.cpp` now reports the correct version string. In v1.3.0
  the CMake project version was bumped to 1.3.0 but the `VERSION`
  constant in `main.cpp` was left at 1.2.0, so `jala -v` and the
  first line of `jala -h` printed the wrong number.

[1.4.0]: https://github.com/Soshianth/jala/compare/v1.3.0...v1.4.0

## [1.3.0] - 2026-10-08

### Added

- `src/gregorian.{hpp,cpp}`: a small, self-contained Gregorian
  calendar implementation based on the Fliegel–Van Flandern JDN
  formulas. It replaces the Boost.Date_Time dependency.
- `scripts/neutralize_overrides.py` and `data/manual_overrides.json`:
  an automatic pipeline that rewrites holiday descriptions to remove
  religious framing, plus a hand-edited exception list that takes
  precedence over the automatic rules.
- `tests/secularization_test.sh`: asserts that the generated holiday
  header contains no forbidden markers.
- A `CHECK_EQ` macro in `tests/jalali_test.cpp` that prints both
  operands on failure, making off-by-one bugs easier to diagnose.
- Wide-range round-trip tests for both the Jalali and the Gregorian
  calendars, covering leap years and century boundaries.
- A `make holidays` target that regenerates the derived data files
  on demand.

### Changed

- Removed the Boost.Date_Time dependency. `jala` now builds with
  only a C++17 compiler and Python 3.9+ for the data generation
  step.
- `src/holidays_data.hpp` and `data/overrides.json` are no longer
  tracked in version control. They are produced by the Python
  scripts in `scripts/` and regenerated automatically by the
  Makefile whenever the JSON source or a generator changes.
- `debian/control`: dropped `libboost-date-time-dev`, added
  `python3` to `Build-Depends`, bumped `Standards-Version` to
  `4.7.2`, and updated the Maintainer to Mahdi Moradi.

### Removed

- The `boost::gregorian::date`-based overloads of `to_persian` and
  `jdn_to_gregorian`. The new signatures use the local
  `GregorianDate` struct.

## [1.2.0] - 2026-10-07

### Added

- `--color=WHEN` (`always`, `never`, `auto`). The default, `auto`,
  emits ANSI colors only when stdout is a terminal and `NO_COLOR`
  is unset. `always` overrides `NO_COLOR`, and `never` disables
  color unconditionally. The existing `-n` / `--nocolor` flag now
  behaves as `--color=never`.
- `persian_is_leap()` as the single source of truth for leap
  years, documented in the public header.
- Unit tests covering leap-year consistency between
  `persian_month_days()` and `persian_to_jdn()`, strict input
  validation in `parse_date()`, and `EventIndex` lookups.

### Changed

- The calendar engine now uses the 33-year cycle exclusively.
  Previously `persian_to_jdn()` and `jdn_to_persian()` used the
  2820-year Birashk cycle while `persian_month_days()` used the
  33-year cycle, and the two disagreed about leap years in some
  years (e.g. Esfand 1403, 1404, 1436, 1437, 1469, 1470). All
  conversion and arithmetic now agree, and the results match the
  official Iranian calendar for the modern era.
- `parse_date()` validates its input strictly: each numeric
  component must be a complete integer, the month and day must be
  in range, and Jalali dates must exist in the 33-year cycle.
- `visible_length()` uses `wcwidth()` and `mbrtowc()` instead of
  counting UTF-8 code points, so CJK characters (two columns) and
  combining marks (zero columns) are measured correctly.
  `setlocale(LC_ALL, "")` is called once at startup, with a
  fallback to `C.UTF-8`.
- `debian/rules` preserves the distribution's build flags by
  including `/usr/share/dpkg/default.mk` and appending project
  flags to `CXXFLAGS` instead of replacing them.
- `debian/copyright` separates the main source code from the
  holiday dataset, which is derived from
  hasan-ahani/shamsi-holidays (MIT, Copyright Hassan Ahani).

### Fixed

- `parse_date()` no longer accepts malformed input such as
  `1405/07/12xyz`, `1405/13/01`, `1405/12/30` in a non-leap year,
  or `2026-02-30`.
- `--no-bidi` in combination with `-p` no longer reverses the
  visual order of the weekday header. The cells are emitted in
  reverse order so that the terminal's own right-to-left rendering
  restores the intended left-to-right sequence.
- Exceptions thrown by `boost::gregorian::date` are caught in
  `cmd_convert()` and `cmd_diff()` and reported as one-line error
  messages instead of terminating the process. A top-level
  `try/catch` in `main()` reports any remaining exception.
- The obsolete monolithic `jala.cpp` at the repository root has
  been removed; the sources now live under `src/`.

[1.3.0]: https://github.com/Soshianth/jala/compare/v1.2.0...v1.3.0
[1.2.0]: https://github.com/Soshianth/jala/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/Soshianth/jala/compare/v1.0.1...v1.1.0
[1.0.1]: https://github.com/Soshianth/jala/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/Soshianth/jala/releases/tag/v1.0.0


## [1.1.0] - 2026-10-06

### Added

- `-E`, `--events`: after the calendar, print every event recorded
  for the displayed range. Holidays appear in red, other events in
  cyan. Composes with `-p` for Persian digits and `-n` to disable
  color.
- `-H`, `--no-holidays`: disable highlighting of Iranian official
  holidays.
- Holiday and event data sourced from
  [hasan-ahani/shamsi-holidays](https://github.com/hasan-ahani/shamsi-holidays)
  (MIT License) and embedded into the binary at build time.
- `scripts/embed_holidays.py` regenerates two C++ tables,
  `HOLIDAYS[]` and `EVENTS[]`, from `data/holidays.json`.

### Changed

- Python tooling is managed with
  [uv](https://github.com/astral-sh/uv);
  `pyproject.toml`, `uv.lock`, and `.python-version` are now tracked.
- The repository moved from `mahdymorady/jala` to `Soshianth/jala`.
- Unit-test target now links against `src/holidays.cpp`.

## [1.0.1] - 2026-10-05

### Added

- `-B`, `--no-bidi`: skip the LRM wrapping of Persian weekday cells.
  Useful on terminals that render LRM as a visible control character
  or that mishandle bidi controls. Without this flag, the previous
  behavior is preserved.
- C++ unit tests in `tests/jalali_test.cpp`, exercising the calendar
  math directly (round-trip conversion, weekday, month lengths,
  day-of-year, parsing, digit translation).
- New `make test-unit` target that builds and runs the unit tests.
- Separate CMake build job in CI, running CTest independently of
  the Makefile path.

### Changed

- Sources moved into `src/` and split into three files:
  - `src/jalali.hpp`: public interface of the calendar library.
  - `src/jalali.cpp`: pure calendar math, no I/O.
  - `src/main.cpp`: CLI, rendering, and dispatch.
- Makefile now compiles each source to an object file and links
  them together, with `jalali.hpp` listed as a dependency.
- CMake enables policy CMP0167 and uses `find_package(Boost CONFIG)`
  so the build is clean on CMake 3.30+ where FindBoost was removed.
- CI matrix now also runs clang++ alongside g++.

### Fixed

- `%b` (short month) in Persian mode no longer truncates the
  multibyte UTF-8 name mid-character. It falls back to the full
  Persian name.

## [1.0.0] - 2026-10-04

First stable release.

### Added

- Calendar rendering for a single month, three consecutive months,
  and a full year
- `-3` and `-y` layouts render months side by side in a 3-column
  grid, with `-s` for stacked (single-column) output
- Persian digits and month/weekday names (`-p`) with proper RTL
  isolation via Unicode LRM
- Day-of-year mode (`-j`)
- Imperial (Shahanshahi) year (`-P`) — Jalali + 1180, labeled `(pa)`
- English weekday abbreviations (`-e`)
- Date conversion between Jalali and Gregorian (`-c`) with
  auto-detection based on the year
- Date difference in days and weeks (`-d`)
- `jdate` mode with custom format strings (`-t [+FORMAT]`),
  supporting `%Y %y %m %d %B %b %A %a %H %M %S %% %n %t`
- ANSI colors: blue weekday header, yellow Fridays, reverse-video
  today
- `NO_COLOR` environment variable support
- Portable `Makefile` with `all`, `debug`, `install`, `uninstall`,
  `test`, and `clean` targets, with `DESTDIR` support
- Complete `jala(1)` roff man page
- Shell-based test suite (`tests/run_tests.sh`) with 24 assertions
- VS Code task definitions for build, run, and test

### Changed

- Renamed the project from the placeholder `cal-fa` to `jala`

### Fixed

- Off-by-one error in the JDN-to-Jalali conversion that shifted every
  date one day forward
- Incorrect weekday offset that placed today on the wrong column
- `-t` argument parsing that consumed the next flag character as its
  format string (e.g. `-tp` used to parse as `-t` with `p`)
- `-y` with a single numeric argument now correctly treats it as the
  year rather than the month