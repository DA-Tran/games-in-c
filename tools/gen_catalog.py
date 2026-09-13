#!/usr/bin/env python3
"""Generate the 1000-game catalog.

The catalog is assembled from real game families, each expanded along genuine
variation axes (board size, rule variants, regional rulesets, themed word
lists). Every entry carries enough of a spec that it can actually be built
later: genre, core mechanic, player count, difficulty and a one-line blurb.

Outputs:
    data/catalog.json    -- consumed by the browser app
    src/catalog_data.c   -- compiled into the terminal menu
"""

import json
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

TARGET = 1000

# Slugs that have a real, playable C + JS implementation in this repo.
IMPLEMENTED = {
    "tictactoe", "connect-four", "reversi", "gomoku", "checkers", "mancala",
    "nim", "dots-and-boxes", "battleship", "mastermind",
    "minesweeper", "sudoku", "fifteen-puzzle", "lights-out", "flood-it",
    "sokoban", "memory-match", "game-2048", "hanoi", "nonogram",
    "snake", "tetris", "pong", "breakout", "space-invaders", "dino-run",
    "flappy", "frogger", "pacman", "asteroids",
    "blackjack", "video-poker", "war", "go-fish", "yahtzee", "pig",
    "slot-machine", "higher-lower",
    "hangman", "wordle", "anagram", "typing-test", "guess-number",
    "bulls-cows", "rock-paper-scissors", "simon", "snakes-and-ladders",
    "dungeon-crawl", "virtual-piano",
}

entries = []
seen = set()


def slugify(name):
    s = name.lower()
    s = s.replace("'", "").replace("&", "and")
    s = re.sub(r"[^a-z0-9]+", "-", s).strip("-")
    return s


def add(title, genre, mechanic, players, difficulty, blurb, slug=None):
    sl = slug or slugify(title)
    if sl in seen:
        return False
    seen.add(sl)
    entries.append({
        "slug": sl,
        "title": title,
        "genre": genre,
        "mechanic": mechanic,
        "players": players,
        "difficulty": max(1, min(5, difficulty)),
        "blurb": blurb,
        "implemented": 1 if sl in IMPLEMENTED else 0,
    })
    return True


# --------------------------------------------------------------------------
# 1. Flagship implementations (these are the ones that actually ship playable)
# --------------------------------------------------------------------------
FLAGSHIP = [
    ("Tic Tac Toe", "Board", "Perfect-information placement", "1-2", 1,
     "Three in a row on 3x3, with an unbeatable minimax opponent.", "tictactoe"),
    ("Connect Four", "Board", "Gravity-fed alignment", "1-2", 2,
     "Drop discs into columns and build four in a line before the AI does.", "connect-four"),
    ("Reversi", "Board", "Flanking capture", "1-2", 3,
     "Outflank enemy discs to flip them; most pieces at the end wins.", "reversi"),
    ("Gomoku", "Board", "Free-style five-in-a-row", "1-2", 3,
     "Five in a row on a 15x15 grid against a threat-scoring AI.", "gomoku"),
    ("Checkers", "Board", "Diagonal capture with promotion", "1-2", 3,
     "Full draughts with forced jumps, multi-captures and kings.", "checkers"),
    ("Mancala", "Board", "Seed sowing", "1-2", 2,
     "Kalah rules: sow seeds, earn free turns, capture across the board.", "mancala"),
    ("Nim", "Board", "Combinatorial subtraction", "1-2", 2,
     "Take from any heap; the AI plays the perfect nim-sum strategy.", "nim"),
    ("Dots and Boxes", "Board", "Edge claiming", "1-2", 2,
     "Draw lines, close boxes, chain captures and count territory.", "dots-and-boxes"),
    ("Battleship", "Board", "Hidden-grid deduction", "1", 2,
     "Hunt an enemy fleet against an AI that hunts yours with parity search.", "battleship"),
    ("Mastermind", "Logic", "Code deduction", "1", 2,
     "Crack the hidden colour code from exact and partial-match feedback.", "mastermind"),
    ("Minesweeper", "Puzzle", "Constraint deduction", "1", 3,
     "Classic sweeper with flood-fill reveal, flags and a safe first click.", "minesweeper"),
    ("Sudoku", "Puzzle", "Latin square constraint", "1", 3,
     "Generated puzzles with a guaranteed unique solution and four difficulties.", "sudoku"),
    ("Fifteen Puzzle", "Puzzle", "Sliding tile", "1", 2,
     "Slide tiles into order from a guaranteed-solvable shuffle.", "fifteen-puzzle"),
    ("Lights Out", "Puzzle", "Toggle parity", "1", 3,
     "Switch every light off; each press flips its neighbours too.", "lights-out"),
    ("Flood It", "Puzzle", "Flood fill", "1", 2,
     "Recolour from the corner and claim the whole board within a move budget.", "flood-it"),
    ("Sokoban", "Puzzle", "Crate pushing", "1", 3,
     "Push every crate onto a goal without wedging yourself into a dead end.", "sokoban"),
    ("Memory Match", "Puzzle", "Pair recall", "1", 1,
     "Flip cards two at a time and clear the board from memory.", "memory-match"),
    ("2048", "Puzzle", "Merge sliding", "1", 2,
     "Slide and merge powers of two toward the 2048 tile.", "game-2048"),
    ("Towers of Hanoi", "Puzzle", "Recursive stacking", "1", 2,
     "Move the tower one disc at a time, never stacking large on small.", "hanoi"),
    ("Nonogram", "Puzzle", "Line-clue painting", "1", 3,
     "Paint the hidden picture from run-length clues on every row and column.", "nonogram"),
    ("Snake", "Arcade", "Growth and collision", "1", 1,
     "Eat, grow, and avoid your own tail as the speed ramps up.", "snake"),
    ("Tetris", "Arcade", "Falling-block packing", "1", 3,
     "Seven-bag randomiser, wall kicks, hard drop and progressive gravity.", "tetris"),
    ("Pong", "Arcade", "Paddle deflection", "1-2", 1,
     "The original rally, with angle-of-impact physics and a tracking AI.", "pong"),
    ("Breakout", "Arcade", "Brick destruction", "1", 2,
     "Clear the wall across multiple layouts with paddle-english control.", "breakout"),
    ("Space Invaders", "Arcade", "Formation shooter", "1", 2,
     "Hold the line as the formation accelerates and the shields erode.", "space-invaders"),
    ("Dino Run", "Arcade", "Endless runner", "1", 1,
     "One-button jumping over an endlessly accelerating cactus field.", "dino-run"),
    ("Flappy", "Arcade", "Gap navigation", "1", 2,
     "Tap against gravity and thread an unforgiving sequence of gaps.", "flappy"),
    ("Frogger", "Arcade", "Timing and lanes", "1", 3,
     "Cross traffic and a moving river without drowning or being flattened.", "frogger"),
    ("Pacman", "Arcade", "Maze pursuit", "1", 3,
     "Clear the maze with four ghosts running distinct chase personalities.", "pacman"),
    ("Asteroids", "Arcade", "Inertial shooter", "1", 3,
     "Momentum-based flight, screen wrap and splitting rocks.", "asteroids"),
    ("Blackjack", "Cards", "Card counting and odds", "1", 2,
     "Hit, stand, double and split against dealer-stands-on-17 rules.", "blackjack"),
    ("Video Poker", "Cards", "Draw poker payout", "1", 2,
     "Jacks or Better with a hold-and-draw round and a real paytable.", "video-poker"),
    ("War", "Cards", "High-card comparison", "1", 1,
     "The pure-chance card duel, including full war resolution on ties.", "war"),
    ("Go Fish", "Cards", "Set collection", "1", 1,
     "Ask for ranks, build books and track what the opponent has asked for.", "go-fish"),
    ("Yahtzee", "Dice", "Category scoring", "1", 2,
     "Three rolls per turn across all thirteen categories with bonuses.", "yahtzee"),
    ("Pig", "Dice", "Press-your-luck", "1-2", 1,
     "Roll to build a turn total, but a one wipes it out.", "pig"),
    ("Slot Machine", "Chance", "Reel matching", "1", 1,
     "Three reels, weighted symbols and a transparent paytable.", "slot-machine"),
    ("Higher or Lower", "Chance", "Sequential prediction", "1", 1,
     "Call the next card and push a streak as the odds tighten.", "higher-lower"),
    ("Hangman", "Word", "Letter deduction", "1", 1,
     "Guess the word letter by letter across several themed dictionaries.", "hangman"),
    ("Wordle", "Word", "Positional feedback", "1", 2,
     "Six guesses, green and yellow feedback, with a validated word list.", "wordle"),
    ("Anagram", "Word", "Letter rearrangement", "1", 2,
     "Unscramble against the clock with a hint system that costs points.", "anagram"),
    ("Typing Test", "Skill", "Input speed", "1", 1,
     "Words per minute and accuracy measured over timed passages.", "typing-test"),
    ("Guess the Number", "Logic", "Binary search", "1", 1,
     "Narrow the range from higher/lower hints in as few guesses as possible.", "guess-number"),
    ("Bulls and Cows", "Logic", "Digit deduction", "1", 2,
     "Deduce the secret number from bull and cow counts.", "bulls-cows"),
    ("Rock Paper Scissors", "Chance", "Simultaneous selection", "1", 1,
     "Best-of series against an AI that learns your frequency bias.", "rock-paper-scissors"),
    ("Simon", "Skill", "Sequence memory", "1", 2,
     "Repeat a growing colour-and-tone sequence until you slip.", "simon"),
    ("Snakes and Ladders", "Board", "Dice racing", "1-2", 1,
     "The classic climb, with animated movement along snakes and ladders.", "snakes-and-ladders"),
    ("Dungeon Crawl", "RPG", "Procedural exploration", "1", 4,
     "A compact roguelike: generated dungeons, combat, loot and descent.", "dungeon-crawl"),
    ("Virtual Piano", "Music", "Note triggering", "1", 1,
     "Playable keyboard with visual keys, note names and audible tones.", "virtual-piano"),
]

