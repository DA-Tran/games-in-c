#!/usr/bin/env python3
"""Generate web/js/quiz.js from the C question bank.

The questions are written once, in C, and the browser copy is derived. Same
reasoning as sync_words.py: two hand-maintained copies of 384 questions would
drift, and a wrong answer in only one build is the kind of bug nobody notices.

Validates as it goes: every entry must have four answers and a correct index
in range, and every (topic, level) pair must have enough questions for a round.
"""
import json
import os
import re
import collections

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
OUT = os.path.join(ROOT, "web", "js", "quiz.js")
ROUND = 6

topics = []
items = []

for fname in ("quiz_data.c", "quiz_data2.c"):
    src = open(os.path.join(ROOT, "src", "games", fname), encoding="utf-8").read()
    src = re.sub(r"/\*.*?\*/", "", src, flags=re.S)

    m = re.search(r"QUIZ_TOPIC\s*\[[^\]]*\]\s*=\s*\{(.*?)\};", src, re.S)
    if m:
        topics = re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(1))

    body = re.search(r"QUIZ_BANK2?\s*\[\]\s*=\s*\{(.*)\};", src, re.S)
    if not body:
        raise SystemExit("could not find the bank array in %s" % fname)

    row = re.compile(
        r'\{\s*(\d+)\s*,\s*(\d+)\s*,\s*'
        r'"((?:[^"\\]|\\.)*)"\s*,\s*\{\s*'
        r'"((?:[^"\\]|\\.)*)"\s*,\s*"((?:[^"\\]|\\.)*)"\s*,\s*'
        r'"((?:[^"\\]|\\.)*)"\s*,\s*"((?:[^"\\]|\\.)*)"\s*\}\s*,\s*(\d+)\s*\}')
    for g in row.finditer(body.group(1)):
        topic, level = int(g.group(1)), int(g.group(2))
        answers = [g.group(i) for i in range(4, 8)]
        correct = int(g.group(8))
        if correct < 0 or correct > 3:
            raise SystemExit("correct index out of range: %s" % g.group(3))
        if len(set(answers)) != 4:
            raise SystemExit("duplicate answers in: %s" % g.group(3))
        items.append({"t": topic, "l": level, "q": g.group(3),
                      "a": answers, "c": correct})

if not topics:
    raise SystemExit("no topic table found")

counts = collections.Counter((i["t"], i["l"]) for i in items)
thin = [k for k, v in counts.items() if v < ROUND]
if thin:
    raise SystemExit("too few questions for a %d-question round: %s"
                     % (ROUND, sorted(thin)))

with open(OUT, "w", encoding="utf-8") as f:
    f.write("/* GENERATED from src/games/quiz_data*.c by tools/sync_quiz.py. "
            "Do not edit. */\n")
    f.write("window.GICQUIZ = ")
    json.dump({"topics": topics, "items": items}, f,
              separators=(",", ":"), ensure_ascii=False)
    f.write(";\n")

print("quiz.js: %d topics, %d questions, %d (topic,level) pools"
      % (len(topics), len(items), len(counts)))
