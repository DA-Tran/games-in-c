#!/bin/sh
# smoke_test.sh - the normal test: every playable catalogue entry is launched
# with a scripted run of ordinary keystrokes and must exit cleanly.
#
# This is the "does it work" pass. It answers one question per game: given
# input a person might plausibly produce, does the game run and quit without
# crashing, hanging or returning a failure code.
#
#   ./tools/smoke_test.sh                 every entry
#   FAMILY=solitaire ./tools/smoke_test.sh    one family
#   FILTER=spider ./tools/smoke_test.sh       slugs containing "spider"
#   START=200 COUNT=50 ./tools/smoke_test.sh  a slice
#   JOBS=4 ./tools/smoke_test.sh              four at a time
#   BIN=./games-asan ./tools/smoke_test.sh    against the sanitised build
#   VERBOSE=1 ./tools/smoke_test.sh           print every game, not just failures
#
# TIMEOUT defaults to 30s. Games with spin or race animations legitimately
# take several seconds of scripted input; anything under ten will report them
# as false failures.

set -u

cd "$(dirname "$0")/.." || exit 1

BIN=${BIN:-./games}
TIMEOUT=${TIMEOUT:-30}
JOBS=${JOBS:-2}
VERBOSE=${VERBOSE:-0}

[ -x "$BIN" ] || { echo "build first: make"; exit 1; }

# Arrow keys, enter, space, the letters the menus and games use, then a wall
# of quits so any game that reads keys one at a time still reaches an exit.
INPUT=$(printf '\033[A\033[B\033[C\033[D\n \n1234hsrdflpxngbyu\n\033[A\n \nq\nq\nq\nq\nq\nq\nq\nq\nq\nq\nn\nq\nq\nq\nq\n')

# One game. Writes "rc<TAB>milliseconds<TAB>slug" so the summary can be
# assembled from parallel workers without their output interleaving.
run_one() {
    slug=$1
    start=$(date +%s%N 2>/dev/null || echo 0)
    printf '%s' "$INPUT" | timeout "$TIMEOUT" "$BIN" "$slug" >/dev/null 2>&1
    rc=$?
    end=$(date +%s%N 2>/dev/null || echo 0)
    printf '%s\t%s\t%s\n' "$rc" "$(( (end - start) / 1000000 ))" "$slug"
}

# Worker mode: one game, then out. Must be handled before the catalogue is
# read or anything is printed.
if [ -n "${GIC_ONE:-}" ]; then
    run_one "$GIC_ONE"
    exit 0
fi

SLUGS=$(FILTER="${FILTER:-}" FAMILY="${FAMILY:-}" START="${START:-0}" COUNT="${COUNT:-0}" python3 -c "
import json, os
d = json.load(open('data/catalog.json'))['games']
fam = os.environ.get('FAMILY', '')
if fam: d = [g for g in d if g['family'] == fam]
s = [g['slug'] for g in d if g['implemented']]
f = os.environ.get('FILTER', '')
if f: s = [x for x in s if f in x]
start = int(os.environ.get('START') or 0)
count = int(os.environ.get('COUNT') or 0)
s = s[start:start + count] if count else s[start:]
print(' '.join(s))
")
[ -n "$SLUGS" ] || { echo "no entries matched"; exit 1; }

TOTAL=$(printf '%s\n' $SLUGS | wc -l | tr -d ' ')
RESULTS=$(mktemp)
trap 'rm -f "$RESULTS"' EXIT INT TERM

echo "normal test: $TOTAL entries, $JOBS at a time, ${TIMEOUT}s limit each"
echo


printf '%s\n' $SLUGS | xargs -P "$JOBS" -I{} sh -c 'GIC_ONE={} BIN="$1" TIMEOUT="$2" "$0"' \
    "$0" "$BIN" "$TIMEOUT" > "$RESULTS" 2>/dev/null

pass=0
fail=0
failed=""
slowest=""
slowms=0

while IFS="$(printf '\t')" read -r rc ms slug; do
    [ -n "${slug:-}" ] || continue
    if [ "$ms" -gt "$slowms" ]; then slowms=$ms; slowest=$slug; fi
    case $rc in
        0)   pass=$((pass+1))
             [ "$VERBOSE" = "1" ] && printf '  %-36s ok   %sms\n' "$slug" "$ms" ;;
        124) fail=$((fail+1)); failed="$failed $slug(timeout)"
             printf '  %-36s TIMEOUT after %ss\n' "$slug" "$TIMEOUT" ;;
        139) fail=$((fail+1)); failed="$failed $slug(segv)"
             printf '  %-36s SEGFAULT\n' "$slug" ;;
        134) fail=$((fail+1)); failed="$failed $slug(abort)"
             printf '  %-36s ABORT\n' "$slug" ;;
        *)   fail=$((fail+1)); failed="$failed $slug($rc)"
             printf '  %-36s exit %s\n' "$slug" "$rc" ;;
    esac
done < "$RESULTS"

checked=$((pass+fail))
echo
echo "passed: $pass   failed: $fail   of $checked run"
[ -n "$slowest" ] && echo "slowest: $slowest at ${slowms}ms"

if [ "$checked" -ne "$TOTAL" ]; then
    echo "WARNING: $TOTAL were selected but only $checked reported — the run was cut short"
    exit 1
fi
[ $fail -eq 0 ] || { echo "failures:$failed"; exit 1; }
echo "all $pass entries ran and exited cleanly"
