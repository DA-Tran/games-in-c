#!/bin/sh
# rng_parity.sh - the two builds must agree, number for number, on a pinned seed.
#
# 61 entries carry a fixed seed so that a name like "Daily Sudoku 3" means one
# particular puzzle rather than a second name for an existing configuration.
# That promise only holds if the browser and the terminal generate the same
# puzzle from that seed, and the two generators are separate implementations -
# xorshift32 in C, the same algorithm rewritten in JavaScript, where every
# intermediate is a signed 32-bit int unless it is explicitly coerced back.
# Nothing else in the project compares them, so a divergence would show up as a
# player insisting the daily is different on their phone.
#
# Both integer and fractional draws are checked: rnd_f is a separate code path
# taking the top 24 bits, and it is what the arcade games use for velocities.
#
#   ./tools/rng_parity.sh
set -u
cd "$(dirname "$0")/.." || exit 1

SEEDS="1 7 12345 2463534242 4294967295 999983 2147483647"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT INT TERM

# The C generator, compiled on its own so the harness pulls in no I/O.
#
# Cut at the first function that is not part of the generator rather than at a
# line number: this was "sed -n 1,35p" until an #include was added at the top
# of util.c, which shifted every line by one and left the extract ending in the
# middle of rnd_f. Nothing about a line number says what it is pointing at.
awk '/^void shuffle_int/ { exit } { print }' src/engine/util.c \
    | sed 's|#include "engine.h"||' > "$TMP/rng.c"
cat >> "$TMP/rng.c" <<'EOF'
#include <stdio.h>
int main(int argc, char **argv) {
    int i, k;
    for (k = 1; k < argc; k++) {
        unsigned s = (unsigned)strtoul(argv[k], 0, 10);
        rng_seed(s);
        printf("%s i:", argv[k]);
        for (i = 0; i < 16; i++) printf(" %d", rnd(1000));
        rng_seed(s);
        printf(" f:");
        for (i = 0; i < 8; i++) printf(" %.9f", rnd_f());
        printf("\n");
    }
    return 0;
}
EOF
sed -i '1i #include <stdlib.h>' "$TMP/rng.c"
${CC:-cc} -std=c99 -w -o "$TMP/rng" "$TMP/rng.c" || { echo "could not build the C generator"; exit 1; }
"$TMP/rng" $SEEDS > "$TMP/c.txt" || exit 1

node -e '
const fs=require("fs"),vm=require("vm");
const sb={console,performance:{now:()=>0},requestAnimationFrame:()=>1,
  cancelAnimationFrame(){},localStorage:{getItem:()=>null,setItem(){}},
  devicePixelRatio:1,innerWidth:1000,addEventListener(){},removeEventListener(){}};
sb.window=sb;sb.global=sb;vm.createContext(sb);
vm.runInContext(fs.readFileSync("web/js/catalog.js","utf8"),sb);
vm.runInContext(fs.readFileSync("web/js/engine.js","utf8"),sb);
const G=sb.GIC, out=[];
for (const s of process.argv.slice(1)) {
  G.rngSeed(Number(s));
  const ints=[]; for(let i=0;i<16;i++) ints.push(G.rnd(1000));
  G.rngSeed(Number(s));
  const fs_=[];  for(let i=0;i<8;i++)  fs_.push(G.rndF().toFixed(9));
  out.push(s+" i: "+ints.join(" ")+" f: "+fs_.join(" "));
}
console.log(out.join("\n"));
' $SEEDS > "$TMP/js.txt" || { echo "could not run the JS generator"; exit 1; }

if diff -u "$TMP/c.txt" "$TMP/js.txt" > "$TMP/diff.txt"; then
    n=$(printf '%s\n' $SEEDS | wc -w | tr -d ' ')
    echo "rng parity: C and JS agree across $n seeds, 16 integer and 8 fractional draws each"
    exit 0
fi
echo "rng parity: the two builds disagree - a pinned seed does not mean the same puzzle"
echo
cat "$TMP/diff.txt"
exit 1
