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

/* -------------------------------------------------------- rock paper scissors
 * With the throws in this cyclic order, a throw beats the n/2 entries behind
 * it — one rule covering both the 3-throw and 5-throw games.
 */
reg('rps', {
  title: 'Rock Paper Scissors', help: 'Press a throw key · Q quits',
  start: function (host, p) {
    var VAR = p.variant | 0;
    var N3 = ['Rock','Paper','Scissors'], K3 = ['r','p','s'];
    var N5 = ['Rock','Spock','Paper','Lizard','Scissors'], K5 = ['r','k','p','l','s'];
    var VERB = [
      ['','','covers','crushes','crushes'],
      ['vaporizes','','','poisons','smashes'],
      ['covers','disproves','','','eats'],
      ['crushes','poisons','eats','',''],
      ['crushes','smashes','cuts','decapitates','']];
    var n = VAR === 1 ? 5 : 3;
    var NAME = VAR === 1 ? N5 : N3, KEYS = VAR === 1 ? K5 : K3;
    var target = VAR === 2 ? 5 : 0;
    var freq = [0,0,0,0,0], wins = 0, losses = 0, draws = 0, last = null, done = null;

    return {
      key: function (k) {
        if (done) { wins = losses = draws = 0; freq = [0,0,0,0,0]; last = null; done = null; return; }
        var me = KEYS.indexOf(k);
        if (me < 0) return;
        var most = 0, i;
        for (i = 1; i < n; i++) if (freq[i] > freq[most]) most = i;
        var ai = (freq[most] > 0 && rnd(100) < 65) ? (most + 1) % n : rnd(n);
        freq[me]++;
        var d = (me - ai + n) % n;
        if (d === 0) { draws++; last = 'Draw.'; }
        else if (d <= n / 2) { wins++; last = NAME[me] + ' ' + (VERB[me][ai] || 'beats') + ' ' + NAME[ai] + ' — you win!'; }
        else { losses++; last = NAME[ai] + ' ' + (VERB[ai][me] || 'beats') + ' ' + NAME[me] + ' — computer wins.'; }
        if (target && (wins >= target || losses >= target)) {
          done = wins >= target ? 'You take the match!' : 'Computer takes the match.';
          host.saveScore(wins);
        }
      },
      draw: function (t) {
        t.header('ROCK PAPER SCISSORS',
                 KEYS.map(function (c, i) { return c + ' ' + NAME[i]; }).join('   '));
        t.text(22, 6, 'Wins ' + wins + '   Losses ' + losses + '   Draws ' + draws + '   ', C.fg);
        t.text(22, 8, target ? 'First to ' + target + ' takes the match.'
                             : 'The computer is tracking your habits.', C.dim);
        if (last) t.text(22, 11, last + '                    ', C.white, null, true);
        if (done) t.center(14, done + ' Press any key.', C.green, true);
      }
    };
  }
});

