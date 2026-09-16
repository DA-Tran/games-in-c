#!/usr/bin/env python3
"""catalog_test.py - integrity checks on the catalogue itself.

The other suites ask whether a game runs. This one asks whether the catalogue
is coherent, which is a different question and catches a different class of
problem. Nothing else in the project checks it: the marker gate proves an
engine *reads* its parameters, but not that the parameters it is handed are
sane, in range, or different from another entry's.

Checks, each of which can fail independently:

  slugs        unique, non-empty and URL-safe
  titles       unique - two entries with the same name are indistinguishable
               in the browser and in the menu
  families     every family named by an entry is registered in registry.c
  variants     a family's variant indices are contiguous from zero. Whether
               they produce different games is checked by running them, in
               screen_test.py, not by reading clamps out of the C - several
               engines share a source file and that mis-attributes them
  params       sizes within the largest board any engine declares
  duplicates   no two entries in a family share an identical parameter set
  readme       the counts quoted in README.md still match the catalogue
"""
import json
import os
import re
import sys
from collections import Counter, defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

failures = []
notes = []


def fail(check, msg):
    failures.append("%-12s %s" % (check, msg))


def note(msg):
    notes.append(msg)


games = json.load(open(os.path.join(ROOT, "data", "catalog.json")))["games"]

# The biggest board any engine declares, taken from the sources rather than
# assumed. Maze really does go to 41.
MAX_SIZE = 8
for _p in os.listdir(os.path.join(ROOT, "src", "games")):
    if _p.endswith(".c"):
        for _m in re.finditer(r"#define\s+(?:MAXN|GMAX|TMAX|MAXHEAP|MAXN\w*)\s+(\d+)",
                              open(os.path.join(ROOT, "src", "games", _p)).read()):
            MAX_SIZE = max(MAX_SIZE, int(_m.group(1)))
registry = open(os.path.join(ROOT, "src", "registry.c")).read()
registered = set(re.findall(r'\{\s*"([a-z0-9]+)"\s*,\s*fam_\w+\s*\}', registry))

# ------------------------------------------------------------------- slugs
slug_counts = Counter(g["slug"] for g in games)
for slug, n in slug_counts.items():
    if n > 1:
        fail("slugs", "%r appears %d times" % (slug, n))
for g in games:
    if not g["slug"]:
        fail("slugs", "empty slug for %r" % g["title"])
    elif not re.fullmatch(r"[a-z0-9]+(-[a-z0-9]+)*", g["slug"]):
        fail("slugs", "%r is not a clean url slug" % g["slug"])

# ------------------------------------------------------------------ titles
title_counts = Counter(g["title"] for g in games)
for title, n in title_counts.items():
    if n > 1:
        dupes = [g["slug"] for g in games if g["title"] == title]
        fail("titles", "%r used by %d entries: %s" % (title, n, ", ".join(dupes)))

# ---------------------------------------------------------------- families
for g in games:
    if g["family"] not in registered:
        fail("families", "%s names family %r, not in registry.c" % (g["slug"], g["family"]))

# ---------------------------------------------------------------- variants
# Only structural checks here. Whether two variants actually produce different
# games is answered by running them - see screen_test.py - because several
# engines share a source file and reading clamps out of the C mis-attributes
# them between families.
by_family = defaultdict(list)
for g in games:
    by_family[g["family"]].append(g)

for fam, entries in sorted(by_family.items()):
    used = sorted({e["params"].get("variant", 0) for e in entries})
    if used and used[0] != 0:
        note("%s: variants start at %d, not 0" % (fam, used[0]))
    gaps = [v for v in range(used[0], used[-1] + 1) if v not in used] if used else []
    if gaps:
        note("%s: unused variant indices %s" % (fam, gaps))

# ------------------------------------------------------- duplicate params
# Two entries in the same family with the same parameters are the same game
# under two names. That is the failure mode this architecture is most exposed
# to, so it is checked explicitly rather than trusted.
for fam, entries in sorted(by_family.items()):
    if len(entries) < 2:
        continue
    seen = {}
    for e in entries:
        key = tuple(sorted((k, v) for k, v in e["params"].items() if k != "title"))
        if key in seen:
            fail("duplicates",
                 "%s and %s (family %s) have identical parameters"
                 % (seen[key], e["slug"], fam))
        else:
            seen[key] = e["slug"]

# ------------------------------------------------------------------ params
# The engines clamp out-of-range sizes, which is safe but means the entry does
# not get the game it advertises.
for g in games:
    p = g["params"]
    size = p.get("size", 0)
    if size and not (2 <= size <= MAX_SIZE):
        fail("params", "%s asks for size %d; no engine declares a board that "
                       "large (largest MAXN is %d)" % (g["slug"], size, MAX_SIZE))
    diff = p.get("difficulty", 0)
    if diff and not (1 <= diff <= 5):
        fail("params", "%s has difficulty %d, expected 1-5" % (g["slug"], diff))

# ---------------------------------------------------------------- readme
# The README quotes counts that are easy to leave behind: it claimed 686
# playable for some time after the number reached 1000, and 17 genres after a
# genre was added. Numbers in prose drift silently, so the ones that can be
# derived are checked here against the catalogue.
readme_path = os.path.join(ROOT, "README.md")
if os.path.exists(readme_path):
    readme = open(readme_path, encoding="utf-8").read()
    genres = len({g["genre"] for g in games})
    playable = sum(1 for g in games if g["implemented"])
    for pattern, actual, what in (
            (r"across (\d+) genres",            genres,       "genres"),
            (r"Playable now \| (\d+)",          playable,     "playable entries"),
            (r"pass (\d+)/\d+",                 playable,     "suite totals"),
            (r"\*\*(\d+) families done",        len(by_family), "families"),
    ):
        m = re.search(pattern, readme)
        if m and int(m.group(1)) != actual:
            fail("readme", "says %s %s, catalogue has %d"
                           % (m.group(1), what, actual))

# ------------------------------------------------------------------ report
print("catalogue integrity: %d entries, %d families, %d registered engines"
      % (len(games), len(by_family), len(registered)))
if notes:
    print("\nnotes (not failures):")
    for n in notes:
        print("  " + n)
if failures:
    print("\n%d problem(s):" % len(failures))
    for f in failures:
        print("  " + f)
    sys.exit(1)
print("\nno duplicate slugs, no duplicate titles, no duplicate parameter sets,")
print("every family registered, every board within the largest an engine declares,")
print("and the counts quoted in the README still match")