for t, g, m, p, d, b, s in FLAGSHIP:
    add(t, g, m, p, d, b, slug=s)


# --------------------------------------------------------------------------
# 2. Named classics, grouped by family
# --------------------------------------------------------------------------

ABSTRACT = [
    ("Hex", "Connection race on a rhombic hex board; one side always wins.", 3),
    ("Havannah", "Build a ring, bridge or fork on a hexagonal board.", 4),
    ("Y", "Connect all three sides of a triangular hex board.", 3),
    ("TwixT", "Place pegs and link them into an unbroken bridge chain.", 4),
    ("Game of Amazons", "Move queens and burn squares until someone is entombed.", 4),
    ("Lines of Action", "Gather every piece into one connected group.", 3),
    ("Breakthrough", "Push a single pawn through the enemy line.", 2),
    ("Clobber", "Capture adjacent enemy stones until one side cannot move.", 3),
    ("Konane", "Hawaiian jumping capture; stalemate loses.", 3),
    ("Surakarta", "Capture by looping pieces around the board's outer rails.", 3),
    ("Quoridor", "Race to the far wall while dropping fences in the way.", 3),
    ("Abalone", "Shove enemy marbles off the hexagonal rim.", 3),
    ("Nine Men's Morris", "Form mills, remove enemy men, reduce them to two.", 2),
    ("Three Men's Morris", "The compact ancestor of Morris on a 3x3 grid.", 1),
    ("Twelve Men's Morris", "Morris with diagonals and a fuller board.", 2),
    ("Alquerque", "The medieval jumping-capture ancestor of draughts.", 2),
    ("Fanorona", "Malagasy capture by approach and withdrawal.", 4),
    ("Halma", "Move your whole camp into the opposite corner.", 2),
    ("Chinese Checkers", "Star-board hopping race for up to six camps.", 2),
    ("Pentago", "Place a marble, then rotate a quadrant.", 3),
    ("Gobblet", "Nest pieces and swallow the opponent's to make a line.", 3),
    ("Score Four", "Connect four in three dimensions on a peg grid.", 3),
    ("Quarto", "Your opponent chooses the piece you must place.", 4),
    ("Blokus", "Fit polyominoes that touch only at corners.", 3),
    ("Hnefatafl", "Asymmetric Viking siege: king escapes or is surrounded.", 4),
    ("Brandubh", "The small 7x7 Irish tafl variant.", 3),
    ("Tablut", "Sami tafl on a 9x9 board with a fortified centre.", 3),
    ("Ard Ri", "Scottish tafl with tight escape conditions.", 3),
    ("Alea Evangelii", "The vast 19x19 tafl variant.", 5),
    ("Go", "Surround territory; the deepest abstract of them all.", 5),
    ("Go 9x9", "Full Go rules on the beginner board.", 4),
    ("Go 13x13", "Mid-size Go for a faster full-strategy game.", 5),
    ("Atari Go", "Capture-the-first-stone Go, the standard teaching game.", 2),
    ("Sprouts", "Topological pencil game of dots and curves.", 3),
    ("Brussels Sprouts", "The deterministic cousin of Sprouts.", 2),
    ("Hackenbush", "Chop edges and drop everything no longer grounded.", 3),
    ("Chomp", "Bite the poset; whoever eats the poisoned square loses.", 2),
    ("Sim", "Avoid completing a triangle in your own colour.", 2),
    ("Col", "Graph colouring as a combinatorial game.", 3),
    ("Snort", "Colouring game where neighbours must agree.", 3),
]
for name, blurb, diff in ABSTRACT:
    add(name, "Abstract", "Positional strategy", "1-2", diff, blurb)

CHESS = [
    ("Chess", "Full standard rules: castling, en passant, promotion.", 4),
    ("Chess960", "Fischer random back-rank shuffle with castling rules.", 4),
    ("King of the Hill", "Win by marching your king to the four centre squares.", 3),
    ("Three-Check Chess", "Check the enemy king three times to win.", 3),
    ("Atomic Chess", "Captures detonate, destroying the surrounding squares.", 4),
    ("Horde Chess", "One side fields a wall of pawns against a full army.", 4),
    ("Racing Kings", "No checks allowed; first king to the eighth rank wins.", 3),
    ("Antichess", "Captures are compulsory and losing everything wins.", 3),
    ("Crazyhouse", "Captured pieces change sides and can be dropped.", 5),
    ("Minichess 5x5", "Gardner's miniature with the full piece set.", 3),
    ("Los Alamos Chess", "The 6x6 bishop-less board used by MANIAC I.", 3),
    ("Dark Chess", "Fog of war: you see only what you attack.", 4),
    ("Knightmate", "The knight is royal; the king is an ordinary piece.", 4),
    ("Extinction Chess", "Lose a whole piece type and you lose the game.", 3),
    ("Progressive Chess", "Turn one is one move, turn two is two, and so on.", 4),
    ("Shogi", "Japanese chess with piece drops and promotion zones.", 5),
    ("Minishogi", "Shogi on 5x5 with a compact army.", 4),
    ("Xiangqi", "Chinese chess across the river, with cannons and a palace.", 5),
    ("Janggi", "Korean chess with diagonal palace moves.", 5),
    ("Makruk", "Thai chess with short-range pieces and its own promotion.", 4),
    ("Shatranj", "The medieval Persian ancestor with the weak ferz.", 4),
    ("Courier Chess", "The 12x8 medieval European expansion.", 5),
    ("Capablanca Chess", "10x8 board adding the archbishop and chancellor.", 5),
    ("Grand Chess", "10x10 with a broad opening array.", 5),
    ("Omega Chess", "Adds wizards and champions on a 10x10 board.", 5),
    ("Cylinder Chess", "The a and h files wrap around.", 4),
    ("Alice Chess", "Two boards; every move teleports the piece to the other.", 5),
    ("Kriegspiel", "Blind chess arbitrated by an umpire.", 5),
]
for name, blurb, diff in CHESS:
    add(name, "Chess", "Piece movement strategy", "1-2", diff, blurb)

DRAUGHTS = [
    ("International Draughts", "10x10 with flying kings and long captures.", 4),
    ("Russian Draughts", "Promotion mid-capture and flying kings.", 3),
    ("Brazilian Draughts", "International rules compressed onto 8x8.", 3),
    ("Turkish Draughts", "Orthogonal movement and capture.", 4),
    ("Canadian Checkers", "The 12x12 board with 30 pieces a side.", 5),
    ("Italian Draughts", "Men cannot capture kings; strict capture priority.", 3),
    ("Spanish Draughts", "Backwards-capturing kings on 8x8.", 3),
    ("Pool Checkers", "American pool rules with flying kings.", 3),
    ("Suicide Checkers", "Lose all your pieces to win.", 2),
    ("Dameo", "Modern draughts with linear sliding moves.", 4),
    ("Frisian Draughts", "Orthogonal capture alongside diagonal movement.", 5),
    ("Armenian Draughts", "Tama rules mixing orthogonal and diagonal capture.", 4),
]
for name, blurb, diff in DRAUGHTS:
    add(name, "Board", "Capture and promotion", "1-2", diff, blurb)

MANCALA = [
    ("Kalah", "The standard two-row sowing game with stores.", 2),
    ("Oware", "Akan sowing with capture on twos and threes.", 3),
    ("Congkak", "Malay sowing with simultaneous opening moves.", 3),
    ("Bao", "The deep Swahili four-row game.", 5),
    ("Sungka", "Filipino sowing on seven houses a side.", 3),
    ("Omweso", "Ugandan four-row capture game.", 5),
    ("Toguz Kumalak", "Kazakh sowing with tuzdik capture holes.", 4),
    ("Pallanguzhi", "Tamil sowing with pit-by-pit capture.", 3),
    ("Ayo", "Yoruba sowing variant of Oware.", 3),
    ("Dakon", "Javanese sowing with continuous relay laps.", 3),
]
for name, blurb, diff in MANCALA:
    add(name, "Board", "Seed sowing", "1-2", diff, blurb)

