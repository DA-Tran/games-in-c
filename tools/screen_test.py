#!/usr/bin/env python3
"""screen_test.py - looks at what each game actually puts on the screen.

The smoke test only asks whether a game exited cleanly. A game can exit
cleanly having drawn nothing, having drawn "(null)", or having leaked a bare
"%d" because a format string lost its argument. None of that crashes, so
nothing else in the project notices.

Checking that properly means emulating the terminal. These games position with
cursor escapes rather than newlines, so the raw byte stream is one enormous
line and measuring it tells you nothing. This replays the output into an
80x24 character grid the way a terminal would, then inspects the grid:

  blank      the game drew something of its own below the header, rather
             than leaving the title on an otherwise empty screen
  offscreen  nothing positioned beyond the 80x24 a terminal is assumed to
             have - text written off the edge is invisible to the player
  format     no "(null)" and no unsubstituted "%d"/"%s" left on screen
  numbers    no "nan", "inf" or INT_MIN, the signatures of arithmetic that
             went wrong on its way to being displayed
  restore    the cursor is shown again on exit, so the terminal is usable

It then renders each entry a second time at the same seed and compares the
grids, which catches non-determinism: a game reading uninitialised memory
usually differs between two runs that ought to be identical.

Finally it compares entries within a family. Two entries whose opening screen
is identical are, to a player, the same game - the failure this architecture
is most exposed to, and one nothing else checks.

  python3 tools/screen_test.py                   every entry
  python3 tools/screen_test.py --family chess
  python3 tools/screen_test.py --start 0 --count 50
  python3 tools/screen_test.py --jobs 8
"""
import argparse
import collections
import concurrent.futures
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROWS, COLS = 24, 80

ESC = re.compile(rb"\x1b\[([0-9;?]*)([A-Za-z])")


