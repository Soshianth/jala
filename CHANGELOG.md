# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

[Unreleased]: https://github.com/YOUR_USERNAME/jala/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/YOUR_USERNAME/jala/releases/tag/v1.0.0
