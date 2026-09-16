#!/bin/sh
# stress_test.sh - the stress test: every entry is attacked with adversarial
# input across many random seeds, under AddressSanitizer.
#
# The normal test asks "does it work". This one asks "can it be broken". The
# two differ in three ways that matter:
#
#   input   the normal test sends keystrokes a person might produce. This one
#           sends random bytes, truncated escape sequences, walls of Enter,
#           arrow spam that pins every cursor against every boundary, and
#           hundred-kilobyte streams.
#   seeds   the engine seeds its RNG from the clock, so a fast test run deals
#           the same cards to every game. This sweeps GIC_SEED, so each round
#           gets a different shuffle, maze, board and deal.
#   binary  it runs the sanitised build, so a write one element past the end
#           of a tableau is a failure rather than a silent corruption.
#   speed   it sets GIC_NODELAY, which turns every animation pause into a
#           no-op. A race game spends most of a second per round on purpose,
#           so without this a few hundred keystrokes ask for half an hour of
#           real time and the game gets misreported as hung. With it, these
#           patterns drive games thousands of turns deep instead.
#
# Every failure is printed with the seed and pattern that produced it, so it
# can be reproduced exactly:  GIC_SEED=<n> ./games-asan <slug> < <pattern file>
#
# A run that exceeds the limit is re-tried against the plain binary before it
# is called a hang. AddressSanitizer costs roughly an order of magnitude, and
# forty thousand keystrokes aimed at a 30x30 grid is a lot of genuine work, so
# "slower than the limit" and "stuck" are reported as different things.
#
# That distinction is measured in wall-clock time, so it degrades when JOBS is
# well above the core count: a run that takes ten seconds alone can take forty
# under eight-way contention on two cores and be called a hang. The re-try is
# therefore given a generous budget, and JOBS defaults to something modest.
# Raise JOBS for throughput, but confirm any HANG at JOBS=1 before believing
# it.
#
#   ./tools/stress_test.sh                    every entry, default rounds
#   ROUNDS=8 ./tools/stress_test.sh           eight rounds per entry
#   FAMILY=chess ./tools/stress_test.sh       one family
#   START=0 COUNT=50 ./tools/stress_test.sh   a slice
#   JOBS=4 ./tools/stress_test.sh             four at a time
#   PATTERNS="random esc" ./tools/stress_test.sh   only those patterns

set -u

cd "$(dirname "$0")/.." || exit 1

BIN=${BIN:-./games-asan}
TIMEOUT=${TIMEOUT:-40}
JOBS=${JOBS:-2}
ROUNDS=${ROUNDS:-4}
PATTERNS=${PATTERNS:-"empty random arrows enter esc digits yes huge binary"}

[ -x "$BIN" ] || { echo "build the sanitised binary first: make asan"; exit 1; }

FUZZDIR=${FUZZDIR:-/tmp/gic-stress-$$}

# ---------------------------------------------------------------- patterns
# Each pattern is a file of bytes fed to the game on stdin, built once and
# reused so a thousand games do not each pay to generate them.
#
# The sizes are deliberate. A turn-based game with a reveal animation spends
# most of a second per round on purpose, so a pattern of two thousand Enters
# asks for half an hour of legitimate play and then gets reported as a hang.
# These are sized to hit every boundary, every replay path and every decoder
# branch several times over while still finishing: a hundred and fifty replays
# re-enters every deal and reset path, which is where the bugs are.
build_patterns() {
    mkdir -p "$FUZZDIR" || exit 1

    : > "$FUZZDIR/empty"

    # Genuinely random bytes: exercises the escape decoder and every default
    # branch in every key switch.
    dd if=/dev/urandom of="$FUZZDIR/random" bs=1024 count=4 2>/dev/null

    # Arrow spam. Pins every cursor against every boundary repeatedly, which
    # is where off-by-one indexing shows up.
    : > "$FUZZDIR/arrows"
    i=0
    while [ $i -lt 400 ]; do
        printf '\033[A\033[A\033[D\033[D\033[B\033[B\033[C\033[C' >> "$FUZZDIR/arrows"
        i=$((i+1))
    done

    # Nothing but Enter: drives every selection, confirmation and menu.
    i=0
    : > "$FUZZDIR/enter"
    while [ $i -lt 2000 ]; do printf '\n' >> "$FUZZDIR/enter"; i=$((i+1)); done

    # Truncated and malformed escape sequences. A decoder that reads ahead
    # without checking for the end of the stream will run off it here.
    : > "$FUZZDIR/esc"
    i=0
    while [ $i -lt 300 ]; do
        printf '\033\033[\033[9\033[999999;999999R\033[?\033O\033[[A' >> "$FUZZDIR/esc"
        i=$((i+1))
    done

    # Digits only: every numeric selection, group size and bet control.
    : > "$FUZZDIR/digits"
    i=0
    while [ $i -lt 1000 ]; do printf '0123456789' >> "$FUZZDIR/digits"; i=$((i+1)); done

    # "Yes" to everything: keeps games replaying round after round, which
    # re-enters every deal and reset path hundreds of times in one process.
    : > "$FUZZDIR/yes"
    i=0
    while [ $i -lt 800 ]; do printf 'y\n \n' >> "$FUZZDIR/yes"; i=$((i+1)); done

    # A large mixed stream, to catch anything that accumulates per keystroke.
    : > "$FUZZDIR/huge"
    i=0
    while [ $i -lt 800 ]; do
        printf 'wasdhjkl123456789 \n\033[A\033[Bypnrdfcmuvt' >> "$FUZZDIR/huge"
        i=$((i+1))
    done

    # High-bit bytes and control characters, including NUL.
    printf 'a' | tr 'a' '\000' > "$FUZZDIR/binary"
    i=0
    while [ $i -lt 200 ]; do
        printf '\377\376\200\201\001\002\007\010\013\014\016\017\177' >> "$FUZZDIR/binary"
        printf 'a' | tr 'a' '\000' >> "$FUZZDIR/binary"
        i=$((i+1))
    done
}

