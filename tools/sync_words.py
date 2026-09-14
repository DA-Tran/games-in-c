#!/usr/bin/env python3
"""Generate web/js/words.js from src/games/words.c.

The dictionaries are written once, in C, and the browser copy is derived from
them. That keeps the two builds from drifting: if a word is added to a theme,
both sides get it on the next `make catalog`.

Parses the specific shapes words.c uses (static string arrays plus the three
index tables) rather than pretending to be a general C parser.
"""

import json
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SRC = os.path.join(ROOT, "src", "games", "words.c")
OUT = os.path.join(ROOT, "web", "js", "words.js")

src = open(SRC, encoding="utf-8").read()

# Strip comments so their contents cannot be mistaken for data.
src = re.sub(r"/\*.*?\*/", "", src, flags=re.S)


def string_array(name):
    """Return the list of string literals in `<type> NAME[...] = { ... };`."""
    m = re.search(r"\b%s\s*\[[^\]]*\]\s*=\s*\{(.*?)\};" % re.escape(name), src, re.S)
    if not m:
        raise SystemExit("could not find array %s in words.c" % name)
    return re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(1))


def ident_array(name):
    """Return the identifiers listed in a table of array pointers."""
    m = re.search(r"\b%s\s*\[[^\]]*\]\s*=\s*\{(.*?)\};" % re.escape(name), src, re.S)
    if not m:
        raise SystemExit("could not find table %s in words.c" % name)
    return [t.strip() for t in m.group(1).split(",") if t.strip()]


theme_names = string_array("THEME_NAME")
theme_tables = ident_array("THEME_WORDS")
themes = {}
for name, table in zip(theme_names, theme_tables):
    themes[name] = string_array(table)

len_tables = ident_array("WORDS_BY_LEN")
by_len = {}
for i, table in enumerate(len_tables):
    if table == "0":
        continue
    words = string_array(table)
    # Trust the words themselves rather than the slot index.
    lengths = {len(w) for w in words}
    if len(lengths) != 1:
        raise SystemExit("mixed lengths in %s: %s" % (table, sorted(lengths)))
    by_len[str(lengths.pop())] = words

mode_names = string_array("TYPING_MODE_NAME")
mode_tables = ident_array("TYPING_TEXT")
typing = {}
for name, table in zip(mode_names, mode_tables):
    typing[name] = string_array(table)

payload = {"themes": themes, "byLength": by_len, "typing": typing,
           "themeOrder": theme_names, "typingOrder": mode_names}

with open(OUT, "w", encoding="utf-8") as f:
    f.write("/* GENERATED from src/games/words.c by tools/sync_words.py. "
            "Do not edit. */\n")
    f.write("window.GICWORDS = ")
    json.dump(payload, f, separators=(",", ":"), ensure_ascii=False)
    f.write(";\n")

print("words.js: %d themes, lengths %s, %d typing modes"
      % (len(themes), ",".join(sorted(by_len, key=int)), len(typing)))
