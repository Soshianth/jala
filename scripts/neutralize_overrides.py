#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""
neutralize_overrides.py — automatically secularize holiday descriptions.

Reads data/holidays.json (upstream, untouched), applies a set of
secularization rules to every event description, and writes the result
to data/overrides.json. The build script scripts/embed_holidays.py
then applies these overrides on top of the originals, so the compiled
binary never shows religious framing.

Example transformation:

    شهادت امام حسین علیه السلام [ ١٠ محرم ]
        →  درگذشت حسین‌ابن‌علی [ ١٠ محرم ]

Rules are grouped into five phases, applied in this order:

    1. NAMES      — religious title → neutral historical name
    2. VERBS      — religious event verb → neutral verb
    3. ADJECTIVES — religious adjective removed or simplified
    4. HONORIFICS — stripped completely
    5. PUNCT      — spacing around Persian punctuation, collapse spaces

The order matters: names must run before the generic honorific
fallbacks ("امام ", "حضرت "), otherwise those fallbacks would strip
the prefix that a specific name rule would have matched.

Rules are idempotent: running the script twice produces the same
output. overrides.json is fully regenerated on each run; edit the
rule tables below and re-run to change behavior.

For exceptions that the rules cannot express, put them in
data/manual_overrides.json. Its entries win over automatic rules and
are preserved across regenerations of overrides.json.

Run:
    python3 scripts/neutralize_overrides.py