CARDS = [
    ("Texas Hold'em", "Two hole cards, five community cards, four betting rounds.", 4),
    ("Omaha Hold'em", "Four hole cards, exactly two must play.", 5),
    ("Five Card Draw", "The classic draw poker round.", 3),
    ("Seven Card Stud", "Up and down cards across five betting streets.", 4),
    ("Razz", "Seven-card stud played for the lowest hand.", 4),
    ("Deuces Wild", "Video poker where every two is a wild card.", 2),
    ("Joker Poker", "Video poker with a single wild joker.", 2),
    ("Double Bonus Poker", "Aggressive four-of-a-kind paytable.", 3),
    ("Baccarat", "Punto banco with the standard third-card rules.", 2),
    ("Pontoon", "British blackjack with twist, stick and buy.", 2),
    ("Red Dog", "Bet on whether the next card falls between two others.", 1),
    ("Casino War", "Casino-rules War with a surrender option on ties.", 1),
    ("Hearts", "Trick-avoidance with passing and shooting the moon.", 3),
    ("Spades", "Bid your tricks and make your contract with a partner.", 3),
    ("Euchre", "Trump-calling trick play with the right and left bower.", 4),
    ("Whist", "The plain-trick ancestor of bridge.", 3),
    ("Contract Bridge", "Full auction and declarer play.", 5),
    ("Pinochle", "Melding and trick play with a 48-card deck.", 5),
    ("Cribbage", "Pegging and the fifteen-two-fifteen-four count.", 4),
    ("Gin Rummy", "Knock and gin with deadwood scoring.", 3),
    ("Rummy 500", "Draw, meld and lay off toward 500 points.", 3),
    ("Canasta", "Build canastas from wild-heavy melds.", 4),
    ("Crazy Eights", "Match rank or suit; eights are wild.", 1),
    ("Old Maid", "Shed pairs and avoid the odd queen.", 1),
    ("Slapjack", "Reaction game for the jack on top of the pile.", 1),
    ("Snap", "Race to call matching cards.", 1),
    ("Beggar My Neighbour", "Fully deterministic card duel with penalty cards.", 1),
    ("Durak", "Russian attack-and-defend shedding game.", 4),
    ("President", "Climbing shedding game with social ranks.", 2),
    ("Cheat", "Bluff your discards and call out liars.", 2),
    ("Egyptian Ratscrew", "Slap-driven challenge variant of War.", 2),
    ("Kemps", "Partner signalling and four-of-a-kind calls.", 3),
    ("Oh Hell", "Bid exactly your tricks as the hand size changes.", 3),
    ("Nertz", "Real-time competitive solitaire.", 4),
    ("Casino", "Capture and build cards from a table layout.", 3),
    ("Scopa", "Italian capture game with the settebello.", 3),
    ("Briscola", "Italian trick play with a trump suit.", 2),
    ("Skat", "German three-player bidding and declarer play.", 5),
    ("Schafkopf", "Bavarian partnership trick game.", 5),
    ("Tarot Nouveau", "French tarot with the trump suit and the excuse.", 5),
    ("Napoleon", "Trick-taking with a declaration of tricks won.", 3),
    ("Sevens", "Build suit runs outward from the sevens.", 1),
    ("Golf Card Game", "Lowest visible grid of cards wins.", 2),
    ("Ninety-Nine", "Additive card game that must not exceed 99.", 2),
    ("Blackjack Switch", "Two hands with the option to swap top cards.", 3),
    ("Spit", "Real-time two-player speed shedding.", 2),
    ("Garbage", "Fill a ten-card layout in numeric order.", 1),
    ("Kings Corner", "Solitaire-style shedding around a cross layout.", 2),
]
for name, blurb, diff in CARDS:
    add(name, "Cards", "Trick, draw or shedding play", "1-4", diff, blurb)

SOLITAIRE = [
    ("Klondike Solitaire", "The default patience: seven piles and four foundations.", 2),
    ("Spider Solitaire One Suit", "Build descending runs with a single suit.", 2),
    ("Spider Solitaire Two Suit", "Spider with two suits in play.", 3),
    ("Spider Solitaire Four Suit", "The full-difficulty Spider game.", 5),
    ("FreeCell", "Open-information patience; almost every deal is solvable.", 3),
    ("Pyramid Solitaire", "Pair cards summing to thirteen to clear the pyramid.", 2),
    ("TriPeaks", "Chain one-away cards across three peaks.", 2),
    ("Golf Solitaire", "Run up and down the waste pile to clear the tableau.", 2),
    ("Yukon Solitaire", "Move buried groups without ordering restrictions.", 3),
    ("Canfield", "Reserve-driven patience with a fixed foundation rank.", 3),
    ("Scorpion", "Spider-like building with fully free moves.", 4),
    ("Forty Thieves", "Two decks, strict single-card moves.", 4),
    ("Aces Up", "Discard low cards of matching suit to leave four aces.", 2),
    ("Accordion", "Compress the whole deck into a single pile.", 4),
    ("Baker's Dozen", "Thirteen columns, no empty-column moves.", 3),
    ("Beleaguered Castle", "Free-cell-free patience around four foundations.", 3),
    ("Clock Patience", "Pure chance patience dealt as a clock face.", 1),
    ("La Belle Lucie", "Fans of three cards with limited redeals.", 4),
    ("Russian Solitaire", "Yukon with suit-strict building.", 5),
    ("Napoleon at St Helena", "Two-deck patience with a long tableau.", 4),
    ("Eight Off", "FreeCell with eight reserve cells.", 2),
    ("Penguin", "FreeCell relative with a fixed founding rank.", 3),
    ("Seahaven Towers", "Tight FreeCell variant with four cells.", 4),
    ("Calculation", "Build foundations in arithmetic progressions.", 4),
]
for name, blurb, diff in SOLITAIRE:
    add(name, "Solitaire", "Patience card sorting", "1", diff, blurb)

DICE = [
    ("Farkle", "Press your luck with scoring dice combinations.", 2),
    ("Craps", "Pass line, come bets and the point.", 3),
    ("Liar's Dice", "Bid on hidden dice across the table and call bluffs.", 3),
    ("Bunco", "Fast rolling game of rounds and target numbers.", 1),
    ("Shut the Box", "Close numbered tiles matching your roll.", 2),
    ("Cee-lo", "Three-dice street game with instant win rolls.", 1),
    ("Chuck-a-Luck", "Bet a number and roll three dice.", 1),
    ("Mexico", "Bluffing dice game with the 21 special roll.", 2),
    ("Beetle", "Roll body parts to complete a beetle first.", 1),
    ("Zonk", "Ten-thousand-point press-your-luck dice race.", 2),
    ("Yacht", "The older ancestor of Yahtzee.", 2),
    ("Balut", "Four-category dice scoring popular in Asia.", 2),
    ("Kismet", "Coloured-dice Yahtzee variant.", 2),
    ("Drop Dead", "Keep rolling until every die is dead.", 1),
    ("Threes", "Low-score dice game where threes count zero.", 1),
    ("Going to Boston", "Three-roll dice contest for the highest total.", 1),
    ("Pass the Pigs", "Score by the way the pigs land.", 1),
    ("Dudo", "The Peruvian ancestor of Liar's Dice.", 3),
    ("Sic Bo", "Three-dice casino betting on totals and combinations.", 2),
    ("Klondike Dice", "Beat the banker's five-dice hand.", 1),
]
for name, blurb, diff in DICE:
    add(name, "Dice", "Probability and press-your-luck", "1-4", diff, blurb)

PUZZLES = [
    ("Kakuro", "Cross-sums: fill runs to their clue totals without repeats.", 4),
    ("Killer Sudoku", "Sudoku with caged sums instead of starting digits.", 4),
    ("Futoshiki", "Latin square constrained by inequality signs.", 3),
    ("Hitori", "Shade duplicates without splitting the grid.", 4),
    ("Slitherlink", "Draw one closed loop satisfying every cell clue.", 4),
    ("Masyu", "Route a loop through white and black pearls.", 4),
    ("Shikaku", "Split the grid into rectangles of the clued area.", 3),
    ("Numberlink", "Connect matching numbers with non-crossing paths.", 4),
    ("Hashiwokakero", "Bridge the islands without crossing.", 3),
    ("Nurikabe", "Carve islands of the exact clued size from a sea.", 4),
    ("Heyawake", "Shade cells under room-count and run-length rules.", 5),
    ("Akari", "Light every cell without two bulbs seeing each other.", 3),
    ("Dominosa", "Rebuild a domino set from a grid of pips.", 3),
    ("Galaxies", "Partition the grid into rotationally symmetric regions.", 4),
    ("Tents and Trees", "Pair each tree with an orthogonal tent.", 3),
    ("Skyscrapers", "Latin square deduced from sightline counts.", 4),
    ("KenKen", "Latin square with arithmetic cages.", 3),
    ("Binairo", "Binary grid balanced with no three in a row.", 3),
    ("Str8ts", "Compartments must hold straight consecutive runs.", 4),
    ("Suguru", "Fill each region with 1..n, no touching repeats.", 3),
    ("Norinori", "Shade dominoes, exactly two per region.", 4),
    ("Yajilin", "Loop and shading driven by arrow clues.", 5),
    ("Shakashaka", "Place triangles to form clean rectangles.", 4),
    ("Sudoku Jigsaw", "Irregular regions instead of 3x3 boxes.", 4),
    ("Magic Square", "Arrange numbers so every line shares a sum.", 3),
    ("Word Search", "Find hidden words in eight directions.", 1),
    ("Cryptogram", "Break a substitution cipher on a quotation.", 3),
    ("Rush Hour", "Slide blocking cars to free the red one.", 3),
    ("Peg Solitaire", "Jump pegs down to a single survivor.", 3),
    ("Tangram", "Fit seven fixed pieces into a silhouette.", 3),
    ("Pentomino Packing", "Tile a rectangle with all twelve pentominoes.", 5),
    ("Knight's Tour", "Visit every square once with a single knight.", 4),
    ("Eight Queens", "Place queens so none attack another.", 3),
    ("River Crossing", "Move everything across without a fatal pairing.", 2),
    ("Water Jug Puzzle", "Measure an exact amount with fixed capacities.", 2),
    ("Mastermind Super", "Code breaking with more pegs and more colours.", 4),
    ("Sliding Blocks", "Free the target block from a crowded tray.", 3),
    ("Rubik's Cube", "Restore the cube from a scrambled state.", 5),
    ("Pocket Cube", "The 2x2 cube, solvable in eleven moves or fewer.", 3),
    ("Sokoban Advanced", "Multi-crate levels with deadlock detection.", 5),
    ("Bridges of Konigsberg", "Trace an Eulerian route if one exists.", 3),
    ("Fifteen Puzzle 5x5", "The 24-tile sliding puzzle.", 4),
    ("Mahjong Solitaire", "Clear matched free tiles from a layered pile.", 2),
    ("Bulls and Cows Words", "Word-based deduction with position feedback.", 3),
    ("Picross Colour", "Nonograms with multiple paint colours.", 4),
    ("Crossword", "Fill the grid from across and down clues.", 3),
    ("Acrostic Puzzle", "Answers spell an author and a quotation.", 4),
    ("Logic Grid Puzzle", "Match every attribute from elimination clues.", 3),
    ("Einstein's Riddle", "The five-house constraint classic.", 4),
    ("Tower Defence Puzzle", "Fixed-solution routing under a build budget.", 3),
]
for name, blurb, diff in PUZZLES:
    add(name, "Puzzle", "Logical deduction", "1", diff, blurb)

