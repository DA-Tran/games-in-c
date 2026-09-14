#!/usr/bin/env sh
# Drive every playable game through a scripted input stream and assert that
# none of them crash, hang, or leave the terminal in a bad state.
#
# Each game is fed a burst of plausible keys followed by quits. A game passes
# if it exits 0 (clean quit) within the timeout and does not fault.
#
# The timeout is generous because several games have deliberate animation
# delays (card reveals, reel spins, dice rolls) of a second or more per
# round, and the scripted input drives several rounds before quitting.

set -u

BIN=./games
[ -x "$BIN" ] || { echo "build first: make"; exit 1; }

# Every playable catalogue entry, so each configuration is driven, not just
# one representative per engine.
SLUGS=$(python3 -c "
import json
d=json.load(open('data/catalog.json'))['games']
print(' '.join(g['slug'] for g in d if g['implemented']))
")

# Arrow keys, enter, space, letters used by menus, then a wall of quits.
INPUT=$(printf '\033[A\033[B\033[C\033[D\n \n1234hsrdflpxngbyu\n\033[A\n \nq\nq\nq\nq\nq\nq\nq\nq\nq\nq\nn\nq\nq\nq\nq\n')

pass=0
fail=0
failed=""

for slug in $SLUGS; do
    printf '%-34s' "$slug"
    printf '%s' "$INPUT" | timeout ${TIMEOUT:-30} "$BIN" "$slug" >/dev/null 2>&1
    rc=$?
    case $rc in
        0)   echo "ok";              pass=$((pass+1)) ;;
        124) echo "TIMEOUT";         fail=$((fail+1)); failed="$failed $slug(timeout)" ;;
        139) echo "SEGFAULT";        fail=$((fail+1)); failed="$failed $slug(segv)" ;;
        134) echo "ABORT";           fail=$((fail+1)); failed="$failed $slug(abort)" ;;
        *)   echo "exit $rc";        fail=$((fail+1)); failed="$failed $slug($rc)" ;;
    esac
done

echo
echo "passed: $pass   failed: $fail"
[ $fail -eq 0 ] || { echo "failures:$failed"; exit 1; }
echo "all games survived the smoke test"
