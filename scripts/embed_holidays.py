#!/usr/bin/env -S uv run --script
# SPDX-License-Identifier: MIT
# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///
"""
embed_holidays.py — convert data/holidays.json into C++ headers.

The source data is the JSON calendar published by
  https://github.com/hasan-ahani/shamsi-holidays  (MIT licensed)

Its shape is a flat array of day objects:

    [
      {
        "date": "1405-01-01",
        "events": [
          {"description": "...", "is_holiday": true},
          ...
        ],
        "is_holiday": true
      },
      ...
    ]

We emit two sorted tables into src/holidays_data.hpp:

  HOLIDAYS[] — one entry per holiday day. When multiple holiday
               events share a day, their descriptions are joined
               with the Persian comma "،".
  EVENTS[]   — one entry per event (holiday or not), used by the
               -E/--events flag to list everything happening on a
               given day.

Run:
    python3 scripts/embed_holidays.py
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------

REPO_ROOT  = Path(__file__).resolve().parent.parent
INPUT_JSON = REPO_ROOT / "data" / "holidays.json"
OUTPUT_HPP = REPO_ROOT / "src" / "holidays_data.hpp"

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

# Persian comma (U+060C) used to join multiple holiday names.
SEP = "، "

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def normalise_date(raw: str) -> str:
    """Convert '1405-01-01' to '1405/01/01'."""
    return raw.replace("-", "/")


def validate_date(date: str) -> None:
    parts = date.split("/")
    if len(parts) != 3 or not all(p.isdigit() for p in parts):
        raise ValueError(f"invalid date: {date!r}")
    y, m, d = (int(p) for p in parts)
    if not (1 <= y <= 9999 and 1 <= m <= 12 and 1 <= d <= 31):
        raise ValueError(f"date out of range: {date!r}")


def escape_cpp(s: str) -> str:
    out = []
    for ch in s:
        if ch == "\\":  out.append("\\\\")
        elif ch == '"': out.append('\\"')
        elif ch == "\n": out.append("\\n")
        elif ch == "\t": out.append("\\t")
        else:            out.append(ch)
    return "".join(out)


# ---------------------------------------------------------------------------
# Parsing
# ---------------------------------------------------------------------------

def load_raw(path: Path) -> list[dict]:
    with path.open(encoding="utf-8") as f:
        data = json.load(f)
    if not isinstance(data, list):
        raise ValueError("expected a JSON array at the top level")
    return data


def collect(entries: list[dict]) -> tuple[dict[str, str], list[tuple[str, str, bool]]]:
    """
    Return:
      holidays — {date: "name1، name2"} for days with is_holiday=true
      events   — [(date, description, is_holiday), ...] for every event
    """
    holidays: dict[str, str] = {}
    events:   list[tuple[str, str, bool]] = []

    for day in entries:
        date = normalise_date(day.get("date", ""))
        if not date:
            continue
        validate_date(date)

        is_holiday_day = bool(day.get("is_holiday"))
        holiday_names: list[str] = []

        for ev in day.get("events", []):
            desc = (ev.get("description") or "").strip()
            if not desc:
                continue
            is_holiday = bool(ev.get("is_holiday"))
            events.append((date, desc, is_holiday))
            if is_holiday:
                holiday_names.append(desc)

        if is_holiday_day and holiday_names:
            holidays[date] = SEP.join(holiday_names)

    return holidays, events


# ---------------------------------------------------------------------------
# Header generation
# ---------------------------------------------------------------------------

def generate(holidays: dict[str, str],
             events: list[tuple[str, str, bool]]) -> str:
    L: list[str] = []
    L.append("// =============================================================================")
    L.append("// holidays_data.hpp — auto-generated. DO NOT EDIT MANUALLY.")
    L.append("// Regenerate with: python3 scripts/embed_holidays.py")
    L.append("// Source: data/holidays.json")
    L.append("// Upstream: https://github.com/hasan-ahani/shamsi-holidays (MIT)")
    L.append("// =============================================================================")
    L.append("")
    L.append("#pragma once")
    L.append("")
    L.append("namespace jala {")
    L.append("")
    L.append("// One holiday day (used for colouring).")
    L.append("struct HolidayEntry {")
    L.append("    const char* date;  // YYYY/MM/DD (Jalali)")
    L.append("    const char* name;  // UTF-8, may contain multiple names joined by '،'")
    L.append("};")
    L.append("")
    L.append("// One individual event on a given day (used by -E).")
    L.append("struct EventEntry {")
    L.append("    const char* date;        // YYYY/MM/DD (Jalali)")
    L.append("    const char* description; // UTF-8")
    L.append("    bool        is_holiday;  // true if the event is an official holiday")
    L.append("};")
    L.append("")

    # --- HOLIDAYS ---
    L.append(f"inline constexpr int HOLIDAYS_COUNT = {len(holidays)};")
    L.append("")
    L.append("inline constexpr HolidayEntry HOLIDAYS[] = {")
    for date in sorted(holidays):
        L.append(f'    {{"{escape_cpp(date)}", "{escape_cpp(holidays[date])}"}},')
    L.append("};")
    L.append("")

    # --- EVENTS ---
    L.append(f"inline constexpr int EVENTS_COUNT = {len(events)};")
    L.append("")
    L.append("inline constexpr EventEntry EVENTS[] = {")
    for date, desc, is_h in sorted(events, key=lambda x: (x[0], x[1])):
        flag = "true" if is_h else "false"
        L.append(f'    {{"{escape_cpp(date)}", "{escape_cpp(desc)}", {flag}}},')
    L.append("};")
    L.append("")
    L.append("}  // namespace jala")
    L.append("")
    return "\n".join(L)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> int:
    if not INPUT_JSON.exists():
        print(f"error: {INPUT_JSON} not found", file=sys.stderr)
        return 1

    entries = load_raw(INPUT_JSON)
    holidays, events = collect(entries)

    if not holidays and not events:
        print("warning: no data parsed", file=sys.stderr)
        return 1

    OUTPUT_HPP.write_text(generate(holidays, events), encoding="utf-8")
    print(f"wrote {OUTPUT_HPP}")
    print(f"  {len(holidays)} holiday days")
    print(f"  {len(events)} total events")
    return 0


if __name__ == "__main__":
    sys.exit(main())