ARCADE = [
    ("Centipede", "Shoot a segmenting centipede through a mushroom field.", 3),
    ("Missile Command", "Intercept incoming warheads before the cities fall.", 3),
    ("Galaga", "Formation shooter with diving attackers and capture.", 3),
    ("Dig Dug", "Tunnel underground and inflate your pursuers.", 3),
    ("Q*bert", "Hop the pyramid and recolour every cube.", 3),
    ("Donkey Kong", "Climb girders past rolling barrels.", 3),
    ("Lunar Lander", "Burn fuel to touch down softly on rough terrain.", 3),
    ("Moon Patrol", "Drive, jump craters and shoot while scrolling.", 3),
    ("Defender", "Rescue colonists across a wrapping scrolling world.", 4),
    ("Tempest", "Shoot up a tube from the rim.", 4),
    ("Robotron", "Twin-stick survival against a swarm.", 4),
    ("Berzerk", "Shoot your way through electrified maze rooms.", 3),
    ("Bomberman", "Blast walls and trap opponents in a grid arena.", 3),
    ("Boulder Dash", "Dig for diamonds under unstable rocks.", 3),
    ("Lode Runner", "Dig traps and collect gold past the guards.", 4),
    ("Pitfall", "Timed jumping across a scrolling jungle.", 3),
    ("Kaboom", "Catch falling bombs with a stack of buckets.", 2),
    ("Breakout Multiball", "Breakout with simultaneous balls in play.", 3),
    ("Arkanoid", "Breakout with power-up capsules and boss patterns.", 3),
    ("Snake Battle", "Two snakes competing for the same board.", 2),
    ("Nibbles", "Level-based snake with walls and gates.", 2),
    ("Tron Light Cycles", "Leave a wall behind and force a crash.", 2),
    ("Doodle Jump", "Endless upward platform hopping.", 2),
    ("Icy Tower", "Build speed and climb before the screen catches you.", 3),
    ("Helicopter Game", "One-button cave flying.", 2),
    ("Jetpack Joyride", "Endless runner with hazards and pickups.", 2),
    ("Crossy Road", "Endless Frogger with procedural lanes.", 2),
    ("Whack-a-Mole", "Hit the targets before the timer expires.", 1),
    ("Space Rocks Deluxe", "Asteroids with shields, hyperspace and UFOs.", 4),
    ("Scramble", "Side-scrolling flight with fuel management.", 3),
    ("Time Pilot", "Rotational dogfight across eras.", 3),
    ("Gorillas", "Artillery duel over a city skyline.", 2),
    ("Scorched Earth", "Turn-based tank artillery with wind.", 3),
    ("Worms Lite", "Turn-based squad artillery on destructible terrain.", 4),
    ("Tank Battle", "Two-player tank duel in a maze.", 2),
    ("Air Hockey", "Fast-paced puck deflection duel.", 2),
    ("Brick Breaker Ultra", "Extended Breakout campaign with boss layouts.", 3),
    ("Pinball", "Flippers, bumpers and multiball scoring.", 3),
    ("Snake 3D Grid", "Snake constrained to a wrapping torus.", 3),
]
for name, blurb, diff in ARCADE:
    add(name, "Arcade", "Real-time reflexes", "1-2", diff, blurb)

SIM = [
    ("Conway's Game of Life", "Cellular automaton with editable seeds.", 2),
    ("Langton's Ant", "An ant whose simple rules build a highway.", 2),
    ("Wa-Tor", "Predator and prey on a toroidal ocean.", 3),
    ("Hammurabi", "Ancient resource management by the bushel.", 2),
    ("Lemonade Stand", "Price, weather and inventory over a season.", 2),
    ("Oregon Trail", "Supplies, rivers and dysentery on a long road.", 3),
    ("Hunt the Wumpus", "Deduce the beast's cave from drafts and bats.", 2),
    ("Star Trek 1971", "Grid-based quadrant hunt for Klingons.", 3),
    ("Drug Wars", "Buy low and sell high across risky boroughs.", 2),
    ("Sim Farm", "Crops, weather and a balance sheet.", 3),
    ("Sim City Lite", "Zone a grid and watch demand respond.", 4),
    ("Railroad Tycoon Lite", "Lay track and run profitable routes.", 4),
    ("Civilisation Lite", "Settle, research and expand on a small map.", 5),
    ("Risk", "Territory conquest with dice-resolved battles.", 3),
    ("Diplomacy Lite", "Simultaneous order resolution without dice.", 5),
    ("Stock Market Sim", "Trade a random-walk market against the clock.", 3),
    ("Elevator Simulator", "Schedule lifts to minimise waiting time.", 3),
    ("Traffic Light Sim", "Tune signal timings to clear congestion.", 3),
    ("Ant Colony Sim", "Pheromone-driven foraging emergence.", 4),
    ("Epidemic Sim", "SIR model with policy levers.", 3),
    ("Ecosystem Balance", "Tune populations to avoid a collapse.", 3),
    ("Power Grid Sim", "Match generation to demand without a blackout.", 4),
    ("Airport Control", "Sequence landings with limited runways.", 4),
    ("Restaurant Tycoon", "Staffing, menu pricing and queue management.", 3),
    ("Space Colony", "Life support and resource chains off-world.", 4),
]
for name, blurb, diff in SIM:
    add(name, "Simulation", "Systems management", "1", diff, blurb)

WORD = [
    ("Boggle", "Trace adjacent letters for words against the clock.", 2),
    ("Scrabble Lite", "Tile placement scored by letter values.", 4),
    ("Ghost", "Add letters without completing a word.", 2),
    ("Superghost", "Ghost where letters may be added at either end.", 3),
    ("Lexicant", "Avoid finishing a word while keeping a valid prefix.", 3),
    ("Word Ladder", "Change one letter at a time between two words.", 3),
    ("Categories", "Name things by letter across categories.", 2),
    ("Spelling Bee", "Build words from seven letters around a required one.", 2),
    ("Jotto", "Deduce a five-letter word from shared-letter counts.", 3),
    ("Hangman Phrases", "Hangman played on multi-word phrases.", 2),
    ("Crossword Mini", "A quick 5x5 crossword.", 2),
    ("Word Chain", "Each word starts with the previous word's last letter.", 1),
    ("Palindrome Hunt", "Spot and build palindromic strings.", 2),
    ("Rhyme Time", "Match rhyming pairs against a timer.", 1),
    ("Definition Match", "Pair words to their definitions.", 2),
    ("Missing Vowels", "Restore the vowels stripped from a phrase.", 2),
    ("Word Unscramble Race", "Timed anagram sprint.", 2),
    ("Countdown Letters", "Longest word from nine drawn letters.", 3),
    ("Text Twist", "Find every word in a six-letter set.", 3),
    ("Cryptic Clue Trainer", "Solve cryptic crossword clues one at a time.", 5),
]
for name, blurb, diff in WORD:
    add(name, "Word", "Vocabulary and pattern", "1-2", diff, blurb)

MATH = [
    ("Countdown Numbers", "Reach a target with six numbers and four operators.", 3),
    ("24 Game", "Make 24 from four cards.", 3),
    ("Factor Game", "Claim numbers and score their proper divisors.", 2),
    ("Prime Hunt", "Sieve primes against the clock.", 2),
    ("Mental Arithmetic Drill", "Timed four-operation practice.", 1),
    ("Binary Conversion Race", "Convert between bases at speed.", 2),
    ("Collatz Race", "Predict hailstone sequence lengths.", 2),
    ("Nim Sum Trainer", "Learn the XOR strategy behind Nim.", 3),
    ("Fizz Buzz Duel", "Two-player speed Fizz Buzz.", 1),
    ("Equation Builder", "Build a true equation from given digits.", 3),
    ("Fraction Match", "Pair equivalent fractions quickly.", 2),
    ("Number Sequence", "Predict the next term in a hidden rule.", 3),
    ("Modular Clock", "Arithmetic under a modulus.", 3),
    ("Dice Probability Quiz", "Estimate odds before rolling.", 2),
    ("Estimation Challenge", "Approximate large computations under time.", 2),
    ("Magic Triangle", "Balance sums along each edge.", 3),
    ("Cryptarithm", "Solve SEND plus MORE equals MONEY puzzles.", 4),
    ("Base Conversion Puzzle", "Decode numbers written in odd bases.", 3),
    ("Greatest Common Divisor Duel", "Race the Euclidean algorithm.", 2),
    ("Pi Digit Memory", "Recall as many digits as you can.", 2),
]
for name, blurb, diff in MATH:
    add(name, "Maths", "Numeric reasoning", "1", diff, blurb)

