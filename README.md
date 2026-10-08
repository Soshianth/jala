# jala

> Persian (Jalali) calendar in the terminal

[![CI](https://github.com/Soshianth/jala/actions/workflows/ci.yml/badge.svg)](https://github.com/Soshianth/jala/actions/workflows/ci.yml)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**jala** is a small, fast command-line calendar for the terminal that
renders the Persian (Jalali) calendar. It is written in C++17 and
ships as a single static binary with no runtime dependencies beyond
the C++ standard library.

## Features

- Single month, three months side by side, or full year (grid or stacked)
- Persian digits and month/weekday names (`-p`) with proper RTL
  handling via Unicode LRM
- Iranian official holidays highlighted automatically — data is
  embedded at build time, no network access required
- Full event listing for the displayed range (`-E`)
- Day-of-year mode (`-j`)
- Imperial (Shahanshahi) year (`-P`) — Jalali + 1180
- English weekday abbreviations (`-e`)
- Date conversion between Jalali and Gregorian (`-c`)
- Date difference in days and weeks (`-d`)
- `jdate`-style custom formatting (`-t [+FORMAT]`)
- ANSI colors with `--color=WHEN` and `NO_COLOR` support

## Preview

```text
$ jala -p
      مهر ۱۴۰۵
 ش‎ ‎ ی‎ ‎ د‎ ‎ س‎ ‎ چ‎ ‎ پ‎ ‎ ج‎
             ۱  ۲  ۳
 ۴  ۵  ۶  ۷  ۸  ۹ ۱۰
۱۱ ۱۲ ۱۳ ۱۴ ۱۵ ۱۶ ۱۷
۱۸ ۱۹ ۲۰ ۲۱ ۲۲ ۲۳ ۲۴
۲۵ ۲۶ ۲۷ ۲۸ ۲۹ ۳۰

$ jala -3 7 1405
Shahrivar 1405           Mehr 1405           Aban 1405
Sh Ye Do Se Ch Pa Jo   Sh Ye Do Se Ch Pa Jo   Sh Ye Do Se Ch Pa Jo
 1  2  3  4  5  6  7                 1  2  3     1  2  3  4  5  6  7
 8  9 10 11 12 13 14     4  5  6  7  8  9 10     8  9 10 11 12 13 14
15 16 17 18 19 20 21    11 12 13 14 15 16 17    15 16 17 18 19 20 21
22 23 24 25 26 27 28    18 19 20 21 22 23 24    22 23 24 25 26 27 28
29 30 31                25 26 27 28 29 30       29 30

$ jala -t
Monday 13 Mehr 1405  18:26:16

$ jala -c 1405/07/12
Jalali:    1405/07/12  (Sunday)
Gregorian: 2026-10-04
```

## Installation

### Requirements

- A C++17 compiler (GCC 8+ or Clang 6+)
- Python 3.9+ (for the build-time data generation step)

### Debian / Ubuntu

```bash
sudo apt install build-essential python3
```

### From source

```bash
git clone https://github.com/Soshianth/jala.git
cd jala
make
sudo make install
```

### Pre-built binaries

Debian / Ubuntu (x86_64):

```bash
wget https://github.com/Soshianth/jala/releases/download/v1.4.0/jala_1.4.0-1_amd64.deb
sudo dpkg -i jala_1.3.0-1_amd64.deb
```

Other Linux distributions (x86_64):

```bash
wget https://github.com/Soshianth/jala/releases/download/v1.4.0/jala-v1.4.0-linux-x86_64.tar.gz
tar xzf jala-v1.4.0-linux-x86_64.tar.gz
sudo install -m 755 jala-v1.4.0-linux-x86_64 /usr/local/bin/jala
```

## Usage

```text
jala [OPTIONS] [MONTH] [YEAR]
jala -c <DATE>
jala -d <DATE1> <DATE2>
jala -t [+FORMAT]
```

See `man jala` for the complete manual.

## Options

| Flag | Long form | Description |
| :--- | :--- | :--- |
| `-y` | | Full year (3x4 grid) |
| `-3` | | Three months side by side |
| `-s` | | Stacked (single column) |
| `-p` | | Persian digits and names |
| `-e` | | English weekday abbreviations |
| `-j` | | Day-of-year |
| `-P` | | Imperial (Shahanshahi) year |
| `-c` | `--convert` | Convert Jalali and Gregorian |
| `-d` | `--diff` | Days between two dates |
| `-t` | `--today` | Current date and time (optional format) |
| `-B` | `--no-bidi` | Do not wrap Persian weekday cells in LRM |
| `-H` | `--no-holidays` | Do not highlight Iranian holidays |
| `-E` | `--events` | List all events after the calendar |
| | `--color=WHEN` | Colorize output: `always`, `never`, or `auto` |
| | `--calendar=WHEN` | Interpret `-c` and `-d` dates: `auto`, `jalali`, `gregorian` |
| `-n` | | No color (same as `--color=never`) |
| `-h` | `--help` | Show help |
| `-v` | `--version` | Show version |

## Examples

```bash
jala                      # current month
jala 7 1405               # Mehr 1405
jala -3 7 1405            # three months side by side
jala -ys 1405             # full year, stacked
jala -p                   # Persian
jala -e                   # English weekday names
jala -j 7 1405            # day-of-year
jala -P 7 1405            # Imperial year
jala -t                   # now
jala -tp '+%A %d %B %Y'   # Persian formatted
jala -E 1 1405            # calendar + events for Farvardin
jala -H 1 1405            # calendar without holiday highlighting
jala -Bp                  # Persian, without LRM wrapping
jala --color=always        # force color even when piped
jala -c 1405/07/12        # Jalali to Gregorian
jala -d 1405/07/12 today  # days since
```

## Format specifiers for `-t`

| Specifier | Meaning |
| :--- | :--- |
| `%Y` | Full year |
| `%y` | Two-digit year |
| `%m` | Month (zero-padded) |
| `%d` | Day (zero-padded) |
| `%B` | Full month name |
| `%b` | Short month name |
| `%A` | Full weekday name |
| `%a` | Short weekday name |
| `%H` `%M` `%S` | Hour, minute, second |
| `%%` | Literal `%` |
| `%n` `%t` | Newline, tab |

## Data and secularization

Holiday descriptions are derived from
[hasan-ahani/shamsi-holidays](https://github.com/hasan-ahani/shamsi-holidays)
(MIT License) and passed through an automatic secularization step
before being embedded into the binary. The transformation:

- replaces religious titles with neutral historical names using the
  Persian nasab form, e.g. `امام حسین` becomes `حسین‌ابن‌علی` and
  `رسول اکرم` becomes `محمد‌ابن‌عبدالله`
- neutralizes event verbs: `شهادت` becomes `درگذشت`, `ولادت` becomes
  `زادروز`
- strips Shia honorifics such as `علیه السلام`, `(ص)`, `حاج`
- removes devotional adjectives: `عید سعید` becomes `عید`

This keeps `jala` usable regardless of belief and suitable for
distribution in `main` of Debian and Ubuntu.

### Pipeline

```text
data/holidays.json             (upstream, verbatim)
       |
       |-- scripts/neutralize_overrides.py  ->  data/overrides.json
       |
       |-- data/manual_overrides.json       (optional, hand-edited)
       |
       v
scripts/embed_holidays.py      ->  src/holidays_data.hpp
```

Both `data/overrides.json` and `src/holidays_data.hpp` are generated
files and are not tracked in version control. They are produced
automatically by the Makefile whenever the JSON source, the manual
overrides file, or a generator script changes.

### Custom wording

For exceptions the rules cannot express, create or edit
`data/manual_overrides.json`. Its entries take precedence over the
automatic rules and are never overwritten. To drop an event entirely,
set its value to the empty string:

```json
{
  "_comment": "Hand-edited exceptions.",
  "original text from holidays.json": "your preferred wording",
  "another original text": ""
}

```

To regenerate the data files manually:

```bash
make holidays
```

To revert to the original wording, delete `data/manual_overrides.json`
and rebuild.

## Testing

```bash
make test        # shell integration tests + secularization checks
make test-unit   # C++ unit tests
```

## Contributing

Bug reports and pull requests are welcome on
[GitHub](https://github.com/Soshianth/jala/issues). Please keep commits
focused and add tests for behavioral changes.

## License

This project is licensed under the MIT License. See
[LICENSE](LICENSE) for the full text.

## Acknowledgements

- Inspired by `jcal` / `jdate`
- Calendar math based on the standard JDN algorithm
- Holiday and event data from
  [hasan-ahani/shamsi-holidays](https://github.com/hasan-ahani/shamsi-holidays)
  (MIT License)
