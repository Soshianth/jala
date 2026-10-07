#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""
prepare_release.py — bump jala's version strings and update the
changelogs for a new release.

Usage:
    python3 scripts/prepare_release.py 1.2.0

Updates:
    src/main.cpp        VERSION = "X.Y.Z"
    CMakeLists.txt      VERSION X.Y.Z
    jala.1              "jala X.Y.Z"
    CHANGELOG.md        new section + [Unreleased] link
    debian/changelog    new entry

The script is idempotent: running it twice with the same version
number does not duplicate any content.
"""

from __future__ import annotations

import re
import sys
from datetime import date
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def write(path: Path, text: str) -> None:
    path.write_text(text, encoding="utf-8")


def replace_once(path: Path, old: str, new: str) -> bool:
    """Replace the first occurrence of `old` with `new`.

    Returns True if a change was made, False otherwise.
    """
    text = read(path)
    if new in text:
        return False        # already updated
    if old not in text:
        print(f"  WARN: pattern not found in {path.name}: {old!r}",
              file=sys.stderr)
        return False
    write(path, text.replace(old, new, 1))
    return True


def insert_before(path: Path, marker: str, block: str) -> bool:
    """Insert `block` immediately before the first occurrence of
    `marker` in `path`. Returns True if a change was made."""
    text = read(path)
    if block.strip() in text:
        return False        # already inserted
    idx = text.find(marker)
    if idx == -1:
        print(f"  WARN: marker not found in {path.name}: {marker!r}",
              file=sys.stderr)
        return False
    write(path, text[:idx] + block + text[idx:])
    return True


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: prepare_release.py X.Y.Z", file=sys.stderr)
        return 1

    new_version = sys.argv[1]

    # Validate the version format.
    if not re.fullmatch(r"\d+\.\d+\.\d+", new_version):
        print(f"Error: invalid version {new_version!r}", file=sys.stderr)
        return 1

    # Detect the current version from src/main.cpp.
    main_cpp = REPO_ROOT / "src" / "main.cpp"
    m = re.search(r'VERSION = "([0-9.]+)"', read(main_cpp))
    if not m:
        print("Error: could not read current version from src/main.cpp",
              file=sys.stderr)
        return 1
    old_version = m.group(1)

    if old_version == new_version:
        print(f"Already at {new_version}; nothing to do.")
        return 0

    today = date.today().isoformat()

    print(f"Bumping {old_version} -> {new_version}")
    print()

    # ---- src/main.cpp ----
    replace_once(main_cpp,
                 f'VERSION = "{old_version}"',
                 f'VERSION = "{new_version}"')
    print(f"  ok: src/main.cpp")

    # ---- CMakeLists.txt ----
    replace_once(REPO_ROOT / "CMakeLists.txt",
                 f"VERSION {old_version}",
                 f"VERSION {new_version}")
    print(f"  ok: CMakeLists.txt")

    # ---- jala.1 ----
    replace_once(REPO_ROOT / "jala.1",
                 f'"jala {old_version}"',
                 f'"jala {new_version}"')
    print(f"  ok: jala.1")

    # ---- CHANGELOG.md ----
    changelog = REPO_ROOT / "CHANGELOG.md"

    # Build the new section. See the project's history for the format.
    new_section = f"""## [{new_version}] - {today}

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

[{new_version}]: https://github.com/Soshianth/jala/compare/v{old_version}...v{new_version}

"""

    # Insert before the previous version's section.
    marker = f"## [{old_version}]"
    inserted = insert_before(changelog, marker, new_section)
    print(f"  ok: CHANGELOG.md (section inserted={inserted})")

    # Update the [Unreleased] link.
    replace_once(changelog,
                 f"[Unreleased]: https://github.com/Soshianth/jala/compare/v{old_version}...main",
                 f"[Unreleased]: https://github.com/Soshianth/jala/compare/v{new_version}...main")
    print(f"  ok: CHANGELOG.md ([Unreleased] link)")

    # ---- debian/changelog ----
    debian_changelog = REPO_ROOT / "debian" / "changelog"
    debian_block = f"""jala ({new_version}-1) unstable; urgency=medium

  * New upstream release.
    - Add --color=always|never|auto.
    - Use the 33-year cycle consistently for all conversions.
    - Strict input validation in parse_date().
    - Use wcwidth() for accurate terminal column widths.
    - Fix --no-bidi with -p (weekday header order).
    - Catch Boost exceptions and report clean errors.
    - Preserve Debian hardening flags in debian/rules.
    - Split debian/copyright for the holiday dataset.
    - Remove the obsolete monolithic jala.cpp.

 -- Mahdi Moradi <Soshianth@users.noreply.github.com>  Tue, 07 Oct 2026 10:00:00 +0330

"""
    inserted = insert_before(debian_changelog,
                             f"jala ({old_version}-1)",
                             debian_block)
    print(f"  ok: debian/changelog (entry inserted={inserted})")

    # ---- Verify ----
    print()
    print("Verifying...")
    errors = 0

    def check(path: Path, needle: str, label: str) -> None:
        nonlocal errors
        if needle in read(path):
            print(f"  OK   {label}")
        else:
            print(f"  FAIL {label}: {needle!r} not found")
            errors += 1

    check(main_cpp,               f'VERSION = "{new_version}"', "src/main.cpp")
    check(REPO_ROOT / "CMakeLists.txt",
          f"VERSION {new_version}", "CMakeLists.txt")
    check(REPO_ROOT / "jala.1",   f'"jala {new_version}"',      "jala.1")
    check(changelog,              f"## [{new_version}]",       "CHANGELOG.md section")
    check(changelog,
          f"compare/v{new_version}...main", "[Unreleased] link")
    check(debian_changelog,
          f"jala ({new_version}-1)", "debian/changelog")

    if errors:
        print(f"\n{errors} verification error(s).", file=sys.stderr)
        return 1

    print()
    print(f"Done. Review with: git diff")
    print(f"Then commit with a message such as:")
    print(f'  release: v{new_version}')
    return 0


if __name__ == "__main__":
    sys.exit(main())
