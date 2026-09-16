# Games in C

A thousand games catalogued, coded and playable — all 1000 of them, in the
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
| Catalogued games | 1000, across 18 genres |
| Playable now | 1000, in both C and JavaScript |
| External dependencies | none, on either side |
| Compiler warnings | zero, and zero again under `make strict` |

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

Current state: **80 families done, all 1000 entries playable**, no engine is
left ignoring its parameters, and no engine is unwritten. `make catalog`
recomputes those numbers from the source markers rather than from a hand-kept
tally, so the claim above cannot drift away from the code.

Both builds move together: every entry is playable in the terminal binary and
in the browser, driven from the same catalogue and the same parameters.

## Verifying

Ten layers, each answering a question the others cannot.

    make            build; the tree builds at zero warnings
    make strict     compile every file under a much harsher set, with -Werror
    make unit       the engine primitives every game shares
    make smoke      the normal test: does it work
    make stress     the stress test: can it be broken
    make parity     do the C and JS generators agree on a seed
    python3 tools/catalog_test.py     is the catalogue coherent
    python3 tools/screen_test.py      what does each game actually draw
    python3 tools/behaviour_test.py   does each entry behave, not just exit 0
    node tools/web_test.js            drive every entry in the browser build

**`make unit`** tests the engine directly rather than through a game: that
`rnd(n)` lands in `[0,n)` for every `n`, that a shuffle is a permutation, that
seeding reproduces, and that the score file round-trips. Contracts, not
remembered values - it asserts `rnd(n) < n`, never that seed 7 gives 583, so it
stays true when the implementation changes and still fails when it breaks.

This layer exists because of a bug the other nine could not reach. Writing the
score file is what corrupted it, and whether a scripted run scores at all
depends on the game, so driving whole games found it only by luck. Running one
game a thousand ways is not a substitute for testing the thing directly.

**`behaviour_test.py`** asks whether an entry *behaves*, which nothing else
does: a game that ignores every key, or whose pinned seed pins nothing, passes
build, smoke, stress and screen. Every expectation is derived from the
catalogue or from the run itself - there is no table of "chess should draw a
king", because a thousand such entries would be wrong within a week and would
only ever cover the ones somebody remembered to write down. It checks that a
seeded entry draws the same screen whatever `GIC_SEED` says, that Ctrl-C leaves
the terminal usable, that two very different runs of keys do not produce an
identical screen, and that playing one game does not destroy another's score.

**`catalog_test.py`** checks the catalogue rather than the code: unique slugs
and titles, every family registered, and - the one that matters for this
architecture - no two entries in a family sharing an identical parameter set,
because that means one game under two names.

**`screen_test.py`** emulates a terminal and inspects the grid each game
draws. These games position with cursor escapes rather than newlines, so the
raw byte stream is one enormous line and measuring it tells you nothing. It
catches a game that draws only its header, `(null)` or an unsubstituted `%d`
reaching the screen, `nan`/`inf`/INT_MIN on display, content positioned off
the edge, a terminal left with the cursor hidden, and non-determinism between
two runs at the same seed.

### Terminal size

256 of the 1000 entries need more than an 80x24 terminal; at 100x30 that falls
to about one in twenty, and the largest board in the catalogue wants 94x52.
Some of that is unavoidable: a 16x16 sudoku needs 27 rows for the grid alone,
so these are real requirements rather than layout bugs to be shrunk away.

The two builds answer that differently, because they can.

A terminal cannot be made bigger from inside the program, so the terminal build
says so. The engine measures the extent each game asks for -- every character
reaches the screen through `scr_move` or the `draw_*` helpers, so one place
sees all of it -- and when that exceeds the real window, a line appears above
the title reading `needs 94x52, terminal is 80x24 - part of this game is off
screen`. Previously the board was simply cut off at the edge with nothing to
explain why.

The browser terminal is a canvas, so it grows instead. A write past the edge
extends the grid, keeping what is already drawn, and the player sees the whole
board. 91 entries were being clipped at the old fixed 84x28; none are now.
`web_test.js` fails any entry that still loses a glyph.

`screen_test.py --cols N --rows M` reports exactly which entries exceed a given
size and what each one needs.

### Two bugs worth recording