class Screen:
    """Just enough VT100 to see what the player would see."""

    def __init__(self, rows=None, cols=None):
        rows = rows if rows is not None else ROWS
        cols = cols if cols is not None else COLS
        self.rows, self.cols = rows, cols
        self.grid = [[" "] * cols for _ in range(rows)]
        self.r = self.c = 0
        self.overflow = []          # (row, col) writes that fell off the screen
        self.cursor_shown = True
        # Games clear the screen on the way out, so the final grid is empty.
        # Keep the fullest frame drawn at any point instead - that is the one
        # the player actually looked at.
        self.best = ""
        self.best_n = -1
        self.need_rows = 0
        self.need_cols = 0

    def put(self, ch):
        if 0 <= self.r < self.rows and 0 <= self.c < self.cols:
            self.grid[self.r][self.c] = ch
        elif ch != " ":
            # Only real glyphs count as running off the screen. Games pad with
            # "%-20s" at the right margin, so trailing spaces routinely spill
            # past the edge and cost the player nothing - counting those
            # reported games as too big for a terminal they actually fit.
            self.overflow.append((self.r, self.c))
        # Track the extent actually used, so a game that does not fit can be
        # reported as "needs 96x30" rather than just "overflowed".
        if ch != " ":
            if self.r + 1 > self.need_rows: self.need_rows = self.r + 1
            if self.c + 1 > self.need_cols: self.need_cols = self.c + 1
        self.c += 1

    def feed(self, data):
        i, n = 0, len(data)
        while i < n:
            b = data[i:i + 1]
            if b == b"\x1b":
                m = ESC.match(data, i)
                if not m:
                    i += 1
                    continue
                args, cmd = m.group(1).decode("ascii", "replace"), m.group(2)
                nums = [int(x) for x in args.split(";") if x.isdigit()]
                if cmd == b"H" or cmd == b"f":
                    self.r = (nums[0] - 1) if nums else 0
                    self.c = (nums[1] - 1) if len(nums) > 1 else 0
                elif cmd == b"J":
                    self._remember()
                    self.grid = [[" "] * self.cols for _ in range(self.rows)]
                    self.r = self.c = 0
                elif cmd == b"K":
                    if 0 <= self.r < self.rows:
                        for x in range(max(self.c, 0), self.cols):
                            self.grid[self.r][x] = " "
                elif cmd == b"h" and args == "?25":
                    self.cursor_shown = True
                elif cmd == b"l" and args == "?25":
                    self.cursor_shown = False
                i = m.end()
                continue
            i += 1
            if b == b"\n":
                self.r += 1
                self.c = 0
            elif b == b"\r":
                self.c = 0
            elif b == b"\t":
                self.c = (self.c // 8 + 1) * 8
            elif b < b" ":
                pass
            else:
                try:
                    self.put(b.decode("utf-8"))
                except UnicodeDecodeError:
                    # A multi-byte glyph: consume its continuation bytes and
                    # count it as one cell, which is what a terminal does.
                    extra = 1
                    while i < n and 0x80 <= data[i] < 0xC0:
                        extra += 1
                        i += 1
                    self.put(data[i - extra:i].decode("utf-8", "replace") or "?")

    def _remember(self):
        cur = "\n".join("".join(row).rstrip() for row in self.grid)
        n = len("".join(cur.split()))
        if n > self.best_n:
            self.best_n, self.best = n, cur

    def text(self):
        """The fullest frame drawn, not the blank one left at exit."""
        self._remember()
        return self.best


# A few harmless keys before the quit. Some games poll for input before their
# first render, so feeding "q" alone makes them exit having drawn nothing -
# which looks like a blank screen but is only an artefact of piped input being
# instantly available. "z" is a no-op everywhere, so these just force frames.
PROBE_INPUT = b"zzz\nz\nq\nq\n"


def render(slug, seed, binary, timeout, settle=0.35):
    """Run one game and return the screen it drew.

    Input is written after a short pause rather than piped in up front. A
    real-time game polls for input before its first render, so input that is
    already waiting makes it quit having drawn nothing - which looks like a
    blank screen but only means the pipe was faster than the first frame. A
    player always gets a frame, so the test should too.
    """
    env = dict(os.environ, GIC_SEED=str(seed))
    # Each render gets its own directory so the high-score file cannot leak
    # between runs. Without this, a game that set a new best on the first run
    # shows "NEW BEST" and on the second shows the stored score, and the
    # determinism check reports a difference that is nothing to do with the
    # game's logic.
    tmp = tempfile.mkdtemp(prefix="gic-screen-")
    proc = None
    try:
        proc = subprocess.Popen([os.path.abspath(binary), slug],
                                stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                stderr=subprocess.DEVNULL, env=env, cwd=tmp)
        time.sleep(settle)                     # let it draw its opening frame
        out, _ = proc.communicate(PROBE_INPUT, timeout=timeout)
    except subprocess.TimeoutExpired:
        if proc:
            proc.kill()
            proc.communicate()
        return None, "TIMEOUT", ""
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    if proc.returncode != 0:
        return None, "EXIT%d" % proc.returncode, ""
    s = Screen()
    s.feed(out)
    return s, None, out


FORMAT_LEAK = re.compile(r"%-?[0-9]*\.?[0-9]*(d|s|ld|lu|f|x)(?![a-zA-Z0-9])")
BADNUM = re.compile(r"(?<![a-zA-Z])(nan|-nan|inf|-inf)(?![a-zA-Z])", re.I)


def check(slug, args):
    problems = []
    scr, err, raw = render(slug, args.seed, args.binary, args.timeout, args.settle)
    if err:
        return slug, ["%-9s %s" % (err, slug)], None, False
    body = scr.text()
    dense = "".join(body.split())

    # "Drew nothing" means nothing below the header, not "drew little". Simon
    # and the reaction tests legitimately show one prompt and a round counter;
    # a raw character threshold called those blank while a game that really had
    # drawn only its title sat just under the same line. Rows 0-2 are the title
    # and subtitle that draw_title always writes, so the game's own area is
    # everything from row 3 down.
    own = "".join("".join(body.split("\n")[3:]).split())
    if not own:
        problems.append("%-9s %s  drew its header and nothing else"
                        % ("BLANK", slug))
    if scr.overflow:
        problems.append("%-9s %s  needs %dx%d, does not fit %dx%d"
                        % ("TOOBIG", slug, scr.need_cols, scr.need_rows, COLS, ROWS))
    if "(null)" in body:
        problems.append("%-9s %s  \"(null)\" reached the screen" % ("NULLSTR", slug))
    m = FORMAT_LEAK.search(body)
    if m:
        problems.append("%-9s %s  unsubstituted %s on screen" % ("FORMAT", slug, m.group(0)))
    m = BADNUM.search(body)
    if m:
        problems.append("%-9s %s  %r on screen" % ("NUMBER", slug, m.group(0)))
    if "-2147483648" in body:
        problems.append("%-9s %s  INT_MIN on screen, so something overflowed"
                        % ("NUMBER", slug))
    if not scr.cursor_shown:
        problems.append("%-9s %s  terminal left with the cursor hidden" % ("CURSOR", slug))

    # Same seed twice must draw the same screen.
    scr2, err2, _ = render(slug, args.seed, args.binary, args.timeout, args.settle)
    varies = err2 is None and scr2.text() != body

    return slug, problems, body, varies


def main():
    global ROWS, COLS
    ap = argparse.ArgumentParser()
    ap.add_argument("--family")
    ap.add_argument("--filter")
    ap.add_argument("--start", type=int, default=0)
    ap.add_argument("--count", type=int, default=0)
    ap.add_argument("--jobs", type=int, default=6)
    ap.add_argument("--seed", type=int, default=4242)
    ap.add_argument("--timeout", type=int, default=20)
    ap.add_argument("--settle", type=float, default=0.35,
                    help="seconds to let a game draw before sending input")
    ap.add_argument("--cols", type=int, default=80)
    ap.add_argument("--rows", type=int, default=24)
    ap.add_argument("--binary", default=os.path.join(ROOT, "games"))
    ap.add_argument("--skip-distinct", action="store_true")
    args = ap.parse_args()

    ROWS, COLS = args.rows, args.cols

    games = json.load(open(os.path.join(ROOT, "data", "catalog.json")))["games"]
    if args.family:
        games = [g for g in games if g["family"] == args.family]
    sel = [g for g in games if g["implemented"]]
    if args.filter:
        sel = [g for g in sel if args.filter in g["slug"]]
    sel = sel[args.start:args.start + args.count] if args.count else sel[args.start:]
    if not sel:
        print("no entries matched")
        return 1

    fam_of = {g["slug"]: g["family"] for g in sel}
    print("screen test: %d entries at seed %d, %d at a time, %dx%d terminal"
          % (len(sel), args.seed, args.jobs, COLS, ROWS))
    print()

    problems, screens = [], {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        nondet = []
        for slug, probs, body, varies in pool.map(lambda g: check(g["slug"], args), sel):
            problems.extend(probs)
            if varies:
                nondet.append(slug)
            if body is not None:
                screens[slug] = body

    for p in problems:
        print("  " + p)

    print()
    print("%d of %d entries rendered and inspected, %d problem(s)"
          % (len(screens), len(sel), len(problems)))
    if nondet:
        print()
        print("%d entry(s) drew a different screen on a second run at the same seed."
              % len(nondet))
        print("For an animated or real-time game that is expected - what is on")
        print("screen depends on how much time passed. Worth a look only if a")
        print("turn-based game appears here:")
        print("  " + ", ".join(sorted(nondet)[:12]))

    if not args.skip_distinct:
        print()
        print("--- distinctness: entries in a family with an identical opening screen ---")
        by_hash = collections.defaultdict(list)
        for slug, body in screens.items():
            by_hash[(fam_of[slug], body)].append(slug)
        dupes = [v for v in by_hash.values() if len(v) > 1]
        if dupes:
            for group in sorted(dupes):
                print("  " + " = ".join(sorted(group)))
            print()
            print("  Identical opening screens. Some are legitimate - two entries")
            print("  may differ only in a rule that shows up later - but each group")
            print("  is worth confirming, because a parameter being ignored looks")
            print("  exactly like this.")
        else:
            print("  none - every entry drew a different opening screen")

    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
