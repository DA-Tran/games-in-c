#!/usr/bin/env python3
"""Generate the 1000-game catalogue.

Every entry names a **family** (the engine that plays it) and the concrete
**parameters** that configure it. A catalogue entry is playable exactly when
its family appears in src/registry.c, so each batch of engines lights up its
entries automatically — nothing here needs editing when a batch lands.

Outputs:
    data/catalog.json     -- browser app
    src/catalog_data.c    -- terminal menu
    web/js/catalog.js     -- browser app, no-fetch copy
"""

import glob
import json
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
TARGET = 1000

entries = []
seen = set()


def slugify(name):
    s = name.lower().replace("'", "").replace("&", "and").replace("+", "plus")
    return re.sub(r"[^a-z0-9]+", "-", s).strip("-")


def emit(family, title, genre, mech, players, diff, blurb, **params):
    """Add one catalogue entry bound to `family` with `params`."""
    slug = params.pop("slug", None) or slugify(title)
    if slug in seen:
        raise SystemExit("duplicate slug: " + slug)
    seen.add(slug)
    p = {
        "width": 0, "height": 0, "size": 0, "variant": 0, "count": 0,
        "level": 0, "difficulty": diff, "players": 0, "theme": "",
    }
    p.update(params)
    entries.append({
        "slug": slug, "title": title, "genre": genre, "mechanic": mech,
        "players": players, "difficulty": max(1, min(5, diff)), "blurb": blurb,
        "family": family, "params": p,
    })


# =========================================================================
# GRID AND CONSTRAINT PUZZLES
# =========================================================================

# --- sudoku: 5 sizes x 6 rule variants ----------------------------------
SUDOKU_VARIANTS = [
    ("Classic",     "standard row, column and box constraints", 0),
    ("Diagonal",    "both long diagonals must also hold every digit", 1),
    ("Even-Odd",    "shaded cells are restricted by parity", 2),
    ("Consecutive", "markers reveal every adjacent consecutive pair", 3),
    ("Anti-Knight", "no digit repeats a knight's move away", 4),
    ("Windoku",     "four extra shaded regions must also be complete", 5),
]
for size, base in [(4, 1), (6, 2), (9, 3), (12, 4), (16, 5)]:
    for name, why, var in SUDOKU_VARIANTS:
        emit("sudoku", "Sudoku %dx%d %s" % (size, size, name), "Puzzle",
             "Latin square constraint", "1", min(5, base + (var > 0)),
             "%dx%d sudoku where %s." % (size, size, why),
             size=size, variant=var)

# --- minesweeper: 6 boards x 4 rule variants ----------------------------
MS_BOARDS = [(9, 9, 10, 2), (12, 12, 25, 3), (16, 16, 40, 3),
             (20, 20, 80, 4), (30, 16, 99, 4), (40, 20, 180, 5)]
MS_VARIANTS = [("Classic", "standard adjacency counting", 0),
               ("No Guess", "every board is solvable by pure logic", 1),
               ("Wrap", "the grid wraps at the edges", 2),
               ("Knight", "adjacency follows a knight's move", 3)]
for w, h, mines, diff in MS_BOARDS:
    for name, why, var in MS_VARIANTS:
        emit("minesweeper", "Minesweeper %dx%d %s" % (w, h, name), "Puzzle",
             "Constraint deduction", "1", diff,
             "%dx%d grid, %d mines, %s." % (w, h, mines, why),
             width=w, height=h, count=mines, variant=var)

# --- nonogram: 6 sizes x mono/colour ------------------------------------
for size, diff in [(5, 1), (10, 2), (15, 3), (20, 4), (25, 5), (30, 5)]:
    emit("nonogram", "Nonogram %dx%d" % (size, size), "Puzzle",
         "Line-clue painting", "1", diff,
         "Reveal the hidden %dx%d image from row and column runs." % (size, size),
         size=size, variant=0)
    emit("nonogram", "Nonogram %dx%d Colour" % (size, size), "Puzzle",
         "Line-clue painting", "1", min(5, diff + 1),
         "Multi-colour %dx%d nonogram with per-colour run clues." % (size, size),
         size=size, variant=1)

# --- sliding tile puzzles -----------------------------------------------
for n, diff in [(3, 1), (4, 2), (5, 3), (6, 4), (7, 5)]:
    emit("sliding", "Sliding Puzzle %dx%d" % (n, n), "Puzzle", "Sliding tile",
         "1", diff, "Order %d tiles in an %dx%d frame." % (n * n - 1, n, n),
         size=n)

# --- lights out: 6 sizes x 3 toggle patterns ----------------------------
for n in [3, 4, 5, 6, 7, 8]:
    for name, why, var, diff in [("Classic", "plus-shaped toggle", 0, 2),
                                 ("Diagonal", "diagonal toggle pattern", 1, 3),
                                 ("Row-Column", "whole row and column toggle", 2, 3)]:
        emit("lightsout", "Lights Out %dx%d %s" % (n, n, name), "Puzzle",
             "Toggle parity", "1", diff,
             "%dx%d board using a %s." % (n, n, why), size=n, variant=var)

# --- merge / 2048 --------------------------------------------------------
for n, diff in [(3, 3), (4, 2), (5, 2), (6, 2), (8, 3)]:
    emit("merge", "2048 %dx%d" % (n, n), "Puzzle", "Merge sliding", "1", diff,
         "Merge-sliding on an %dx%d grid." % (n, n), size=n, variant=0)
for name, why, var, diff in [("Fibonacci", "tiles merge along the Fibonacci sequence", 2, 4),
                             ("Threes", "ones and twos combine before tripling", 3, 3),
                             ("Powers of Three", "tiles triple instead of doubling", 1, 3)]:
    emit("merge", "2048 %s" % name, "Puzzle", "Merge sliding", "1", diff,
         "Sliding merge game where %s." % why, size=4, variant=var)

# --- memory, flood it, hanoi, sokoban -----------------------------------
for pairs, diff in [(6, 1), (8, 1), (12, 2), (18, 3), (24, 3), (32, 4)]:
    emit("memory", "Memory Match %d Pairs" % pairs, "Puzzle", "Pair recall",
         "1", diff, "Clear %d hidden pairs from memory." % pairs, count=pairs)
for n, cols, moves, diff in [(8, 4, 16, 1), (10, 5, 19, 2), (12, 6, 22, 2),
                             (14, 6, 25, 3), (16, 7, 28, 4), (18, 8, 32, 5)]:
    emit("floodit", "Flood It %dx%d" % (n, n), "Puzzle", "Flood fill", "1", diff,
         "Recolour an %dx%d board from the corner within %d moves." % (n, n, moves),
         size=n, count=cols, level=moves)
