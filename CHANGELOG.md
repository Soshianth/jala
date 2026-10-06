# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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

[1.0.1]: https://github.com/Soshianth/jala/compare/v1.0.0...v1.0.1


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

[Unreleased]: https://github.com/Soshianth/jala/compare/v1.1.0...main
[1.1.0]: https://github.com/Soshianth/jala/compare/v1.0.1...v1.1.0
[1.0.0]: https://github.com/Soshianth/jala/releases/tag/v1.0.0