/* ------------------------------------------------------------------- simon */
reg('simon', {
  title: 'Simon', help: 'Repeat the sequence · Q quits',
  start: function (host, p) {
    var VAR = p.variant | 0;
    var CN = ['RED','GREEN','BLUE','YELLOW','MAGENTA','CYAN','WHITE','GREY'];
    var CC = [C.red,C.green,C.blue,C.yellow,C.magenta,C.cyan,C.white,C.grey];
    var KEYS = ['r','g','b','y','m','c','w','k'];
    var ncol = VAR === 1 ? 6 : VAR === 2 ? 8 : 4;
    var reverse = VAR === 3, silent = VAR === 4;
    var showTicks = VAR === 5 ? 3 : 6, gapTicks = VAR === 5 ? 1 : 2;

    var seq, step, phase, flashIdx, timer, msg, best = 0;
    function reset() { seq = []; step = 0; phase = 'grow'; flashIdx = 0; timer = 0; msg = null; }
    reset();

    return {
      tick: function () {
        if (phase === 'grow') { seq.push(rnd(ncol)); flashIdx = 0; timer = 0; phase = 'show'; return; }
        if (phase !== 'show') return;
        timer++;
        if (timer >= showTicks + gapTicks) { timer = 0; flashIdx++; if (flashIdx >= seq.length) { phase = 'input'; step = 0; } }
      },
      key: function (k) {
        if (msg) { reset(); return; }
        if (phase !== 'input') return;
        var pick = KEYS.indexOf(k);
        if (pick < 0 || pick >= ncol) return;
        var want = reverse ? seq[seq.length - 1 - step] : seq[step];
        if (pick !== want) {
          msg = 'Wrong — you reached round ' + seq.length + '.';
          host.saveScore(seq.length - 1);
          return;
        }
        step++;
        if (step >= seq.length) { if (seq.length > best) best = seq.length; phase = 'grow'; }
      },
      draw: function (t) {
        t.header('SIMON', 'Repeat' + (reverse ? ' BACKWARDS' : '') + ' with ' +
                 KEYS.slice(0, ncol).join(' ') + (silent ? ' (no sound)' : ''));
        t.text(28, 6, 'Round ' + Math.max(1, seq.length) + '   Best ' + best + '  ', C.fg);
        if (phase === 'show' && flashIdx < seq.length && timer < showTicks) {
          var s = seq[flashIdx];
          t.text(28, 9, '  ######  ', CC[s], null, true);
          t.text(28, 11, CN[s] + '        ', CC[s]);
        } else {
          t.text(28, 9, '          ', C.grey);
          t.text(28, 11, '            ', C.grey);
        }
        if (phase === 'input')
          t.text(28, 13, 'Your turn — ' + seq.length + ' step' + (seq.length === 1 ? '' : 's') +
                 (reverse ? ', backwards' : '') + '   ', C.cyan);
        else t.text(28, 13, '                                  ', C.grey);
        if (msg) t.center(16, msg + ' Press any key.', C.white, true);
      }
    };
  }
});

/* --------------------------------------------------------------- race games
 * Eight historical race games separated by five switches: board length, what
 * you throw, exact finish, an entry roll, and squares granting another turn.
 */
