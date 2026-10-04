#!/usr/bin/env bash
set -u

BIN="./jala"
PASS=0
FAIL=0

GREEN='\033[32m'
RED='\033[31m'
RESET='\033[0m'

assert_contains() {
    local desc="$1" output="$2" expected="$3"
    if echo "$output" | grep -qF "$expected"; then
        echo -e "${GREEN}✓${RESET} $desc"
        PASS=$((PASS + 1))
    else
        echo -e "${RED}✗${RESET} $desc"
        echo "    expected: $expected"
        FAIL=$((FAIL + 1))
    fi
}

assert_not_contains() {
    local desc="$1" output="$2" unexpected="$3"
    if echo "$output" | grep -qF "$unexpected"; then
        echo -e "${RED}✗${RESET} $desc"
        FAIL=$((FAIL + 1))
    else
        echo -e "${GREEN}✓${RESET} $desc"
        PASS=$((PASS + 1))
    fi
}

assert_exit() {
    local desc="$1" expected="$2" actual="$3"
    if [ "$expected" -eq "$actual" ]; then
        echo -e "${GREEN}✓${RESET} $desc"
        PASS=$((PASS + 1))
    else
        echo -e "${RED}✗${RESET} $desc (expected $expected, got $actual)"
        FAIL=$((FAIL + 1))
    fi
}

echo "Running jala tests..."
echo

out=$("$BIN" -v)
assert_contains "version output" "$out" "jala"

out=$("$BIN" -h)
assert_contains "help mentions -y" "$out" "-y"
assert_contains "help mentions -p" "$out" "-p"
assert_contains "help mentions -t" "$out" "-t"

out=$("$BIN" -n 7 1405)
assert_contains "Mehr 1405 header" "$out" "Mehr 1405"
assert_contains "weekday header" "$out" "Sh Ye Do"
assert_contains "day 30 present" "$out" "30"

out=$("$BIN" -n -c 1405/07/12)
assert_contains "convert Jalali" "$out" "Jalali"
assert_contains "convert Gregorian" "$out" "Gregorian"
assert_contains "converted year 2026" "$out" "2026"

out=$("$BIN" -n -c 2026-10-04)
assert_contains "reverse year 1405" "$out" "1405"

out=$("$BIN" -n -d 1405/07/12 1405/07/14)
assert_contains "diff 2 days" "$out" "2 day"

out=$("$BIN" -n -y 1405)
assert_contains "full year Farvardin" "$out" "Farvardin"
assert_contains "full year Esfand" "$out" "Esfand"

out=$("$BIN" -n -3 7 1405)
assert_contains "three months Shahrivar" "$out" "Shahrivar"
assert_contains "three months Aban" "$out" "Aban"

out=$("$BIN" -n -p 7 1405)
assert_contains "Persian month name" "$out" "مهر"

out=$("$BIN" -n -e 7 1405)
assert_contains "English Sat" "$out" "Sat"
assert_contains "English Sun" "$out" "Sun"

out=$("$BIN" -n -P 7 1405)
assert_contains "Imperial year 2585" "$out" "2585"
assert_contains "pa label" "$out" "(pa)"

"$BIN" 13 >/dev/null 2>&1
assert_exit "invalid month exits 1" 1 $?

"$BIN" 7 10000 >/dev/null 2>&1
assert_exit "invalid year exits 1" 1 $?

out=$(NO_COLOR=1 "$BIN" 7 1405)
assert_not_contains "NO_COLOR disables color" "$out" $'\033['

echo
echo "─────────────────────"
echo -e "Passed: ${GREEN}$PASS${RESET}"
echo -e "Failed: ${RED}$FAIL${RESET}"
echo "─────────────────────"

[ "$FAIL" -gt 0 ] && exit 1
exit 0
