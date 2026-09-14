/* drills.js - the number-drill family, mirroring src/games/mathdrill.c.
 *
 * Twenty drills sharing a prompt-and-check loop and nothing else; each
 * generates its own problem and validates its own answer.
 */
(function () {
'use strict';
var G = window.GIC, C = G.COL, reg = G.register, rnd = G.rnd;

var MNAME = ['Countdown Numbers','24 Game','Factor Game','Prime Hunt',
  'Mental Arithmetic','Binary Conversion Race','Collatz Race','Nim-Sum Trainer',
  'Fizz Buzz Duel','Equation Builder','Fraction Match','Number Sequence',
  'Modular Clock','Dice Probability','Estimation','Magic Triangle',
  'Cryptarithm','Base Conversion','GCD Duel','Pi Digit Memory'];
var PI = '141592653589793238462643383279502884197169399375';

function gcd(a, b) { while (b) { var t = a % b; a = b; b = t; } return a; }
function isPrime(n) {
  if (n < 2) return false;
  for (var i = 2; i * i <= n; i++) if (n % i === 0) return false;
  return true;
}
function collatz(n) { var s = 0; while (n !== 1 && s < 1000) { n = (n & 1) ? 3*n+1 : n/2; s++; } return s; }
/* Can these numbers reach the target with + - * / ? */
function reach(v, target) {
  if (v.length === 1) return v[0] === target;
  for (var i = 0; i < v.length; i++) for (var j = 0; j < v.length; j++) {
    if (i === j) continue;
    var rest = [], a = v[i], b = v[j], k;
    for (k = 0; k < v.length; k++) if (k !== i && k !== j) rest.push(v[k]);
    var cand = [a + b, a * b];
    if (a > b) cand.push(a - b);
    if (b && a % b === 0) cand.push(a / b);
    for (k = 0; k < cand.length; k++)
      if (reach(rest.concat([cand[k]]), target)) return true;
  }
  return false;
}

reg('mathdrill', {
  title: 'Number Drill', help: 'Type an answer, Enter submits · Q quits',
  start: function (host, p) {
    var VAR = (p.variant >= 0 && p.variant < 20) ? p.variant : 0;
    var score = 0, asked = 0, typed = '', prompt = '', note = '', expect = '', verdict = null;

    function make() {
      typed = ''; note = ''; verdict = null;
      var i, v, n, a, b;
      switch (VAR) {
      case 0: {
        var BIG = [25, 50, 75, 100], target = 0;
        v = [];
        for (var t = 0; t < 200; t++) {
          v = [];
          for (i = 0; i < 6; i++) v.push(i < 2 ? BIG[rnd(4)] : 1 + rnd(10));
          target = 100 + rnd(900);
          if (reach(v, target)) break;
        }
        prompt = 'Reach ' + target + ' using ' + v.join(' ');
        note = 'Type the target back once you have found a route.';
        expect = String(target);
        break;
      }
      case 1:
        v = [];
        for (i = 0; i < 4; i++) v.push(1 + rnd(9));
        prompt = 'Make 24 from ' + v.join(' ');
        note = reach(v, 24) ? 'This one is solvable.' : 'This one may not be solvable - type 0.';
        expect = reach(v, 24) ? '24' : '0';
        break;
      case 2:
        n = 12 + rnd(80);
        var sum = 0;
        for (i = 1; i < n; i++) if (n % i === 0) sum += i;
        prompt = 'Sum of the proper divisors of ' + n + '?';
        expect = String(sum);
        break;
      case 3:
        n = 100 + rnd(400);
        var nx = n;
        while (!isPrime(nx)) nx++;
        prompt = 'First prime at or above ' + n + '?';
        expect = String(nx);
        break;
      case 4:
        a = 2 + rnd(30); b = 2 + rnd(30);
        var op = rnd(3);
        if (op === 0) { prompt = a + ' + ' + b + ' = ?'; expect = String(a + b); }
        else if (op === 1) { prompt = a + ' x ' + b + ' = ?'; expect = String(a * b); }
        else { if (a < b) { var tmp = a; a = b; b = tmp; } prompt = a + ' - ' + b + ' = ?'; expect = String(a - b); }
        break;
      case 5:
        n = 1 + rnd(255);
        prompt = 'Write ' + n + ' in binary';
        expect = n.toString(2);
        break;
      case 6:
        n = 3 + rnd(60);
        prompt = 'Collatz steps from ' + n + ' down to 1?';
        expect = String(collatz(n));
        break;
      case 7:
        a = 1 + rnd(15); b = 1 + rnd(15);
        var c3 = 1 + rnd(15);
        prompt = 'Nim-sum (xor) of heaps ' + a + ', ' + b + ', ' + c3 + '?';
        expect = String(a ^ b ^ c3);
        break;
      case 8:
        n = 1 + rnd(100);
        prompt = 'Fizz Buzz for ' + n + '?';
        expect = n % 15 === 0 ? 'fizzbuzz' : n % 3 === 0 ? 'fizz' : n % 5 === 0 ? 'buzz' : String(n);
        note = 'Type fizz, buzz, fizzbuzz or the number.';
        break;
      case 9:
        a = 2 + rnd(12);
        var x = 1 + rnd(12);
        b = 1 + rnd(20);
        prompt = 'Solve for x:  ' + a + ' x + ' + b + ' = ' + (a * x + b);
        expect = String(x);
        break;
      case 10:
        a = 1 + rnd(9); b = 2 + rnd(9);
        var m = 2 + rnd(6), g = gcd(a * m, b * m);
        prompt = 'Simplify ' + (a * m) + '/' + (b * m) + ' - give the numerator';
        expect = String((a * m) / g);
        break;
      case 11: {
        var start = 1 + rnd(9), step = 2 + rnd(7), kind = rnd(3), seq = [];
        for (i = 0; i < 5; i++)
          seq.push(kind === 0 ? start + step * i : kind === 1 ? start * (i+1) * (i+1) : start * (1 << i));
        prompt = 'Next in the sequence: ' + seq.slice(0, 4).join(' ') + ' ?';
        expect = String(seq[4]);
        break;
      }
      case 12: {
        var h = rnd(12), add = 1 + rnd(40);
        prompt = 'It is ' + h + " o'clock. What time in " + add + ' hours? (0-11)';
        expect = String((h + add) % 12);
        break;
      }
      case 13: {
        var target2 = 2 + rnd(11), ways = 0;
        for (a = 1; a <= 6; a++) for (b = 1; b <= 6; b++) if (a + b === target2) ways++;
        prompt = 'Two dice: how many of the 36 ways total ' + target2 + '?';
        expect = String(ways);
        break;
      }
      case 14:
        a = 100 + rnd(900); b = 10 + rnd(90);
        prompt = 'Estimate ' + a + ' x ' + b + ' to the nearest thousand';
        expect = String(Math.round(a * b / 1000) * 1000);
        note = 'Within 10 percent counts as correct.';
        break;
      case 15:
        prompt = 'Place 1-6 on a triangle so each side sums to ' + (9 + rnd(6)) +
                 '. Total of all six?';
        expect = '21';
        break;
      case 16:
        a = 1 + rnd(8); b = 1 + rnd(8);
        prompt = 'If A=' + a + ' and B=' + b + ', what is AB + BA as a number?';
        expect = String((a * 10 + b) + (b * 10 + a));
        break;
      case 17: {
        n = 10 + rnd(200);
        var base = rnd(2) ? 8 : 16;
        prompt = 'Write ' + n + ' in base ' + base;
        expect = n.toString(base).toUpperCase();
        break;
      }
      case 18:
        a = 12 + rnd(200); b = 12 + rnd(200);
        prompt = 'Greatest common divisor of ' + a + ' and ' + b + '?';
        expect = String(gcd(a, b));
        break;
      default: {
        var pos = rnd(40);
        prompt = 'Digit ' + (pos + 1) + ' of pi after the point?';
        expect = PI[pos];
      }
      }
    }
    make();
    return {
      key: function (k) {
        if (verdict) { asked++; if (verdict === 'ok') score++; host.saveScore(score); make(); return; }
        if (k === 'backspace') { typed = typed.slice(0, -1); return; }
        if (k === 'enter') {
          var ok;
          if (VAR === 14) {
            var want = +expect, got = +typed, slack = want / 10 + 1;
            ok = got >= want - slack && got <= want + slack;
          } else {
            ok = typed.replace(/\s/g, '').toLowerCase() === expect.replace(/\s/g, '').toLowerCase();
          }
          verdict = ok ? 'ok' : 'no';
          return;
        }
        if (k.length === 1 && k >= ' ' && k <= '~') typed += k;
      },
      draw: function (t) {
        t.header('NUMBER DRILL', MNAME[VAR]);
        t.text(10, 6, prompt + '                                        ', C.white, null, true);
        if (note) t.text(10, 8, note + '                                   ', C.dim);
        t.text(10, 10, 'Answer: ' + typed + '_          ', C.cyan);
        if (verdict === 'ok') t.text(10, 12, 'Correct!                              ', C.green, null, true);
        if (verdict === 'no') t.text(10, 12, 'Not quite - the answer was ' + expect + '.        ', C.red, null, true);
        t.text(10, 14, 'Score ' + score + ' of ' + asked + '     ', C.fg);
        if (verdict) t.text(10, 16, 'Press any key for the next problem.', C.dim);
      }
    };
  }
});

/* --------------------------------------------------------------- reaction */
reg('reaction', {
  title: 'Reaction', help: 'Follow the on-screen instruction · Q quits',
  start: function (host, p) {
    var RNAME = ['Reaction Timer','N-Back','Digit Span','Pattern Recall',
      'Colour Match (Stroop)','Aim Trainer','Chimp Test','Visual Span',
      'Sequence Repeat','Card Memory Sprint','Audio Memory','Rhythm Tap',
      'Peripheral Vision','Flick Shot','Multi-Target','Tracking Drill',
      'Sound Cue','Colour Change'];
    var V = (p.variant >= 0 && p.variant < 18) ? p.variant : 0;
    var MEM = [1,2,8,10], GRID = [3,6,7,9], AIM = [5,13,14,15], STROOP = 4;
    var mode = MEM.indexOf(V) >= 0 ? 'mem' : GRID.indexOf(V) >= 0 ? 'grid'
             : AIM.indexOf(V) >= 0 ? 'aim' : V === STROOP ? 'stroop' : 'cue';

    /* ---- shared state ---- */
    var score = 0, best = 0, msg = '', tick = 0;

    /* cue */
    var phase = 'wait', waitTicks = 0, cueAt = 0;
    /* memory */
    var seq = [], len = 3, show = 0, typed = '';
    /* grid */
    var lit, level = 3, found = 0, cr = 0, cc = 0;
    /* aim */
    var targets = [], hits = 0, misses = 0, ax = 20, ay = 5;
    /* stroop */
    var SN = ['RED','GREEN','BLUE','YELLOW'], SC = [C.red,C.green,C.blue,C.yellow];
    var SK = ['r','g','b','y'], word = 0, ink = 0, asked = 0;

    function newGrid() {
      lit = [];
      for (var r = 0; r < 6; r++) lit.push(new Array(6).fill(0));
      for (var i = 0; i < level; i++) {
        var rr, cc2;
        do { rr = rnd(6); cc2 = rnd(6); } while (lit[rr][cc2]);
        lit[rr][cc2] = i + 1;
      }
      found = 0; show = 14; phase = 'show';
    }
    function newSeq() {
      seq = [];
      for (var i = 0; i < len; i++) seq.push(V === 10 ? rnd(6) : rnd(10));
      show = 0; typed = ''; phase = 'show';
    }
    function newTargets() {
      targets = [];
      var n = V === 14 ? 3 : 1;
      for (var i = 0; i < n; i++) targets.push({ x: rnd(40), y: rnd(12) });
    }
    function reset() {
      score = 0; best = 0; msg = ''; tick = 0;
      hits = 0; misses = 0; asked = 0; len = 3; level = 3;
      if (mode === 'grid') newGrid();
      else if (mode === 'mem') newSeq();
      else if (mode === 'aim') newTargets();
      else if (mode === 'stroop') { word = rnd(4); ink = rnd(4); }
      else { phase = 'wait'; waitTicks = 8 + rnd(26); }
    }
    reset();

    return {
      tick: function () {
        tick++;
        if (mode === 'cue') {
          if (phase === 'wait' && --waitTicks <= 0) { phase = 'go'; cueAt = tick; }
        } else if (mode === 'mem' && phase === 'show') {
          if (++show >= (seq.length + 1) * 5) phase = 'input';
        } else if (mode === 'grid' && phase === 'show') {
          if (--show <= 0) phase = 'input';
        } else if (mode === 'aim' && V === 15 && targets.length) {
          /* the tracked target drifts, so aiming has to lead it */
          targets[0].x = Math.max(0, Math.min(39, targets[0].x + rnd(3) - 1));
          targets[0].y = Math.max(0, Math.min(11, targets[0].y + rnd(3) - 1));
        }
      },
      key: function (k) {
        if (mode === 'cue') {
          if (phase === 'wait') { msg = 'Too early!'; waitTicks = 8 + rnd(26); return; }
          if (phase === 'go') {
            var ms = (tick - cueAt) * 50;
            if (!best || ms < best) best = ms;
            msg = ms + ' ms (best ' + best + ')';
            host.saveScore(best ? Math.round(100000 / best) : 0);
            phase = 'wait'; waitTicks = 8 + rnd(26);
          }
          return;
        }
        if (mode === 'stroop') {
          var pick = SK.indexOf(k);
          if (pick < 0) return;
          asked++;
          if (pick === ink) { score++; msg = 'Correct'; } else msg = 'Wrong';
          host.saveScore(Math.round(score * 100 / Math.max(1, asked)));
          word = rnd(4); ink = rnd(4);
          return;
        }
        if (mode === 'mem') {
          if (phase !== 'input') return;
          if (k === 'backspace') { typed = typed.slice(0, -1); return; }
          if (k === 'enter') {
            var ok = true;
            for (var i = 0; i < seq.length; i++) {
              var want = (V === 10 ? seq[i] + 1 : seq[i]) % 10;
              if (typed[i] !== String(want)) ok = false;
            }
            if (ok) { best = len; len++; msg = 'Correct — now ' + len; host.saveScore(best * 100); }
            else { msg = 'Wrong. Best span ' + best; len = 3; }
            newSeq();
            return;
          }
          if (/^[0-9]$/.test(k)) typed += k;
          return;
        }
        if (mode === 'grid') {
          if (phase !== 'input') return;
          if (k === 'up'    && cr > 0) cr--;
          if (k === 'down'  && cr < 5) cr++;
          if (k === 'left'  && cc > 0) cc--;
          if (k === 'right' && cc < 5) cc++;
          if (k !== 'enter' && k !== 'space') return;
          var want2 = (V === 6) ? found + 1 : null;
          if (lit[cr][cc] > 0 && (want2 === null || lit[cr][cc] === want2)) {
            lit[cr][cc] = -1; found++;
            if (found >= level) { best = level; level++; msg = 'Correct — now ' + level; host.saveScore(best * 100); newGrid(); }
          } else { msg = 'Missed. Best ' + best; level = 3; newGrid(); }
          return;
        }
        /* aim */
        if (k === 'up'    && ay > 0)  ay--;
        if (k === 'down'  && ay < 11) ay++;
        if (k === 'left'  && ax > 0)  ax--;
        if (k === 'right' && ax < 39) ax++;
        if (k !== 'enter' && k !== 'space') return;
        var got = false;
        for (var j = 0; j < targets.length; j++)
          if (targets[j].x === ax && targets[j].y === ay) {
            hits++; targets[j] = { x: rnd(40), y: rnd(12) }; got = true; break;
          }
        if (!got) misses++;
        host.saveScore(hits * 10 - misses * 2);
      },
      draw: function (t) {
        t.header('REACTION', RNAME[V]);
        if (mode === 'cue') {
          t.text(24, 8, phase === 'go' ? '>>> PRESS <<<' : 'Wait for the cue...',
                 phase === 'go' ? C.green : C.grey, null, phase === 'go');
          t.text(24, 11, msg + '                      ', C.cyan);
        } else if (mode === 'stroop') {
          t.text(34, 9, SN[word] + '     ', SC[ink], null, true);
          t.text(24, 12, 'r g b y — answer for the INK colour', C.dim);
          t.text(24, 14, 'Score ' + score + ' of ' + asked + '   ' + msg + '     ', C.fg);
        } else if (mode === 'mem') {
          t.text(20, 7, 'Length ' + seq.length + '      ', C.fg);
          if (phase === 'show') {
            var i = Math.floor(show / 5);
            if (i < seq.length && show % 5 < 4)
              t.text(20, 9, '   ' + (V === 10 ? 'tone ' + (seq[i] + 1) : seq[i]) + '   ', C.cyan, null, true);
            else t.text(20, 9, '            ', C.grey);
          } else {
            t.text(20, 9, 'Type it back, Enter submits ', C.grey);
            t.text(20, 11, typed + '_                ', C.cyan);
          }
          t.text(20, 13, msg + '                      ', C.white);
        } else if (mode === 'grid') {
          for (var r = 0; r < 6; r++) for (var c = 0; c < 6; c++) {
            var v = lit[r][c], ch, fg;
            if (phase === 'show' && v > 0) { ch = (V === 6) ? String(v) : '#'; fg = C.yellow; }
            else if (v < 0) { ch = '#'; fg = C.green; }
            else { ch = '.'; fg = C.grey; }
            t.put(30 + c * 3, 6 + r, ch, fg,
                  (phase === 'input' && r === cr && c === cc) ? '#2b4a6b' : null, v !== 0);
          }
          t.text(24, 14, phase === 'show' ? 'Memorise ' + level + ' cells    '
                                          : 'Now pick them' + (V === 6 ? ' in order' : '') + '   ', C.fg);
          t.text(24, 16, msg + '                      ', C.white);
        } else {
          t.box(18, 5, 42, 14, C.blue);
          targets.forEach(function (o) { t.put(19 + o.x, 6 + o.y, 'O', C.red, null, true); });
          t.put(19 + ax, 6 + ay, '+', C.cyan, null, true);
          t.text(18, 21, 'Hits ' + hits + '  Misses ' + misses + '    ', C.fg);
        }
      }
    };
  }
});

/* --------------------------------------------------------------- wordmisc */
reg('wordmisc', {
  title: 'Word Games', help: 'Type an answer, Enter submits · Q quits',
  start: function (host, p) {
    var WNAME = ['Cryptogram','Word Ladder','Boggle','Ghost','Superghost','Word Chain',
                 'Spelling Bee','Jotto','Text Twist','Countdown Letters',
                 'Missing Vowels','Palindrome Hunt'];
    var V = (p.variant >= 0 && p.variant < 12) ? p.variant : 0;
    var W = window.GICWORDS || { themes: {}, byLength: {}, themeOrder: [] };
    var allThemes = W.themeOrder || Object.keys(W.themes);
    var PAL = ['level','rotor','civic','radar','kayak','refer','madam','stats'];
    var NOT = ['table','chair','house','water','plant','stone','cloud','river'];
    var score = 0, rounds = 0, prompt = '', target = '', typed = '', verdict = null;

    function anyWord() {
      var th = allThemes[rnd(allThemes.length)];
      var pool = W.themes[th] || ['alpha'];
      return pool[rnd(pool.length)].toLowerCase();
    }
    function byLen(n) {
      var pool = (W.byLength && W.byLength[String(n)]) || null;
      return pool ? pool[rnd(pool.length)].toLowerCase() : anyWord();
    }
    function make() {
      typed = ''; verdict = null;
      var w, i;
      if (V === 0) {
        w = anyWord();
        var shift = 1 + rnd(25), enc = '';
        for (i = 0; i < w.length; i++)
          enc += String.fromCharCode(97 + ((w.charCodeAt(i) - 97 + shift) % 26));
        prompt = 'Shift cipher: ' + enc;
        target = w;
      } else if (V === 1) {
        w = byLen(4);
        var pos = rnd(4), ch = String.fromCharCode(97 + rnd(26));
        prompt = 'Change one letter of ' + w.slice(0, pos) + ch + w.slice(pos + 1) + ' back to a real word';
        target = w;
      } else if (V === 2 || V === 8 || V === 9) {
        w = byLen(V === 9 ? 8 : 6);
        var pool = w.split('');
        for (i = pool.length - 1; i > 0; i--) { var j = rnd(i + 1), t2 = pool[i]; pool[i] = pool[j]; pool[j] = t2; }
        prompt = 'Make a word from these letters: ' + pool.join('');
        target = w;
      } else if (V === 3 || V === 4 || V === 5) {
        w = anyWord();
        var frag = w.slice(0, 2 + rnd(2));
        prompt = (V === 5 ? "Give a word starting with '" : "Extend '") + frag +
                 (V === 5 ? "'" : "' toward a real word");
        target = w;
      } else if (V === 6) {
        w = byLen(5);
        prompt = "Use every letter of '" + w + "' exactly once";
        target = w;
      } else if (V === 7) {
        w = byLen(5);
        prompt = 'Guess the five-letter word. First letter: ' + w[0];
        target = w;
      } else if (V === 10) {
        w = anyWord();
        prompt = 'Restore the vowels: ' + w.replace(/[aeiou]/g, '');
        target = w;
      } else {
        var pal = rnd(2);
        prompt = "Is '" + (pal ? PAL[rnd(8)] : NOT[rnd(8)]) + "' a palindrome? type yes or no";
        target = pal ? 'yes' : 'no';
      }
    }
    make();
    return {
      key: function (k) {
        if (verdict) { rounds++; if (verdict === 'ok') score++; host.saveScore(score); make(); return; }
        if (k === 'backspace') { typed = typed.slice(0, -1); return; }
        if (k === 'enter') { verdict = (typed === target) ? 'ok' : 'no'; return; }
        if (k.length === 1 && k >= ' ' && k <= '~') typed += k.toLowerCase();
      },
      draw: function (t) {
        t.header('WORD GAMES', WNAME[V]);
        t.text(10, 7, prompt + '                                    ', C.white, null, true);
        t.text(10, 10, 'Answer: ' + typed + '_           ', C.cyan);
        if (verdict === 'ok') t.text(10, 12, 'Correct!                          ', C.green, null, true);
        if (verdict === 'no') t.text(10, 12, "No - looking for '" + target + "'.        ", C.red, null, true);
        t.text(10, 14, 'Score ' + score + ' of ' + rounds + '     ', C.fg);
      }
    };
  }
});

/* ------------------------------------------------------------------- idle */
reg('idle', {
  title: 'Idle', help: 'Space gathers · 1-3 buy upgrades · Q quits',
  start: function (host, p) {
    var TNAME = ['Mine','Bakery','Farm','Factory','Laboratory','Galaxy',
                 'Dungeon','Garden','Kingdom','Reactor'];
    var RES = ['ore','loaves','crops','widgets','data','stars','gold','blooms','taxes','joules'];
    var UP = [['pickaxe','cart','drill'],['oven','mixer','shopfront'],
              ['hoe','tractor','irrigation'],['press','belt','robot'],
              ['assistant','microscope','grant'],['probe','warpdrive','colony'],
              ['torch','sword','map'],['spade','greenhouse','bees'],
              ['village','market','castle'],['rod','coolant','turbine']];
    var BASE = [10,12,8,15,20,25,14,9,18,30];
    var ti = (p.variant | 0) % 10;
    var res = 0, lvl = [0,0,0], secs = 0;

    function rate() { return lvl[0] * 1 + lvl[1] * 4 + lvl[2] * 9; }
    return {
      tick: function () { res += rate() / 20; secs += 0.05; },
      key: function (k) {
        if (k === 'space' || k === 'enter') { res += 1; return; }
        if (k >= '1' && k <= '3') {
          var i = (+k) - 1, cost = BASE[ti] * (1 << lvl[i]);
          if (res >= cost) { res -= cost; lvl[i]++; host.saveScore(Math.round(res)); }
        }
      },
      draw: function (t) {
        t.header('IDLE', TNAME[ti]);
        t.text(18, 5, res.toFixed(1) + ' ' + RES[ti] + '            ', C.yellow, null, true);
        t.text(18, 6, rate().toFixed(1) + ' per second      ', C.dim);
        for (var i = 0; i < 3; i++) {
          var cost = BASE[ti] * (1 << lvl[i]);
          t.text(18, 9 + i * 2, (i + 1) + ') ' + (UP[ti][i] + '            ').slice(0, 13) +
                 ' level ' + lvl[i] + '   cost ' + cost + '      ',
                 res >= cost ? C.green : C.grey);
        }
        t.text(18, 16, 'Space gathers by hand, 1-3 buy upgrades', C.dim);
        t.text(18, 18, 'Running ' + Math.floor(secs) + ' s   ', C.fg);
      }
    };
  }
});


})();