reg('snakesladders', {
  title: 'Race', help: 'Enter throws · Q quits',
  start: function (host, p) {
    var SL_F=[1,4,9,21,28,36,51,71,80,16,47,49,56,62,64,87,93,95,98];
    var SL_T=[38,14,31,42,84,44,67,91,100,6,26,11,53,19,60,24,73,75,78];
    var DX_F=SL_F.concat([6,20,33,45,68]), DX_T=SL_T.concat([27,41,12,72,50]);
    var RACES = [
      {name:'Snakes and Ladders',        sq:100,dice:0,exact:0,entry:0,ros:0,f:SL_F,t:SL_T},
      {name:'Chutes and Ladders Deluxe', sq:100,dice:0,exact:0,entry:0,ros:0,f:DX_F,t:DX_T},
      {name:'Game of the Goose',         sq:63, dice:0,exact:1,entry:0,ros:0,
        f:[6,12,18,24,30,36,42,48,54,58,19,31,42,52], t:[12,18,24,30,36,42,48,54,60,63,55,12,26,30]},
      {name:'Pachisi',                   sq:60, dice:0,exact:1,entry:0,ros:0,f:[12,25,38,51],t:[30,44,57,60]},
      {name:'Ludo',                      sq:56, dice:0,exact:1,entry:6,ros:0,f:[9,22,35,48],t:[27,40,53,56]},
      {name:'Senet',                     sq:30, dice:1,exact:1,entry:0,ros:0,f:[15,26,27,28],t:[1,15,15,15]},
      {name:'Royal Game of Ur',          sq:20, dice:1,exact:1,entry:0,ros:1,f:[],t:[]},
      {name:'Yut Nori',                  sq:29, dice:2,exact:0,entry:0,ros:0,f:[5,10,22],t:[15,20,27]}];
    var R = RACES[(p.variant >= 0 && p.variant < 8) ? p.variant : 0];
    var cols = 10, rows = Math.ceil(R.sq / cols);
    var pos, live, turn, msg, note, cpuWait;

    function rosette(sq) { return sq === 4 || sq === 8 || sq === 14; }
    function roll() {
      if (R.dice === 1) { var s = 0; for (var i = 0; i < 4; i++) s += rnd(2); return s === 0 ? 5 : s; }
      if (R.dice === 2) { var W = [35,30,20,10,5], r = rnd(100), a = 0;
        for (var j = 0; j < 5; j++) { a += W[j]; if (r < a) return j + 1; } return 1; }
      return 1 + rnd(6);
    }
    function reset() {
      pos = [0,0]; live = [!R.entry, !R.entry]; turn = 0; msg = null; note = ''; cpuWait = 0;
    }
    function step() {
      var die = roll(), from = pos[turn], again = 0, who = turn === 0 ? 'You' : 'Computer';
      if (!live[turn]) {
        if (die === R.entry) { live[turn] = 1; pos[turn] = 1; note = who + ' rolled ' + die + ' and enters.'; }
        else note = who + ' rolled ' + die + ' — still waiting for a ' + R.entry + '.';
      } else if (from + die > R.sq && R.exact) {
        note = who + ' rolled ' + die + ' — overshoots, no move.';
      } else {
        pos[turn] = Math.min(R.sq, from + die);
        note = who + ' rolled ' + die + ': ' + from + ' -> ' + pos[turn];
        for (var i = 0; i < R.f.length; i++) if (R.f[i] === pos[turn]) {
          note = who + ' rolled ' + die + ': ' + from + ' -> ' + pos[turn] + ', then ' +
                 (R.t[i] > pos[turn] ? 'climbs' : 'falls back') + ' to ' + R.t[i] + '!';
          pos[turn] = R.t[i];
          break;
        }
        if (R.ros && rosette(pos[turn])) { again = 1; note += '  Rosette — throw again!'; }
      }
      if (pos[turn] >= R.sq) {
        msg = turn === 0 ? 'You reach home — you win!' : 'Computer reaches home first.';
        if (turn === 0) host.saveScore(R.sq);
        return;
      }
      if (!again) turn = 1 - turn;
    }
    reset();
    return {
      tick: function () {
        if (msg || turn !== 1) return;
        if (++cpuWait < 3) return;
        cpuWait = 0;
        step();
      },
      key: function (k) {
        if (msg) { reset(); return; }
        if (turn !== 0) return;
        if (k !== 'enter' && k !== 'space') return;
        step();
      },
      draw: function (t) {
        t.header('RACE', R.name + ' — ' +
                 (R.dice === 1 ? 'throw sticks' : R.dice === 2 ? 'throw yut' : 'roll a die') +
                 (R.exact ? ', exact finish' : '') + (R.entry ? ', roll a 6 to start' : ''));
        for (var i = 0; i < rows; i++) for (var c = 0; c < cols; c++) {
          var row = rows - 1 - i;
          var sq = row * cols + ((row % 2 === 0) ? c + 1 : cols - c);
          var x = 14 + c * 5, y = 4 + i;
          if (sq > R.sq) { t.text(x, y, '     ', C.grey); continue; }
          var col = C.grey, j;
          for (j = 0; j < R.f.length; j++) if (R.f[j] === sq) col = R.t[j] > sq ? C.green : C.red;
          if (R.ros && rosette(sq)) col = C.magenta;
          if (pos[0] === sq && pos[1] === sq) t.text(x, y, '[**] ', C.white, null, true);
          else if (pos[0] === sq) t.text(x, y, '[Y ] ', C.cyan, null, true);
          else if (pos[1] === sq) t.text(x, y, '[ C] ', C.magenta, null, true);
          else t.text(x, y, String(sq).padStart(3, ' ') + '  ', col);
        }
        t.text(14, 5 + rows, 'You: ' + pos[0] + '   CPU: ' + pos[1] + '   ' +
               (turn === 0 ? 'Your turn ' : 'CPU turn  '), C.fg);
        t.text(14, 7 + rows, note + '                                        ', C.white);
        if (msg) t.center(9 + rows, msg + ' Press any key.', C.green, true);
      }
    };
  }
});

