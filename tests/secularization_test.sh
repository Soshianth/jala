#!/usr/bin/env bash
# =============================================================================
# secularization_test.sh — assert that holidays_data.hpp contains no
# religious framing. Fails if any forbidden marker is found.
#
# Run standalone:  bash tests/secularization_test.sh
# Or via ctest:    ctest -R secularization
# =============================================================================
set -u

# Locate the header relative to this script, not the current working
# directory. This way the script works whether it is invoked from the
# project root, from the build directory, or through CTest.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

HEADER="${JALA_HEADER:-$REPO_ROOT/src/holidays_data.hpp}"

if [ ! -f "$HEADER" ]; then
    echo "error: $HEADER not found" >&2
    exit 2
fi

GREEN='\033[32m'
RED='\033[31m'
RESET='\033[0m'

PASS=0
FAIL=0

# ---------------------------------------------------------------------------
# Forbidden markers — if any of these appear, the test fails.
# Each entry is a fixed substring (grep -F) searched in the header.
# ---------------------------------------------------------------------------
FORBIDDEN=(
    # Honorifics
    "علیه السلام"
    "علیه‌السلام"
    "سلام الله"
    "عجل الله"
    "رضی الله"
    "رحمه الله"
    "(ص)"
    "(ره)"
    # Titles
    "امام "
    "حضرت "
    "رسول اکرم"
    "پیامبر اکرم"
    "پیامبر اسلام"
    # Verbs
    "شهادت "
    "ولادت "
    "میلاد "
    "رحلت "
    "وفات "
    # Adjectives
    "عید سعید"
    "عید مبارک"
    # Specific redacted figures
    "سلیمانی"
    "قاسم"
)

for marker in "${FORBIDDEN[@]}"; do
    if grep -qF -e "$marker" "$HEADER"; then
        count=$(grep -cF -e "$marker" "$HEADER")
        echo -e "${RED}✗${RESET} forbidden marker: \"$marker\" ($count hit(s))"
        grep -nF -e "$marker" "$HEADER" | head -3 | sed 's/^/    /'
        FAIL=$((FAIL + 1))
    else
        echo -e "${GREEN}✓${RESET} absent: \"$marker\""
        PASS=$((PASS + 1))
    fi
done

# ---------------------------------------------------------------------------
# Positive assertions — required patterns must be present.
# ---------------------------------------------------------------------------
REQUIRED=(
    "علی‌ابن‌ابی‌طالب"
    "حسین‌ابن‌علی"
    "حسن‌ابن‌علی (مجتبی)"
    "حسن‌ابن‌علی (عسکری)"
    "محمد‌ابن‌عبدالله"
    "درگذشت "
)

for pattern in "${REQUIRED[@]}"; do
    if grep -qF -e "$pattern" "$HEADER"; then
        echo -e "${GREEN}✓${RESET} present: \"$pattern\""
        PASS=$((PASS + 1))
    else
        echo -e "${RED}✗${RESET} missing: \"$pattern\""
        FAIL=$((FAIL + 1))
    fi
done

# ---------------------------------------------------------------------------
# Sanity: header must exist and declare the expected tables.
# ---------------------------------------------------------------------------
if ! grep -q "HOLIDAYS_COUNT" "$HEADER"; then
    echo -e "${RED}✗${RESET} HOLIDAYS_COUNT missing"
    FAIL=$((FAIL + 1))
fi

echo
echo "─────────────────────"
echo -e "Passed: ${GREEN}$PASS${RESET}"
echo -e "Failed: ${RED}$FAIL${RESET}"
echo "─────────────────────"

[ "$FAIL" -gt 0 ] && exit 1
exit 0