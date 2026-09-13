# Games in C

A thousand games catalogued, forty-nine written and playable — in the terminal
and in the browser, from the same design.

- **Terminal:** pure C99, ANSI escapes, zero dependencies. Builds with `make`
  on Linux, macOS and Windows.
- **Browser:** `web/index.html` opens straight from disk. No server, no build
  step, no toolchain.

```
make && ./games          # browse the catalogue and play
make play G=tetris       # launch one game directly
open web/index.html      # the browser version
```

## What is actually here

| | |
|---|---|
| Catalogued games | 1000, across 17 genres |
| Fully implemented | 49, in both C and JavaScript |
| C source | ~9,500 lines, no external libraries |
| Compiler warnings | zero, at `-Wall -Wextra` |

The 951 unimplemented entries are **specifications, not stubs**. Each carries a
genre, core mechanic, player count, difficulty rating and description — enough
to build from. The menu marks them `spec`; the playable ones are marked `play`.

## Playable games

**Board & strategy** — Tic Tac Toe (unbeatable minimax), Connect Four
(alpha-beta), Reversi, Gomoku, Checkers (forced jumps, multi-captures, kings),
Mancala, Nim (perfect nim-sum play), Dots and Boxes, Battleship (parity-search
AI), Snakes and Ladders

**Puzzles** — Minesweeper (safe first click), Sudoku (generated with a
*guaranteed unique* solution, four difficulties), Fifteen Puzzle, Lights Out,
Flood It, Sokoban (6 levels, undo), Memory Match, 2048, Towers of Hanoi,
Nonogram, Mastermind

**Arcade** — Snake, Tetris (seven-bag, wall kicks, hard drop), Pong, Breakout,
Space Invaders, Dino Run, Flappy, Frogger, Pacman (four distinct ghost
personalities), Asteroids

**Cards & dice** — Blackjack, Video Poker (Jacks or Better), War, Go Fish,
Yahtzee, Pig, Slot Machine, Higher or Lower

**Word, logic & other** — Hangman, Wordle, Anagram, Typing Test, Guess the
Number, Bulls and Cows, Rock Paper Scissors, Simon, Dungeon Crawl (procedural
roguelike), Virtual Piano

## Layout

```
include/      engine.h, games.h, cards.h, words.h
src/engine/   platform.c  raw-mode input, arrow decoding, timing, beep
              render.c    ANSI drawing primitives
              util.c      RNG, persistent high scores
src/games/    one file per game (plus shared cards.c and words.c)
src/          main.c (catalogue browser), registry.c, catalog_data.c [generated]
web/          index.html + js/ — the browser build
data/         catalog.json [generated]
tools/        gen_catalog.py, smoke_test.sh, web_test.js
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

**Input works when piped.** `key_poll()` selects on stdin whether it is a tty or
a pipe, and treats EOF as a quit. That is what makes the smoke test possible:
scripted input drives a real-time game and it exits cleanly instead of spinning.

## Tests

```
make smoke              # drives all 49 C games through scripted input
node tools/web_test.js  # plays all 49 JS games headlessly
make catalog            # regenerate the catalogue
```

`make smoke` feeds each game arrows, Enter, digits and quits, then asserts a
clean exit — no crash, no hang, no fault. `web_test.js` loads the browser build
against a DOM stub and drives 400 random keypresses plus 1000 ticks per game.

Both suites currently pass 49/49.

## Adding a game

1. Write `src/games/yourgame.c` exposing `void play_yourgame(void)`.
2. Declare it in `include/games.h` and add a row to `src/registry.c`.
3. Add the slug to `IMPLEMENTED` in `tools/gen_catalog.py`, then `make catalog`.
4. Port it into the matching `web/js/games/*.js` with `GIC.register(slug, {...})`.
5. `make smoke && node tools/web_test.js`.
