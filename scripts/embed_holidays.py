#!/usr/bin/env python3
"""
embed_holidays.py — convert data/holidays.json into a C++ header.

Reads the JSON source of truth and emits src/holidays_data.hpp with
a static, sorted array of (date, name) pairs. The generated header
is checked in, so the build does not require Python at compile time.

Usage:
    python3 scripts/embed_holidays.py
"""

import json
import sys
from pathlib import Path

# ---- Paths (relative to repository root) ----
REPO_ROOT   = Path(__file__).resolve().parent.parent
INPUT_JSON  = REPO_ROOT / "data" / "holidays.json"
OUTPUT_HPP  = REPO_ROOT / "src" / "holidays_data.hpp"


def load_holidays(path: Path) -> dict[str, str]:
    """Load the JSON file and return the holiday map, ignoring keys
    that start with an underscore (metadata/comments)."""
    with path.open(encoding="utf-8") as f:
        data = json.load(f)

    raw = data.get("holidays", {})
    return {k: v for k, v in raw.items() if not k.startswith("_")}


def validate_date(date: str) -> None:
    """Ensure the date is in YYYY/MM/DD form."""
    parts = date.split("/")
    if len(parts) != 3 or not all(p.isdigit() for p in parts):
        raise ValueError(f"invalid date format: {date!r}")
    y, m, d = (int(p) for p in parts)
    if not (1 <= y <= 9999 and 1 <= m <= 12 and 1 <= d <= 31):
        raise ValueError(f"date out of range: {date!r}")


def escape_cpp_string(s: str) -> str:
    """Escape a string for use inside a C++ string literal."""
    out = []
    for ch in s:
        if ch == "\\": out.append("\\\\")
        elif ch == '"': out.append('\\"')
        elif ch == "\n": out.append("\\n")
        elif ch == "\t": out.append("\\t")
        else: out.append(ch)
    return "".join(out)


def generate_header(holidays: dict[str, str]) -> str:
    """Produce the content of src/holidays_data.hpp."""
    entries = sorted(holidays.items())  # sort by date

    lines = [
        "// =============================================================================",
        "// holidays_data.hpp — auto-generated. DO NOT EDIT MANUALLY.",
        "// Regenerate with: python3 scripts/embed_holidays.py",
        "// Source of truth: data/holidays.json",
        "// =============================================================================",
        "",
        "#pragma once",
        "",
        "namespace jala {",
        "",
        "struct HolidayEntry {",
        "    const char* date;  // YYYY/MM/DD (Jalali)",
        "    const char* name;  // UTF-8 name",
        "};",
        "",
        f"inline constexpr int HOLIDAYS_COUNT = {len(entries)};",
        "",
        "inline constexpr HolidayEntry HOLIDAYS[] = {",
    ]

    for date, name in entries:
        validate_date(date)
        lines.append(f'    {{"{escape_cpp_string(date)}", "{escape_cpp_string(name)}"}},')

    lines += [
        "};",
        "",
        "}  // namespace jala",
        "",
    ]
    return "\n".join(lines)


def main() -> int:
    if not INPUT_JSON.exists():
        print(f"error: {INPUT_JSON} not found", file=sys.stderr)
        return 1

    holidays = load_holidays(INPUT_JSON)
    if not holidays:
        print("warning: no holidays found in the input file", file=sys.stderr)

    content = generate_header(holidays)
    OUTPUT_HPP.write_text(content, encoding="utf-8")
    print(f"wrote {OUTPUT_HPP} ({len(holidays)} holidays)")
    return 0


if __name__ == "__main__":
    sys.exit(main())