MEMORY_SKILL = [
    ("Reaction Timer", "Measure raw response latency.", 1),
    ("N-Back", "Track items n steps back in a stream.", 4),
    ("Digit Span", "Repeat growing digit sequences.", 2),
    ("Pattern Recall", "Reproduce a flashed grid pattern.", 2),
    ("Colour Match Stroop", "Name the ink colour, not the word.", 2),
    ("Aim Trainer", "Hit targets accurately under time pressure.", 2),
    ("Rhythm Tap", "Keep time with a generated beat.", 2),
    ("Sequence Repeat", "Longer and longer Simon-style runs.", 2),
    ("Card Memory Sprint", "Memorise a shuffled deck order.", 4),
    ("Chimp Test", "Recall numbered positions after they vanish.", 3),
    ("Visual Span", "How many items can you hold at once.", 2),
    ("Audio Memory", "Recall sequences by tone alone.", 3),
]
for name, blurb, diff in MEMORY_SKILL:
    add(name, "Skill", "Memory and reflex", "1", diff, blurb)

CHANCE = [
    ("Roulette", "European wheel with the full betting layout.", 2),
    ("Keno", "Pick numbers and match the draw.", 1),
    ("Bingo", "Daub your card as numbers are called.", 1),
    ("Lottery Sim", "Long-odds draws with realistic payouts.", 1),
    ("Coin Streak", "Predict flips and chase a run.", 1),
    ("Dice Duel", "Highest roll takes the pot.", 1),
    ("Wheel of Fortune", "Spin for multipliers and bankrupt wedges.", 1),
    ("Scratch Card", "Reveal symbols for instant prizes.", 1),
    ("Three Card Monte", "Track the queen through the shuffle.", 2),
    ("Plinko", "Drop a chip through a peg field.", 1),
    ("Horse Race Sim", "Bet on weighted runners.", 1),
    ("Rock Paper Scissors Lizard Spock", "The five-gesture extension.", 1),
    ("Odds and Evens", "Simultaneous finger-throw duel.", 1),
    ("Matching Pennies", "The canonical zero-sum bluffing game.", 2),
    ("Russian Roulette Safe", "Press-your-luck with no real stakes.", 1),
]
for name, blurb, diff in CHANCE:
    add(name, "Chance", "Probability and betting", "1-2", diff, blurb)

RPG = [
    ("Text Adventure", "Parser-driven exploration of a small world.", 3),
    ("Colossal Cave Lite", "A compact homage to the original cave crawl.", 3),
    ("Zork Lite", "Rooms, items and a lurking grue.", 3),
    ("Rogue", "The original permadeath dungeon crawl.", 4),
    ("NetHack Lite", "Deep item interactions in a compact dungeon.", 5),
    ("Angband Lite", "Deep descent toward a final boss.", 5),
    ("Dungeon of Doom", "Ten floors, escalating monsters.", 3),
    ("Monster Arena", "Turn-based gladiator progression.", 3),
    ("Wizard Duel", "Spell selection with rock-paper-scissors counters.", 2),
    ("Quest for the Grail", "Branching narrative with stat checks.", 2),
    ("Space Trader", "Trade routes, pirates and upgrades.", 3),
    ("Vampire Castle", "Survival horror on a tight resource budget.", 4),
    ("Zombie Survival", "Barricades, scavenging and night waves.", 3),
    ("Pet Monster Battler", "Capture, train and battle creatures.", 3),
    ("Idle Kingdom", "Incremental resource growth with prestige.", 2),
    ("Tower Climb RPG", "Floor-by-floor combat with loot drops.", 3),
    ("Merchant Sim", "Buy low, sell high across towns.", 3),
    ("Gladiator Manager", "Recruit and train a fighting stable.", 3),
    ("Choose Your Path", "Multi-ending branching story engine.", 2),
    ("Escape the Room", "Object puzzles inside a single room.", 3),
]
for name, blurb, diff in RPG:
    add(name, "RPG", "Progression and exploration", "1", diff, blurb)

RACING_BOARD = [
    ("Backgammon", "Race and hit with the doubling cube.", 4),
    ("Nardi", "Long backgammon with no hitting.", 3),
    ("Ludo", "Four-token dice race with captures.", 1),
    ("Parcheesi", "Cross-and-circle racing with blockades.", 2),
    ("Trouble", "Pop-o-matic style dice racing.", 1),
    ("Aggravation", "Marble race with shortcuts.", 2),
    ("Sorry Lite", "Card-driven racing with swaps.", 2),
    ("Game of the Goose", "The Renaissance spiral race.", 1),
    ("Pachisi", "The Indian ancestor of Ludo.", 2),
    ("Senet", "The ancient Egyptian race game.", 3),
    ("Royal Game of Ur", "The oldest known board game, reconstructed.", 3),
    ("Chutes and Ladders Deluxe", "Extended board with event squares.", 1),
    ("Monopoly Lite", "Property, rent and bankruptcy.", 3),
    ("Life Simulator Board", "Spin and advance through life events.", 1),
    ("Yut Nori", "Korean stick-throwing race game.", 2),
    ("Tables", "Medieval backgammon family variant.", 3),
]
for name, blurb, diff in RACING_BOARD:
    add(name, "Board", "Dice-driven racing", "1-4", diff, blurb)

TRIVIA = [
    ("General Knowledge Quiz", "Mixed-topic multiple choice.", 2),
    ("Geography Quiz", "Capitals, flags, rivers and borders.", 2),
    ("History Quiz", "Dates, figures and turning points.", 3),
    ("Science Quiz", "Physics, chemistry and biology basics.", 3),
    ("Film Quiz", "Directors, quotes and release years.", 2),
    ("Music Quiz", "Artists, albums and theory.", 2),
    ("Sports Quiz", "Records, rules and champions.", 2),
    ("Literature Quiz", "Authors, openings and characters.", 3),
    ("Computing Quiz", "Algorithms, hardware and history.", 3),
    ("Mythology Quiz", "Gods and legends across cultures.", 3),
    ("Art Quiz", "Movements, painters and works.", 3),
    ("Nature Quiz", "Species, habitats and behaviour.", 2),
    ("Flag Guesser", "Identify nations from their flags.", 2),
    ("Capital Cities Sprint", "Timed capital recall.", 2),
    ("Periodic Table Quiz", "Symbols, numbers and groups.", 3),
    ("Logo Quiz", "Identify brands from stylised marks.", 1),
    ("Trivia Survival", "One wrong answer ends the run.", 3),
    ("True or False Blitz", "Rapid-fire binary judgements.", 1),
]
for name, blurb, diff in TRIVIA:
    add(name, "Trivia", "Knowledge recall", "1-4", diff, blurb)

MUSIC = [
    ("Rhythm Runner", "Hit notes in time on a scrolling track.", 3),
    ("Note Trainer", "Identify notes on the stave.", 2),
    ("Interval Ear Training", "Name the interval you hear.", 3),
    ("Chord Builder", "Assemble chords from a root and quality.", 3),
    ("Scale Practice", "Play scales across keys and modes.", 2),
    ("Drum Machine", "Program a step-sequenced beat.", 2),
    ("Melody Memory", "Repeat a melodic phrase by ear.", 3),
    ("Perfect Pitch Test", "Identify absolute pitches.", 4),
    ("Metronome Challenge", "Hold tempo without the click.", 2),
    ("Song Sequencer", "Compose and play back a short piece.", 3),
]
for name, blurb, diff in MUSIC:
    add(name, "Music", "Audio and timing", "1", diff, blurb)


# --------------------------------------------------------------------------
# 3. Parametric variants - real variation axes, filled deterministically
# --------------------------------------------------------------------------

