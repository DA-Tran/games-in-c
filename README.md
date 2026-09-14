# Games in C

A thousand games catalogued and bound to engines; 215 playable so far — in the
terminal and in the browser, from the same design.

- **Terminal:** pure C99, ANSI escapes, zero dependencies. Builds with `make`
  on Linux, macOS and Windows.
- **Browser:** `web/index.html` opens straight from disk. No server, no build
  step, no toolchain.

```
make && ./games          # browse the catalogue and play
make play G=sudoku-16x16-diagonal   # launch one entry directly by slug
open web/index.html      # the browser version
```

## What is actually here

| | |
|---|---|
| Catalogued games | 1000, across 17 genres |
| Playable now | 215, in both C and JavaScript |
| External dependencies | none, on either side |
| Compiler warnings | zero, at `-Wall -Wextra` |

### How 1000 games is being reached

Every catalogue entry names a **family** (the engine that plays it) and the
**parameters** that configure it. One well-written engine therefore delivers
many genuinely different games: the sudoku family covers five board sizes
crossed with six rule variants — 4x4 up to 16x16, diagonal, anti-knight,
windoku and the rest — as thirty separate, separately-scored entries.

Work proceeds in batches. When a batch lands, its entries light up on their
own; nothing in the catalogue needs editing.

**An entry only counts as playable when its engine actually honours the
parameters the entry promises.** A family is credited when either its source
carries the `GIC:PARAMETERISED` marker, or it has exactly one catalogue entry
and so has nothing to configure. That is deliberate: without it, one
hard-coded sudoku would claim credit for thirty variants it would silently
ignore. `tools/gen_catalog.py` prints the remaining work every time it runs.

A marker names the family it vouches for (`GIC:PARAMETERISED hangman`), so
several families can share a source file without one of them accidentally
vouching for the others.

Current state: 22 families done (215 entries), 26 engines built but still
ignoring their parameters (240 entries), 32 engines not yet written (545
entries).

## Families completed so far

**Fully parameterised** (every catalogue variant genuinely distinct):

- **Sudoku** — 30 entries. 4x4/6x6/9x9/12x12/16x16, box shape derived from the
  size, plus diagonal, even-odd, consecutive, anti-knight and windoku rules.
  Generated with a proven-unique solution.
- **Minesweeper** — 24 entries. Six board sizes across classic, no-guess, wrap
  and knight adjacency. No-guess boards are re-rolled until a logic-only
  solver can clear them, so you never have to gamble.
- **Lights Out** — 18 entries. Six sizes across plus, diagonal and
  row-and-column toggles.
- **Nonogram** — 12 entries, 5x5 to 30x30, mono and multi-colour.
- **Merge (2048)** — 8 entries: sizes 3x3-8x8 plus tripling, Fibonacci and
  Threes merge rules.
- **Towers of Hanoi** (8), **Memory Match** (6), **Flood It** (6),
  **Sokoban** (6), **Sliding Puzzle** (5).
- **Hangman** — 21 entries, one per themed dictionary.
- **Anagram** — 11 themed entries. **Typing Test** — 9 drill modes.
- **Wordle** — 5 entries, 4 to 8 letters, each with its own word list.
- **Guess the Number** (5 ranges), **Bulls and Cows** (4 digit counts).

Plus single-configuration families: Pacman, Frogger, Flappy, Dino Run, Pig,
Higher or Lower.

**Built, awaiting parameterisation** — Tic Tac Toe, Connect Four, Reversi,
Gomoku, Checkers, Mancala, Nim, Dots and Boxes, Battleship, Mastermind, Snake,
Tetris, Pong, Breakout, Space Invaders, Asteroids, Blackjack, Video Poker,
War, Go Fish, Yahtzee, Slots, Rock Paper Scissors, Simon, Snakes and Ladders,
Dungeon Crawl, Virtual Piano. These play today at their default configuration;
their remaining variants are catalogued and waiting.

## Layout

```
include/      engine.h, games.h, cards.h, words.h
src/engine/   platform.c  raw-mode input, arrow decoding, timing, beep
              render.c    ANSI drawing primitives
              util.c      RNG, persistent high scores
src/games/    one file per family (plus shared cards.c and words.c)
src/          main.c (catalogue browser), registry.c, catalog_data.c [generated]
web/          index.html + js/ — the browser build
data/         catalog.json [generated]
tools/        gen_catalog.py, sync_words.py, smoke_test.sh, web_test.js
legacy/       the original 14-file prototype, kept for reference
```

## Design notes

**One grid, two renderers.** The C games draw to a character grid, so the
browser draws one too — a canvas-rendered terminal. Each JS port mirrors its C
original closely rather than being a loose reimplementation, which is why the
rules and difficulty match exactly.

**The catalogue is a listing.** 1000 entries printed on green-bar continuous-feed
paper: ledger bands, sprocket margins, line numbers. Playing switches to the lit
terminal the games really run in.

**Dictionaries have one source.** `src/games/words.c` holds the themed word
lists, the by-length guess lists and the typing passages; `tools/sync_words.py`
derives `web/js/words.js` from it, so the two builds cannot drift. It also
validates the data — it caught seven words filed under the wrong length.

**Input works when piped.** `key_poll()` selects on stdin whether it is a tty or
a pipe, and treats EOF as a quit. That is what makes the smoke test possible:
scripted input drives a real-time game and it exits cleanly instead of spinning.

## Tests

```
make smoke              # drives every playable entry through scripted input
node tools/web_test.js  # plays every playable entry headlessly
make catalog            # regenerate the catalogue
```

Both harnesses drive **every playable catalogue entry with its own
parameters**, not one representative per engine — so all thirty sudoku
variants are generated and played, not just one.

Both suites currently pass 215/215.

## Adding a game

1. Write `src/games/yourfamily.c` exposing `void fam_yourfamily(const GParams *p)`,
   reading the parameter fields its catalogue entries set. Put
   `GIC:PARAMETERISED` in the header comment once it genuinely honours them.
2. Declare it in `include/games.h` and add a row to `src/registry.c`.
3. Add its entries to `tools/gen_catalog.py`, then `make catalog`.
4. Port it into `web/js/games/*.js` with `GIC.register('yourfamily', {...})`,
   whose `start(host, p)` reads the same fields.
5. `make smoke && node tools/web_test.js`.
