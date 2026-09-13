/* registry.c - maps catalog slugs to the playable game functions. */
#include "engine.h"
#include "games.h"

const GameEntry GAME_REGISTRY[] = {
    {"tictactoe",           "Tic Tac Toe",        "Board",  play_tictactoe},
    {"connect-four",        "Connect Four",       "Board",  play_connect4},
    {"reversi",             "Reversi",            "Board",  play_reversi},
    {"gomoku",              "Gomoku",             "Board",  play_gomoku},
    {"checkers",            "Checkers",           "Board",  play_checkers},
    {"mancala",             "Mancala",            "Board",  play_mancala},
    {"nim",                 "Nim",                "Logic",  play_nim},
    {"dots-and-boxes",      "Dots and Boxes",     "Board",  play_dots_boxes},
    {"battleship",          "Battleship",         "Board",  play_battleship},
    {"mastermind",          "Mastermind",         "Logic",  play_mastermind},

    {"minesweeper",         "Minesweeper",        "Puzzle", play_minesweeper},
    {"sudoku",              "Sudoku",             "Puzzle", play_sudoku},
    {"fifteen-puzzle",      "Fifteen Puzzle",     "Puzzle", play_fifteen},
    {"lights-out",          "Lights Out",         "Puzzle", play_lights_out},
    {"flood-it",            "Flood It",           "Puzzle", play_flood_it},
    {"sokoban",             "Sokoban",            "Puzzle", play_sokoban},
    {"memory-match",        "Memory Match",       "Puzzle", play_memory},
    {"game-2048",           "2048",               "Puzzle", play_2048},
    {"hanoi",               "Towers of Hanoi",    "Puzzle", play_hanoi},
    {"nonogram",            "Nonogram",           "Puzzle", play_nonogram},

    {"snake",               "Snake",              "Arcade", play_snake},
    {"tetris",              "Tetris",             "Arcade", play_tetris},
    {"pong",                "Pong",               "Arcade", play_pong},
    {"breakout",            "Breakout",           "Arcade", play_breakout},
    {"space-invaders",      "Space Invaders",     "Arcade", play_invaders},
    {"dino-run",            "Dino Run",           "Arcade", play_dino},
    {"flappy",              "Flappy",             "Arcade", play_flappy},
    {"frogger",             "Frogger",            "Arcade", play_frogger},
    {"pacman",              "Pacman",             "Arcade", play_pacman},
    {"asteroids",           "Asteroids",          "Arcade", play_asteroids},

    {"blackjack",           "Blackjack",          "Cards",  play_blackjack},
    {"video-poker",         "Video Poker",        "Cards",  play_video_poker},
    {"war",                 "War",                "Cards",  play_war},
    {"go-fish",             "Go Fish",            "Cards",  play_go_fish},
    {"yahtzee",             "Yahtzee",            "Dice",   play_yahtzee},
    {"pig",                 "Pig",                "Dice",   play_pig},
    {"slot-machine",        "Slot Machine",       "Chance", play_slots},
    {"higher-lower",        "Higher or Lower",    "Chance", play_higher_lower},

    {"hangman",             "Hangman",            "Word",   play_hangman},
    {"wordle",              "Wordle",             "Word",   play_wordle},
    {"anagram",             "Anagram",            "Word",   play_anagram},
    {"typing-test",         "Typing Test",        "Skill",  play_typing},
    {"guess-number",        "Guess the Number",   "Logic",  play_guess_number},
    {"bulls-cows",          "Bulls and Cows",     "Logic",  play_bulls_cows},
    {"rock-paper-scissors", "Rock Paper Scissors","Chance", play_rps},
    {"simon",               "Simon",              "Skill",  play_simon},
    {"snakes-and-ladders",  "Snakes and Ladders", "Board",  play_snakes_ladders},
    {"dungeon-crawl",       "Dungeon Crawl",      "RPG",    play_dungeon},
    {"virtual-piano",       "Virtual Piano",      "Music",  play_piano},
};

const int GAME_REGISTRY_COUNT = (int)(sizeof(GAME_REGISTRY) / sizeof(GAME_REGISTRY[0]));