Both were found by the layers above, and both had been present from the start.

**Ctrl-C left the terminal unusable.** The engine restores the cursor and the
termios state from an `atexit` handler, and a signal does not run `atexit`
handlers. Raw mode deliberately keeps signals working so that Ctrl-C quits - so
the most natural way to leave a game returned a terminal with no echo, no line
editing and no cursor, needing a blind `reset` to undo. All 1000 entries. There
are now `SIGINT`/`SIGTERM`/`SIGHUP` handlers that restore and re-raise, so the
exit status still reports the signal.

**Playing one game destroyed other games' high scores.** The score file is
written as `<key> <value>`, and almost every caller passes the entry's title -
"Nine Men's Morris", not "nine-mens-morris". It was read back with
`fscanf("%63s %d")`, which takes the key as one whitespace-delimited token. So
the game never got its own score back, and worse, the failed parse ended the
read: every entry *below* the first spaced name was dropped from the table, and
the next save wrote that truncated table back over the file. One game with a
space in its title silently erased every score recorded after it. The file is
now parsed a line at a time, splitting on the last space, and a damaged line
costs that line rather than the rest of the file.

### The two builds must agree

61 entries carry a fixed seed so that a name like "Daily Sudoku 3" means one
particular puzzle rather than a second name for an existing configuration. That
only means anything if both builds generate the same puzzle from it, and the
generators are two separate implementations of xorshift32 -- the JavaScript one
working in signed 32-bit ints unless coerced back at each step.

**`make parity`** checks them against each other: seven seeds, sixteen integer
and eight fractional draws each, compared number for number.

`web_test.js` additionally plays every entry twice under a pinned seed and
requires the two screens to match. That is what caught asteroids reaching past
the seeded generator for `Math.random` to set its rock velocities -- no seed
could pin it down, and the browser disagreed with the terminal. Worth noting
that the check was first written to cover only the 61 seeded entries, and in
that form it passed while the bug was live; it covers all 1000 now.

**`make smoke`** launches all 1000 entries with a scripted run of ordinary
keystrokes and requires a clean exit from each. It runs them in parallel and
says so if a run was cut short rather than quietly reporting fewer passes.

**`make stress`** attacks each entry with nine adversarial input patterns --
random bytes, truncated and malformed escape sequences, arrow spam pinned
against every boundary, walls of Enter, digit floods, hundreds of forced
replays, a 40KB mixed stream, and high-bit bytes including NUL -- across
swept RNG seeds, under AddressSanitizer and UBSan. Every failure prints the
seed and pattern that caused it, so it can be reproduced exactly.

Both honour `FAMILY`, `FILTER`, `START`, `COUNT`, `TIMEOUT`, `JOBS` and `BIN`.

### Why the two differ, and what that bought

The stress test is not the normal test with more keys. It differs in four
ways, and each one caught something:

- **Random and malformed input.** Found a stack overflow in Quoridor -- the
  pawns move on a 9x9 grid so a wall index reaches 8, but the arrays were
  `[8][8]`. It fired on the very first frame, deterministically, including
  with no input at all.
- **Empty input.** Found an infinite loop in the anagram games. `read_line`
  returned 0 both for "the player pressed Enter on an empty line" and for
  "the stream ended", and the caller retried on an empty line. The normal
  test could never find this: it always sends a wall of quits, so the stream
  never runs dry while the game is still asking for a word.
- **Sustained input.** Found that four games drew the standing high score
  inside their render loop, and `score_load()` opened and parsed the score
  file on every call -- a disk round trip sixty times a second in the
  real-time ones. A ten-thousand-keystroke run of 2048 went from 18.78s to
  0.08s once the table was held in memory.
- **Swept seeds.** The RNG seeded from `time(NULL)`, so a suite that launches
  a thousand games in a few seconds was re-testing a handful of layouts.
  `GIC_SEED` pins it, which both widens coverage and makes failures
  reproducible. Widening from one seed per pattern to four -- 36,000 runs
  rather than 9,000 -- found a signed overflow in the KenKen generator, where
  a large cage's product passes `INT_MAX` and wraps to a value the "keep
  multiplication cages small" test then accepts, producing a cage no
  arrangement can satisfy.

