/* dice.js - the dice and betting families, mirroring src/games/dice.c and
 * src/games/betting.c. Same six dice mechanisms, same wheels.
 */
(function () {
'use strict';
var G = window.GIC, C = G.COL, reg = G.register, rnd = G.rnd, shuffle = G.shuffle;

var FACE = ['   ', ' . ', ': .', ' : ', '::.', ':.:', ':::'];
function pips(t, x, y, d, held, i) {
  var fg = held ? C.green : C.white;
  t.text(x, y,     '+---+', C.grey);
  t.text(x, y + 1, '|' + FACE[d < 1 ? 0 : d] + '|', fg);
  t.text(x, y + 2, '+---+', C.grey);
  t.text(x, y + 3, ' ' + d + ' ', held ? C.green : C.grey);
}
function showDice(t, x, y, dice, keep) {
  for (var i = 0; i < dice.length; i++) pips(t, x + i * 6, y, dice[i], keep && keep[i], i);
}

/* ------------------------------------------------------------ score sheets */
var CAT_NUM = 0, CAT_NKIND = 1, CAT_FULL = 2, CAT_SMALL = 3, CAT_LARGE = 4,
    CAT_CHANCE = 5, CAT_YACHT = 6, CAT_TWOPAIR = 9;

var YACHT = [['Ones',CAT_NUM,1,0],['Twos',CAT_NUM,2,0],['Threes',CAT_NUM,3,0],
  ['Fours',CAT_NUM,4,0],['Fives',CAT_NUM,5,0],['Sixes',CAT_NUM,6,0],
  ['Full House',CAT_FULL,0,0],['Four of a Kind',CAT_NKIND,4,0],
  ['Little Straight',CAT_SMALL,0,30],['Big Straight',CAT_LARGE,0,30],
  ['Choice',CAT_CHANCE,0,0],['Yacht',CAT_YACHT,0,50]];
var BALUT = [['Fours',CAT_NUM,4,0],['Fives',CAT_NUM,5,0],['Sixes',CAT_NUM,6,0],
  ['Straight',CAT_LARGE,0,20],['Full House',CAT_FULL,0,0],
  ['Choice',CAT_CHANCE,0,0],['Balut',CAT_YACHT,0,20]];
var KISMET = [['Ones',CAT_NUM,1,0],['Twos',CAT_NUM,2,0],['Threes',CAT_NUM,3,0],
  ['Fours',CAT_NUM,4,0],['Fives',CAT_NUM,5,0],['Sixes',CAT_NUM,6,0],
  ['Two Pairs Same Colour',CAT_TWOPAIR,0,0],['Three of a Kind',CAT_NKIND,3,0],
  ['Straight',CAT_LARGE,0,30],['Flush',CAT_FULL,0,0],['Full House',CAT_FULL,0,0],
  ['Four of a Kind',CAT_NKIND,4,0],['Yarborough',CAT_CHANCE,0,0]];

function catScore(cat, dice) {
  var cnt = [0,0,0,0,0,0,0], i, f, pairs = 0, three = 0, sum = 0;
  for (i = 0; i < dice.length; i++) { cnt[dice[i]]++; sum += dice[i]; }
  switch (cat[1]) {
    case CAT_NUM:    return cnt[cat[2]] * cat[2];
    case CAT_CHANCE: return sum;
    case CAT_NKIND:  for (f = 1; f <= 6; f++) if (cnt[f] >= cat[2]) return sum; return 0;
    case CAT_FULL:
      for (f = 1; f <= 6; f++) { if (cnt[f] === 2) pairs++; if (cnt[f] === 3) three++; }
      return (pairs && three) ? sum + 2 : 0;
    case CAT_TWOPAIR:
      for (f = 1; f <= 6; f++) if (cnt[f] >= 2) pairs++;
      return pairs >= 2 ? sum : 0;
    case CAT_SMALL: return (cnt[1] && cnt[2] && cnt[3] && cnt[4] && cnt[5]) ? cat[3] : 0;
    case CAT_LARGE: return (cnt[2] && cnt[3] && cnt[4] && cnt[5] && cnt[6]) ? cat[3] : 0;
    case CAT_YACHT: for (f = 1; f <= 6; f++) if (cnt[f] === dice.length) return cat[3]; return 0;
  }
  return 0;
}

function sheet(host, cats, title) {
  var used = [], got = [], dice = [], keep = [], cur = 0, rolls = 1, turn = 0, total = 0;
  function newTurn() {
    dice = []; keep = [];
    for (var i = 0; i < 5; i++) { dice.push(1 + rnd(6)); keep.push(false); }
    rolls = 1; cur = 0;
  }
  function reset() {
    used = cats.map(function () { return false; });
    got = cats.map(function () { return 0; });
    turn = 0; total = 0; newTurn();
  }
  reset();
  return {
    key: function (k) {
      if (turn >= cats.length) { host.saveScore(total); reset(); return; }
      if (k === 'up')   { cur = (cur + cats.length - 1) % cats.length; return; }
      if (k === 'down') { cur = (cur + 1) % cats.length; return; }
      if (/^[1-5]$/.test(k)) { keep[+k - 1] = !keep[+k - 1]; return; }
      if (k === 'r') {
        if (rolls >= 3) return;
        for (var i = 0; i < 5; i++) if (!keep[i]) dice[i] = 1 + rnd(6);
        rolls++;
        return;
      }
      if (k === 'enter') {
        if (used[cur]) return;
        got[cur] = catScore(cats[cur], dice);
        used[cur] = true;
        total += got[cur];
        turn++;
        if (turn < cats.length) newTurn();
      }
    },
    draw: function (t) {
      t.header(title, 'Roll ' + rolls + ' of 3 — arrows pick a line, 1-5 hold dice, R rerolls, Enter scores');
      showDice(t, 8, 3, dice, keep);
      for (var i = 0; i < cats.length; i++) {
        var s = used[i] ? got[i] : catScore(cats[i], dice);
        t.text(6, 8 + i, (cats[i][0] + '                        ').slice(0, 24),
               used[i] ? C.grey : C.white, i === cur ? '#2b4a6b' : null);
        t.text(32, 8 + i, ('    ' + s).slice(-4), used[i] ? C.grey : C.yellow);
        if (used[i]) t.text(38, 8 + i, 'taken', C.grey);
      }
      t.text(6, 9 + cats.length, 'Total ' + total + '   turn ' + Math.min(turn + 1, cats.length) +
             '/' + cats.length + (turn >= cats.length ? '   Sheet complete — press any key.' : ''),
             turn >= cats.length ? C.green : C.white, null, true);
    }
  };
}

/* --------------------------------------------------------- push your luck */
function farkleScore(d) {
  var cnt = [0,0,0,0,0,0,0], i, f, s = 0, used = 0;
  for (i = 0; i < d.length; i++) cnt[d[i]]++;
  for (f = 1; f <= 6; f++) if (cnt[f] >= 3) {
    var base = (f === 1) ? 1000 : f * 100;
    s += base + (cnt[f] - 3) * base;
    used += cnt[f];
    cnt[f] = 0;
  }
  s += cnt[1] * 100 + cnt[5] * 50;
  used += cnt[1] + cnt[5];
  return { score: s, used: used };
}

function press(host, v, name) {
  var target = v === 0 ? 10000 : v === 9 ? 5000 : 100;
  var you = 0, cpu = 0, turnpts = 0, dice = [], nd, bust = false, gain = 0, msg = '';
  function startTurn() {
    nd = (v === 13 || v === 14) ? 5 : (v === 16 ? 1 : 6);
    turnpts = 0;
    roll();
  }
  function roll() {
    var i;
    dice = [];
    for (i = 0; i < nd; i++) dice.push(1 + rnd(6));
    bust = false; gain = 0;
    if (v === 16) { if (dice[0] === 1) bust = true; else gain = dice[0]; }
    else if (v === 13) {
      var surv = [], bad = false;
      for (i = 0; i < dice.length; i++) if (dice[i] === 2 || dice[i] === 5) bad = true;
      if (!bad) for (i = 0; i < dice.length; i++) gain += dice[i];
      for (i = 0; i < dice.length; i++) if (dice[i] !== 2 && dice[i] !== 5) surv.push(dice[i]);
      dice = surv; nd = surv.length;
      if (!nd) bust = true;
    } else if (v === 14) {
      var lowest = -1, li = 0;
      for (i = 0; i < dice.length; i++) { var val = dice[i] === 3 ? 0 : dice[i];
        if (val > lowest) { lowest = val; li = i; } }
      gain = lowest;
      dice.splice(li, 1); nd = dice.length;
    } else {
      var f = farkleScore(dice);
      gain = f.score;
      if (!gain) bust = true; else { nd -= f.used; if (!nd) nd = 6; }
    }
    if (bust) { turnpts = 0; msg = v === 16 ? 'A one — the turn is lost.' :
                v === 13 ? 'Every die dropped dead.' : 'Nothing scores — the turn is lost.'; }
    else { turnpts += gain; msg = ''; }
  }
  startTurn();
  return {
    key: function (k) {
      if (you >= target || cpu >= target) { host.saveScore(you); you = 0; cpu = 0; startTurn(); return; }
      if (bust) { cpu += 30 + rnd(200); startTurn(); return; }
      if (k === 'space') { roll(); return; }
      if (k === 'enter') {
        you += turnpts;
        cpu += 40 + rnd(260);
        startTurn();
      }
    },
    draw: function (t) {
      t.header(name, 'Space rolls again, Enter banks ' + turnpts);
      showDice(t, 8, 3, dice.length ? dice : [0], null);
      t.text(6, 8,  'this roll ' + gain + '   turn total ' + turnpts, C.grey);
      t.text(6, 10, 'you ' + you + '   opponent ' + cpu + '   target ' + target, C.white);
      t.text(6, 12, (you >= target) ? 'You reached the target first — press any key.'
                  : (cpu >= target) ? 'The opponent got there first — press any key.'
                  : msg, bust ? C.red : C.green);
    }
  };
}

/* ----------------------------------------------------------------- wagers */
function wager(host, v, name) {
  var purse = 100, bet = 10, choice = 0, best = 100, dice = [], msg = '', nopt;
  var CRAPS = ['Pass line', "Don't pass", 'Field (2-4, 9-12)'];
  var SICBO = ['Small (4-10)', 'Big (11-17)', 'Any triple', 'Total is 9'];
  nopt = v === 1 ? 3 : v === 18 ? 4 : 6;
  function play() {
    var n = (v === 1) ? 2 : 3, i, total = 0, win = false, payout = 0;
    dice = [];
    for (i = 0; i < n; i++) dice.push(1 + rnd(6));
    total = dice.reduce(function (a, b) { return a + b; }, 0);
    if (v === 1) {
      var point = 0;
      if (choice === 0) {
        if (total === 7 || total === 11) { win = true; payout = bet; }
        else if (total === 2 || total === 3 || total === 12) win = false;
        else point = total;
      } else if (choice === 1) {
        if (total === 2 || total === 3) { win = true; payout = bet; }
        else if (total === 7 || total === 11 || total === 12) win = false;
        else point = total;
      } else if (total <= 4 || total >= 9) { win = true; payout = bet; }
      while (point) {
        dice = [1 + rnd(6), 1 + rnd(6)];
        total = dice[0] + dice[1];
        if (total === point) { win = (choice === 0); payout = bet; point = 0; }
        else if (total === 7) { win = (choice === 1); payout = bet; point = 0; }
      }
      msg = 'the dice came to ' + total;
    } else if (v === 6) {
      var hits = dice.filter(function (d) { return d === choice + 1; }).length;
      win = hits > 0; payout = bet * hits;
      msg = choice + 1 + ' showed on ' + hits + ' dice';
    } else if (v === 18) {
      var trip = dice[0] === dice[1] && dice[1] === dice[2];
      if (choice === 0) { win = !trip && total >= 4 && total <= 10; payout = bet; }
      if (choice === 1) { win = !trip && total >= 11 && total <= 17; payout = bet; }
      if (choice === 2) { win = trip; payout = bet * 30; }
      if (choice === 3) { win = total === 9; payout = bet * 6; }
      msg = 'total ' + total + (trip ? ', a triple' : '');
    } else {
      var a = dice[0], b = dice[1], c = dice[2];
      var lo = Math.min(a, b, c), hi = Math.max(a, b, c);
      if (lo === 4 && hi === 6 && a + b + c === 15) { win = true; payout = bet * 2; msg = '4-5-6 — an outright win'; }
      else if (lo === 1 && hi === 3 && a + b + c === 6) { win = false; msg = '1-2-3 — an outright loss'; }
      else if (a === b || b === c || a === c) {
        var ptv = (a === b) ? c : (b === c) ? a : b;
        win = ptv >= 4; payout = bet; msg = 'your point is ' + ptv;
      } else { win = false; msg = 'no score'; }
    }
    purse += win ? payout : -bet;
    if (purse > best) best = purse;
    if (purse <= 0) { purse = 100; bet = 10; msg += ' — staked up again'; }
    if (bet > purse) bet = purse;
    host.saveScore(best);
  }
  return {
    key: function (k) {
      if (k === 'up' || k === 'left')    { choice = (choice + nopt - 1) % nopt; return; }
      if (k === 'down' || k === 'right') { choice = (choice + 1) % nopt; return; }
      if (k === '+' || k === '=') { if (bet + 5 <= purse) bet += 5; return; }
      if (k === '-')             { if (bet > 5) bet -= 5; return; }
      if (k === 'enter' || k === 'space') play();
    },
    draw: function (t) {
      var opts = v === 1 ? CRAPS : v === 18 ? SICBO : null, i;
      t.header(name, 'Purse ' + purse + ' — arrows choose, +/- stakes ' + bet + ', Enter rolls');
      if (v === 6) {
        t.text(6, 3, 'Pick a number; you are paid once for each die that shows it.', C.grey);
        for (i = 0; i < 6; i++)
          t.text(8 + i * 6, 5, '  ' + (i + 1) + '  ', C.white, i === choice ? '#2b4a6b' : null);
      } else if (v === 5) {
        t.text(6, 3, 'Three dice: 4-5-6 wins, 1-2-3 loses, a pair sets your point.', C.grey);
        t.text(6, 5, 'Press Enter to throw.', C.grey);
      } else {
        for (i = 0; i < opts.length; i++)
          t.text(8, 4 + i, (opts[i] + '                      ').slice(0, 22), C.white,
                 i === choice ? '#2b4a6b' : null);
      }
      if (dice.length) showDice(t, 8, 10, dice, null);
      t.text(6, 15, msg, C.yellow);
      t.text(6, 16, 'best purse ' + best, C.grey);
    }
  };
}

/* ----------------------------------------------------------------- bluffs */
function bluff(host, dudo) {
  var mine = [], theirs = [], bidn = 0, bidf = 1, myturn = true, wins = 0, msg = '', reveal = false;
  function round() {
    var i;
    mine = []; theirs = [];
    for (i = 0; i < 5; i++) mine.push(1 + rnd(6));
    for (i = 0; i < 5; i++) theirs.push(1 + rnd(6));
    bidn = 0; bidf = 1; myturn = true; reveal = false; msg = '';
  }
  function actual() {
    var n = 0, i;
    for (i = 0; i < mine.length; i++)   if (mine[i] === bidf || (dudo && mine[i] === 1)) n++;
    for (i = 0; i < theirs.length; i++) if (theirs[i] === bidf || (dudo && theirs[i] === 1)) n++;
    return n;
  }
  function settle(challengerIsMe) {
    var right = actual() >= bidn, iLose = challengerIsMe ? right : !right;
    reveal = true;
    if (iLose) { mine.pop(); msg = 'bid ' + bidn + ' x ' + bidf + ', actual ' + actual() + ' — you lose a die.'; }
    else { theirs.pop(); wins++; msg = 'bid ' + bidn + ' x ' + bidf + ', actual ' + actual() + ' — they lose a die.'; }
    host.saveScore(wins * 100 + mine.length * 50);
  }
  round();
  return {
    key: function (k) {
      if (reveal) {
        if (!mine.length || !theirs.length) { mine = []; theirs = []; wins = 0; }
        round();
        return;
      }
      if (k === 'up')    { bidn++; return; }
      if (k === 'down')  { if (bidn > 0) bidn--; return; }
      if (k === 'right') { bidf = bidf % 6 + 1; return; }
      if (k === 'left')  { bidf = (bidf + 4) % 6 + 1; return; }
      if (k === 'c')     { settle(true); return; }
      if (k === 'enter') {
        if (!bidn) return;
        /* It challenges when the bid outruns what it holds plus a fair share
         * of the dice it cannot see. */
        var count = 0, i;
        for (i = 0; i < theirs.length; i++) if (theirs[i] === bidf || (dudo && theirs[i] === 1)) count++;
        if (bidn > count + mine.length / 2 + 1) { settle(false); return; }
        bidn++;
        if (bidn > mine.length + theirs.length) settle(false);
      }
    },
    draw: function (t) {
      t.header(dudo ? 'DUDO' : "LIAR'S DICE",
               'Your dice ' + mine.length + ', theirs ' + theirs.length +
               ' — arrows raise the bid, Enter bids, C challenges');
      showDice(t, 8, 3, mine.length ? mine : [0], null);
      t.text(6, 8, 'current bid: ' + bidn + ' x ' + bidf, C.yellow);
      if (dudo) t.text(6, 9, 'ones are wild', C.grey);
      if (reveal) { t.text(6, 11, 'they held:', C.grey); showDice(t, 18, 10, theirs.length ? theirs : [0], null); }
      t.text(6, 16, msg, C.yellow);
    }
  };
}

/* ---------------------------------------------------------- shut the box */
function shutbox(host) {
  var shut, dice, total, cur = 0, best = 99, msg = '';
  function throwDice() {
    var high = false, i;
    for (i = 6; i < 9; i++) if (!shut[i]) high = true;
    dice = high ? [1 + rnd(6), 1 + rnd(6)] : [1 + rnd(6)];
    total = dice.reduce(function (a, b) { return a + b; }, 0);
  }
  function reset() { shut = [0,0,0,0,0,0,0,0,0]; cur = 0; msg = ''; throwDice(); }
  function openTotal() { var s = 0, i; for (i = 0; i < 9; i++) if (shut[i] !== 1) s += i + 1; return s; }
  function stuck() {
    var mask, i, t;
    for (mask = 1; mask < 512; mask++) {
      t = 0;
      var ok = true;
      for (i = 0; i < 9; i++) if (mask & (1 << i)) { if (shut[i] === 1) { ok = false; break; } t += i + 1; }
      if (ok && t === total) return false;
    }
    return true;
  }
  reset();
  return {
    key: function (k) {
      if (k === 'left')  { cur = (cur + 8) % 9; return; }
      if (k === 'right') { cur = (cur + 1) % 9; return; }
      if (k === 'space') { if (shut[cur] !== 1) shut[cur] = shut[cur] === 2 ? 0 : 2; return; }
      if (k !== 'enter') return;
      var chosen = 0, i;
      for (i = 0; i < 9; i++) if (shut[i] === 2) chosen += i + 1;
      if (chosen !== total) { msg = 'Selected ' + chosen + ', need ' + total + '.'; return; }
      for (i = 0; i < 9; i++) if (shut[i] === 2) shut[i] = 1;
      msg = '';
      if (!openTotal()) { host.saveScore(100); reset(); return; }
      throwDice();
      if (stuck()) {
        if (openTotal() < best) best = openTotal();
        host.saveScore(45 - openTotal());
        reset();
      }
    },
    draw: function (t) {
      t.header('SHUT THE BOX', 'Shut any set of flaps adding up to the throw — Space flips, Enter confirms');
      showDice(t, 12, 3, dice, null);
      t.text(6, 8, 'throw ' + total, C.white);
      for (var i = 0; i < 9; i++) {
        t.text(6 + i * 5, 10, ' ' + (i + 1) + ' ',
               shut[i] === 1 ? C.grey : shut[i] === 2 ? C.yellow : C.white,
               i === cur ? '#2b4a6b' : null);
        t.text(6 + i * 5, 11, shut[i] === 1 ? 'shut' : shut[i] === 2 ? ' ^  ' : '    ', C.grey);
      }
      t.text(6, 13, 'open total ' + openTotal() + '   best finish ' + (best === 99 ? '-' : best), C.grey);
      t.text(6, 15, msg, C.yellow);
    }
  };
}

/* ------------------------------------------------------------------ races */
function diceRace(host, v, name) {
  var you = 0, cpu = 0, round = 1, target = v === 3 ? 21 : v === 8 ? 6 : 5;
  var dice = [], msg = '', parts = [0,0,0,0,0,0], cparts = [0,0,0,0,0,0];
  var PARTS = ['body','head','leg','leg','wing','eye'];
  function turn() {
    var n = v === 7 ? 2 : v === 15 ? 2 : v === 3 ? 3 : 1, side, i;
    for (side = 0; side < 2; side++) {
      var gain = 0;
      dice = [];
      for (i = 0; i < n; i++) dice.push(1 + rnd(6));
      if (v === 3) {
        var hits = dice.filter(function (d) { return d === Math.min(round, 6); }).length;
        gain = hits === 3 ? 21 : hits;
      } else if (v === 7) {
        var a = dice[0], b = dice[1];
        if ((a === 2 && b === 1) || (a === 1 && b === 2)) gain = 21;
        else if (a === b) gain = a;
        else gain = Math.floor((Math.max(a, b) * 10 + Math.min(a, b)) / 10);
      } else if (v === 15) gain = Math.max(dice[0], dice[1]);
      else if (v === 19) gain = dice[0];
      else {
        var arr = side ? cparts : parts, f = dice[0] - 1;
        if (f === 0 || arr[0]) { if (!arr[f]) { arr[f] = 1; gain = 1; } }
      }
      if (side) cpu += gain; else you += gain;
    }
    round++;
    msg = 'round ' + round;
  }
  return {
    key: function (k) {
      if (you >= target || cpu >= target) {
        host.saveScore(you);
        you = 0; cpu = 0; round = 1;
        parts = [0,0,0,0,0,0]; cparts = [0,0,0,0,0,0];
        return;
      }
      if (k === 'space' || k === 'enter') turn();
    },
    draw: function (t) {
      t.header(name, 'Space throws');
      if (dice.length) showDice(t, 10, 3, dice, null);
      t.text(6, 9, 'you ' + you + '   opponent ' + cpu + '   target ' + target, C.white);
      if (v === 8)
        for (var i = 0; i < 6; i++)
          t.text(6, 11 + i, (PARTS[i] + '      ').slice(0, 7) + ' you ' + (parts[i] ? 'yes' : ' - ') +
                 '  them ' + (cparts[i] ? 'yes' : ' - '), C.grey);
      t.text(6, 18, (you >= target) ? 'You win the race — press any key.'
                  : (cpu >= target) ? 'The opponent wins — press any key.' : msg,
             (you >= target) ? C.green : (cpu >= target) ? C.red : C.yellow);
    }
  };
}

reg('dice', {
  title: 'Dice', help: 'Space throws · Enter banks or scores · arrows choose',
  start: function (host, p) {
    var v = p.variant | 0;
    if (v < 0 || v > 19) v = 0;
    var NAME = ['FARKLE','CRAPS',"LIAR'S DICE",'BUNCO','SHUT THE BOX','CEE-LO','CHUCK-A-LUCK',
      'MEXICO','BEETLE','ZONK','YACHT','BALUT','KISMET','DROP DEAD','THREES','GOING TO BOSTON',
      'PIG DICE','DUDO','SIC BO','KLONDIKE DICE'];
    if (v === 10) return sheet(host, YACHT, 'YACHT');
    if (v === 11) return sheet(host, BALUT, 'BALUT');
    if (v === 12) return sheet(host, KISMET, 'KISMET');
    if (v === 0 || v === 9 || v === 13 || v === 14 || v === 16) return press(host, v, NAME[v]);
    if (v === 1 || v === 5 || v === 6 || v === 18) return wager(host, v, NAME[v]);
    if (v === 2)  return bluff(host, 0);
    if (v === 17) return bluff(host, 1);
    if (v === 4)  return shutbox(host);
    return diceRace(host, v, NAME[v]);
  }
});

/* ================================================================ betting */

var WHEELS = [['European',37,0,0],['American',38,1,0],['French',37,0,1],['Mini',13,0,0]];
var BETS = ['Red','Black','Odd','Even','Low half','High half','Straight up'];
var RED = [1,3,5,7,9,12,14,16,18,19,21,23,25,27,30,32,34,36];
function isRed(n) { return n > 0 && RED.indexOf(n) >= 0; }

reg('betting', {
  title: 'Games of chance', help: 'Arrows choose · +/- stakes · Enter plays',
  start: function (host, p) {
    var v = p.variant | 0;
    if (v < 0 || v > 17) v = 0;
    var W = WHEELS[v < 4 ? v : 0];
    var purse = 100, bet = 10, choice = 0, straight = 1, cashout = 200, best = 100;
    var pick = [7,14,21,28,35,42], detail = '', slot = -1, lastPennies = 0;
    var NAME = ['ROULETTE','ROULETTE','ROULETTE','ROULETTE','KENO','BINGO','LOTTERY',
      'COIN STREAK','WHEEL OF FORTUNE','SCRATCH CARD','THREE CARD MONTE','PLINKO',
      'HORSE RACE','ODDS AND EVENS','MATCHING PENNIES','DICE DUEL','HIGH CARD DRAW','CRASH'];
    var ODDS = [2,3,4,6,10,16];
    var nopt = v <= 3 ? 7 : v === 12 ? 6 : v === 10 ? 3 : v === 8 ? 4 : 2;

    function spin() {
      var paid = 0, i, n;
      slot = -1;
      if (v <= 3) {
        var s = rnd(W[1]);
        n = (W[2] && s === 37) ? -1 : s;
        slot = n;
        if (n <= 0) {
          if (choice < 6 && W[3]) paid = Math.floor(bet / 2);
          else if (choice === 6 && straight === 0) paid = bet * (W[1] - 1);
          detail = n < 0 ? 'the ball landed on double zero' : 'the ball landed on zero';
        } else {
          var half = W[1] === 13 ? 6 : 18;
          if (choice === 0 && isRed(n))      paid = bet * 2;
          if (choice === 1 && !isRed(n))     paid = bet * 2;
          if (choice === 2 && n % 2)         paid = bet * 2;
          if (choice === 3 && n % 2 === 0)   paid = bet * 2;
          if (choice === 4 && n <= half)     paid = bet * 2;
          if (choice === 5 && n > half)      paid = bet * 2;
          if (choice === 6 && n === straight) paid = bet * W[1];
          detail = 'the ball landed on ' + n + ' (' + (isRed(n) ? 'red' : 'black') + ')';
        }
      } else if (v === 4) {
        var drawn = [], hits = 0, PAYS = [0,0,0,1,3,12,36,100,300,1000,5000];
        for (i = 1; i <= 80; i++) drawn.push(i);
        shuffle(drawn);
        for (i = 0; i < 6; i++) if (drawn.slice(0, 20).indexOf(pick[i]) >= 0) hits++;
        paid = bet * PAYS[Math.min(hits, 10)];
        detail = hits + ' of your 6 numbers came up';
      } else if (v === 5) {
        var lines = rnd(3);
        paid = lines ? bet * (lines + 1) : 0;
        detail = lines + ' line' + (lines === 1 ? '' : 's') + ' after thirty calls';
      } else if (v === 6) {
        var PAYS2 = [0,0,0,3,25,500,50000], h = 0;
        for (i = 0; i < 6; i++) if (rnd(49) < 6) h++;
        paid = bet * PAYS2[h];
        detail = h + ' matching number' + (h === 1 ? '' : 's');
      } else if (v === 9) {
        var sym = [], b2 = 0;
        for (i = 0; i < 9; i++) sym.push(rnd(6));
        for (i = 0; i < 6; i++) { var c2 = sym.filter(function (x) { return x === i; }).length; if (c2 > b2) b2 = c2; }
        paid = b2 >= 3 ? bet * (b2 - 1) : 0;
        detail = 'best match: ' + b2 + ' of a kind';
      } else if (v === 10) {
        var cardAt = rnd(3);
        if (rnd(100) < 25) cardAt = (cardAt + 1) % 3;
        paid = choice === cardAt ? bet * 2 : 0;
        detail = 'the queen was under cup ' + (cardAt + 1);
      } else if (v === 11) {
        var SLOT = [9,4,2,1,0,1,2,4,9], pos = 4;
        for (i = 0; i < 8; i++) { pos += rnd(2) ? 1 : -1; pos = Math.max(0, Math.min(8, pos)); }
        paid = bet * SLOT[pos];
        detail = 'landed in slot ' + (pos + 1) + ', paying ' + SLOT[pos] + 'x';
      } else if (v === 12) {
        var weight = ODDS.map(function (o) { return Math.floor(100 / o); });
        var tot = weight.reduce(function (a, b) { return a + b; }, 0), r = rnd(tot), winner = 0;
        for (i = 0; i < 6; i++) { if (r < weight[i]) { winner = i; break; } r -= weight[i]; }
        paid = winner === choice ? bet * ODDS[choice] : 0;
        detail = 'horse ' + (winner + 1) + ' came home at ' + ODDS[winner] + '-1';
      } else if (v === 17) {
        var bust = 100;
        while (rnd(100) >= 4) bust += 7 + rnd(20);
        paid = cashout <= bust ? Math.floor(bet * cashout / 100) : 0;
        detail = 'crashed at ' + (bust / 100).toFixed(2) + 'x' +
                 (paid ? ', you left at ' : ' before your ') + (cashout / 100).toFixed(2) + 'x';
      } else if (v === 8) {
        var SEG = [2,3,5,20], seg = rnd(24), hit = seg < 12 ? 0 : seg < 18 ? 1 : seg < 23 ? 2 : 3;
        paid = hit === choice ? bet * SEG[hit] : 0;
        detail = 'the wheel stopped on the ' + SEG[hit] + 'x segment';
      } else if (v === 7) {
        var run = 0;
        while (rnd(2) === choice && run < 8) run++;
        paid = run ? bet * (1 << (run - 1)) : 0;
        detail = 'a run of ' + run + ' before the coin turned';
      } else if (v === 13 || v === 15) {
        var a2 = 1 + rnd(6), b3 = 1 + rnd(6);
        if (v === 15) { paid = (choice === 0 ? a2 > b3 : a2 < b3) ? bet * 2 : 0;
                        detail = 'you rolled ' + a2 + ', the bank rolled ' + b3; }
        else { paid = (((a2 + b3) % 2) === choice) ? bet * 2 : 0;
               detail = 'the two dice totalled ' + (a2 + b3); }
      } else if (v === 14) {
        var theirs = (rnd(100) < 58) ? lastPennies : rnd(2);
        paid = theirs === choice ? bet * 2 : 0;
        detail = 'they showed ' + (theirs ? 'tails' : 'heads');
        lastPennies = choice;
      } else {
        var RANKN = ['A','2','3','4','5','6','7','8','9','10','J','Q','K'];
        var m = rnd(13), bk = rnd(13);
        paid = (choice === 0 ? m > bk : m < bk) ? bet * 2 : 0;
        detail = 'you drew ' + RANKN[m] + ', the bank drew ' + RANKN[bk];
      }
      purse += paid - bet;
      if (purse > best) best = purse;
      detail += paid ? '  — won ' + (paid - bet) : '  — lost ' + bet;
      if (purse <= 0) { purse = 100; bet = 10; detail += ', staked up again'; }
      if (bet > purse) bet = purse;
      host.saveScore(best);
    }

    return {
      key: function (k) {
        if (k === 'up') {
          if (v === 4)  { pick[choice] = pick[choice] % 80 + 1; return; }
          if (v === 17) { if (cashout < 2000) cashout += 25; return; }
          choice = (choice + nopt - 1) % nopt; return;
        }
        if (k === 'down') {
          if (v === 4)  { pick[choice] = (pick[choice] + 78) % 80 + 1; return; }
          if (v === 17) { if (cashout > 110) cashout -= 25; return; }
          choice = (choice + 1) % nopt; return;
        }
        if (k === 'left') {
          if (v <= 3 && choice === 6) { straight = straight > 1 ? straight - 1 : W[1] - 1; return; }
          choice = (choice + nopt - 1) % nopt; return;
        }
        if (k === 'right') {
          if (v <= 3 && choice === 6) { straight = straight % (W[1] - 1) + 1; return; }
          choice = (choice + 1) % nopt; return;
        }
        if (k === '+' || k === '=') { if (bet + 5 <= purse) bet += 5; return; }
        if (k === '-')             { if (bet > 5) bet -= 5; return; }
        if (k === 'enter' || k === 'space') spin();
      },
      draw: function (t) {
        var i;
        t.header(NAME[v], 'Purse ' + purse + ' — arrows choose, +/- stakes ' + bet + ', Enter plays');
        if (v <= 3) {
          t.text(6, 3, W[0] + ' wheel: ' + W[1] + ' slots' +
                 (W[2] ? ', zero and double zero' : W[3] ? ', half back on zero' : ', single zero'), C.grey);
          for (i = 0; i < 7; i++)
            t.text(8, 5 + i, (BETS[i] + '                ').slice(0, 16), C.white,
                   i === choice ? '#2b4a6b' : null);
          if (choice === 6) t.text(8, 12, 'number ' + straight + ' (left/right changes it)', C.yellow);
          if (slot >= 0) t.text(8, 14, '  ' + slot + '  ', C.white, slot === 0 ? '#2a6b3d' : isRed(slot) ? '#8b2b2b' : '#2b4a6b');
        } else if (v === 4) {
          t.text(6, 3, 'Your six numbers (up and down change the highlighted one):', C.grey);
          for (i = 0; i < 6; i++)
            t.text(8 + i * 5, 5, ' ' + ('  ' + pick[i]).slice(-2) + ' ', C.white,
                   i === choice ? '#2b4a6b' : null);
        } else if (v === 12) {
          for (i = 0; i < 6; i++)
            t.text(8, 4 + i, 'horse ' + (i + 1) + ' at ' + ('  ' + ODDS[i]).slice(-2) + '-1', C.white,
                   i === choice ? '#2b4a6b' : null);
        } else if (v === 10) {
          for (i = 0; i < 3; i++)
            t.text(10 + i * 8, 5, ' cup ' + (i + 1) + ' ', C.white, i === choice ? '#2b4a6b' : null);
        } else if (v === 17) {
          t.text(6, 4, 'Auto cash-out at ' + (cashout / 100).toFixed(2) + 'x — up and down adjust it.', C.white);
        } else if (nopt === 2 || nopt === 4) {
          var TWO = ['Heads / Odd / High', 'Tails / Even / Low', 'Double', 'Jackpot'];
          for (i = 0; i < nopt; i++)
            t.text(8, 5 + i, (TWO[i] + '                      ').slice(0, 22), C.white,
                   i === choice ? '#2b4a6b' : null);
        } else {
          t.text(8, 5, 'Press Enter to play a ticket.', C.grey);
        }
        t.text(6, 16, detail, C.yellow);
        t.text(6, 18, 'best purse ' + best, C.grey);
      }
    };
  }
});

})();
