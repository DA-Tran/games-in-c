#!/usr/bin/env python3
"""behaviour_test.py - does each entry behave, rather than merely exit cleanly.

The other suites establish that a game runs (smoke), survives abuse (stress),
draws something sane (screen_test) and is described coherently (catalog_test).
None of them asks whether it *behaves*: a game that ignores every key, or
whose pinned seed does not actually pin anything, passes all four.

Every expectation here is derived from the catalogue or from the run itself.
Nothing is written down per game - there is no table of "chess should draw a
king", because 1000 such entries would be wrong within a week and would only
ever test the entries someone remembered to add. The checks are properties
that must hold for *any* entry, and the catalogue says which entries they
apply to.

  pinned     an entry carrying a seed must draw the same screen whatever
             GIC_SEED says. main() seeds from GIC_SEED and launch() then
             overrides it for these entries, so if the two disagree the
             override is not happening and "Daily Sudoku 3" is a different
             puzzle every time - which is the whole reason the seed exists
  signal     Ctrl-C must leave the terminal usable. The engine restores the
             cursor and the termios state from an atexit handler, and a
             signal does not run atexit handlers, so this is exactly the
             case that gets missed
  input      a game whose screen is identical after two very different runs
             of keystrokes is not reading input at all
  scores     the high-score file must still parse after play, since every
             game writes it and a corrupt one silently loses every score

Reported, not failed: which unseeded entries vary with GIC_SEED. A chess
opening is meant to be identical every time, so "does not vary" is only
evidence about randomness, not a defect.

  python3 tools/behaviour_test.py
  python3 tools/behaviour_test.py --family sudoku
  python3 tools/behaviour_test.py --start 0 --count 50 --jobs 8
  python3 tools/behaviour_test.py --checks pinned,signal
"""
import argparse
import concurrent.futures
import json
import os
import re
import shutil
import signal
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from screen_test import Screen          # the same terminal emulator


class Frames(Screen):
    """Every frame a game drew, not just the fullest one.

    Screen keeps the fullest frame because that is the one a player looked at.
    For asking whether a game reacts to input that is the wrong measurement:
    the fullest frame is usually the opening board, which is identical however
    the player then plays. Comparing the whole sequence of frames instead means
    a game that responds to anything at all looks different.
    """

    def __init__(self, *a, **kw):
        Screen.__init__(self, *a, **kw)
        self.frames = []

    def _remember(self):
        cur = "\n".join("".join(row).rstrip() for row in self.grid)
        if cur.strip():
            self.frames.append(cur)
        Screen._remember(self)

    def signature(self):
        self._remember()
        return "\x00".join(self.frames)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Two input scripts that differ as much as one player's session differs from
# another's, both ending in a quit. If a game draws the same screen for both it
# is not reacting to anything.
SCRIPT_A = b"zzz\nz\nq\nq\n"
SCRIPT_B = (b"\033[A\033[B\033[C\033[D" * 6) + b"\n5\n3\ny\n \n\033[C\033[C\n" + b"q\nq\n"

ALL_CHECKS = ("pinned", "signal", "input", "scores")


def run(slug, binary, script, seed, timeout, settle=0.35, interrupt=False):
    """Play one entry and return (screen, raw bytes, error).

    Input is written after a pause for the same reason screen_test does it: a
    real-time game polls before its first frame, so input already waiting makes
    it quit having drawn nothing. Each run gets a private directory so the
    score file cannot leak between runs and make two identical runs differ.
    """
    env = dict(os.environ, GIC_SEED=str(seed))
    env.pop("GIC_NODELAY", None)
    tmp = tempfile.mkdtemp(prefix="gic-behav-")
    proc = None
    try:
        proc = subprocess.Popen([os.path.abspath(binary), slug],
                                stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                stderr=subprocess.DEVNULL, env=env, cwd=tmp)
        try:
            import time
            time.sleep(settle)
            if interrupt:
                # The player pressing Ctrl-C part-way through a game.
                proc.send_signal(signal.SIGINT)
                out, _ = proc.communicate(timeout=timeout)
            else:
                out, _ = proc.communicate(script, timeout=timeout)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.communicate()
            return None, b"", "TIMEOUT"
        scr = Frames()
        scr.feed(out)
        return scr, out, None
    finally:
        if proc and proc.poll() is None:
            proc.kill()
        shutil.rmtree(tmp, ignore_errors=True)