`ROUNDS` controls seeds per pattern and defaults to 4. Reading the
`runs completed, N problem(s)` line matters: grepping only for failure markers
cannot tell a clean run from one that was cut short.

### Daily entries are fixed puzzles

Sixty-one entries whose names promise a specific puzzle - `Sudoku Daily 3`,
`Maze Daily 7`, `Mixed Quiz Round 9` - carry a `seed` in their parameters,
derived from the slug so it never shifts when the catalogue is regenerated.
`launch()` seeds the generator before handing over, so every player opening one
gets the same grid, and no engine had to change: they all draw from the same
`rnd()`. Before this they shared every parameter with an ordinary entry and
were the same game under a second name, which is what `catalog_test.py` flagged.
The browser build uses the same xorshift so a pinned seed agrees across both.

`GIC_NODELAY` turns animation pauses into no-ops. Without it a race game
spends most of a second per round on purpose, so a few hundred scripted
keystrokes ask for half an hour of legitimate play and then look like a hang.
With it, Senet driven three hundred turns deep runs in 0.02s instead of
timing out. Neither variable changes anything for a player, and the ordinary
smoke run leaves both unset so the timing paths run at their real speed.

`make strict` earns its place too: it found six defects hiding behind ternary
arms that were identical on both sides -- the chessboard had silently lost
its light and dark squares, Halma and Chinese Checkers were the same game
with the same nine-man home, and the word-search list rendered a found word
exactly like an unfound one.

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
- **Mancala** — 25 entries. Kalah, Oware, Congkak, Sungka, Ayo, Dakon,
  Pallanguzhi, Toguz Kumalak and Bao, each expressed as a real rule set:
  store-sowing, free turns, relay sowing, three capture rules and tuzdik.
  Board width and starting seeds vary independently.
- **Dungeon Crawl** — 32 entries. Eight settings crossed with four target
  depths; the setting picks the monster roster and the hazard that wears you
  down in the open, and you win by escaping past the target depth.
- **Nim** — 12 entries: Nim and misere Nim, Subtraction, Wythoff, Fibonacci,
  Kayles, Moore's, Dawson's Chess, Turning Turtles, Northcott's, Mock Turtles
  and Chomp. Each has its own move generator, and the opponent plays a
  memoised win/lose search, so it plays every variant correctly — including
  the misere ones, where the last move loses.
- **Mastermind** — 12 entries, 3-6 pegs against 4-10 colours, with the number
  of guesses scaled to the difficulty of the code.
- **Slots** — 11 themed machines. Reel weights and payouts are identical
  across themes, so the return-to-player does not change with the artwork.
- **Gomoku** — 8 entries, 9x9 to 19x19, free-style or Renju (where an overline
  or a double four loses for black).
- **Reversi** — 5 board sizes, 4x4 to 12x12, with positional weights derived
  from the size rather than a hard-coded 8x8 table.
- **Dots and Boxes** — 5 lattice sizes, 3x3 to 7x7.

- **Maze** — 55 entries: ten generation algorithms across four sizes. The
  algorithms are the point, not difficulty tiers. Recursive backtracking
  leaves long winding corridors; Prim and Kruskal leave short branchy ones;
  binary tree and sidewinder leave a permanent bias along two edges; Wilson
  and Aldous-Broder are uniform over all spanning trees and look it. A BFS
  shortest path drives the hint and the par score.
- **Quiz** — 63 entries: sixteen topics across four difficulty bands. The
  bands are separate question pools, not a shorter timer over the same
  questions — band 0 is common knowledge and band 3 is specialist. 384
  questions in all, written once in C and derived for the browser.
- **Constrained grids** — 32 entries across sixteen puzzle types: kakuro,
  futoshiki, hitori, binairo, suguru, skyscrapers, kenken, magic square,
  unruly, dominosa, str8ts, norinori, killer and jigsaw sudoku, nurikabe and
  shikaku. Each generates a real solution first and derives its clues from it,
  so no board is unsolvable; the win test checks the puzzle's own constraints
  rather than equality with the generated grid, because several legitimately
  admit more than one solution.
- **Number drills** (20), **reaction tests** (18 — latency, span, Stroop
  interference, aim and tracking are different faculties, not one timer
  relabelled), **word games** (12) and **idle games** (10, each with its own
  resource, upgrades and cost curve).