for n in [3, 4, 5, 6, 7, 8, 9, 10]:
    emit("hanoi", "Towers of Hanoi %d Discs" % n, "Puzzle", "Recursive stacking",
         "1", min(5, 1 + n // 3),
         "Relocate a %d-disc tower in %d moves." % (n, 2 ** n - 1), size=n)
for i, (pack, diff) in enumerate([("Tutorial", 1), ("Classic", 3), ("Microban", 2),
                                  ("Sasquatch", 4), ("Grand Master", 5), ("Mini Cosmos", 3)]):
    emit("sokoban", "Sokoban %s" % pack, "Puzzle", "Crate pushing", "1", diff,
         "%s crate-pushing level set." % pack, level=i)

# --- maze: 10 generation algorithms x 4 sizes ---------------------------
MAZE_ALGOS = [
    ("Recursive Backtracker", "long winding corridors"),
    ("Prim", "a bushy, many-branched layout"),
    ("Kruskal", "a uniformly random spanning structure"),
    ("Eller", "a maze generated row by row"),
    ("Wilson", "an unbiased uniform spanning tree"),
    ("Aldous-Broder", "a slow but perfectly uniform random walk"),
    ("Hunt and Kill", "long passages with sparse dead ends"),
    ("Binary Tree", "a strong diagonal texture"),
    ("Sidewinder", "horizontal runs with upward links"),
    ("Growing Tree", "a tunable mix of Prim and backtracker"),
]
for ai, (algo, texture) in enumerate(MAZE_ALGOS):
    for size, label, diff in [(11, "Small", 1), (21, "Medium", 2),
                              (31, "Large", 3), (41, "Huge", 4)]:
        emit("maze", "Maze %s %s" % (algo, label), "Puzzle", "Path finding",
             "1", diff, "%s maze generated with %s, producing %s."
             % (label, algo, texture), size=size, variant=ai)

# --- constraint grids ----------------------------------------------------
CONSTRAINT = [
    ("Kakuro", "fill runs to their clue totals without repeating a digit", 4),
    ("Futoshiki", "a Latin square constrained by inequality signs", 3),
    ("Hitori", "shade duplicates without splitting the grid", 4),
    ("Binairo", "balance ones and zeros with no three in a row", 3),
    ("Suguru", "fill each region with 1..n, no touching repeats", 3),
    ("Skyscrapers", "a Latin square deduced from sightline counts", 4),
    ("KenKen", "a Latin square with arithmetic cages", 3),
    ("Magic Square", "arrange numbers so every line shares a sum", 3),
    ("Unruly", "balance black and white with no three in a row", 3),
    ("Dominosa", "rebuild a domino set from a grid of pips", 3),
    ("Str8ts", "compartments must hold consecutive runs", 4),
    ("Norinori", "shade dominoes, exactly two per region", 4),
    ("Killer Sudoku", "sudoku with caged sums instead of given digits", 4),
    ("Jigsaw Sudoku", "irregular regions instead of square boxes", 4),
    ("Nurikabe", "carve islands of the clued size from a sea", 4),
    ("Shikaku", "split the grid into rectangles of the clued area", 3),
]
for ci, (name, why, diff) in enumerate(CONSTRAINT):
    for size, label in [(5, "Small"), (7, "Standard")]:
        emit("constraint", "%s %s" % (name, label), "Puzzle", "Logical deduction",
             "1", diff if label == "Small" else min(5, diff + 1),
             "%s: %s." % (label, why), variant=ci, size=size)

# --- misc single puzzles -------------------------------------------------
for i, (name, mech, diff, blurb) in enumerate([
    ("Peg Solitaire English", "Jump elimination", 3, "English board peg jumping down to one survivor."),
    ("Peg Solitaire European", "Jump elimination", 3, "European board peg jumping."),
    ("Peg Solitaire Triangular", "Jump elimination", 3, "Triangular peg board."),
    ("Peg Solitaire Diamond", "Jump elimination", 4, "Diamond peg board."),
    ("Peg Solitaire Square", "Jump elimination", 4, "Square peg board."),
]):
    emit("pegsolitaire", name, "Puzzle", mech, "1", diff, blurb, variant=i)

for i, theme in enumerate(["Gems", "Fruit", "Runes", "Candy", "Stars", "Blocks"]):
    emit("matchthree", "Match Three %s" % theme, "Puzzle", "Tile matching", "1", 2,
         "Swap adjacent %s to form lines of three or more." % theme.lower(),
         theme=theme, variant=i)

for i, (name, why, diff) in enumerate([
    ("Tile Rotate", "rotate tiles until the network connects", 3),
    ("Loop Closer", "rotate pieces to close a single loop", 3),
    ("Flip Grid", "flip rows and columns to reach a target pattern", 3),
    ("Ball Sort", "stack matching balls into single-colour tubes", 2),
    ("Colour Sort", "pour liquids until each tube is one colour", 2),
    ("Rush Hour", "slide blocking cars to free the red one", 3),
]):
    emit("tilepuzzle", name, "Puzzle", "Spatial rearrangement", "1", diff,
         why.capitalize() + ".", variant=i)


# =========================================================================
# BOARD AND ABSTRACT
# =========================================================================

TTT = [("3x3", 3, 3, 0, 1, "three in a row on the classic board"),
       ("4x4", 4, 4, 0, 2, "four in a row on a 4x4 board"),
       ("5x5", 5, 4, 0, 2, "four in a row on 5x5"),
       ("6x6", 6, 5, 0, 3, "five in a row on 6x6"),
       ("Misere", 3, 3, 1, 2, "making three in a row loses"),
       ("Wild", 3, 3, 2, 3, "either player may place either mark"),
       ("Order and Chaos", 6, 5, 3, 4, "one player wants five in a row, the other wants none"),
       ("Toroidal", 4, 4, 4, 3, "the board wraps at every edge"),
       ("Notakto", 3, 3, 5, 4, "misere play where both players use the same mark"),
       ("Nine Board", 3, 3, 6, 4, "your move chooses the board your opponent must play in"),
       ("Ultimate", 3, 3, 7, 4, "nine boards, won by winning three in a row of them"),
       ("Numerical", 3, 3, 8, 3, "place numbers that must sum to fifteen in a line")]
for name, size, need, var, diff, why in TTT:
    emit("tictactoe", "Tic Tac Toe %s" % name, "Board",
         "Perfect-information placement", "1-2", diff,
         "Noughts and crosses where %s." % why, size=size, count=need, variant=var)

for need, w, h, diff in [(4, 7, 6, 2), (5, 9, 7, 3), (6, 11, 8, 3), (7, 13, 9, 4)]:
    emit("connect", "Connect %d" % need, "Board", "Gravity-fed alignment",
         "1-2", diff, "Drop pieces into a %dx%d grid and line up %d."
         % (w, h, need), width=w, height=h, count=need, variant=0)
    emit("connect", "Connect %d Pop Out" % need, "Board", "Gravity-fed alignment",
         "1-2", min(5, diff + 1),
         "Connect %d where you may also pop a piece from the bottom row." % need,
         width=w, height=h, count=need, variant=1)
emit("connect", "Connect Four Wide", "Board", "Gravity-fed alignment", "1-2", 3,
     "Connect Four on a 10x6 board.", width=10, height=6, count=4, variant=0)
emit("connect", "Connect Four Tall", "Board", "Gravity-fed alignment", "1-2", 3,
     "Connect Four on a 7x10 board.", width=7, height=10, count=4, variant=0)

for n, diff in [(4, 1), (6, 2), (8, 3), (10, 4), (12, 4)]:
    emit("reversi", "Reversi %dx%d" % (n, n), "Board", "Flanking capture",
         "1-2", diff, "Othello-style flipping on an %dx%d board." % (n, n), size=n)

for n, diff in [(9, 2), (13, 3), (15, 3), (19, 4)]:
    emit("gomoku", "Gomoku %dx%d" % (n, n), "Board", "Five-in-a-row", "1-2", diff,
         "Five in a row on %dx%d." % (n, n), size=n, variant=0)
    emit("gomoku", "Renju %dx%d" % (n, n), "Board", "Five-in-a-row", "1-2",
         min(5, diff + 1),
         "Gomoku on %dx%d with forbidden moves for black." % (n, n),
         size=n, variant=1)

DRAUGHTS = [("Checkers", 8, 0, 3, "standard English draughts with forced jumps"),
            ("International Draughts", 10, 1, 4, "10x10 with flying kings and long captures"),
            ("Russian Draughts", 8, 2, 3, "promotion mid-capture and flying kings"),
            ("Brazilian Draughts", 8, 3, 3, "international rules on an 8x8 board"),
            ("Turkish Draughts", 8, 4, 4, "orthogonal movement and capture"),
            ("Italian Draughts", 8, 5, 3, "men cannot capture kings"),
            ("Spanish Draughts", 8, 6, 3, "backwards-capturing kings"),
            ("Pool Checkers", 8, 7, 3, "American pool rules with flying kings"),
            ("Suicide Checkers", 8, 8, 2, "lose all your pieces to win"),
            ("Frisian Draughts", 10, 9, 5, "orthogonal capture alongside diagonal movement"),
            ("Armenian Draughts", 8, 10, 4, "mixed orthogonal and diagonal capture"),
            ("Canadian Checkers", 12, 11, 5, "the 12x12 board with thirty pieces a side"),
            ("Dameo", 8, 12, 4, "modern draughts with linear sliding moves")]
for name, size, var, diff in [(d[0], d[1], d[2], d[3]) for d in DRAUGHTS]:
    why = [d[4] for d in DRAUGHTS if d[0] == name][0]
    emit("checkers", name, "Board", "Capture and promotion", "1-2", diff,
         "Draughts with %s." % why, size=size, variant=var)

MANCALA_RULES = [("Kalah", 0), ("Oware", 1), ("Congkak", 2), ("Sungka", 3),
                 ("Ayo", 4), ("Dakon", 5), ("Pallanguzhi", 6), ("Toguz Kumalak", 7)]
for name, var in MANCALA_RULES:
    for houses, seeds in [(6, 4), (6, 5), (7, 5)]:
        emit("mancala", "%s %d-%d" % (name, houses, seeds), "Board", "Seed sowing",
             "1-2", 3, "%s sowing with %d houses and %d seeds each."
             % (name, houses, seeds), width=houses, count=seeds, variant=var)
emit("mancala", "Bao", "Board", "Seed sowing", "1-2", 5,
     "The deep Swahili four-row sowing game.", width=8, count=2, variant=8)

NIM_VARIANTS = [("Nim", "the player taking the last object wins", 0, 2),
                ("Nim Misere", "taking the last object loses", 1, 2),
                ("Nim Subtraction", "removals come from a fixed set", 2, 3),
                ("Wythoff's Game", "remove from one pile or equally from both", 3, 4),
                ("Fibonacci Nim", "each take is bounded by twice the last", 4, 4),
                ("Kayles", "knock down one or two adjacent pins", 5, 3),
                ("Moore's Nim", "remove from up to k piles at once", 6, 4),
                ("Dawson's Chess", "a pawn race that reduces to a nim value", 7, 4),
                ("Turning Turtles", "flip coins under octal game rules", 8, 4),
                ("Northcott's Game", "move checkers along rows toward each other", 9, 3),
                ("Mock Turtles", "an octal game with a surprising pattern", 10, 5),
                ("Chomp", "bite the grid; the poisoned square loses", 11, 3)]
for name, why, var, diff in NIM_VARIANTS:
    emit("nim", name, "Logic", "Combinatorial subtraction", "1-2", diff,
         "Take-away game where %s." % why, size=4, variant=var)

for n in [3, 4, 5, 6, 7]:
    emit("dotsboxes", "Dots and Boxes %dx%d" % (n, n), "Board", "Edge claiming",
         "1-2", min(5, 1 + n // 2),
         "Claim boxes on an %dx%d dot lattice." % (n, n), size=n)

BATTLESHIP = [("Battleship", 10, 0, 2, "the standard ten-by-ten hunt"),
              ("Battleship Salvo", 10, 1, 3, "you fire one shot per surviving ship"),
              ("Battleship Big Board", 12, 2, 3, "a twelve-by-twelve ocean and a larger fleet"),
              ("Battleship Moving Ships", 10, 3, 4, "the fleet relocates between turns"),
              ("Battleship Fog", 10, 4, 4, "hits are reported only at the end of a round"),
              ("Battleship Small", 8, 5, 2, "a tight eight-by-eight duel")]
for name, size, var, diff in [(b[0], b[1], b[2], b[3]) for b in BATTLESHIP]:
    why = [b[4] for b in BATTLESHIP if b[0] == name][0]
    emit("battleship", name, "Board", "Hidden-grid deduction", "1-2", diff,
         "Naval deduction: %s." % why, size=size, variant=var)

for pegs in [3, 4, 5, 6]:
    for colours in [6, 8, 10]:
        emit("mastermind", "Mastermind %dx%d" % (pegs, colours), "Logic",
             "Code deduction", "1", min(5, 1 + (pegs * colours) // 12),
             "Break a %d-peg code drawn from %d colours." % (pegs, colours),
             count=pegs, level=colours)

for men, diff in [(3, 1), (6, 2), (9, 2), (12, 3)]:
    emit("morris", "%s Men's Morris" % {3: "Three", 6: "Six", 9: "Nine", 12: "Twelve"}[men],
         "Abstract", "Mill formation", "1-2", diff,
         "Form mills and reduce your opponent to two men.", count=men)

TAFL = [("Hnefatafl", 11, 0, 4), ("Brandubh", 7, 1, 3), ("Tablut", 9, 2, 3),
        ("Ard Ri", 7, 3, 3), ("Alea Evangelii", 19, 4, 5)]
for name, size, var, diff in TAFL:
    emit("tafl", name, "Abstract", "Asymmetric siege", "1-2", diff,
         "Norse siege game on %dx%d: the king escapes or is surrounded." % (size, size),
         size=size, variant=var)

HEXFAM = [("Hex", 0), ("Havannah", 1), ("Y", 2)]
for name, var in HEXFAM:
    for size in [7, 9, 11, 13, 15, 19]:
        emit("hexconnect", "%s Size %d" % (name, size), "Abstract",
             "Connection race", "1-2", min(5, 1 + size // 5),
             "%s on a size-%d hexagonal board." % (name, size), size=size, variant=var)
emit("hexconnect", "TwixT", "Abstract", "Connection race", "1-2", 4,
     "Place pegs and link them into an unbroken bridge chain.", size=12, variant=3)

ABSTRACT2 = [("Breakthrough", 0, 2, "push a single pawn through the enemy line"),
             ("Clobber", 1, 3, "capture adjacent enemy stones until nobody can move"),
             ("Konane", 2, 3, "Hawaiian jumping capture; stalemate loses"),
             ("Surakarta", 3, 3, "capture by looping around the board's outer rails"),
             ("Game of Amazons", 4, 4, "move queens and burn squares until someone is entombed"),
             ("Lines of Action", 5, 3, "gather every piece into one connected group"),
             ("Alquerque", 6, 2, "the medieval jumping-capture ancestor of draughts"),
             ("Fanorona", 7, 4, "Malagasy capture by approach and withdrawal"),
             ("Halma", 8, 2, "move your whole camp into the opposite corner"),
             ("Chinese Checkers", 9, 2, "star-board hopping race"),
             ("Pentago", 10, 3, "place a marble, then rotate a quadrant"),
             ("Quarto", 11, 4, "your opponent chooses the piece you must place"),
             ("Gobblet", 12, 3, "nest pieces and swallow the opponent's to make a line"),
             ("Quoridor", 13, 3, "race to the far wall while dropping fences"),
             ("Sim", 14, 2, "avoid completing a triangle in your own colour"),
             ("Col", 15, 3, "graph colouring as a combinatorial game")]
for name, var, diff, why in ABSTRACT2:
    emit("abstract", name, "Abstract", "Positional strategy", "1-2", diff,
         why.capitalize() + ".", variant=var)


# =========================================================================
# CHESS, GO, BACKGAMMON
# =========================================================================

CHESS = [("Chess", 0, 4, "full standard rules: castling, en passant, promotion"),
         ("Chess960", 1, 4, "Fischer random back-rank shuffle"),
         ("King of the Hill", 2, 3, "win by marching your king to the centre"),
         ("Three-Check Chess", 3, 3, "check the enemy king three times to win"),
         ("Atomic Chess", 4, 4, "captures detonate the surrounding squares"),
         ("Horde Chess", 5, 4, "one side fields a wall of pawns"),
         ("Racing Kings", 6, 3, "no checks; first king to the eighth rank wins"),
         ("Antichess", 7, 3, "captures are compulsory and losing everything wins"),
         ("Extinction Chess", 8, 3, "lose a whole piece type and you lose"),
         ("Knightmate", 9, 4, "the knight is royal; the king is an ordinary piece"),
         ("Dark Chess", 10, 4, "fog of war: you see only what you attack"),
         ("Progressive Chess", 11, 4, "turn one is one move, turn two is two"),
         ("Minichess 5x5", 12, 3, "Gardner's miniature with the full piece set"),
         ("Los Alamos Chess", 13, 3, "the 6x6 bishop-less board used by MANIAC I"),
         ("Cylinder Chess", 14, 4, "the a and h files wrap around")]
for name, var, diff, why in CHESS:
    emit("chess", name, "Chess", "Piece movement strategy", "1-2", diff,
         "Chess where %s." % why, variant=var)

CHESSPUZZLE = [("Mate in One", 0, 1), ("Mate in Two", 1, 3), ("Mate in Three", 2, 5),
               ("King and Pawn", 3, 3), ("Rook Endgame", 4, 4), ("Knight Fork", 5, 2),
               ("Pin and Skewer", 6, 2), ("Discovered Attack", 7, 3),
               ("Back Rank Mate", 8, 2), ("Stalemate Trap", 9, 3)]
for name, var, diff in CHESSPUZZLE:
    emit("chesspuzzle", "Chess Puzzle %s" % name, "Chess", "Tactical drill", "1",
         diff, "Tactics trainer focused on %s." % name.lower(), variant=var)

for size, diff in [(5, 2), (7, 3), (9, 4), (13, 5), (19, 5)]:
    emit("go", "Go %dx%d" % (size, size), "Abstract", "Territory capture", "1-2",
         diff, "Go on a %dx%d board with area scoring." % (size, size),
         size=size, variant=0)
for size in [5, 7, 9]:
    emit("go", "Atari Go %dx%d" % (size, size), "Abstract", "Territory capture",
         "1-2", 2, "First capture wins — the standard teaching game.",
         size=size, variant=1)

BACKGAMMON = [("Backgammon", 0, 4), ("Hypergammon", 1, 3), ("Nackgammon", 2, 4),
              ("Acey Deucey", 3, 3), ("Plakoto", 4, 4), ("Fevga", 5, 3),
              ("Nardi", 6, 3)]
for name, var, diff in BACKGAMMON:
    emit("backgammon", name, "Board", "Dice-driven racing", "1-2", diff,
         "Race and hit across twenty-four points.", variant=var)

RACE = [("Snakes and Ladders", 0, 1), ("Chutes and Ladders Deluxe", 1, 1),
        ("Game of the Goose", 2, 1), ("Pachisi", 3, 2), ("Ludo", 4, 1),
        ("Senet", 5, 3), ("Royal Game of Ur", 6, 3), ("Yut Nori", 7, 2)]
for name, var, diff in RACE:
    emit("snakesladders", name, "Board", "Dice racing", "1-2", diff,
         "Dice-driven race to the final square.", variant=var)


# =========================================================================
# CARDS
# =========================================================================

SOLITAIRE = [("Klondike Draw One", 0, 2), ("Klondike Draw Three", 1, 3),
             ("Spider One Suit", 2, 2), ("Spider Two Suit", 3, 3),
             ("Spider Four Suit", 4, 5), ("FreeCell", 5, 3),
             ("Eight Off", 6, 2), ("Seahaven Towers", 7, 4),
             ("Penguin", 8, 3), ("Pyramid Solitaire", 9, 2),
             ("TriPeaks", 10, 2), ("Golf Solitaire", 11, 2),
             ("Yukon Solitaire", 12, 3), ("Russian Solitaire", 13, 5),
             ("Canfield", 14, 3), ("Scorpion", 15, 4),
             ("Forty Thieves", 16, 4), ("Aces Up", 17, 2),
             ("Accordion", 18, 4), ("Baker's Dozen", 19, 3),
             ("Beleaguered Castle", 20, 3), ("Clock Patience", 21, 1),
             ("La Belle Lucie", 22, 4), ("Calculation", 23, 4),
             ("Napoleon at St Helena", 24, 4), ("Garbage", 25, 1),
             ("Kings Corner", 26, 2), ("Nertz", 27, 4), ("Spit", 28, 2)]
for name, var, diff in SOLITAIRE:
    emit("solitaire", name, "Solitaire", "Patience card sorting", "1", diff,
         "Patience: %s." % name.lower(), variant=var)

BLACKJACK = [("Blackjack", 0, 2), ("Blackjack Single Deck", 1, 2),
             ("Blackjack Six Deck", 2, 2), ("Double Exposure", 3, 3),
             ("Spanish 21", 4, 3), ("Vegas Strip", 5, 2),
             ("Atlantic City", 6, 2), ("Face Up 21", 7, 3), ("Pontoon", 8, 2)]
for name, var, diff in BLACKJACK:
    emit("blackjack", name, "Cards", "Card counting and odds", "1", diff,
         "Twenty-one under %s rules." % name.lower(), variant=var)

VIDEOPOKER = [("Jacks or Better", 0), ("Bonus Poker", 1), ("Double Bonus Poker", 2),
              ("Double Double Bonus", 3), ("Deuces Wild", 4), ("Joker Poker", 5),
              ("Aces and Faces", 6), ("Tens or Better", 7), ("All American", 8)]
for name, var in VIDEOPOKER:
    emit("videopoker", "Video Poker %s" % name, "Cards", "Draw poker payout",
         "1", 2, "Five-card draw video poker on the %s paytable." % name, level=var)

TRICK = [("Hearts", 0, 3), ("Spades", 1, 3), ("Whist", 2, 3), ("Oh Hell", 3, 3),
         ("Euchre", 4, 4), ("Briscola", 5, 2), ("Napoleon", 6, 3),
         ("Sevens", 7, 1), ("Knockout Whist", 8, 2), ("German Whist", 9, 3),
         ("Ninety-Nine", 10, 2), ("Scopa", 11, 3)]
for name, var, diff in TRICK:
    emit("tricktaking", name, "Cards", "Trick-taking play", "2-4", diff,
         "Trick play: %s." % name.lower(), variant=var)

SHED = [("Crazy Eights", 0, 1), ("Crazy Eights Wild Draw", 1, 2),
        ("President", 2, 2), ("Cheat", 3, 2), ("Durak", 4, 4),
        ("Old Maid", 5, 1), ("Snap", 6, 1), ("Slapjack", 7, 1),
        ("Beggar My Neighbour", 8, 1), ("Egyptian Ratscrew", 9, 2),
        ("Go Fish", 10, 1), ("Rummy", 11, 3)]
for name, var, diff in SHED:
    emit("shedding", name, "Cards", "Shedding and matching", "2-4", diff,
         "Shed your hand: %s." % name.lower(), variant=var)

for name, var, diff in [("War", 0, 1), ("Casino War", 1, 1), ("Red Dog", 2, 1)]:
    emit("war", name, "Cards", "High-card comparison", "1-2", diff,
         "Card comparison duel.", variant=var)

CASINO = [("Baccarat", 0, 2), ("Cribbage", 1, 4), ("Gin Rummy", 2, 3),
          ("Canasta", 3, 4), ("Pinochle", 4, 5), ("Casino", 5, 3),
          ("Golf Card Game", 6, 2), ("Blackjack Switch", 7, 3),
          ("Three Card Poker", 8, 2), ("Five Card Draw", 9, 3)]
for name, var, diff in CASINO:
    emit("cardmisc", name, "Cards", "Melding and comparison", "1-4", diff,
         "Classic card game: %s." % name.lower(), variant=var)


# =========================================================================
# DICE AND CHANCE
# =========================================================================

DICE = [("Farkle", 0, 2), ("Craps", 1, 3), ("Liar's Dice", 2, 3), ("Bunco", 3, 1),
        ("Shut the Box", 4, 2), ("Cee-lo", 5, 1), ("Chuck-a-Luck", 6, 1),
        ("Mexico", 7, 2), ("Beetle", 8, 1), ("Zonk", 9, 2), ("Yacht", 10, 2),
        ("Balut", 11, 2), ("Kismet", 12, 2), ("Drop Dead", 13, 1),
        ("Threes", 14, 1), ("Going to Boston", 15, 1), ("Pig Dice", 16, 1),
        ("Dudo", 17, 3), ("Sic Bo", 18, 2), ("Klondike Dice", 19, 1)]
for name, var, diff in DICE:
    emit("dice", name, "Dice", "Probability and press-your-luck", "1-4", diff,
         "Dice game: %s." % name.lower(), variant=var)

for name, var, diff in [("Yahtzee", 0, 2), ("Yahtzee Triple", 1, 3),
                        ("Yahtzee Six Dice", 2, 3), ("Yahtzee Duplicate", 3, 3),
                        ("Yahtzee Speed", 4, 3), ("Yahtzee Solo Challenge", 5, 3)]:
    emit("yahtzee", name, "Dice", "Category scoring", "1", diff,
         "Thirteen-category dice scoring.", variant=var)
emit("pig", "Pig", "Dice", "Press-your-luck", "1-2", 1,
     "Roll to build a turn total, but a one wipes it out.", variant=0)

BETTING = [("Roulette European", 0, 2), ("Roulette American", 1, 2),
           ("Roulette French", 2, 2), ("Roulette Mini", 3, 1),
           ("Keno", 4, 1), ("Bingo", 5, 1), ("Lottery Sim", 6, 1),
           ("Coin Streak", 7, 1), ("Wheel of Fortune", 8, 1),
           ("Scratch Card", 9, 1), ("Three Card Monte", 10, 2),
           ("Plinko", 11, 1), ("Horse Race Sim", 12, 1),
           ("Odds and Evens", 13, 1), ("Matching Pennies", 14, 2),
           ("Dice Duel", 15, 1), ("High Card Draw", 16, 1), ("Crash", 17, 2)]
for name, var, diff in BETTING:
    emit("betting", name, "Chance", "Probability and betting", "1", diff,
         "Betting game: %s." % name.lower(), variant=var)

for i, theme in enumerate(["Fruit", "Egypt", "Space", "Pirate", "Jungle", "Diamond",
                           "Western", "Aztec", "Neon", "Deep Sea", "Classic"]):
    emit("slots", "Slots %s" % theme, "Chance", "Reel matching", "1", 1,
         "%s-themed reels with a published paytable." % theme, theme=theme, variant=i)

for name, var in [("Rock Paper Scissors", 0), ("Rock Paper Scissors Lizard Spock", 1),
                  ("Rock Paper Scissors Best of Nine", 2)]:
    emit("rps", name, "Chance", "Simultaneous selection", "1-2", 1,
         "Simultaneous-throw duel against a frequency-tracking AI.", variant=var)
emit("higherlower", "Higher or Lower", "Chance", "Sequential prediction", "1", 1,
     "Call the next card and push a streak as the odds tighten.", variant=0)


# =========================================================================
# ARCADE
# =========================================================================

SNAKE = [("Snake", 0, 1, "the classic walled arena"),
         ("Snake Wrap", 1, 1, "you pass through the edges"),
         ("Snake Maze", 2, 3, "interior obstacles block the field"),
         ("Snake Speed", 3, 3, "the snake accelerates continuously"),
         ("Snake Portal", 4, 3, "paired teleporters reroute the snake"),
         ("Snake Shrinking", 5, 4, "the arena closes in over time"),
         ("Snake Poison", 6, 3, "some food shortens you instead"),
         ("Nibbles", 7, 2, "level-based snake with walls and gates")]
for name, var, diff, why in SNAKE:
    emit("snake", name, "Arcade", "Growth and collision", "1", diff,
         "Snake where %s." % why, variant=var)

TETRIS = [("Tetris", 0, 3), ("Tetris Sprint 40", 1, 3), ("Tetris Ultra", 2, 3),
          ("Tetris Zen", 3, 1), ("Tetris Big Mode", 4, 3), ("Tetris Master", 5, 5),
          ("Tetris Invisible", 6, 5), ("Tetris Cascade", 7, 4), ("Pentix", 8, 4)]
for name, var, diff in TETRIS:
    emit("tetris", name, "Arcade", "Falling-block packing", "1", diff,
         "Falling-block stacking.", variant=var)

for name, var, diff in [("Pong", 0, 1), ("Pong Curve", 1, 2), ("Pong Obstacle", 2, 2),
                        ("Pong Shrinking Paddle", 3, 3), ("Pong Four Player", 4, 3),
                        ("Air Hockey", 5, 2)]:
    emit("pong", name, "Arcade", "Paddle deflection", "1-2", diff,
         "Paddle rally with angle-of-impact physics.", variant=var)

for name, var, diff in [("Breakout", 0, 2), ("Arkanoid", 1, 3), ("Breakout Multiball", 2, 3),
                        ("Breakout Gravity", 3, 3), ("Breakout Boss", 4, 4),
                        ("Breakout Endless", 5, 3), ("Brick Breaker Ultra", 6, 3)]:
    emit("breakout", name, "Arcade", "Brick destruction", "1", diff,
         "Clear the wall with paddle-english control.", variant=var)

for name, var, diff in [("Space Invaders", 0, 2), ("Galaga", 1, 3)]:
    emit("invaders", name, "Arcade", "Formation shooter", "1", diff,
         "Hold the line as the formation accelerates.", variant=var)
emit("dino", "Dino Run", "Arcade", "Endless runner", "1", 1,
     "One-button jumping over an accelerating cactus field.", variant=0)
emit("flappy", "Flappy", "Arcade", "Gap navigation", "1", 2,
     "Tap against gravity and thread an unforgiving sequence of gaps.", variant=0)
emit("frogger", "Frogger", "Arcade", "Timing and lanes", "1", 3,
     "Cross traffic and a moving river without drowning.", variant=0)
emit("pacman", "Pacman", "Arcade", "Maze pursuit", "1", 3,
     "Clear the maze with four ghosts running distinct chase rules.", variant=0)
for name, var, diff in [("Asteroids", 0, 3), ("Space Rocks Deluxe", 1, 4)]:
    emit("asteroids", name, "Arcade", "Inertial shooter", "1", diff,
         "Momentum-based flight, screen wrap and splitting rocks.", variant=var)

ARCADE_MISC = [("Centipede", 0, 3), ("Missile Command", 1, 3), ("Dig Dug", 2, 3),
               ("Q*bert", 3, 3), ("Donkey Kong", 4, 3), ("Lunar Lander", 5, 3),
               ("Moon Patrol", 6, 3), ("Defender", 7, 4), ("Tempest", 8, 4),
               ("Robotron", 9, 4), ("Berzerk", 10, 3), ("Bomberman", 11, 3),
               ("Boulder Dash", 12, 3), ("Lode Runner", 13, 4), ("Pitfall", 14, 3),
               ("Kaboom", 15, 2), ("Tron Light Cycles", 16, 2), ("Doodle Jump", 17, 2),
               ("Icy Tower", 18, 3), ("Helicopter Game", 19, 2),
               ("Jetpack Joyride", 20, 2), ("Crossy Road", 21, 2),
               ("Whack-a-Mole", 22, 1), ("Scramble", 23, 3), ("Time Pilot", 24, 3),
               ("Pinball", 25, 3), ("Minefield Run", 26, 3), ("Snake Charmer", 27, 2),
               ("Cannon Angle", 28, 2), ("Echo Maze", 29, 3)]
for name, var, diff in ARCADE_MISC:
    emit("arcademisc", name, "Arcade", "Real-time reflexes", "1", diff,
         "Arcade classic: %s." % name.lower(), variant=var)

ARTILLERY = [("Gorillas", 0, 2), ("Scorched Earth", 1, 3), ("Artillery Duel Wind", 2, 3),
             ("Artillery Duel Gravity", 3, 3), ("Artillery Duel Terrain", 4, 3),
             ("Artillery Duel Multi Shot", 5, 3), ("Artillery Moving Target", 6, 4),
             ("Tank Battle", 7, 2), ("Worms Lite", 8, 4)]
for name, var, diff in ARTILLERY:
    emit("artillery", name, "Arcade", "Projectile physics", "1-2", diff,
         "Angle-and-power duel.", variant=var)


# =========================================================================
# WORD
# =========================================================================

WORD_THEMES = ["Animals", "Countries", "Capitals", "Computing", "Space", "Food",
               "Sports", "Films", "Music", "Nature", "Science", "History",
               "Occupations", "Colours", "Vehicles", "Instruments", "Weather",
               "Body Parts", "Tools", "Plants", "Mythology"]
for i, theme in enumerate(WORD_THEMES):
    emit("hangman", "Hangman %s" % theme, "Word", "Letter deduction", "1", 2,
         "Letter-guessing on a %s dictionary." % theme.lower(), theme=theme, variant=i)
for n in [4, 5, 6, 7, 8]:
    emit("wordle", "Wordle %d Letters" % n, "Word", "Positional feedback", "1",
         2 if n <= 5 else 3,
         "Guess a hidden %d-letter word from colour feedback." % n, count=n)
for i, theme in enumerate(WORD_THEMES[:11]):
    emit("anagram", "Anagram %s" % theme, "Word", "Letter rearrangement", "1", 2,
         "Unscramble %s vocabulary against the clock." % theme.lower(),
         theme=theme, variant=i)
for i, theme in enumerate(WORD_THEMES[:10]):
    emit("wordsearch", "Word Search %s" % theme, "Word", "Grid word finding", "1", 1,
         "Find hidden %s words in eight directions." % theme.lower(),
         theme=theme, variant=i)
WORD_MISC = [("Cryptogram", 0, 3), ("Word Ladder", 1, 3), ("Boggle", 2, 2),
             ("Ghost", 3, 2), ("Superghost", 4, 3), ("Word Chain", 5, 1),
             ("Spelling Bee", 6, 2), ("Jotto", 7, 3), ("Text Twist", 8, 3),
             ("Countdown Letters", 9, 3), ("Missing Vowels", 10, 2),
             ("Palindrome Hunt", 11, 2)]
for name, var, diff in WORD_MISC:
    emit("wordmisc", name, "Word", "Vocabulary and pattern", "1-2", diff,
         "Word game: %s." % name.lower(), variant=var)
TYPING = [("Common Words", 0), ("Quotations", 1), ("Code Snippets", 2),
          ("Numbers", 3), ("Punctuation", 4), ("Long Passage", 5),
          ("Sixty Second", 6), ("Accuracy Mode", 7), ("Pangrams", 8)]
for name, var in TYPING:
    emit("typing", "Typing Test %s" % name, "Skill", "Input speed", "1", 2,
         "Words-per-minute drill on %s." % name.lower(), variant=var)


# =========================================================================
# LOGIC, MATHS, TRIVIA, SKILL
# =========================================================================

for hi, diff in [(10, 1), (100, 1), (1000, 2), (10000, 2), (1000000, 3)]:
    emit("guessnumber", "Guess the Number to %d" % hi, "Logic", "Binary search",
         "1", diff, "Find a hidden number between 1 and %d." % hi, count=hi)
for n in [3, 4, 5, 6]:
    emit("bullscows", "Bulls and Cows %d Digits" % n, "Logic", "Digit deduction",
         "1", min(5, n - 1),
         "Deduce an %d-digit secret from bull and cow counts." % n, count=n)

LOGICGRID = ["Detective", "Dinner Party", "Race Results", "Office", "School",
             "Zoo", "Festival", "Hotel", "Voyage", "Bakery"]
for i, theme in enumerate(LOGICGRID):
    for tier, diff in [("Standard", 2), ("Advanced", 4)]:
        emit("logicgrid", "Logic Grid %s %s" % (theme, tier), "Puzzle",
             "Logical deduction", "1", diff,
             "%s constraint puzzle set at a %s." % (tier, theme.lower()),
             theme=theme, variant=i, difficulty=diff)

MATH = [("Countdown Numbers", 0, 3), ("24 Game", 1, 3), ("Factor Game", 2, 2),
        ("Prime Hunt", 3, 2), ("Mental Arithmetic Drill", 4, 1),
        ("Binary Conversion Race", 5, 2), ("Collatz Race", 6, 2),
        ("Nim Sum Trainer", 7, 3), ("Fizz Buzz Duel", 8, 1),
        ("Equation Builder", 9, 3), ("Fraction Match", 10, 2),
        ("Number Sequence", 11, 3), ("Modular Clock", 12, 3),
        ("Dice Probability Quiz", 13, 2), ("Estimation Challenge", 14, 2),
        ("Magic Triangle", 15, 3), ("Cryptarithm", 16, 4),
        ("Base Conversion Puzzle", 17, 3), ("Greatest Common Divisor Duel", 18, 2),
        ("Pi Digit Memory", 19, 2)]
for name, var, diff in MATH:
    emit("mathdrill", name, "Maths", "Numeric reasoning", "1", diff,
         "Numeric drill: %s." % name.lower(), variant=var)

QUIZ_TOPICS = ["General Knowledge", "Geography", "History", "Science", "Film",
               "Music", "Sport", "Literature", "Computing", "Mythology",
               "Art", "Nature", "Space", "Food", "Language", "Inventions"]
for i, topic in enumerate(QUIZ_TOPICS):
    for tier, diff in [("Easy", 1), ("Hard", 3), ("Expert", 5)]:
        emit("quiz", "%s Quiz %s" % (topic, tier), "Trivia", "Knowledge recall",
             "1", diff, "%s tier %s questions." % (tier, topic.lower()),
             theme=topic, variant=i, difficulty=diff)

REACTION = [("Reaction Timer", 0, 1), ("N-Back", 1, 4), ("Digit Span", 2, 2),
            ("Pattern Recall", 3, 2), ("Colour Match Stroop", 4, 2),
            ("Aim Trainer", 5, 2), ("Chimp Test", 6, 3), ("Visual Span", 7, 2),
            ("Sequence Repeat", 8, 2), ("Card Memory Sprint", 9, 4),
            ("Audio Memory", 10, 3), ("Rhythm Tap", 11, 2),
            ("Peripheral Vision", 12, 3), ("Flick Shot", 13, 2),
            ("Multi Target", 14, 3), ("Tracking Drill", 15, 3),
            ("Sound Cue", 16, 2), ("Colour Change", 17, 2)]
for name, var, diff in REACTION:
    emit("reaction", name, "Skill", "Memory and reflex", "1", diff,
         "Precision drill: %s." % name.lower(), variant=var)
for name, var in [("Simon", 0), ("Simon Six Colour", 1), ("Simon Eight Colour", 2),
                  ("Simon Reverse", 3), ("Simon Silent", 4), ("Simon Speed", 5)]:
    emit("simon", name, "Skill", "Sequence memory", "1", 2,
         "Repeat a growing colour-and-tone sequence.", variant=var)


# =========================================================================
# RPG, SIMULATION, MUSIC
# =========================================================================

DUNGEON_THEMES = [("Catacombs", "undead and narrow crypt corridors"),
                  ("Caverns", "flooded caves and cave-ins"),
                  ("Sewers", "poison hazards and rats"),
                  ("Ice Caves", "slippery floors and frost damage"),
                  ("Volcano", "lava flows and heat pressure"),
                  ("Sky Temple", "wind gusts and long falls"),
                  ("Derelict Ship", "vacuum breaches and drones"),
                  ("Forest Depths", "dense growth that blocks line of sight")]
for ti, (theme, why) in enumerate(DUNGEON_THEMES):
    for depth, diff in [(5, 2), (10, 3), (20, 4), (50, 5)]:
        emit("dungeon", "%s Crawl %dF" % (theme, depth), "RPG",
             "Procedural exploration", "1", diff,
             "A %d-floor descent through %s." % (depth, why),
             theme=theme, level=depth, variant=ti)

TEXTADV = [("Text Adventure", 0, 3), ("Colossal Cave Lite", 1, 3), ("Zork Lite", 2, 3),
           ("Escape the Room", 3, 3), ("Quest for the Grail", 4, 2),
           ("Choose Your Path", 5, 2), ("Vampire Castle", 6, 4),
           ("Zombie Survival", 7, 3), ("Space Trader", 8, 3),
           ("Monster Arena", 9, 3), ("Wizard Duel", 10, 2),
           ("Gladiator Manager", 11, 3), ("Merchant Sim", 12, 3),
           ("Tower Climb RPG", 13, 3), ("Pet Monster Battler", 14, 3),
           ("Dungeon of Doom", 15, 3), ("Rogue", 16, 4), ("NetHack Lite", 17, 5),
           ("Angband Lite", 18, 5), ("Hunt the Wumpus", 19, 2)]
for name, var, diff in TEXTADV:
    emit("textadv", name, "RPG", "Narrative exploration", "1", diff,
         "Adventure: %s." % name.lower(), variant=var)

SIM = [("Hammurabi", 0, 2), ("Lemonade Stand", 1, 2), ("Oregon Trail", 2, 3),
       ("Star Trek 1971", 3, 3), ("Drug Wars", 4, 2), ("Sim Farm", 5, 3),
       ("Sim City Lite", 6, 4), ("Railroad Tycoon Lite", 7, 4),
       ("Civilisation Lite", 8, 5), ("Risk", 9, 3), ("Stock Market Sim", 10, 3),
       ("Elevator Simulator", 11, 3), ("Traffic Light Sim", 12, 3),
       ("Ant Colony Sim", 13, 4), ("Epidemic Sim", 14, 3),
       ("Ecosystem Balance", 15, 3), ("Power Grid Sim", 16, 4),
       ("Airport Control", 17, 4), ("Restaurant Tycoon", 18, 3),
       ("Space Colony", 19, 4), ("Wa-Tor", 20, 3), ("Bridge Builder", 21, 4)]
for name, var, diff in SIM:
    emit("sim", name, "Simulation", "Systems management", "1", diff,
         "Simulation: %s." % name.lower(), variant=var)

for i, rule in enumerate([30, 54, 90, 110, 150, 182, 22, 60]):
    emit("automata", "Elementary Automaton Rule %d" % rule, "Simulation",
         "Cellular automaton", "1", 2,
         "One-dimensional automaton evolving under rule %d." % rule,
         variant=0, count=rule)
LIFE = [("Conway's Game of Life", 0), ("Life Variant HighLife", 1),
        ("Life Variant Day and Night", 2), ("Life Variant Seeds", 3),
        ("Life Variant Brian's Brain", 4), ("Life Variant Maze", 5),
        ("Life Variant Coral", 6), ("Langton's Ant", 7)]
for name, var in LIFE:
    emit("automata", name, "Simulation", "Cellular automaton", "1", 2,
         "Two-dimensional automaton: %s." % name.lower(), variant=1, count=var)

for i, theme in enumerate(["Mine", "Bakery", "Farm", "Factory", "Laboratory",
                           "Galaxy", "Dungeon", "Garden", "Kingdom", "Reactor"]):
    emit("idle", "Idle %s" % theme, "Simulation", "Incremental growth", "1", 2,
         "Incremental %s management with prestige resets." % theme.lower(),
         theme=theme, variant=i)
for i, theme in enumerate(["Forest", "Desert", "Space", "Castle", "Cyber", "Underwater"]):
    emit("towerdefence", "Tower Defence %s" % theme, "Strategy", "Lane defence",
         "1", 3, "Place towers along a %s route and survive the waves." % theme.lower(),
         theme=theme, variant=i)

MUSIC = [("Virtual Piano", 0, 1), ("Rhythm Runner", 1, 3), ("Note Trainer", 2, 2),
         ("Interval Ear Training", 3, 3), ("Chord Builder", 4, 3),
         ("Scale Practice", 5, 2), ("Drum Machine", 6, 2), ("Melody Memory", 7, 3),
         ("Perfect Pitch Test", 8, 4), ("Metronome Challenge", 9, 2),
         ("Song Sequencer", 10, 3)]
for name, var, diff in MUSIC:
    emit("piano", name, "Music", "Audio and timing", "1", diff,
         "Music tool: %s." % name.lower(), variant=var)


# =========================================================================
# Pad to exactly TARGET using further real configurations of built families
# =========================================================================
PAD_SOURCES = [
    ("sudoku",      lambda i: ("Sudoku Daily %d" % (i + 1), "Puzzle",
                               "Latin square constraint", 3,
                               "A fresh 9x9 sudoku, difficulty tier %d." % (1 + i % 4),
                               dict(size=9, variant=0, difficulty=1 + i % 4))),
    ("minesweeper", lambda i: ("Minesweeper Daily %d" % (i + 1), "Puzzle",
                               "Constraint deduction", 3,
                               "A fresh 16x16 field with %d mines." % (30 + i),
                               dict(width=16, height=16, count=30 + i, variant=0))),
    ("maze",        lambda i: ("Maze Daily %d" % (i + 1), "Puzzle", "Path finding", 2,
                               "A fresh maze, algorithm %d." % (i % 10),
                               dict(size=21, variant=i % 10))),
    ("quiz",        lambda i: ("Mixed Quiz Round %d" % (i + 1), "Trivia",
                               "Knowledge recall", 2,
                               "A mixed-topic round of ten questions.",
                               dict(theme="General Knowledge", variant=0,
                                    difficulty=1 + i % 3))),
]
pad = 0
while len(entries) < TARGET:
    fam, mk = PAD_SOURCES[pad % len(PAD_SOURCES)]
    title, genre, mech, diff, blurb, params = mk(pad // len(PAD_SOURCES))
    emit(fam, title, genre, mech, "1", diff, blurb, **params)
    pad += 1

if len(entries) > TARGET:
    raise SystemExit("over target: %d entries, trim a block" % len(entries))

assert len(entries) == TARGET, len(entries)


# =========================================================================
# Which families actually have an engine, and does that engine honour the
# parameters its entries specify?
#
# A family is only counted as playable when either it reads its GParams
# (its source carries the GIC:PARAMETERISED marker) or it has exactly one
# catalogue entry, in which case there is nothing to configure. That stops
# a single hard-coded engine from claiming credit for thirty variants it
# would silently ignore.
# =========================================================================
reg_src = open(os.path.join(ROOT, "src", "registry.c")).read()
fam_to_fn = dict(re.findall(r'\{"([a-z0-9]+)",\s*fam_(\w+)\}', reg_src))
built = set(fam_to_fn)

marked = set()
for fam, fn in fam_to_fn.items():
    for path in glob.glob(os.path.join(ROOT, "src", "games", "*.c")):
        src = open(path).read()
        if re.search(r'\bvoid fam_%s\s*\(' % re.escape(fn), src):
            if "GIC:PARAMETERISED" in src:
                marked.add(fam)
            break

counts = {}
for e in entries:
    counts[e["family"]] = counts.get(e["family"], 0) + 1

for e in entries:
    fam = e["family"]
    ok = fam in built and (fam in marked or counts[fam] == 1)
    e["implemented"] = 1 if ok else 0
    e["param_aware"] = 1 if fam in marked else 0

families = {}
for e in entries:
    families.setdefault(e["family"], 0)
    families[e["family"]] += 1

playable = sum(e["implemented"] for e in entries)
genres = {}
for e in entries:
    genres[e["genre"]] = genres.get(e["genre"], 0) + 1


# =========================================================================
# Emit
# =========================================================================
with open(os.path.join(ROOT, "data", "catalog.json"), "w", encoding="utf-8") as f:
    json.dump({"count": len(entries), "playable": playable,
               "families": dict(sorted(families.items())),
               "genres": dict(sorted(genres.items(), key=lambda kv: -kv[1])),
               "games": entries}, f, indent=1, ensure_ascii=False)


def cstr(s):
    return '"' + str(s).replace("\\", "\\\\").replace('"', '\\"') + '"'


lines = ["/* catalog_data.c - GENERATED by tools/gen_catalog.py. Do not edit. */",
         '#include "engine.h"', "",
         "const CatalogEntry CATALOG[] = {"]
for e in entries:
    p = e["params"]
    lines.append("    {%s, %s, %s, %s, %s, %d, %s, %s, %d,\n     {%d,%d,%d,%d,%d,%d,%d,%d, %s, %s}}," % (
        cstr(e["slug"]), cstr(e["title"]), cstr(e["genre"]), cstr(e["mechanic"]),
        cstr(e["players"]), e["difficulty"], cstr(e["blurb"]), cstr(e["family"]),
        e["implemented"],
        p["width"], p["height"], p["size"], p["variant"], p["count"],
        p["level"], p["difficulty"], p["players"],
        cstr(p["theme"]), cstr(e["title"])))
lines += ["};", "",
          "const int CATALOG_COUNT = (int)(sizeof(CATALOG) / sizeof(CATALOG[0]));", ""]
with open(os.path.join(ROOT, "src", "catalog_data.c"), "w") as f:
    f.write("\n".join(lines))

with open(os.path.join(ROOT, "web", "js", "catalog.js"), "w", encoding="utf-8") as f:
    f.write("/* GENERATED by tools/gen_catalog.py. Do not edit. */\n")
    f.write("window.CATALOG = ")
    json.dump(entries, f, separators=(",", ":"), ensure_ascii=False)
    f.write(";\n")

done_fams = sorted(f for f in families if f in built and (f in marked or families[f] == 1))
todo_fams = sorted(f for f in families if f not in built)
stub_fams = sorted(f for f in families
                   if f in built and f not in marked and families[f] > 1)

print("catalogue: %d entries, %d playable" % (len(entries), playable))
print("  families done            %3d  (%d entries)"
      % (len(done_fams), sum(families[f] for f in done_fams)))
print("  engines needing params   %3d  (%d entries)"
      % (len(stub_fams), sum(families[f] for f in stub_fams)))
print("  engines not yet written  %3d  (%d entries)"
      % (len(todo_fams), sum(families[f] for f in todo_fams)))
if stub_fams:
    print("\nbuilt but still ignoring their parameters:")
    for m in stub_fams:
        print("   %-14s %3d entries" % (m, families[m]))
if todo_fams:
    print("\nnot yet written:")
    for m in todo_fams:
        print("   %-14s %3d entries" % (m, families[m]))