# One game, one pattern, one seed. Prints a line only when something is wrong.
run_one() {
    slug=$1
    n=0
    for pat in $PATTERNS; do
        r=0
        while [ $r -lt "$ROUNDS" ]; do
            seed=$(( (r * 7919 + n * 104729 + 1) % 2147483647 ))
            err=$(GIC_SEED=$seed GIC_NODELAY=1 ASAN_OPTIONS=detect_leaks=0:abort_on_error=0 \
                  timeout "$TIMEOUT" "$BIN" "$slug" < "$FUZZDIR/$pat" 2>&1 >/dev/null)
            rc=$?
            case "$err" in
                *"ERROR: AddressSanitizer"*|*"runtime error"*)
                    printf 'SANITISER\t%s\t%s\t%s\n' "$slug" "$pat" "$seed"
                    printf '%s\n' "$err" | grep -E "ERROR: AddressSanitizer|runtime error" | head -1 | sed 's/^/          /'
                    ;;
                *)
                    case $rc in
                        0)   ;;
                        124)
                            # Over the limit under the sanitiser. Re-run on the
                            # plain build with twice the budget: if that
                            # finishes, the game is slow, not stuck.
                            if GIC_SEED=$seed GIC_NODELAY=1 timeout $((TIMEOUT * 6)) ./games "$slug" \
                                   < "$FUZZDIR/$pat" >/dev/null 2>&1; then
                                printf 'SLOW\t%s\t%s\t%s\n' "$slug" "$pat" "$seed"
                            else
                                printf 'HANG\t%s\t%s\t%s\n' "$slug" "$pat" "$seed"
                            fi
                            ;;
                        139) printf 'SEGV\t%s\t%s\t%s\n' "$slug" "$pat" "$seed" ;;
                        134) printf 'ABORT\t%s\t%s\t%s\n' "$slug" "$pat" "$seed" ;;
                        *)   printf 'EXIT%s\t%s\t%s\t%s\n' "$rc" "$slug" "$pat" "$seed" ;;
                    esac
                    ;;
            esac
            r=$((r+1))
        done
        n=$((n+1))
    done
}

# Worker mode: one game, then out. Handled before anything prints.
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

build_patterns
trap 'rm -rf "$FUZZDIR"' EXIT INT TERM

TOTAL=$(printf '%s\n' $SLUGS | wc -l | tr -d ' ')
NPAT=$(printf '%s\n' $PATTERNS | wc -w | tr -d ' ')
RUNS=$((TOTAL * NPAT * ROUNDS))
REPORT=$(mktemp)

echo "stress test: $TOTAL entries x $NPAT patterns x $ROUNDS seeds = $RUNS runs"
echo "binary: $BIN   limit: ${TIMEOUT}s each   jobs: $JOBS"
echo "patterns: $PATTERNS"
echo

printf '%s\n' $SLUGS | xargs -P "$JOBS" -I{} sh -c \
    'GIC_ONE={} BIN="$1" TIMEOUT="$2" ROUNDS="$3" PATTERNS="$4" FUZZDIR="$5" "$0"' \
    "$0" "$BIN" "$TIMEOUT" "$ROUNDS" "$PATTERNS" "$FUZZDIR" | tee "$REPORT"

problems=$(grep -c -E '^(SANITISER|HANG|SEGV|ABORT|EXIT)' "$REPORT" 2>/dev/null)
[ -n "$problems" ] || problems=0
slow=$(grep -c -E '^SLOW' "$REPORT" 2>/dev/null)
[ -n "$slow" ] || slow=0
echo
echo "$RUNS runs completed, $problems problem(s)"
[ "$slow" -gt 0 ] && echo "$slow run(s) were slower than the limit but finished on the plain build - not a hang"
if [ "$problems" -gt 0 ]; then
    echo
    echo "reproduce a line with:  GIC_SEED=<seed> $BIN <slug> < \$FUZZDIR/<pattern>"
    rm -f "$REPORT"
    exit 1
fi
rm -f "$REPORT"
echo "every entry survived random bytes, malformed escapes, boundary spam and repeated replay"