def variants():
    out = []

    # Sudoku family across sizes and rule variants.
    for size in ["4x4", "6x6", "9x9", "12x12", "16x16"]:
        for kind, extra, d in [
            ("Classic", "standard row, column and box constraints", 3),
            ("Diagonal", "both long diagonals must also hold 1..n", 3),
            ("Even-Odd", "shaded cells are restricted by parity", 3),
            ("Consecutive", "markers reveal every adjacent consecutive pair", 4),
            ("Anti-Knight", "no digit repeats a knight's move away", 4),
            ("Windoku", "four extra shaded regions must also be complete", 4),
        ]:
            out.append((f"Sudoku {size} {kind}", "Puzzle", "Latin square constraint", "1", d,
                        f"{size} sudoku where {extra}."))

    # Minesweeper across board sizes and densities.
    for w, h, mines, d in [(9, 9, 10, 2), (16, 16, 40, 3), (30, 16, 99, 4),
                           (20, 20, 80, 4), (40, 20, 180, 5), (12, 12, 25, 3)]:
        for kind, extra in [
            ("Classic", "standard adjacency counting"),
            ("No Guess", "every board is solvable by pure logic"),
            ("Wrap", "the grid wraps at the edges"),
            ("Knight", "adjacency follows a knight's move"),
        ]:
            out.append((f"Minesweeper {w}x{h} {kind}", "Puzzle", "Constraint deduction", "1", d,
                        f"{w}x{h} grid, {mines} mines, {extra}."))

    # Nonogram sizes.
    for size, d in [("5x5", 1), ("10x10", 2), ("15x15", 3), ("20x20", 4), ("25x25", 5), ("30x30", 5)]:
        out.append((f"Nonogram {size}", "Puzzle", "Line-clue painting", "1", d,
                    f"Reveal the hidden {size} image from row and column runs."))
        out.append((f"Nonogram {size} Colour", "Puzzle", "Line-clue painting", "1", min(5, d + 1),
                    f"Multi-colour {size} nonogram with per-colour run clues."))

    # Sliding puzzles.
    for n, d in [(3, 1), (4, 2), (5, 3), (6, 4), (7, 5)]:
        out.append((f"Sliding Puzzle {n}x{n}", "Puzzle", "Sliding tile", "1", d,
                    f"Order {n*n-1} tiles in an {n}x{n} frame."))

    # Lights Out sizes and rules.
    for n in [3, 4, 5, 6, 7, 8]:
        for kind, extra, d in [("Classic", "plus-shaped toggle", 2),
                               ("Diagonal", "diagonal toggle pattern", 3),
                               ("Row-Column", "whole row and column toggle", 3)]:
            out.append((f"Lights Out {n}x{n} {kind}", "Puzzle", "Toggle parity", "1", d,
                        f"{n}x{n} board using a {extra}."))

    # Tetris rule variants.
    for kind, extra, d in [
        ("Pentix", "five-cell pentomino pieces", 4),
        ("Sprint 40", "clear forty lines as fast as possible", 3),
        ("Ultra", "maximise score in two minutes", 3),
        ("Cascade", "gravity re-settles blocks after a clear", 4),
        ("Invisible", "placed blocks fade from view", 5),
        ("Master", "twenty-G instant drop speed", 5),
        ("Big Mode", "double-size pieces on a half-width well", 3),
        ("Zen", "no topping out, endless practice", 1),
    ]:
        out.append((f"Tetris {kind}", "Arcade", "Falling-block packing", "1", d,
                    f"Falling-block stacking with {extra}."))

    # Snake variants.
    for kind, extra, d in [
        ("Walls", "the border kills on contact", 2),
        ("Wrap", "you pass through the edges", 1),
        ("Maze", "interior obstacles block the field", 3),
        ("Speed", "the snake accelerates continuously", 3),
        ("Two Player", "two snakes share one board", 2),
        ("Portal", "paired teleporters reroute the snake", 3),
        ("Shrinking", "the arena closes in over time", 4),
        ("Poison", "some food shortens you instead", 3),
    ]:
        out.append((f"Snake {kind}", "Arcade", "Growth and collision", "1-2", d,
                    f"Snake where {extra}."))

    # 2048 variants.
    for n in [3, 4, 5, 6, 8]:
        out.append((f"2048 {n}x{n}", "Puzzle", "Merge sliding", "1", 2 if n >= 5 else 3,
                    f"Merge-sliding on an {n}x{n} grid."))
    for kind, extra, d in [("Fibonacci", "tiles merge along the Fibonacci sequence", 4),
                           ("Threes", "ones and twos combine before tripling", 3),
                           ("Hex", "six-directional movement on a hex grid", 4),
                           ("Powers of Three", "tiles triple instead of doubling", 3)]:
        out.append((f"2048 {kind}", "Puzzle", "Merge sliding", "1", d,
                    f"Sliding merge game where {extra}."))

    # Gomoku / connection sizes.
    for n, d in [(9, 2), (13, 3), (15, 3), (19, 4)]:
        out.append((f"Gomoku {n}x{n}", "Board", "Five-in-a-row", "1-2", d,
                    f"Five in a row on {n}x{n}."))
        out.append((f"Renju {n}x{n}", "Board", "Five-in-a-row", "1-2", min(5, d + 1),
                    f"Gomoku on {n}x{n} with forbidden moves for black."))
    for n in [5, 6, 7, 8]:
        out.append((f"Connect {n}", "Board", "Gravity-fed alignment", "1-2", 3,
                    f"Drop pieces and line up {n} in a row."))

    # Tic-tac-toe family.
    for kind, extra, d in [
        ("4x4", "four in a row on a 4x4 board", 2),
        ("5x5", "four in a row on 5x5", 2),
        ("Ultimate", "nine boards where your move picks the next board", 4),
        ("Misere", "making three in a row loses", 2),
        ("Wild", "either player may place either mark", 3),
        ("Order and Chaos", "one player wants five in a row, the other wants none", 4),
        ("Toroidal", "the board wraps at every edge", 3),
        ("3D", "three in a row through a 3x3x3 cube", 3),
        ("Notakto", "misere play across several boards at once", 4),
        ("Quantum", "moves stay in superposition until they collapse", 5),
    ]:
        out.append((f"Tic Tac Toe {kind}", "Board", "Perfect-information placement", "1-2", d,
                    f"Noughts and crosses where {extra}."))

    # Nim family.
    for kind, extra, d in [
        ("Misere", "taking the last object loses", 2),
        ("Wythoff", "remove from one pile or equally from both", 4),
        ("Fibonacci", "each take is bounded by twice the last", 4),
        ("Subtraction", "removals come from a fixed set", 3),
        ("Kayles", "knock down one or two adjacent pins", 3),
        ("Dawson's Chess", "a pawn race that reduces to a nim value", 4),
        ("Turning Turtles", "flip coins under octal game rules", 4),
        ("Northcott", "move checkers along rows toward each other", 3),
        ("Mock Turtles", "an octal game with a surprising Sprague-Grundy pattern", 5),
        ("Moore's", "remove from up to k piles at once", 4),
    ]:
        out.append((f"Nim {kind}", "Logic", "Combinatorial subtraction", "1-2", d,
                    f"Nim variant where {extra}."))

    # Hangman / word themes.
    for theme in ["Animals", "Countries", "Capitals", "Fruits", "Sports", "Films",
                  "Science", "Computing", "Music", "Space", "Ocean", "Mythology",
                  "Occupations", "Colours", "Vehicles", "Instruments", "Weather",
                  "Body Parts", "Tools", "Plants"]:
        out.append((f"Hangman {theme}", "Word", "Letter deduction", "1", 2,
                    f"Letter-guessing on a {theme.lower()} dictionary."))
    for n in [4, 5, 6, 7, 8]:
        out.append((f"Wordle {n} Letters", "Word", "Positional feedback", "1", 2 if n <= 5 else 3,
                    f"Guess a hidden {n}-letter word from colour feedback."))

    # Word search and anagram themes.
    for theme in ["Animals", "Geography", "Science", "History", "Sports", "Food",
                  "Technology", "Literature", "Music", "Nature"]:
        out.append((f"Word Search {theme}", "Word", "Grid word finding", "1", 1,
                    f"Find hidden {theme.lower()} words in eight directions."))
        out.append((f"Anagram {theme}", "Word", "Letter rearrangement", "1", 2,
                    f"Unscramble {theme.lower()} vocabulary against the clock."))

    # Mazes by generation algorithm and size.
    for algo, extra in [
        ("Recursive Backtracker", "long winding corridors"),
        ("Prim", "a bushy, many-branched layout"),
        ("Kruskal", "uniformly random spanning structure"),
        ("Eller", "a maze generated row by row"),
        ("Wilson", "an unbiased uniform spanning tree"),
        ("Aldous-Broder", "a slow but perfectly uniform walk"),
        ("Hunt and Kill", "long passages with sparse dead ends"),
        ("Binary Tree", "a strong diagonal texture"),
        ("Sidewinder", "horizontal runs with upward links"),
        ("Growing Tree", "a tunable mix of Prim and backtracker"),
    ]:
        for size, d in [("Small", 1), ("Medium", 2), ("Large", 3), ("Huge", 4)]:
            out.append((f"Maze {algo} {size}", "Puzzle", "Path finding", "1", d,
                        f"{size} maze generated with {algo}, producing {extra}."))

    # Blackjack rule sets.
    for kind, extra, d in [
        ("Single Deck", "one deck, dealer stands on soft 17", 2),
        ("Six Deck", "a six-deck shoe with late surrender", 2),
        ("Double Exposure", "both dealer cards face up, ties lose", 3),
        ("Spanish 21", "the tens are removed and bonuses added", 3),
        ("Vegas Strip", "dealer peeks and you may double after split", 2),
        ("Atlantic City", "late surrender with a re-split limit", 2),
        ("Face Up 21", "full information, tighter payouts", 3),
        ("Perfect Pairs", "side bet on matched opening pairs", 2),
    ]:
        out.append((f"Blackjack {kind}", "Cards", "Card counting and odds", "1", d,
                    f"Blackjack with {extra}."))

    # Poker paytables.
    for kind, extra in [
        ("Jacks or Better", "the 9/6 full-pay table"),
        ("Bonus Poker", "enhanced four-of-a-kind payouts"),
        ("Double Double Bonus", "kicker-sensitive quad bonuses"),
        ("Aces and Faces", "premium payouts on high quads"),
        ("Tens or Better", "a lower qualifying pair"),
        ("All American", "boosted straights and flushes"),
        ("Loose Deuces", "deuces wild with a large quad-deuce payout"),
    ]:
        out.append((f"Video Poker {kind}", "Cards", "Draw poker payout", "1", 2,
                    f"Five-card draw video poker using {extra}."))

    # Solitaire deal counts.
    for kind in ["Draw One", "Draw Three", "Vegas Scoring", "Timed", "Thoughtful"]:
        out.append((f"Klondike {kind}", "Solitaire", "Patience card sorting", "1", 3,
                    f"Klondike patience played in {kind.lower()} mode."))

    # Roguelike depths and themes.
    for theme, extra in [
        ("Catacombs", "undead and narrow crypt corridors"),
        ("Caverns", "flooded caves and cave-ins"),
        ("Sewers", "poison hazards and rats"),
        ("Ice Caves", "slippery floors and frost damage"),
        ("Volcano", "lava flows and heat pressure"),
        ("Sky Temple", "wind gusts and long falls"),
        ("Derelict Ship", "vacuum breaches and malfunctioning drones"),
        ("Forest Depths", "dense growth that blocks line of sight"),
    ]:
        for depth, d in [(5, 2), (10, 3), (20, 4), (50, 5)]:
            out.append((f"{theme} Crawl {depth}F", "RPG", "Procedural exploration", "1", d,
                        f"A {depth}-floor descent through {extra}."))

    # Reversi sizes.
    for n, d in [(4, 1), (6, 2), (8, 3), (10, 4), (12, 4)]:
        out.append((f"Reversi {n}x{n}", "Board", "Flanking capture", "1-2", d,
                    f"Othello-style flipping on an {n}x{n} board."))

    # Peg solitaire and Hanoi sizes.
    for shape in ["English", "European", "Triangular", "Diamond", "Square"]:
        out.append((f"Peg Solitaire {shape}", "Puzzle", "Jump elimination", "1", 3,
                    f"{shape} board peg jumping down to one survivor."))
    for n in [3, 4, 5, 6, 7, 8, 10]:
        out.append((f"Towers of Hanoi {n} Discs", "Puzzle", "Recursive stacking", "1",
                    min(5, 1 + n // 3), f"Relocate a {n}-disc tower in {2**n - 1} moves."))

    # Battleship rule sets.
    for kind, extra, d in [
        ("Salvo", "fire one shot per surviving ship", 3),
        ("Big Board", "a 12x12 ocean with a larger fleet", 3),
        ("Moving Ships", "the fleet relocates between turns", 4),
        ("Fog", "hits are reported only at the end of a round", 4),
        ("Hex Grid", "a hexagonal ocean", 3),
    ]:
        out.append((f"Battleship {kind}", "Board", "Hidden-grid deduction", "1-2", d,
                    f"Naval deduction where you {extra}."))

    # Mastermind widths and palettes.
    for pegs in [3, 4, 5, 6]:
        for colours in [6, 8, 10]:
            out.append((f"Mastermind {pegs}x{colours}", "Logic", "Code deduction", "1",
                        min(5, 1 + (pegs * colours) // 12),
                        f"Break a {pegs}-peg code drawn from {colours} colours."))

    # Memory grid sizes.
    for pairs, d in [(6, 1), (8, 1), (12, 2), (18, 3), (24, 3), (32, 4)]:
        out.append((f"Memory Match {pairs} Pairs", "Puzzle", "Pair recall", "1", d,
                    f"Clear {pairs} hidden pairs from memory."))

    # Typing drills.
    for kind in ["Common Words", "Quotations", "Code Snippets", "Numbers",
                 "Punctuation", "Long Passage", "Sixty Second", "Accuracy Mode"]:
        out.append((f"Typing Test {kind}", "Skill", "Input speed", "1", 2,
                    f"Words-per-minute drill on {kind.lower()}."))

    # Yahtzee variants.
    for kind, extra in [("Triple", "three scorecards filled at once"),
                        ("Duplicate", "everyone rolls the same dice"),
                        ("Six Dice", "an extra die and extra categories"),
                        ("Speed", "a strict timer on every turn"),
                        ("Solo Challenge", "chase a fixed target score")]:
        out.append((f"Yahtzee {kind}", "Dice", "Category scoring", "1", 3,
                    f"Category dice scoring with {extra}."))

    # Quiz difficulty tiers.
    for topic in ["Geography", "History", "Science", "Film", "Music", "Sport",
                  "Literature", "Computing", "Art", "Nature"]:
        for tier, d in [("Easy", 1), ("Hard", 3), ("Expert", 5)]:
            out.append((f"{topic} Quiz {tier}", "Trivia", "Knowledge recall", "1-4", d,
                        f"{tier} tier {topic.lower()} questions."))

    # Chess endgame and puzzle drills.
    for kind, extra, d in [
        ("Mate in One", "find the immediate checkmate", 1),
        ("Mate in Two", "force mate in two moves", 3),
        ("Mate in Three", "force mate in three moves", 5),
        ("King and Pawn", "master the opposition", 3),
        ("Rook Endgame", "Lucena and Philidor positions", 4),
        ("Knight Fork", "spot the forking square", 2),
        ("Pin and Skewer", "exploit lined-up pieces", 2),
        ("Discovered Attack", "unmask an attack with tempo", 3),
        ("Zugzwang", "win by forcing a move", 4),
        ("Stalemate Trap", "save a lost game with stalemate", 3),
    ]:
        out.append((f"Chess Puzzle {kind}", "Chess", "Tactical drill", "1", d,
                    f"Tactics trainer: {extra}."))

    # Go problems.
    for kind, d in [("Life and Death", 4), ("Capturing Race", 4), ("Ladder", 2),
                    ("Joseki Drill", 4), ("Endgame Counting", 5), ("Tesuji", 5)]:
        out.append((f"Go Problem {kind}", "Abstract", "Positional strategy", "1", d,
                    f"Go training set focused on {kind.lower()}."))

    # Cellular automata rules.
    for rule in [30, 54, 90, 110, 150, 182, 22, 60]:
        out.append((f"Elementary Automaton Rule {rule}", "Simulation", "Cellular automaton", "1", 2,
                    f"One-dimensional automaton evolving under rule {rule}."))
    for name, extra in [("HighLife", "an extra birth rule producing replicators"),
                        ("Day and Night", "a symmetric on-off rule"),
                        ("Seeds", "explosive growth from any seed"),
                        ("Brian's Brain", "a three-state firing rule"),
                        ("Maze", "growth that settles into maze corridors"),
                        ("Coral", "slow crystalline accretion")]:
        out.append((f"Life Variant {name}", "Simulation", "Cellular automaton", "1", 2,
                    f"Life-like automaton with {extra}."))

    # Artillery and physics duels.
    for kind, extra in [("Wind", "variable crosswind"), ("Gravity", "adjustable gravity"),
                        ("Terrain", "destructible landscapes"), ("Multi Shot", "salvo firing"),
                        ("Moving Target", "targets that reposition")]:
        out.append((f"Artillery Duel {kind}", "Arcade", "Projectile physics", "1-2", 3,
                    f"Angle-and-power duel featuring {extra}."))

    # Breakout and Pong variants.
    for kind, extra in [("Neon", "a fast neon layout"), ("Boss", "armoured boss bricks"),
                        ("Gravity", "a downward pull on the ball"),
                        ("Multiball", "several balls at once"),
                        ("Rotating", "a brick field that slowly rotates"),
                        ("Endless", "procedurally generated walls")]:
        out.append((f"Breakout {kind}", "Arcade", "Brick destruction", "1", 3,
                    f"Brick breaking with {extra}."))
    for kind, extra in [("Four Player", "paddles on all four walls"),
                        ("Curve", "spin that bends the ball"),
                        ("Obstacle", "blockers mid-court"),
                        ("Shrinking Paddle", "paddles that shrink on every rally"),
                        ("Gravity Well", "a central mass that bends flight")]:
        out.append((f"Pong {kind}", "Arcade", "Paddle deflection", "1-2", 2,
                    f"Paddle rally with {extra}."))

    # Sokoban level packs.
    for pack, d in [("Tutorial", 1), ("Classic", 3), ("Microban", 2),
                    ("Sasquatch", 4), ("Grand Master", 5), ("Mini Cosmos", 3)]:
        out.append((f"Sokoban {pack}", "Puzzle", "Crate pushing", "1", d,
                    f"{pack} crate-pushing level set."))

    # Card shedding and matching variants.
    for kind, extra in [("Wild Draw", "stacking draw penalties"),
                        ("Reverse Chaos", "direction reverses constantly"),
                        ("Colour Lock", "one suit is locked each round"),
                        ("Speed", "no turn order, play as fast as you can")]:
        out.append((f"Crazy Eights {kind}", "Cards", "Shedding", "2-4", 2,
                    f"Eights-style shedding with {extra}."))

    # Dots and boxes sizes.
    for n in [3, 4, 5, 6, 7]:
        out.append((f"Dots and Boxes {n}x{n}", "Board", "Edge claiming", "1-2",
                    min(5, 1 + n // 2), f"Claim boxes on an {n}x{n} dot lattice."))

    # Mancala house counts.
    for houses in [4, 5, 6, 7, 8]:
        for seeds in [3, 4, 5, 6]:
            out.append((f"Kalah {houses}-{seeds}", "Board", "Seed sowing", "1-2", 3,
                        f"Kalah with {houses} houses and {seeds} seeds each."))

    # Checkers board sizes.
    for n in [6, 8, 10, 12]:
        out.append((f"Draughts {n}x{n}", "Board", "Capture and promotion", "1-2",
                    min(5, n // 3), f"Draughts played on an {n}x{n} board."))

    # Idle/incremental.
    for theme in ["Mine", "Bakery", "Farm", "Factory", "Laboratory", "Galaxy",
                  "Dungeon", "Garden", "Kingdom", "Reactor"]:
        out.append((f"Idle {theme}", "Simulation", "Incremental growth", "1", 2,
                    f"Incremental {theme.lower()} management with prestige resets."))

    # Reaction and precision drills.
    for kind in ["Single Target", "Moving Target", "Multi Target", "Tracking",
                 "Flick Shot", "Peripheral", "Colour Change", "Sound Cue"]:
        out.append((f"Reaction Drill {kind}", "Skill", "Memory and reflex", "1", 2,
                    f"Precision timing drill: {kind.lower()}."))

    # Logic grid themes.
    for theme in ["Detective", "Dinner Party", "Race Results", "Office", "School",
                  "Zoo", "Festival", "Hotel", "Voyage", "Bakery"]:
        for d in [2, 4]:
            tier = "Standard" if d == 2 else "Advanced"
            out.append((f"Logic Grid {theme} {tier}", "Puzzle", "Logical deduction", "1", d,
                        f"{tier} constraint puzzle set at a {theme.lower()}."))

    # Board-size sweeps for connection games.
    for game, mech in [("Hex", "Connection race"), ("Havannah", "Connection race"),
                       ("Y", "Connection race")]:
        for n in [7, 9, 11, 13, 15, 19]:
            out.append((f"{game} Size {n}", "Abstract", mech, "1-2",
                        min(5, 1 + n // 5), f"{game} on a size-{n} hexagonal board."))

    # Guess-the-number ranges.
    for hi, d in [(10, 1), (100, 1), (1000, 2), (10000, 2), (1000000, 3)]:
        out.append((f"Guess the Number to {hi}", "Logic", "Binary search", "1", d,
                    f"Find a hidden number between 1 and {hi}."))

    # Bulls and cows digit counts.
    for n in [3, 4, 5, 6]:
        out.append((f"Bulls and Cows {n} Digits", "Logic", "Digit deduction", "1",
                    min(5, n - 1), f"Deduce an {n}-digit secret from bull and cow counts."))

    # Simon sequence modes.
    for kind in ["Four Colour", "Six Colour", "Eight Colour", "Reverse", "Silent", "Speed"]:
        out.append((f"Simon {kind}", "Skill", "Sequence memory", "1", 3,
                    f"Sequence repetition in {kind.lower()} mode."))

    # Slot machine themes.
    for theme in ["Fruit", "Egypt", "Space", "Pirate", "Jungle", "Diamond",
                  "Western", "Aztec", "Neon", "Deep Sea"]:
        out.append((f"Slots {theme}", "Chance", "Reel matching", "1", 1,
                    f"{theme}-themed reels with a published paytable."))

    # Roulette variants.
    for kind, extra in [("European", "a single zero"), ("American", "a double zero"),
                        ("French", "la partage on even-money bets"),
                        ("Mini", "a thirteen-pocket wheel")]:
        out.append((f"Roulette {kind}", "Chance", "Probability and betting", "1", 2,
                    f"Roulette with {extra}."))

    # Backgammon variants.
    for kind, extra in [("Hypergammon", "three checkers a side"),
                        ("Nackgammon", "a tougher starting position"),
                        ("Acey Deucey", "special rolls and re-entry"),
                        ("Plakoto", "pinning instead of hitting"),
                        ("Fevga", "no hitting at all"),
                        ("Gul Bara", "a rapid running variant")]:
        out.append((f"Backgammon {kind}", "Board", "Dice-driven racing", "1-2", 4,
                    f"Backgammon variant with {extra}."))

    # Tower defence and lane strategy.
    for theme in ["Forest", "Desert", "Space", "Castle", "Cyber", "Underwater"]:
        out.append((f"Tower Defence {theme}", "Strategy", "Lane defence", "1", 3,
                    f"Place towers along a {theme.lower()} route and survive the waves."))

    # Match-three.
    for theme in ["Gems", "Fruit", "Runes", "Candy", "Stars", "Blocks"]:
        out.append((f"Match Three {theme}", "Puzzle", "Tile matching", "1", 2,
                    f"Swap adjacent {theme.lower()} to form lines of three or more."))

    # Solitaire dice/patience hybrids and misc singles.
    for name, genre, mech, d, blurb in [
        ("Pipe Dream", "Puzzle", "Route building", 3, "Lay pipe ahead of the flowing water."),
        ("Lights and Mirrors", "Puzzle", "Beam routing", 3, "Steer a laser to every target."),
        ("Circuit Maze", "Puzzle", "Route building", 4, "Complete a circuit under component limits."),
        ("Colour Sort", "Puzzle", "Sorting", 2, "Pour liquids until each tube is one colour."),
        ("Ball Sort", "Puzzle", "Sorting", 2, "Stack matching balls into single-colour tubes."),
        ("Hanoi Race", "Puzzle", "Recursive stacking", 3, "Two-player race to move the tower."),
        ("Number Slide", "Puzzle", "Sliding tile", 2, "Slide numbers into ascending order."),
        ("Unruly", "Puzzle", "Binary balance", 3, "Balance black and white with no three in a row."),
        ("Flip Grid", "Puzzle", "Toggle parity", 3, "Flip rows and columns to a target pattern."),
        ("Tile Rotate", "Puzzle", "Route building", 3, "Rotate tiles until the network connects."),
        ("Loop Closer", "Puzzle", "Route building", 3, "Rotate pieces to close a single loop."),
        ("Bridge Builder", "Simulation", "Structural physics", 4, "Build a span that carries the load."),
        ("Cannon Angle", "Arcade", "Projectile physics", 2, "Solve for the firing solution."),
        ("Orbit Sim", "Simulation", "Gravity", 4, "Achieve a stable orbit on a fuel budget."),
        ("Snake Charmer", "Arcade", "Growth and collision", 2, "Snake with a music-driven tempo."),
        ("Echo Maze", "Puzzle", "Path finding", 3, "Navigate a dark maze by sonar pings."),
        ("Minefield Run", "Arcade", "Timing and lanes", 3, "Cross a mined field from memory."),
        ("Card Clock", "Solitaire", "Patience card sorting", 1, "Deal the clock and beat the odds."),
        ("Dice Chess", "Chess", "Piece movement strategy", 3, "A die decides which piece you may move."),
        ("Blind Maze", "Puzzle", "Path finding", 4, "Map a maze you cannot see."),
    ]:
        out.append((name, genre, mech, "1", d, blurb))

    return out


for item in variants():
    title, genre, mech, players, diff, blurb = item
    add(title, genre, mech, players, diff, blurb)


# --------------------------------------------------------------------------
# 4. Trim or extend to exactly TARGET entries
# --------------------------------------------------------------------------
if len(entries) > TARGET:
    # Never drop an implemented game; trim from the tail of the generated set.
    keep = [e for e in entries if e["implemented"]]
    rest = [e for e in entries if not e["implemented"]]
    rest = rest[: TARGET - len(keep)]
    order = {e["slug"]: i for i, e in enumerate(entries)}
    entries = sorted(keep + rest, key=lambda e: order[e["slug"]])
elif len(entries) < TARGET:
    # Deterministic top-up across remaining board-size axes.
    n = 4
    while len(entries) < TARGET:
        for fam, genre, mech in [("Gomoku", "Board", "Five-in-a-row"),
                                 ("Reversi", "Board", "Flanking capture"),
                                 ("Lights Out", "Puzzle", "Toggle parity"),
                                 ("Sliding Puzzle", "Puzzle", "Sliding tile"),
                                 ("Memory Match", "Puzzle", "Pair recall")]:
            if len(entries) >= TARGET:
                break
            add(f"{fam} Board {n}", genre, mech, "1-2", 3,
                f"{fam} played on a size-{n} board.")
        n += 1

assert len(entries) == TARGET, f"expected {TARGET}, produced {len(entries)}"

impl = sum(e["implemented"] for e in entries)
missing = IMPLEMENTED - {e["slug"] for e in entries}
assert not missing, f"implemented games missing from catalog: {sorted(missing)}"


# --------------------------------------------------------------------------
# 5. Emit JSON and C
# --------------------------------------------------------------------------
genres = {}
for e in entries:
    genres[e["genre"]] = genres.get(e["genre"], 0) + 1

os.makedirs(os.path.join(ROOT, "data"), exist_ok=True)
with open(os.path.join(ROOT, "data", "catalog.json"), "w", encoding="utf-8") as f:
    json.dump({
        "count": len(entries),
        "implemented": impl,
        "genres": dict(sorted(genres.items(), key=lambda kv: -kv[1])),
        "games": entries,
    }, f, indent=1, ensure_ascii=False)


def cstr(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


lines = [
    "/* catalog_data.c - GENERATED by tools/gen_catalog.py. Do not edit by hand. */",
    '#include "engine.h"',
    "",
    "const CatalogEntry CATALOG[] = {",
]
for e in entries:
    lines.append("    {%s, %s, %s, %s, %s, %d, %s, %d}," % (
        cstr(e["slug"]), cstr(e["title"]), cstr(e["genre"]), cstr(e["mechanic"]),
        cstr(e["players"]), e["difficulty"], cstr(e["blurb"]), e["implemented"]))
lines += [
    "};",
    "",
    "const int CATALOG_COUNT = (int)(sizeof(CATALOG) / sizeof(CATALOG[0]));",
    "",
]

with open(os.path.join(ROOT, "src", "catalog_data.c"), "w", encoding="utf-8") as f:
    f.write("\n".join(lines))

# Also emit a JS copy so the browser app works from file:// without fetch().
with open(os.path.join(ROOT, "web", "js", "catalog.js"), "w", encoding="utf-8") as f:
    f.write("/* GENERATED by tools/gen_catalog.py. Do not edit by hand. */\n")
    f.write("window.CATALOG = ")
    json.dump(entries, f, separators=(",", ":"), ensure_ascii=False)
    f.write(";\n")

print(f"catalog: {len(entries)} games, {impl} implemented")
print("genres:")
for g, c in sorted(genres.items(), key=lambda kv: -kv[1]):
    print(f"  {g:12s} {c}")
