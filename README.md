# jala

> Persian (Jalali) calendar in the terminal

[![CI](https://github.com/mahdymorady/jala/actions/workflows/ci.yml/badge.svg)](https://github.com/mahdymorady/jala/actions/workflows/ci.yml)

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

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

## Features

- Single month, three months, or full year — side-by-side or stacked
- Persian digits and names (`-p`) with proper RTL handling
- Day-of-year mode (`-j`)
- Imperial (Shahanshahi) year (`-P`) — Jalali + 1180
- English weekday abbreviations (`-e`)
- Date conversion (`-c`) — Jalali ↔ Gregorian
- Date difference (`-d`)
- jdate mode (`-t [+FORMAT]`) with custom format strings
- ANSI colors + `NO_COLOR` support

## Installation

Dependencies: C++17 compiler, Boost.Date_Time.

Debian/Ubuntu:

    sudo apt install build-essential libboost-all-dev

Build and install:

    git clone https://github.com/YOUR_USERNAME/jala.git
    cd jala
    make
    sudo make install

### Pre-built binaries

**Debian / Ubuntu (x86_64):**

```bash
wget https://github.com/mahdymorady/jala/releases/download/v1.0.1/jala_1.0.0-1_amd64.deb
sudo dpkg -i jala_1.0.0-1_amd64.deb
```

**Other Linux distributions (x86_64):**

```bash
wget https://github.com/mahdymorady/jala/releases/download/v1.0.1/jala-v1.0.0-linux-x86_64.tar.gz
tar xzf jala-v1.0.0-linux-x86_64.tar.gz
sudo install -m 755 jala-v1.0.0-linux-x86_64 /usr/local/bin/jala
```

## Usage

    jala [OPTIONS] [MONTH] [YEAR]
    jala -c <DATE>
    jala -d <DATE1> <DATE2>
    jala -t [+FORMAT]

See `man jala` for the full manual.

## Options

| Flag | Description |
| :--- | :--- |
| `-y` | Full year (3x4 grid) |
| `-3` | Three months side by side |
| `-s` | Stacked (single column) |
| `-p` | Persian digits and names |
| `-e` | English weekday abbreviations |
| `-j` | Day-of-year |
| `-P` | Imperial (Shahanshahi) year |
| `-c, --convert <date>` | Convert Jalali and Gregorian |
| `-d, --diff <d1> <d2>` | Days between two dates |
| `-t, --today [+FORMAT]` | Current date and time |
| `-n` | No color |
| `-h` | Help |
| `-v` | Version |

## Examples

    jala                        # current month
    jala 7 1405                 # Mehr 1405
    jala -3 7 1405              # three months side by side
    jala -ys 1405               # full year, stacked
    jala -p                     # Persian
    jala -e                     # English weekday names
    jala -j 7 1405              # day-of-year
    jala -P 7 1405              # Imperial year
    jala -t                     # now
    jala -tp '+%A %d %B %Y'     # Persian formatted
    jala -c 1405/07/12          # Jalali to Gregorian
    jala -d 1405/07/12 today    # days since

## Format specifiers for -t

`%Y %y %m %d %B %b %A %a %H %M %S %% %n %t`

## Testing

    make test

## License

MIT

## Acknowledgements

- Inspired by [jcal / jdate](https://github.com/ashkang/jcal)
- Calendar math based on the standard JDN algorithm