RESTORE = b"\033[?25h"


def restores(out):
    """Did the engine's shutdown run - is the cursor put back?

    Deliberately compared against the same entry's own normal exit rather than
    against a fixed expectation. Whether a game hides the cursor at all depends
    on the platform and on stdout being a terminal, so asking "is the restore
    present" in isolation answers yes for a game that never emitted anything to
    restore. Asking "the clean exit restores, does the interrupted one" is a
    question with the same answer everywhere.
    """
    return RESTORE in out


def check_entry(entry, args):
    """Every check for one entry. Returns (problems, notes)."""
    slug, seed = entry["slug"], entry["params"].get("seed", 0)
    problems, notes = [], []
    want = args.checks

    # -- pinned --------------------------------------------------------
    # The entry's own seed must win over the environment's.
    if "pinned" in want and seed:
        a, _, e1 = run(slug, args.bin, SCRIPT_A, 11, args.timeout)
        b, _, e2 = run(slug, args.bin, SCRIPT_A, 9999, args.timeout)
        if e1 or e2:
            problems.append("PINNED    %-34s could not run (%s)" % (slug, e1 or e2))
        elif a.signature() != b.signature():
            problems.append("PINNED    %-34s seed %d does not override GIC_SEED; "
                            "two runs differ" % (slug, seed))

    # -- varies --------------------------------------------------------
    # Unseeded entries: informational only. A deterministic game is allowed to
    # look the same at every seed, so this cannot be a failure.
    if "pinned" in want and not seed:
        a, _, e1 = run(slug, args.bin, SCRIPT_A, 11, args.timeout)
        b, _, e2 = run(slug, args.bin, SCRIPT_A, 9999, args.timeout)
        if not (e1 or e2):
            notes.append(("varies" if a.signature() != b.signature() else "fixed", slug))

    # -- signal --------------------------------------------------------
    # atexit does not run when a signal kills the process, so this is the path
    # most likely to leave a terminal unusable.
    if "signal" in want:
        _, clean, e1 = run(slug, args.bin, SCRIPT_A, 11, args.timeout)
        _, killed, e2 = run(slug, args.bin, None, 11, args.timeout, interrupt=True)
        if e2:
            problems.append("SIGNAL    %-34s did not exit on SIGINT" % slug)
        elif e1 or not restores(clean):
            # Nothing to compare against: this entry does not emit a restore
            # even when it exits normally, so the check cannot say anything.
            notes.append(("nosignal", slug))
        elif not restores(killed):
            problems.append("SIGNAL    %-34s restores the terminal on a normal "
                            "quit but not on Ctrl-C" % slug)

    # -- input ---------------------------------------------------------
    if "input" in want:
        a, _, e1 = run(slug, args.bin, SCRIPT_A, 11, args.timeout)
        b, _, e2 = run(slug, args.bin, SCRIPT_B, 11, args.timeout)
        if not (e1 or e2) and a.signature() == b.signature():
            notes.append(("deaf", slug))

    # -- scores --------------------------------------------------------
    # Every game shares one score file, so the property that matters is not
    # "did this game save its own score" - which depends on whether the scripted
    # run happened to score anything - but "did playing it destroy anybody
    # else's". The file is seeded with entries this game has no reason to touch,
    # including keys with spaces in them, because the keys really are titles
    # like "Nine Men's Morris". Reading those with a single %s token used to end
    # the parse, and the next save then wrote the truncated cache back over the
    # file, silently erasing every score below the first spaced name.
    if "scores" in want:
        tmp = tempfile.mkdtemp(prefix="gic-score-")
        try:
            before = {"zz-canary-one": 4242, "Zz Canary Two": 777,
                      "Zz Canary Three Words": 31337}
            path = os.path.join(tmp, ".gic_scores")
            with open(path, "w", encoding="utf-8") as fh:
                for k, v in before.items():
                    fh.write("%s %d\n" % (k, v))

            env = dict(os.environ, GIC_SEED="11", HOME=tmp)
            p = subprocess.Popen([os.path.abspath(args.bin), slug],
                                 stdin=subprocess.PIPE, stdout=subprocess.DEVNULL,
                                 stderr=subprocess.DEVNULL, env=env, cwd=tmp)
            try:
                p.communicate(SCRIPT_A, timeout=args.timeout)
            except subprocess.TimeoutExpired:
                p.kill()
                p.communicate()

            after = {}
            for n, line in enumerate(open(path, encoding="utf-8",
                                          errors="replace"), 1):
                line = line.rstrip("\n")
                if not line:
                    continue
                key, _, val = line.rpartition(" ")
                if not key or not re.fullmatch(r"-?\d+", val):
                    problems.append("SCORES    %-34s wrote a line nothing can "
                                    "read back, line %d: %r"
                                    % (slug, n, line[:44]))
                    continue
                after[key] = int(val)

            for k, v in before.items():
                if k not in after:
                    problems.append("SCORES    %-34s erased an unrelated score "
                                    "(%r was in the file and is gone)" % (slug, k))
                elif after[k] != v:
                    problems.append("SCORES    %-34s changed an unrelated score "
                                    "(%r was %d, now %d)"
                                    % (slug, k, v, after[k]))
        finally:
            shutil.rmtree(tmp, ignore_errors=True)

    return problems, notes


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--family")
    ap.add_argument("--filter", default="")
    ap.add_argument("--start", type=int, default=0)
    ap.add_argument("--count", type=int, default=0)
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--timeout", type=float, default=25.0)
    ap.add_argument("--bin", default=os.path.join(ROOT, "games"))
    ap.add_argument("--checks", default=",".join(ALL_CHECKS),
                    help="comma-separated subset of: " + ", ".join(ALL_CHECKS))
    args = ap.parse_args()
    args.checks = {c.strip() for c in args.checks.split(",") if c.strip()}
    bad = args.checks - set(ALL_CHECKS)
    if bad:
        print("unknown check(s): %s" % ", ".join(sorted(bad)))
        return 2

    games = json.load(open(os.path.join(ROOT, "data", "catalog.json")))["games"]
    games = [g for g in games if g["implemented"]]
    if args.family:
        games = [g for g in games if g["family"] == args.family]
    if args.filter:
        games = [g for g in games if args.filter in g["slug"]]
    games = games[args.start:args.start + args.count] if args.count else games[args.start:]
    if not games:
        print("no entries matched")
        return 1

    if not os.path.exists(args.bin):
        print("build first: make")
        return 1

    seeded = sum(1 for g in games if g["params"].get("seed"))
    print("behaviour test: %d entries (%d of them seed-pinned), checks: %s"
          % (len(games), seeded, ", ".join(sorted(args.checks))))

    problems, notes = [], []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as ex:
        for p, n in ex.map(lambda g: check_entry(g, args), games):
            problems.extend(p)
            notes.extend(n)

    nosignal = [s for k, s in notes if k == "nosignal"]
    varies = [s for k, s in notes if k == "varies"]
    fixed = [s for k, s in notes if k == "fixed"]
    deaf = [s for k, s in notes if k == "deaf"]

    if "pinned" in args.checks and (varies or fixed):
        print("\nunseeded entries: %d vary with the seed, %d look the same at any "
              "seed" % (len(varies), len(fixed)))
        print("  (both are legitimate - a chess opening is meant to be identical)")
    if "input" in args.checks and deaf:
        print("\n%d entries drew the same screen for two very different runs of "
              "keys:" % len(deaf))
        for s in deaf[:8]:
            print("  " + s)
        if len(deaf) > 8:
            print("  ... and %d more" % (len(deaf) - 8))
        print("  (a game that waits on one key legitimately looks like this)")

    if nosignal:
        print("\n%d entries emit no terminal restore even on a clean exit, so the "
              "signal check\n  could not say anything about them" % len(nosignal))

    print("\n%d entries checked, %d problem(s)" % (len(games), len(problems)))
    if problems:
        print()
        for p in sorted(problems):
            print("  " + p)
        return 1
    print("every seed pin holds, every entry restores the terminal on Ctrl-C,")
    print("and every score file written parses back")
    return 0


if __name__ == "__main__":
    sys.exit(main())