"""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

REPO_ROOT      = Path(__file__).resolve().parent.parent
INPUT_JSON     = REPO_ROOT / "data" / "holidays.json"
OVERRIDES_JSON = REPO_ROOT / "data" / "overrides.json"
MANUAL_JSON    = REPO_ROOT / "data" / "manual_overrides.json"


# =============================================================================
# Phase 1 — proper names (religious title → neutral historical name)
# =============================================================================
# Format: [given]‌ابن‌[father] for men, [given]‌دختر‌[father] for
# women, with an optional nisba in parentheses when two people would
# otherwise share the same nasab:
#   حسن‌ابن‌علی (مجتبی)   vs   حسن‌ابن‌علی (عسکری)
# All names use ZWNJ (U+200C) as the separator between components.

NAMES: list[tuple[str, str]] = [
    # Fourteen Infallibles and close relatives.
    ("امام علی النقی الهادی",  "علی‌ابن‌محمد (هادی)"),
    ("امام محمد باقر",          "محمد‌ابن‌علی (باقر)"),
    ("امام محمد تقی",           "محمد‌ابن‌علی (تقی)"),
    ("امام حسن عسکری",          "حسن‌ابن‌علی (عسکری)"),
    ("امام حسن مجتبی",          "حسن‌ابن‌علی (مجتبی)"),
    ("امام زین العابدین",       "علی‌ابن‌حسین (سجاد)"),
    ("امام جعفر صادق",          "جعفر‌ابن‌محمد"),
    ("امام موسی کاظم",          "موسی‌ابن‌جعفر"),
    ("امام رضا",                "علی‌ابن‌موسی"),
    ("امام حسین",               "حسین‌ابن‌علی"),
    ("امام علی",                "علی‌ابن‌ابی‌طالب"),
    ("سالار شهیدان، امام حسین", "حسین‌ابن‌علی"),
    ("سالار شهیدان",            "حسین‌ابن‌علی"),
    ("ابوالفضل العباس",         "عباس‌ابن‌علی"),
    ("علی اکبر",                "علی‌اکبر‌ابن‌حسین"),
    ("حضرت فاطمه زهرا",         "فاطمه‌دختر‌محمد"),
    ("حضرت زینب",               "زینب‌دختر‌علی"),
    ("حضرت معصومه",             "فاطمه‌دختر‌موسی"),
    ("حضرت قائم",               "مهدی‌ابن‌حسن"),
    ("رسول اکرم",               "محمد‌ابن‌عبدالله"),
    ("پیامبر اکرم",             "محمد‌ابن‌عبدالله"),
    ("پیامبر اسلام",            "محمد‌ابن‌عبدالله"),
    # Political-religious figures with religious titles.
    ("حضرت امام خمینی",         "روح‌الله خمینی"),
    ("امام خمینی",              "روح‌الله خمینی"),
    # Common observance names with a religious suffix.
    ("تاسوعای حسینی",           "تاسوعا"),
    ("عاشورای حسینی",           "عاشورا"),
    ("اربعین حسینی",            "اربعین"),
]

# Sort longest-first so that "امام زین العابدین" is matched before
# the generic fallbacks in HONORIFICS ("امام ", "حضرت "). Must be
# defined here — before apply_rules() is ever called.
_NAMES_SORTED: list[tuple[str, str]] = sorted(
    NAMES, key=lambda kv: len(kv[0]), reverse=True
)


# =============================================================================
# Phase 2 — verbs (religious framing → neutral framing)
# =============================================================================

VERBS: list[tuple[str, str]] = [
    ("شهادت ",      "درگذشت "),
    ("ولادت ",      "زادروز "),
    ("میلاد ",      "زادروز "),
    ("رحلت ",       "درگذشت "),
    ("وفات ",       "درگذشت "),
    ("ضربت خوردن ", "ترور "),
]


# =============================================================================
# Phase 3 — adjectives (removed or simplified)
# =============================================================================

ADJECTIVES: list[tuple[str, str]] = [
    ("عید سعید",  "عید"),
    ("عید مبارک", "عید"),
    ("با سعادت",  ""),
    ("مقدس",      ""),
    ("مبارک",     ""),
]


# =============================================================================
# Phase 4 — honorifics (removed completely)
# =============================================================================
# Each entry includes its leading space so that removing it does not
# leave a stray space behind. Rules are plain substrings.

HONORIFICS: list[str] = [
    # Shia honorifics after a name
    " علیه السلام",
    " علیه‌السلام",
    " سلام الله علیها",
    " سلام الله علیه",
    " سلام‌الله‌علیها",
    " سلام‌الله‌علیه",
    " عجل الله تعالی فرجه",
    " عجل‌الله‌تعالی‌فرجه",
    " رضی الله عنه",
    " رحمه الله",
    # Abbreviations in parentheses
    " (ص)",
    " (ع)",
    " (س)",
    " (ره)",
    " (قدس سره)",
    # Titles of respect
    "حاج ",     # precedes a proper name
    # Fallbacks (applied last; specific names already replaced)
    "حضرت ",
    "امام ",
]


# =============================================================================
# Phase 5 — punctuation and whitespace
# =============================================================================

# Add a space after Persian punctuation if there isn't one. Repeated
# whitespace is collapsed by _MULTI_SPACE, so this is safe to apply
# unconditionally.
PUNCT_FIXES: list[tuple[str, str]] = [
    ("،", "، "),
    ("؛", "؛ "),
]

_MULTI_SPACE = re.compile(r" {2,}")


# =============================================================================
# Rule application
# =============================================================================

def apply_rules(text: str) -> str:
    # ORDER MATTERS. Names must run before the generic honorific
    # fallbacks ("امام ", "حضرت "); otherwise those fallbacks strip the
    # prefix that a specific name rule would have matched. Example:
    #
    #   "امام زین العابدین" --(fallback "امام ")--> "زین العابدین"  ✗
    #
    # With names first, the specific rule fires and produces
    # "علی‌ابن‌حسین (سجاد)"; the honorific phase then only removes
    # genuine suffixes such as " علیه السلام".
    for old, new in _NAMES_SORTED:
        text = text.replace(old, new)

    for old, new in VERBS:
        text = text.replace(old, new)

    for old, new in ADJECTIVES:
        text = text.replace(old, new)

    for honorific in HONORIFICS:
        text = text.replace(honorific, "")

    for old, new in PUNCT_FIXES:
        text = text.replace(old, new)

    text = _MULTI_SPACE.sub(" ", text)
    return text.strip()


# =============================================================================
# I/O
# =============================================================================

def load_holidays() -> list[dict]:
    with INPUT_JSON.open(encoding="utf-8") as f:
        return json.load(f)


def load_manual() -> dict[str, str]:
    if not MANUAL_JSON.exists():
        return {}
    try:
        with MANUAL_JSON.open(encoding="utf-8") as f:
            data = json.load(f)
        return {k: v for k, v in data.items() if not k.startswith("_")}
    except (json.JSONDecodeError, OSError) as exc:
        print(f"warning: could not read {MANUAL_JSON}: {exc}",
              file=sys.stderr)
        return {}


def collect_descriptions(entries: list[dict]) -> set[str]:
    descriptions: set[str] = set()
    for day in entries:
        for ev in day.get("events", []):
            desc = (ev.get("description") or "").strip()
            if desc:
                descriptions.add(desc)
    return descriptions


# =============================================================================
# Sanity check
# =============================================================================

RESIDUAL_MARKERS: list[str] = [
    "علیه السلام", "سلام الله", "عجل الله",
    "امام ", "حضرت ", "رسول اکرم", "پیامبر اکرم",
    "شهادت ", "ولادت ", "رحلت ", "وفات ",
    "عید سعید",
]


def check_residuals(result: dict[str, str]) -> None:
    """Warn if any output still contains a religious marker."""
    problems: list[tuple[str, str]] = []
    for key, value in result.items():
        for marker in RESIDUAL_MARKERS:
            if marker in value:
                problems.append((key, marker))
                break
    if not problems:
        return
    print(f"warning: {len(problems)} entr(ies) still contain a "
          f"religious marker:", file=sys.stderr)
    for key, marker in problems[:10]:
        print(f"  [{marker}] {key[:70]}", file=sys.stderr)
    if len(problems) > 10:
        print(f"  ... and {len(problems) - 10} more", file=sys.stderr)


# =============================================================================
# Entry point
# =============================================================================

def main() -> int:
    if not INPUT_JSON.exists():
        print(f"error: {INPUT_JSON} not found", file=sys.stderr)
        return 1

    entries      = load_holidays()
    descriptions = collect_descriptions(entries)
    manual       = load_manual()

    result: dict[str, str] = {}
    changed = 0

    for desc in sorted(descriptions):
        neutral = apply_rules(desc)
        if neutral != desc:
            changed += 1
        if desc in manual:
            neutral = manual[desc]  # manual wins
        result[desc] = neutral

    # Keep any manual entry whose key is no longer in upstream data,
    # so users do not lose edits when the dataset changes.
    dead_manual = [k for k in manual if k not in result]
    for key in dead_manual:
        result[key] = manual[key]

    payload = {
        "_comment": (
            "Auto-generated by scripts/neutralize_overrides.py — do not "
            "edit this file by hand. For custom exceptions, use "
            "data/manual_overrides.json instead; that file is never "
            "overwritten by any script."
        ),
        **result,
    }

    OVERRIDES_JSON.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )

    print(f"wrote {OVERRIDES_JSON}")
    print(f"  {len(result)} entries"
          f" ({changed} changed by rules,"
          f" {len(manual)} from manual,"
          f" {len(dead_manual)} orphan manual)")

    check_residuals(result)
    return 0


if __name__ == "__main__":
    sys.exit(main())