- **Word Search** (10 themed grids), **Match Three** (6 symbol sets),
  **Peg Solitaire** (5 board shapes — English, European, triangular, diamond
  and square, which are genuinely different puzzles, not skins).

Plus single-configuration families: Pacman, Frogger, Flappy, Dino Run, Pig,
Higher or Lower.

- **Tic Tac Toe** — 12 entries. 3x3 to 6x6 with the target run scaling, plus
  misere, wild, Order and Chaos, toroidal wrapping, Notakto, nine-board,
  ultimate and numerical. Each needs its own win test; they are separate
  games, not board sizes.
- **Draughts** — 13 entries. English, International, Russian, Brazilian,
  Turkish, Italian, Spanish, Pool, Suicide, Frisian, Armenian, Canadian and
  Dameo, separated by backward capture, flying kings, compulsory maximum
  capture, orthogonal versus diagonal movement, and whether men may take
  kings. Giveaway inverts the goal.
- **Connect** — 10 entries, 4-to-7 in a row on boards from 7x6 to 13x9, with
  an optional pop-out rule that can complete a line for both players at once.
- **The race games** — 8 entries. Snakes and Ladders, Chutes and Ladders
  Deluxe, Game of the Goose, Pachisi, Ludo, Senet, the Royal Game of Ur and
  Yut Nori, differing in board length, what you throw, exact finishes, entry
  rolls and rosette squares.
- **Keyboard instrument** — 11 entries: free play, rhythm runner, note
  trainer, interval ear-training, chord builder, scale practice, drum
  machine, melody memory, perfect-pitch test, metronome and sequencer.
- **Simon** (6: four/six/eight colours, reverse, silent, speed),
  **Rock Paper Scissors** (3: classic, lizard-Spock, best of nine).

- **Twenty-one** — 9 rule sets. Shoe size, exposed hole card, dealer soft-17,
  natural payout, who takes ties, stripped tens and the five-card trick. These
  move the house edge, not the wallpaper.
- **Video Poker** — 9 paytables, two of them wild-card games (Deuces Wild,
  Joker Poker) that need a different evaluator with five-of-a-kind and wild
  royals, plus the bonus tables that split four-of-a-kind by rank.
- **Tetris** — 9 entries: sprint, ultra, zen, big mode, master, invisible,
  cascade and Pentix. Rotations are derived at run time, which is what makes
  the twelve pentominoes practical.
- **Snake** (8: wrap, maze, speed, portals, shrinking arena, poison, nibbles),
  **Breakout** (7: Arkanoid capsules, multiball, gravity, boss, endless),
  **Battleship** (6: salvo, moving ships, fog, three board sizes),
  **Yahtzee** (6: triple card, six dice, duplicate, speed, solo target),
  **Pong** (6: curve, obstacles, shrinking paddle, four-player, air hockey),
  **War** (3: attrition war, casino war, red dog),
  **Space Invaders** (2, Galaga peels divers out of the formation),
  **Asteroids** (2, deluxe adds a hunting saucer and a deeper split).

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

**Shared data has one source.** `src/games/words.c` holds the themed word
lists, the by-length guess lists and the typing passages, and
`src/games/quiz_data*.c` holds the 384 quiz questions. `tools/sync_words.py`
and `tools/sync_quiz.py` derive the browser copies, so the two builds cannot
drift. Both validate as they go: the word sync caught seven words filed under
the wrong length, and the quiz sync caught a topic-and-band pool one question
short of a full round, plus a malformed question.

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

Both suites currently pass 1000/1000.

## Adding a game

1. Write `src/games/yourfamily.c` exposing `void fam_yourfamily(const GParams *p)`,
   reading the parameter fields its catalogue entries set. Put
   `GIC:PARAMETERISED` in the header comment once it genuinely honours them.
2. Declare it in `include/games.h` and add a row to `src/registry.c`.
3. Add its entries to `tools/gen_catalog.py`, then `make catalog`.
4. Port it into `web/js/games/*.js` with `GIC.register('yourfamily', {...})`,
   whose `start(host, p)` reads the same fields.
5. `make smoke && node tools/web_test.js`.
