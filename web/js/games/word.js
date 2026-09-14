/* word.js - ports of src/games/{hangman,wordle,anagram}.c and the games in
 * src/games/misc.c (typing, guess-number, bulls & cows, RPS, simon,
 * snakes & ladders, piano) plus src/games/dungeon.c.
 * Dictionaries mirror src/games/words.c.
 */
(function () {
'use strict';
var G = window.GIC, C = G.COL, reg = G.register, rnd = G.rnd, shuffle = G.shuffle;

var W = window.GICWORDS;
var THEME_NAMES = W.themeOrder;
var THEMES = W.themes;
var BYLEN = W.byLength;
var TYPING_MODES = W.typingOrder;

function themeIndex(p) {
  var i = THEME_NAMES.indexOf(p.theme || '');
  if (i >= 0) return i;
  return (p.variant >= 0 && p.variant < THEME_NAMES.length) ? p.variant : 0;
}
function pick(a) { return a[rnd(a.length)]; }

/* ---------------------------------------------------------------- hangman */
reg('hangman', {
  title: 'Hangman', help: 'Type a letter · Q quits',
  start: function (host, p) {
    var GALLOWS = [
      ["  +---+","  |   |","      |","      |","      |","========="],
      ["  +---+","  |   |","  O   |","      |","      |","========="],
      ["  +---+","  |   |","  O   |","  |   |","      |","========="],
      ["  +---+","  |   |","  O   |"," /|   |","      |","========="],
      ["  +---+","  |   |","  O   |"," /|\\  |","      |","========="],
      ["  +---+","  |   |","  O   |"," /|\\  |"," /    |","========="],
      ["  +---+","  |   |","  O   |"," /|\\  |"," / \\  |","========="]
    ];
    var LIVES = 6, theme = themeIndex(p), word, guessed, wrong, won, lost;
    function reset() {
      word = pick(THEMES[THEME_NAMES[theme]]);
      guessed = {}; wrong = 0; won = false; lost = false;
    }
    reset();
    return {
      key: function (k) {
        if (won || lost) { reset(); return; }
        if (k.length !== 1 || k < 'a' || k > 'z') return;
        if (guessed[k]) return;
        guessed[k] = true;
        if (word.indexOf(k) < 0) wrong++;
        if (wrong >= LIVES) lost = true;
        if (word.split('').every(function (ch) { return guessed[ch]; })) {
          won = true;
          host.saveScore((LIVES - wrong) * 100);
        }
      },
      draw: function (t) {
        t.header('HANGMAN', 'Theme: ' + THEME_NAMES[theme] + ' · guess a letter · Q quits');
        GALLOWS[wrong].forEach(function (line, i) { t.text(30, 5 + i, line, C.yellow); });
        var shown = word.split('').map(function (ch) {
          return (guessed[ch] || lost) ? ch : '_';
        }).join(' ');
        t.text(30, 13, shown, won ? C.green : C.white, null, true);
        var bad = Object.keys(guessed).filter(function (ch) { return word.indexOf(ch) < 0; });
        t.text(30, 15, 'Wrong: ' + bad.join(' ') + '                   ', C.red);
        t.text(30, 16, 'Lives: ' + (LIVES - wrong) + '   ', C.fg);
        if (won) t.center(19, 'You got it! Press any key.', C.green, true);
        if (lost) t.center(19, 'Out of lives — it was "' + word + '". Press any key.', C.red, true);
      }
    };
  }
});

/* ----------------------------------------------------------------- wordle */
reg('wordle', {
  title: 'Wordle', help: 'Type letters · Enter submits · Backspace deletes',
  start: function (host, p) {
    var LEN = Math.max(4, Math.min(8, (p.count > 0 ? p.count : 5)));
    var LIST = BYLEN[String(LEN)] || BYLEN['5'];
    var TRIES = 6;
    var secret, guesses, typing, letters, done, note;
    function reset() {
      secret = pick(LIST);
      guesses = []; typing = ''; letters = {}; done = null; note = '';
    }
    function score(g) {
      var out = new Array(LEN).fill('.'), used = new Array(LEN).fill(false), i, j;
      for (i = 0; i < LEN; i++) if (g[i] === secret[i]) { out[i] = 'g'; used[i] = true; }
      for (i = 0; i < LEN; i++) {
        if (out[i] === 'g') continue;
        for (j = 0; j < LEN; j++) {
          if (used[j] || secret[j] !== g[i]) continue;
          out[i] = 'y'; used[j] = true; break;
        }
      }
      return out;
    }
    reset();
    return {
      key: function (k) {
        if (done) { reset(); return; }
        if (k === 'back') { typing = typing.slice(0, -1); note = ''; return; }
        if (k.length === 1 && k >= 'a' && k <= 'z') {
          if (typing.length < LEN) typing += k;
          note = '';
          return;
        }
        if (k !== 'enter') return;
        if (typing.length !== LEN) { note = 'Needs ' + LEN + ' letters.'; return; }
        if (LIST.indexOf(typing) < 0) { note = 'Not in the word list.'; return; }
        var marks = score(typing);
        guesses.push({ g: typing, m: marks });
        for (var i = 0; i < LEN; i++) {
          var v = marks[i] === 'g' ? 2 : marks[i] === 'y' ? 1 : -1;
          if ((letters[typing[i]] || 0) < v) letters[typing[i]] = v;
        }
        if (typing === secret) {
          done = 'Solved in ' + guesses.length + '!';
          host.saveScore((TRIES - guesses.length + 1) * 100);
        } else if (guesses.length >= TRIES) done = 'Out of guesses — it was "' + secret + '".';
        typing = '';
      },
      draw: function (t) {
        t.header('WORDLE', LEN + ' letters · type · Enter submits · Backspace deletes');
        var left = Math.max(1, 42 - LEN * 2);
        for (var r = 0; r < TRIES; r++) {
          for (var i = 0; i < LEN; i++) {
            var ch = ' ', bg = null;
            if (r < guesses.length) {
              ch = guesses[r].g[i].toUpperCase();
              bg = guesses[r].m[i] === 'g' ? '#2f6b34' : guesses[r].m[i] === 'y' ? '#8a6d1f' : '#2a2e34';
            } else if (r === guesses.length && i < typing.length) {
              ch = typing[i].toUpperCase();
              bg = '#23262c';
            }
            t.text(left + i * 3, 4 + r * 2, ' ' + ch + ' ', C.white, bg, true);
          }
        }
        var row = 'abcdefghijklmnopqrstuvwxyz';
        for (var j = 0; j < 26; j++) {
          var st = letters[row[j]] || 0;
          t.put(24 + j * 2, 18, row[j].toUpperCase(),
                st === 2 ? C.green : st === 1 ? C.yellow : st === -1 ? '#40454c' : C.white);
        }
        if (note) t.center(20, note, C.red);
        if (done) t.center(20, done + ' Press any key.', C.white, true);
      }
    };
  }
});

/* ---------------------------------------------------------------- anagram */
reg('anagram', {
  title: 'Anagram', help: 'Type your answer · Enter submits · H hints · Q quits',
  start: function (host, p) {
    var theme = themeIndex(p);
    var word, scram, typing, score, round, hint, note;
    function newWord() {
      word = pick(THEMES[THEME_NAMES[theme]]);
      scram = shuffle(word.split('')).join('');
      if (scram === word && word.length > 1)
        scram = word.slice(-1) + word.slice(1, -1) + word[0];
      typing = ''; hint = false;
    }
    function reset() { score = 0; round = 1; note = ''; newWord(); }
    reset();
    return {
      key: function (k) {
        if (round > 8) { reset(); return; }
        if (k === 'back') { typing = typing.slice(0, -1); return; }
        if (k.length === 1 && k >= 'a' && k <= 'z') { typing += k; return; }
        if (k === 'h') { hint = true; return; }
        if (k !== 'enter') return;
        if (typing === word) {
          var pts = hint ? 60 : 100;
          score += pts;
          round++;
          if (round > 8) { host.saveScore(score); return; }
          newWord();
          note = 'Correct! +' + pts;
        } else { note = 'Not quite — try again.'; typing = ''; }
      },
      draw: function (t) {
        if (round > 8) {
          t.header('ANAGRAM', 'Round complete');
          t.center(10, 'Final score: ' + score, C.yellow, true);
          t.center(12, 'Press any key to play again.', C.dim);
          return;
        }
        t.header('ANAGRAM', 'Round ' + round + ' of 8 · theme ' + THEME_NAMES[theme]);
        t.text(28, 6, 'Scrambled: ' + scram + '            ', C.yellow, null, true);
        if (hint) t.text(28, 8, 'Hint: starts with "' + word[0] + '", ' + word.length + ' letters  ', C.dim);
        t.text(28, 10, 'Your answer: ' + typing + '_          ', C.cyan);
        t.text(28, 12, 'Score: ' + score + '   (H for a hint)   ', C.fg);
        if (note) t.text(28, 14, note + '                     ', C.white, null, true);
      }
    };
  }
});

/* ------------------------------------------------------------ typing test */
reg('typing', {
  title: 'Typing Test', help: 'Type the passage exactly · Q quits',
  start: function (host, p) {
    var mode = (p.variant >= 0 && p.variant < TYPING_MODES.length) ? p.variant : 0;
    var BANK = W.typing[TYPING_MODES[mode]];
    var text, pos, errors, t0, done;
    function reset() { text = pick(BANK); pos = 0; errors = 0; t0 = 0; done = null; }
    reset();
    return {
      key: function (k) {
        if (done) { reset(); return; }
        if (!t0) t0 = Date.now();
        if (k === 'back') { if (pos) pos--; return; }
        var ch = (k === 'space' || k === 'enter') ? ' ' : k;
        if (ch.length !== 1) return;
        if (ch === text[pos]) pos++;
        else errors++;
        if (pos >= text.length) {
          var el = Math.max(1, (Date.now() - t0) / 1000);
          var wpm = Math.round((text.length / 5) / (el / 60));
          var acc = Math.round(text.length * 100 / (text.length + errors));
          done = wpm + ' WPM · ' + acc + '% accuracy · ' + Math.round(el) + 's';
          host.saveScore(wpm);
        }
      },
      draw: function (t) {
        t.header('TYPING TEST', TYPING_MODES[mode] + ' · type it exactly · Q quits');
        var x = 4, y = 5;
        for (var i = 0; i < text.length; i++) {
          if (x >= 80) { x = 4; y++; }
          t.put(x++, y, text[i],
                i < pos ? C.green : i === pos ? C.black : C.grey,
                i === pos ? C.yellow : null, i === pos);
        }
        var el = t0 ? Math.round((Date.now() - t0) / 1000) : 0;
        var wpm = (t0 && el) ? Math.round((pos / 5) / (el / 60)) : 0;
        t.text(4, y + 3, 'Time ' + el + 's   WPM ' + wpm + '   Errors ' + errors + '    ', C.fg);
        if (done) t.center(y + 5, done + ' — press any key.', C.green, true);
      }
    };
  }
});

/* ------------------------------------------------------- guess the number */
reg('guessnumber', {
  title: 'Guess the Number', help: 'Type digits · Enter guesses · Q quits',
  start: function (host, p) {
    var hi = Math.max(2, p.count > 0 ? p.count : 100);
    var optimal = 1, span = hi;
    while (span > 1) { span = Math.floor(span / 2); optimal++; }
    var secret, tries, typing, lo_b, hi_b, done;
    function reset() {
      secret = G.rndRange(1, hi);
      tries = 0; typing = ''; lo_b = 1; hi_b = hi; done = null;
    }
    reset();
    return {
      key: function (k) {
        if (done) { reset(); return; }
        if (k === 'back') { typing = typing.slice(0, -1); return; }
        if (k >= '0' && k <= '9') { if (typing.length < 9) typing += k; return; }
        if (k !== 'enter') return;
        var g = parseInt(typing, 10);
        typing = '';
        if (!g || g < 1 || g > hi) return;
        tries++;
        if (g === secret) { done = 'Got it — ' + secret + ' in ' + tries + '.'; host.saveScore(Math.floor(optimal * 200 / tries)); }
        else if (g < secret) lo_b = Math.max(lo_b, g + 1);
        else hi_b = Math.min(hi_b, g - 1);
      },
      draw: function (t) {
        t.header('GUESS THE NUMBER', 'A number from 1 to ' + hi);
        t.text(28, 6, 'Known range: ' + lo_b + ' to ' + hi_b + '           ', C.fg);
        t.text(28, 7, 'Guesses so far: ' + tries + '        ', C.fg);
        t.text(28, 9, 'Your guess: ' + typing + '_          ', C.cyan);
        t.text(28, 11, 'Binary search needs about ' + optimal + '.', C.dim);
        if (done) t.center(14, done + ' Press any key.', C.green, true);
      }
    };
  }
});

/* ---------------------------------------------------------- bulls & cows */
reg('bullscows', {
  title: 'Bulls and Cows', help: 'Type digits · Enter guesses · Q quits',
  start: function (host, p) {
    var nd = Math.max(2, Math.min(8, p.count > 0 ? p.count : 4));
    var allowed = 10 - nd + 2;
    var secret, hist, typing, done, note;
    function reset() {
      var used = {}, d;
      secret = '';
      while (secret.length < nd) {
        d = rnd(10);
        if (used[d] || (secret.length === 0 && d === 0)) continue;
        used[d] = 1; secret += d;
      }
      hist = []; typing = ''; done = null; note = '';
    }
    reset();
    return {
      key: function (k) {
        if (done) { reset(); return; }
        if (k === 'back') { typing = typing.slice(0, -1); return; }
        if (k >= '0' && k <= '9') { if (typing.length < nd) typing += k; note = ''; return; }
        if (k !== 'enter') return;
        if (typing.length !== nd) { note = nd + ' digits, please.'; return; }
        if (new Set(typing.split('')).size !== nd) {
          note = 'All ' + nd + ' digits must differ.';
          typing = '';
          return;
        }
        var bulls = 0, cows = 0;
        for (var i = 0; i < nd; i++) {
          if (typing[i] === secret[i]) bulls++;
          else if (secret.indexOf(typing[i]) >= 0) cows++;
        }
        hist.push(typing + '   ' + bulls + ' bulls, ' + cows + ' cows');
        if (bulls === nd) { done = 'Cracked it in ' + hist.length + '!'; host.saveScore((allowed - hist.length + 1) * 100); }
        else if (hist.length >= allowed) done = 'Out of guesses — it was ' + secret + '.';
        typing = '';
      },
      draw: function (t) {
        t.header('BULLS AND COWS', nd + ' digits, all different · ' + allowed + ' guesses');
        hist.forEach(function (h, i) { t.text(28, 4 + i, h, C.dim); });
        t.text(28, 5 + allowed, 'Guess ' + Math.min(allowed, hist.length + 1) + ' of ' + allowed +
                                ': ' + typing + '_     ', C.cyan);
        if (note) t.text(28, 7 + allowed, note + '                    ', C.red);
        if (done) t.center(9 + allowed, done + ' Press any key.', C.white, true);
      }
    };
  }
});

/* ------------------------------------------------- rock paper scissors */
reg('rps', {
  title: 'Rock Paper Scissors', help: 'R rock · P paper · S scissors · Q quits',
  start: function (host, p) {
    var NAME = ['Rock','Paper','Scissors'], ART = ['✊','✋','✌'];
    var freq = [0,0,0], wins = 0, losses = 0, draws = 0, me = null, ai = null, msg = '';
    return {
      key: function (k) {
        var pick = k === 'r' ? 0 : k === 'p' ? 1 : k === 's' ? 2 : -1;
        if (pick < 0) return;
        me = pick;
        var most = 0;
        for (var i = 1; i < 3; i++) if (freq[i] > freq[most]) most = i;
        /* Counter the player's most frequent throw most of the time. */
        ai = (freq[most] > 0 && rnd(100) < 65) ? (most + 1) % 3 : rnd(3);
        freq[me]++;
        var result = (me - ai + 3) % 3;
        if (result === 1) { wins++; msg = 'You win the round!'; }
        else if (result === 2) { losses++; msg = 'Computer wins.'; }
        else { draws++; msg = 'Draw.'; }
        host.saveScore(wins);
      },
      draw: function (t) {
        t.header('ROCK PAPER SCISSORS', 'R rock · P paper · S scissors · Q quits');
        t.text(28, 6, 'Wins ' + wins + '  Losses ' + losses + '  Draws ' + draws + '   ', C.fg);
        t.text(28, 8, 'The computer tracks your habits.', C.dim);
        if (me !== null) {
          t.text(28, 11, 'You: ' + ART[me] + ' ' + (NAME[me] + '        ').slice(0, 9) +
                         '   CPU: ' + ART[ai] + ' ' + NAME[ai] + '        ', C.white);
          t.text(28, 13, msg + '                    ', C.yellow, null, true);
        }
      }
    };
  }
});

/* ------------------------------------------------------------------ simon */
reg('simon', {
  title: 'Simon', help: 'Watch, then repeat with R G B Y · Q quits',
  realtime: true, step: 450,
  start: function (host, p) {
    var CN = ['RED','GREEN','BLUE','YELLOW'], CC = [C.red, C.green, C.blue, C.yellow];
    var KEYS = ['r','g','b','y'];
    var seq, showIdx, mode, inputIdx, flash, over;
    function newRound() {
      seq.push(rnd(4));
      showIdx = 0; mode = 'show'; inputIdx = 0; flash = -1;
    }
    function reset() { seq = []; over = null; newRound(); }
    reset();
    return {
      key: function (k) {
        if (over) { reset(); return; }
        if (mode !== 'input') return;
        var pick = KEYS.indexOf(k);
        if (pick < 0) return;
        flash = pick;
        if (pick !== seq[inputIdx]) {
          over = 'Wrong — you reached round ' + seq.length + '.';
          host.saveScore(seq.length - 1);
          return;
        }
        inputIdx++;
        if (inputIdx >= seq.length) newRound();
      },
      tick: function () {
        if (over || mode !== 'show') return;
        if (showIdx >= seq.length) { mode = 'input'; flash = -1; return; }
        flash = seq[showIdx];
        showIdx++;
      },
      draw: function (t) {
        t.header('SIMON', 'Watch the sequence, then repeat with R G B Y');
        t.text(30, 6, 'Round ' + seq.length, C.fg, null, true);
        if (flash >= 0) {
          t.text(30, 9, '  ██████  ', CC[flash], null, true);
          t.text(30, 11, (CN[flash] + '        ').slice(0, 8), CC[flash], null, true);
        } else {
          t.text(30, 9, '          ', C.grey);
          t.text(30, 11, '        ', C.grey);
        }
        t.text(30, 14, mode === 'show' ? 'Watch...' :
                       'Your turn — ' + inputIdx + '/' + seq.length + '        ', C.cyan);
        if (over) t.center(17, over + ' Press any key.', C.white, true);
      }
    };
  }
});

/* ------------------------------------------------------ snakes and ladders */
reg('snakesladders', {
  title: 'Snakes and Ladders', help: 'Enter rolls the die · Q quits',
  start: function (host, p) {
    var FROM = [1,4,9,21,28,36,51,71,80,16,47,49,56,62,64,87,93,95,98];
    var TO   = [38,14,31,42,84,44,67,91,100,6,26,11,53,19,60,24,73,75,78];
    var pos, turn, msg, over;
    function reset() { pos = [0, 0]; turn = 0; msg = 'Press Enter to roll.'; over = null; }
    function move(who) {
      var die = G.rndRange(1, 6), from = pos[who];
      if (from + die <= 100) pos[who] = from + die;
      msg = (who ? 'Computer' : 'You') + ' rolled ' + die + ': ' + from + ' → ' + pos[who];
      var i = FROM.indexOf(pos[who]);
      if (i >= 0) {
        msg += ', then ' + (TO[i] > pos[who] ? 'climbs' : 'slides') + ' to ' + TO[i] + '!';
        pos[who] = TO[i];
      }
      if (pos[who] >= 100) {
        over = who ? 'Computer reaches 100.' : 'You reach 100 — you win!';
        host.saveScore(who ? 0 : 100);
      }
    }
    reset();
    return {
      key: function (k) {
        if (over) { reset(); return; }
        if (k !== 'enter' && k !== 'space') return;
        move(0);
        if (!over) move(1);
      },
      draw: function (t) {
        t.header('SNAKES AND LADDERS', 'Enter rolls the die · Q quits');
        for (var i = 0; i < 10; i++) {
          for (var c = 0; c < 10; c++) {
            var row = 9 - i;
            var sq = row * 10 + (row % 2 === 0 ? c + 1 : 10 - c);
            var j = FROM.indexOf(sq);
            var col = j < 0 ? C.grey : (TO[j] > sq ? C.green : C.red);
            var label;
            if (pos[0] === sq && pos[1] === sq) { label = '[**]'; col = C.white; }
            else if (pos[0] === sq) { label = '[Y ]'; col = C.cyan; }
            else if (pos[1] === sq) { label = '[ C]'; col = C.magenta; }
            else label = String(sq).padStart(3, ' ') + ' ';
            t.text(22 + c * 4, 4 + i, label, col, null, label[0] === '[');
          }
        }
        t.text(22, 15, 'You: ' + pos[0] + '   CPU: ' + pos[1] + '     ', C.fg);
        t.text(22, 16, 'Green = ladder, red = snake', C.dim);
        t.text(22, 18, (msg || '') + '                                        ', C.white);
        if (over) t.center(20, over + ' Press any key.', C.white, true);
      }
    };
  }
});

/* ----------------------------------------------------------- virtual piano */
reg('piano', {
  title: 'Virtual Piano', help: 'Play with z s x d c v g b h n j m and q 2 w 3 e r 5 t 6 y 7 u',
  start: function (host, p) {
    var KEYS = 'zsxdcvgbhnjm,l.;/q2w3er5t6y7ui9o0p';
    var NAMES = ['C4','C#4','D4','D#4','E4','F4','F#4','G4','G#4','A4','A#4','B4',
                 'C5','C#5','D5','D#5','E5','F5','F#5','G5','G#5','A5','A#5','B5',
                 'C6','C#6','D6','D#6','E6','F6','F#6','G6','G#6','A6'];
    var FREQ = [262,277,294,311,330,349,370,392,415,440,466,494,
                523,554,587,622,659,698,740,784,831,880,932,988,
                1047,1109,1175,1245,1319,1397,1480,1568,1661,1760];
    var last = null, history = [], audio = null;
    function tone(hz) {
      try {
        if (!audio) audio = new (window.AudioContext || window.webkitAudioContext)();
        var osc = audio.createOscillator(), gain = audio.createGain();
        osc.type = 'triangle';
        osc.frequency.value = hz;
        gain.gain.setValueAtTime(0.18, audio.currentTime);
        gain.gain.exponentialRampToValueAtTime(0.001, audio.currentTime + 0.45);
        osc.connect(gain).connect(audio.destination);
        osc.start();
        osc.stop(audio.currentTime + 0.45);
      } catch (e) { /* audio blocked: the keyboard still shows the note */ }
    }
    return {
      key: function (k) {
        var idx = KEYS.indexOf(k);
        if (idx < 0) return;
        last = idx;
        tone(FREQ[idx]);
        history.push(NAMES[idx]);
        if (history.length > 16) history.shift();
      },
      draw: function (t) {
        t.header('VIRTUAL PIANO', 'z s x d c v g b h n j m  ·  q 2 w 3 e r 5 t 6 y 7 u');
        t.text(10, 5, '  ┌─┬┬─┬┬─┬─┬┬─┬┬─┬┬─┬─┐  ┌─┬┬─┬┬─┬─┬┬─┬┬─┬┬─┬─┐', C.white);
        t.text(10, 6, '  │ ││ ││ │ ││ ││ ││ │ │  │ ││ ││ │ ││ ││ ││ │ │', C.white);
        t.text(10, 7, '  │ └┘ └┘ │ └┘ └┘ └┘ │ │  │ └┘ └┘ │ └┘ └┘ └┘ │ │', C.white);
        t.text(10, 8, '  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │', C.white);
        t.text(10, 9, '  └──┴──┴──┴──┴──┴──┴──┘  └──┴──┴──┴──┴──┴──┴──┘', C.white);
        t.text(10, 10, '   z  x  c  v  b  n  m     q  w  e  r  t  y  u', C.dim);
        t.text(10, 13, 'Recent: ' + (history.join(' ') + '                                        ').slice(0, 50), C.dim);
        if (last !== null)
          t.text(10, 15, '♪  ' + NAMES[last] + '   ' + FREQ[last] + ' Hz        ', C.yellow, null, true);
      }
    };
  }
});

/* --------------------------------------------------------- dungeon crawl */
reg('dungeon', {
  title: 'Dungeon Crawl', help: 'Arrows move · P drinks a potion · > descends · Q quits',
  start: function (host, p) {
    var W = 64, H = 20, MAXROOM = 9;
    /* Theme drives the monster roster and the hazard that bites in the open,
     * so a Volcano run plays differently rather than just reading differently. */
    var THEMES = [
      {name:'Catacombs',    glyphs:'zZsSwW', hazard:'the crypt air saps you'},
      {name:'Caverns',      glyphs:'bBrRtT', hazard:'loose rock underfoot'},
      {name:'Sewers',       glyphs:'rRsSgG', hazard:'the fumes burn'},
      {name:'Ice Caves',    glyphs:'iIwWyY', hazard:'you slide on the ice'},
      {name:'Volcano',      glyphs:'fFdDmM', hazard:'the heat scorches'},
      {name:'Sky Temple',   glyphs:'aAhHgG', hazard:'the wind tears at you'},
      {name:'Derelict Ship',glyphs:'dDkKxX', hazard:'a hull breach hisses'},
      {name:'Forest Depths',glyphs:'wWbBsS', hazard:'dense growth blocks your view'}
    ];
    var TI = -1, ti;
    for (ti = 0; ti < THEMES.length; ti++) if (THEMES[ti].name === p.theme) TI = ti;
    if (TI < 0) TI = (p.variant >= 0 && p.variant < THEMES.length) ? p.variant : 0;
    var TH = THEMES[TI];
    var TARGET = p.level > 0 ? p.level : 10;
    var map, seen, rooms, mon, px, py, hp, maxhp, atk, gold, depth, potions, log, dead, won;
    function logmsg(s) { log.unshift(s); if (log.length > 2) log.pop(); }
    function carveH(x1, x2, y) {
      for (var x = Math.min(x1, x2); x <= Math.max(x1, x2); x++)
        if (y > 0 && y < H - 1) map[y][x] = '.';
    }
    function carveV(y1, y2, x) {
      for (var y = Math.min(y1, y2); y <= Math.max(y1, y2); y++)
        if (x > 0 && x < W - 1) map[y][x] = '.';
    }
    function generate() {
      map = []; seen = [];
      for (var r = 0; r < H; r++) {
        map.push(new Array(W).fill('#'));
        seen.push(new Array(W).fill(0));
      }
      rooms = [];
      for (var t2 = 0; t2 < 120 && rooms.length < MAXROOM; t2++) {
        var n = { w: G.rndRange(5, 12), h: G.rndRange(3, 5) };
        n.x = G.rndRange(1, W - n.w - 2);
        n.y = G.rndRange(1, H - n.h - 2);
        var ok = rooms.every(function (o) {
          return !(n.x < o.x + o.w + 1 && n.x + n.w + 1 > o.x &&
                   n.y < o.y + o.h + 1 && n.y + n.h + 1 > o.y);
        });
        if (!ok) continue;
        for (var rr = n.y; rr < n.y + n.h; rr++)
          for (var cc = n.x; cc < n.x + n.w; cc++) map[rr][cc] = '.';
        if (rooms.length) {
          var prev = rooms[rooms.length - 1];
          var cx = n.x + (n.w >> 1), cy = n.y + (n.h >> 1);
          var pxx = prev.x + (prev.w >> 1), pyy = prev.y + (prev.h >> 1);
          if (rnd(2)) { carveH(pxx, cx, pyy); carveV(pyy, cy, cx); }
          else { carveV(pyy, cy, pxx); carveH(pxx, cx, cy); }
        }
        rooms.push(n);
      }
      px = rooms[0].x + (rooms[0].w >> 1);
      py = rooms[0].y + (rooms[0].h >> 1);
      var lastR = rooms[rooms.length - 1];
      map[lastR.y + (lastR.h >> 1)][lastR.x + (lastR.w >> 1)] = '>';

      mon = [];
      var G_ = TH.glyphs;
      for (var i = 0; i < Math.min(12, rooms.length * 2); i++) {
        var ri = 1 + rnd(Math.max(1, rooms.length - 1));
        var room = rooms[ri];
        mon.push({ x: room.x + rnd(room.w), y: room.y + rnd(room.h),
                   hp: 4 + depth * 2 + rnd(4), atk: 1 + (depth >> 1) + rnd(2),
                   glyph: G_[rnd(6)] });
      }
      for (var j = 0; j < 6 + depth; j++) {
        var rm = rooms[rnd(rooms.length)];
        var x2 = rm.x + rnd(rm.w), y2 = rm.y + rnd(rm.h);
        if (map[y2][x2] === '.') map[y2][x2] = rnd(5) === 0 ? '!' : '$';
      }
      vision();
    }
    function vision() {
      for (var r = py - 4; r <= py + 4; r++)
        for (var c = px - 8; c <= px + 8; c++)
          if (r >= 0 && r < H && c >= 0 && c < W) seen[r][c] = 1;
    }
    function blocked(x, y) {
      return x < 0 || x >= W || y < 0 || y >= H || map[y][x] === '#';
    }
    function monAt(x, y) {
      for (var i = 0; i < mon.length; i++) if (mon[i].x === x && mon[i].y === y) return i;
      return -1;
    }
    function monstersAct() {
      mon.forEach(function (m) {
        var dx = px - m.x, dy = py - m.y;
        if (dx * dx + dy * dy > 64) return;
        if (Math.abs(dx) <= 1 && Math.abs(dy) <= 1) {
          hp -= m.atk;
          logmsg('The ' + m.glyph + ' hits you for ' + m.atk + '.');
          return;
        }
        var nx = m.x + Math.sign(dx), ny = m.y + Math.sign(dy);
        if (!blocked(nx, m.y) && monAt(nx, m.y) < 0) m.x = nx;
        else if (!blocked(m.x, ny) && monAt(m.x, ny) < 0) m.y = ny;
      });
    }
    function reset() {
      depth = 1; hp = maxhp = 24; atk = 4; gold = 0; potions = 1;
      log = []; dead = false; won = false;
      logmsg('You enter the ' + TH.name + '. Reach depth ' + TARGET + ' to escape.');
      generate();
    }
    reset();
    return {
      key: function (k) {
        if (dead || won) { reset(); return; }
        var nx = px, ny = py, acted = false;
        if (k === 'up') ny--;
        if (k === 'down') ny++;
        if (k === 'left') nx--;
        if (k === 'right') nx++;
        if (k === 'p') {
          if (potions > 0) {
            var heal = 10 + rnd(8);
            potions--; hp = Math.min(maxhp, hp + heal);
            logmsg('You drink a potion and recover ' + heal + ' HP.');
            acted = true;
          } else logmsg('You have no potions.');
        }
        if (k === '>' || k === '.') {
          if (map[py][px] === '>') {
            depth++; maxhp += 4; hp = maxhp; atk++;
            if (depth > TARGET) { won = true; host.saveScore(gold + TARGET * 300); return; }
            generate();
            logmsg('You descend to depth ' + depth + ' of ' + TARGET + '.');
            return;
          }
          logmsg('There are no stairs here.');
        }
        if (nx !== px || ny !== py) {
          var mi = monAt(nx, ny);
          if (mi >= 0) {
            var dmg = atk + rnd(3);
            mon[mi].hp -= dmg;
            if (mon[mi].hp <= 0) {
              logmsg('You kill the ' + mon[mi].glyph + '!');
              gold += 5 + rnd(10) * depth;
              mon.splice(mi, 1);
            } else logmsg('You hit the ' + mon[mi].glyph + ' for ' + dmg + '.');
            acted = true;
          } else if (!blocked(nx, ny)) {
            px = nx; py = ny; acted = true;
            if (map[py][px] === '$') {
              var g2 = 5 + rnd(20);
              gold += g2; map[py][px] = '.';
              logmsg('You pick up ' + g2 + ' gold.');
            } else if (map[py][px] === '!') {
              potions++; map[py][px] = '.';
              logmsg('You find a healing potion.');
            }
          }
        }
        if (acted) {
          monstersAct();
          if (rnd(100) < 6 + depth) { hp -= 1 + Math.floor(depth / 5); logmsg(TH.hazard); }
          vision();
          if (hp <= 0) { dead = true; host.saveScore(gold + depth * 100); }
        }
      },
      draw: function (t) {
        t.header('DUNGEON CRAWL', TH.name + ' · reach depth ' + TARGET +
                 ' · arrows move · P potion · > descends');
        for (var r = 0; r < H; r++) for (var c = 0; c < W; c++) {
          if (!seen[r][c]) continue;
          var lit = r >= py - 4 && r <= py + 4 && c >= px - 8 && c <= px + 8;
          var ch = map[r][c], g2, fg;
          if (ch === '#') { g2 = '#'; fg = lit ? C.blue : '#2a2f36'; }
          else if (ch === '>') { g2 = '>'; fg = C.magenta; }
          else if (ch === '$') { g2 = '$'; fg = C.yellow; }
          else if (ch === '!') { g2 = '!'; fg = C.green; }
          else { g2 = '.'; fg = lit ? C.grey : '#2a2f36'; }
          t.put(8 + c, 4 + r, g2, fg);
        }
        mon.forEach(function (m) {
          if (m.y < py - 4 || m.y > py + 4 || m.x < px - 8 || m.x > px + 8) return;
          t.put(8 + m.x, 4 + m.y, m.glyph, C.red, null, true);
        });
        t.put(8 + px, 4 + py, '@', C.cyan, null, true);
        t.text(8, H + 5, 'HP ' + hp + '/' + maxhp + '  Atk ' + atk + '  Gold ' + gold +
                         '  Potions ' + potions + '  Depth ' + depth + '/' + TARGET + '   ',
               hp < maxhp / 3 ? C.red : C.green);
        log.forEach(function (l, i) { t.text(8, H + 6 + i, (l + '                                                      ').slice(0, 60), C.dim); });
        if (dead) t.center(H + 9, 'You have died on depth ' + depth + '. Press any key.', C.red, true);
        if (won) t.center(H + 9, 'You escaped the ' + TH.name + ' with ' + gold + ' gold!', C.green, true);
      }
    };
  }
});

}());