/* ------------------------------------------------------ keyboard instrument */
reg('piano', {
  title: 'Piano', help: 'Play the keys · Q quits',
  start: function (host, p) {
    var PKEYS = 'zsxdcvgbhnjm,l.;/q2w3er5t6y7ui9o0p'.split('');
    var PN = ['C4','C#4','D4','D#4','E4','F4','F#4','G4','G#4','A4','A#4','B4',
              'C5','C#5','D5','D#5','E5','F5','F#5','G5','G#5','A5','A#5','B5',
              'C6','C#6','D6','D#6','E6','F6','F#6','G6','G#6','A6'];
    var PF = [262,277,294,311,330,349,370,392,415,440,466,494,523,554,587,622,
              659,698,740,784,831,880,932,988,1047,1109,1175,1245,1319,1397,
              1480,1568,1661,1760];
    var IVAL = ['unison','minor 2nd','major 2nd','minor 3rd','major 3rd',
                'perfect 4th','tritone','perfect 5th','minor 6th','major 6th',
                'minor 7th','major 7th','octave'];
    var CHORD = ['major','minor','diminished','augmented','dominant 7th'];
    var CIVL = [[0,4,7],[0,3,7],[0,3,6],[0,4,8],[0,4,7,10]];
    var SCALE = ['major','natural minor','pentatonic','blues'];
    var SIVL = [[0,2,4,5,7,9,11,12],[0,2,3,5,7,8,10,12],[0,2,4,7,9,12],[0,3,5,6,7,10,12]];
    var V = p.variant | 0;

    /* WebAudio if the browser gives it to us; silent everywhere else. */
    var actx = null;
    /* delay is scheduled on the audio clock rather than with a timer: it is
     * sample-accurate, and it keeps this working under the headless test stub
     * where no setTimeout exists. */
    function tone(f, ms, delay) {
      try {
        if (!actx && typeof AudioContext !== 'undefined') actx = new AudioContext();
        if (!actx) return;
        var at = actx.currentTime + (delay || 0) / 1000;
        var o = actx.createOscillator(), g = actx.createGain();
        o.frequency.value = f; o.type = 'sine';
        g.gain.value = 0.06;
        o.connect(g); g.connect(actx.destination);
        o.start(at); o.stop(at + ms / 1000);
      } catch (e) { /* no audio available */ }
    }
    function idxOf(k) { return PKEYS.indexOf(k); }

    var history = [], score = 0, asked = 0, note = '', target = null, step = 0;
    var seq = [], phase = 'grow', flash = 0, timer = 0, sub = [], nlen = 0, root = 0, kind = 0;
    var lane = [0,0,0,0,0,0].map(function(){return -1;}), misses = 0, tick = 0;
    var drum = [[],[],[],[]], i0;
    for (i0 = 0; i0 < 4; i0++) for (var j0 = 0; j0 < 16; j0++) drum[i0][j0] = 0;

    function newQuestion() {
      if (V === 2 || V === 8) { target = rnd(24); note = V === 2 ? 'Find ' + PN[target] : 'Listen...'; if (V === 8) tone(PF[target], 320); }
      else if (V === 3) { root = rnd(21); step = 1 + rnd(12); note = 'Listen to the two notes...';
                          tone(PF[root], 200); tone(PF[root + step], 200, 270); }
      else if (V === 4 || V === 5) {
        root = rnd(12); kind = rnd(V === 4 ? 5 : 4);
        sub = V === 4 ? CIVL[kind] : SIVL[kind]; nlen = sub.length; step = 0;
        note = PN[root] + ' ' + (V === 4 ? CHORD[kind] : SCALE[kind]);
      }
    }
    if (V >= 2 && V <= 5 || V === 8) newQuestion();

    var MODE = {0:'VIRTUAL PIANO',1:'RHYTHM RUNNER',2:'NOTE TRAINER',3:'EAR TRAINING',
                4:'CHORD BUILDER',5:'SCALE PRACTICE',6:'DRUM MACHINE',7:'MELODY MEMORY',
                8:'PERFECT PITCH',9:'METRONOME',10:'SEQUENCER'}[V] || 'PIANO';

    return {
      tick: function () {
        tick++;
        if (V === 1) {
          for (var i = 0; i < 6; i++) {
            if (lane[i] >= 0) lane[i]++;
            if (lane[i] > 8) { lane[i] = -1; misses++; }
          }
          if (rnd(100) < 40) lane[rnd(6)] = 0;
        } else if (V === 6) {
          for (var d = 0; d < 4; d++) if (drum[d][tick % 16]) tone(120 + d * 180, 60);
        } else if (V === 9) {
          if (tick % 4 === 0) tone(880, 70);
        } else if (V === 7 && phase === 'show') {
          timer++;
          if (timer >= 5) { timer = 0; flash++; if (flash >= seq.length) { phase = 'input'; step = 0; } }
        } else if (V === 7 && phase === 'grow') {
          seq.push(rnd(12)); flash = 0; timer = 0; phase = 'show';
        }
      },
      key: function (k) {
        var idx = idxOf(k);
        if (V === 0 || V === 10) {
          if (idx < 0) return;
          tone(PF[idx], 180);
          history.push(PN[idx]);
          if (history.length > 14) history.shift();
          if (V === 10) score++;
          return;
        }
        if (V === 1) {
          if (idx < 0 || idx >= 6) return;
          if (lane[idx] >= 6) { score++; tone(PF[idx], 120); lane[idx] = -1; } else misses++;
          return;
        }
        if (V === 6) { if (k >= '1' && k <= '4') drum[+k - 1][tick % 16] = drum[+k - 1][tick % 16] ? 0 : 1; return; }
        if (V === 9) { if (k === 'space') { if (tick % 4 !== 2) score++; else misses++; } return; }
        if (V === 7) {
          if (phase !== 'input') return;
          if (idx < 0) return;
          tone(PF[idx], 150);
          if (idx !== seq[step]) { note = 'Wrong note — phrase was ' + seq.length + ' long.'; host.saveScore(seq.length - 1); seq = []; phase = 'grow'; return; }
          step++;
          if (step >= seq.length) { score = seq.length; phase = 'grow'; }
          return;
        }
        if (V === 3) {
          var DIG = ['0','1','2','3','4','5','6','7','8','9','0','-','='];
          var got = -1;
          for (var i = 1; i <= 12; i++) if (k === DIG[i]) got = i;
          if (got < 0) return;
          asked++;
          if (got === step) { score++; note = 'Correct — ' + IVAL[step]; }
          else note = 'It was a ' + IVAL[step] + '.';
          host.saveScore(score);
          newQuestion();
          return;
        }
        if (V === 4 || V === 5) {
          if (idx < 0) return;
          tone(PF[idx], 160);
          if (idx !== root + sub[step]) { note = 'Not quite — wanted ' + PN[root + sub[step]] + '.'; asked++; newQuestion(); return; }
          step++;
          if (step >= nlen) { score++; asked++; note = 'Correct!'; host.saveScore(score); newQuestion(); }
          return;
        }
        /* modes 2 and 8: name the note */
        if (idx < 0) return;
        tone(PF[idx], 180);
        asked++;
        if (idx === target) { score++; note = 'Correct — ' + PN[target]; }
        else note = 'That was ' + PN[idx] + '; wanted ' + PN[target] + '.';
        host.saveScore(score);
        newQuestion();
      },
      draw: function (t) {
        t.header(MODE, {0:'Play with z s x d c v g b h n j m / q 2 w 3 e r 5 t 6 y 7 u',
                        1:'Hit the note key as it reaches the line',
                        2:'Find the named note on the keyboard',
                        3:'Name the interval: 1-9 then 0 - = for 10-12',
                        4:'Play the chord, lowest note first',
                        5:'Play the scale upwards',
                        6:'1-4 toggle a drum on the current step',
                        7:'Listen, then play the phrase back',
                        8:'Which note did you hear?',
                        9:'Tap SPACE on the beat',
                        10:'Press keys to record'}[V] || '');
        if (V === 0 || V === 10) {
          t.text(10, 6, '  +-++-++-+-++-++-++-+-+  +-++-++-+-++-++-++-+-+', C.grey);
          t.text(10, 7, '  | || || | || || || | |  | || || | || || || | |', C.grey);
          t.text(10, 8, '  | ++ ++ | ++ ++ ++ | |  | ++ ++ | ++ ++ ++ | |', C.grey);
          t.text(10, 9, '  +--+--+--+--+--+--+--+  +--+--+--+--+--+--+--+', C.grey);
          t.text(10, 10, '   z  x  c  v  b  n  m     q  w  e  r  t  y  u', C.dim);
          t.text(10, 13, 'Recent: ' + history.join(' ').padEnd(52, ' '), C.yellow);
          if (V === 10) t.text(10, 15, 'Recorded ' + score + ' notes   ', C.cyan);
        } else if (V === 1) {
          for (var i = 0; i < 6; i++) {
            var x = 18 + i * 8;
            t.text(x, 5, PKEYS[i], C.grey);
            for (var y = 0; y <= 9; y++) t.text(x, 6 + y, '  ', C.grey);
            if (lane[i] >= 0) t.text(x, 6 + lane[i], lane[i] >= 6 ? '##' : '[]',
                                     lane[i] >= 6 ? C.green : C.cyan, null, true);
          }
          t.hline(16, 12, 50, C.dim);
          t.text(18, 15, 'Hits ' + score + '   Misses ' + misses + '   ', C.fg);
        } else if (V === 6) {
          var DN = ['kick','snare','hihat','clap'];
          for (var d = 0; d < 4; d++) {
            t.text(14, 6 + d * 2, DN[d].padEnd(7, ' '), C.cyan);
            for (var s2 = 0; s2 < 16; s2++)
              t.put(22 + s2 * 2, 6 + d * 2, drum[d][s2] ? '#' : '.',
                    drum[d][s2] ? C.yellow : C.grey,
                    s2 === tick % 16 ? '#2b4a6b' : null);
          }
        } else if (V === 9) {
          t.text(30, 8, tick % 4 === 0 ? '  * BEAT *  ' : '            ', C.yellow, null, true);
          t.text(28, 12, 'Hits ' + score + '   Misses ' + misses + '   ', C.fg);
        } else if (V === 7) {
          t.text(24, 7, 'Phrase of ' + Math.max(1, seq.length) + ' note(s)   ', C.fg);
          if (phase === 'show' && flash < seq.length && timer < 3)
            t.text(24, 9, PN[seq[flash]] + '      ', C.yellow, null, true);
          else t.text(24, 9, '          ', C.grey);
          if (phase === 'input') t.text(24, 11, 'Your turn      ', C.cyan);
          else t.text(24, 11, '               ', C.grey);
          t.text(24, 13, note + '                              ', C.white);
        } else {
          t.text(20, 7, note + '                                        ', C.white, null, true);
          t.text(20, 9, 'Score ' + score + ' / ' + asked + '     ', C.fg);
          if ((V === 4 || V === 5) && sub.length)
            t.text(20, 11, 'Note ' + (step + 1) + ' of ' + nlen + '     ', C.cyan);
        }
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
