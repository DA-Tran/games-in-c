/* cardgames.js - the trick-taking, shedding and cardmisc families, mirroring
 * src/games/tricktaking.c, shedding.c and cardmisc.c.
 */
(function () {
'use strict';
var G = window.GIC, C = G.COL, reg = G.register, rnd = G.rnd, shuffle = G.shuffle;

var RANK = ['A','2','3','4','5','6','7','8','9','10','J','Q','K'];
var SUIT = ['♠','♥','♦','♣'];
function rk(c) { return c % 13; }
function su(c) { return Math.floor(c / 13); }
function red(c) { var s = su(c); return s === 1 || s === 2; }
function nm(c) { return (RANK[rk(c)] + SUIT[su(c)] + '   ').slice(0, 4); }
function col(c) { return red(c) ? C.red : C.white; }
function deck52() { var d = [], i; for (i = 0; i < 52; i++) d.push(i); return shuffle(d); }
function card(t, x, y, c, hi, dim) {
  if (c === undefined || c < 0) { t.text(x, y, '[  ]', C.grey, hi ? '#2b4a6b' : null); return; }
  t.text(x, y, nm(c), dim ? C.grey : col(c), hi ? '#2b4a6b' : null);
}

/* =========================================================== trick taking */
var T_NONE = 0, T_FIXED = 1, T_TURNUP = 2, T_BID = 3;
var S_TRICKS = 0, S_POINTS = 1, S_AVOID = 2;
var B_NONE = 0, B_TRICKS = 1, B_EXACT = 2;

var TR = [
  ['Hearts',          4,13,52,T_NONE,  0,S_AVOID, B_NONE, 50],
  ['Spades',          4,13,52,T_FIXED, 0,S_TRICKS,B_TRICKS,200],
  ['Whist',           4,13,52,T_TURNUP,0,S_TRICKS,B_NONE,  5],
  ['Oh Hell',         4, 7,52,T_TURNUP,0,S_TRICKS,B_EXACT, 50],
  ['Euchre',          4, 5,24,T_TURNUP,0,S_TRICKS,B_NONE, 10],
  ['Briscola',        2, 3,40,T_TURNUP,0,S_POINTS,B_NONE, 61],
  ['Napoleon',        4, 5,52,T_BID,   0,S_TRICKS,B_TRICKS,20],
  ['Sevens',          4,13,52,T_NONE,  0,S_TRICKS,B_NONE,  0],
  ['Knockout Whist',  4, 7,52,T_TURNUP,0,S_TRICKS,B_NONE,  1],
  ['German Whist',    2,13,52,T_TURNUP,0,S_TRICKS,B_NONE,  7],
  ['Ninety-Nine',     3, 9,52,T_NONE,  0,S_TRICKS,B_EXACT, 0],
  ['Scopa',           2, 3,40,T_NONE,  0,S_POINTS,B_NONE, 11]
];

function cardPoints(c) {
  switch (rk(c)) { case 0: return 11; case 2: return 10; case 12: return 4;
                   case 11: return 3; case 10: return 2; default: return 0; }
}
function penalty(c) { return su(c) === 1 ? 1 : (c === 11 ? 13 : 0); }

function tricks(host, v) {
  var R = TR[v], hand = [], hn = [], trump = -1, taken = [], pts = [], total = [], bids = [];
  var cur = 0, led = -1, played = [], leader = 0, trickno = 0, hs = R[2], phase = 'play';
  var myBid = 2, msg = '';

  function buildDeck() {
    var d = [], c;
    for (c = 0; c < 52; c++) {
      var r = rk(c);
      if (R[3] === 40 && r >= 7 && r <= 9) continue;
      if (R[3] === 24 && !(r === 0 || r >= 8)) continue;
      d.push(c);
    }
    return shuffle(d);
  }
  function value(c, ledSuit) {
    if (v === 4) {                       /* Euchre bowers */
      var s = su(c), r = rk(c);
      var same = trump === 0 ? 3 : trump === 3 ? 0 : trump === 1 ? 2 : 1;
      if (r === 10 && s === trump) return 100;
      if (r === 10 && s === same)  return 99;
      if (s === trump) return 50 + r;
      if (s === ledSuit) return r;
      return -1;
    }
    if (trump >= 0 && su(c) === trump) return 50 + rk(c);
    if (su(c) === ledSuit) return rk(c) === 0 ? 20 : rk(c);
    return -1;
  }
  function hasSuit(pl, s) { return hand[pl].some(function (c) { return su(c) === s; }); }
  function legal(pl, i) {
    if (led < 0) return !(R[6] === S_AVOID && trickno === 0 && penalty(hand[pl][i]));
    return !(hasSuit(pl, led) && su(hand[pl][i]) !== led);
  }
  function deal() {
    var d = buildDeck(), pl, i;
    hs = Math.min(R[2], Math.floor(d.length / R[1]));
    hand = []; hn = []; taken = []; pts = [];
    for (pl = 0; pl < R[1]; pl++) {
      hand.push(d.slice(pl * hs, pl * hs + hs));
      taken.push(0); pts.push(0);
    }
    /* Whist deals the whole pack, so there is no spare card to turn; the
     * dealer exposes their own last card instead. Indexing past the deck here
     * silently produced NaN. */
    var upIdx = Math.min(R[1] * hs, d.length - 1);
    trump = R[4] === T_FIXED ? R[5] : R[4] === T_TURNUP ? su(d[upIdx]) : R[4] === T_BID ? rnd(4) : -1;
    leader = 0; trickno = 0; played = []; led = -1; cur = 0;
    for (pl = 0; pl < R[1]; pl++) played.push(-1);
    phase = (R[7] === B_NONE) ? 'play' : 'bid';
    if (phase === 'play') autoLead();
  }
  function finishBids() {
    bids = [myBid];
    for (var pl = 1; pl < R[1]; pl++) {
      var strong = hand[pl].filter(function (c) { return rk(c) >= 10 || su(c) === trump; }).length;
      bids.push(Math.floor(strong / 2));
    }
    phase = 'play';
    autoLead();
  }
  /* Everyone before you in the order plays first. */
  function autoLead() {
    var i, pl;
    for (i = 0; i < R[1]; i++) {
      pl = (leader + i) % R[1];
      if (pl === 0) return;
      aiPlay(pl);
    }
  }
  function aiPlay(pl) {
    var best = -1, bestv = -1, i, wantwin = R[6] !== S_AVOID;
    if (R[7] === B_EXACT && bids.length) wantwin = taken[pl] < bids[pl];
    var bestval = -1;
    for (i = 0; i < R[1]; i++) if (played[i] >= 0) bestval = Math.max(bestval, value(played[i], led));
    for (i = 0; i < hand[pl].length; i++) {
      if (!legal(pl, i)) continue;
      var val = value(hand[pl][i], led < 0 ? su(hand[pl][i]) : led);
      if (best < 0) { best = i; bestv = val; continue; }
      if (wantwin) {
        var beats = val > bestval, pb = bestv > bestval;
        if (beats && (!pb || val < bestv)) { best = i; bestv = val; }
        else if (!beats && !pb && val < bestv) { best = i; bestv = val; }
      } else if (val < bestv) { best = i; bestv = val; }
    }
    if (best < 0) best = 0;
    played[pl] = hand[pl].splice(best, 1)[0];
    if (led < 0) led = su(played[pl]);
  }
  function resolve() {
    var bestpl = leader, bestval = -1, i;
    for (i = 0; i < R[1]; i++) {
      var val = value(played[i], led);
      if (val > bestval) { bestval = val; bestpl = i; }
    }
    taken[bestpl]++;
    for (i = 0; i < R[1]; i++) {
      if (R[6] === S_POINTS) pts[bestpl] += cardPoints(played[i]);
      if (R[6] === S_AVOID)  pts[bestpl] += penalty(played[i]);
    }
    msg = 'Player ' + (bestpl + 1) + ' takes the trick.';
    leader = bestpl;
    trickno++;
    played = [];
    for (i = 0; i < R[1]; i++) played.push(-1);
    led = -1;
    if (trickno >= hs) {
      for (i = 0; i < R[1]; i++) {
        if (!total[i]) total[i] = 0;
        if (R[6] === S_AVOID || R[6] === S_POINTS) total[i] += pts[i];
        else if (R[7] === B_EXACT)  total[i] += (taken[i] === bids[i]) ? 10 + taken[i] : -2;
        else if (R[7] === B_TRICKS) total[i] += (taken[i] >= bids[i]) ? bids[i] * 10 : -bids[i] * 10;
        else total[i] += taken[i];
      }
      host.saveScore(R[6] === S_AVOID ? Math.max(0, 200 - total[0]) : total[0] * 10);
      deal();
    } else autoLead();
  }
  total = [0,0,0,0];
  deal();
  return {
    key: function (k) {
      if (phase === 'bid') {
        if (k === 'up' && myBid < hs) myBid++;
        if (k === 'down' && myBid > 0) myBid--;
        if (k === 'enter') finishBids();
        return;
      }
      if (k === 'left')  { if (hand[0].length) cur = (cur + hand[0].length - 1) % hand[0].length; return; }
      if (k === 'right') { if (hand[0].length) cur = (cur + 1) % hand[0].length; return; }
      if (k !== 'enter') return;
      if (!hand[0].length || !legal(0, cur)) { msg = 'That card is not legal.'; return; }
      played[0] = hand[0].splice(cur, 1)[0];
      if (led < 0) led = su(played[0]);
      if (cur >= hand[0].length && cur > 0) cur--;
      for (var i = 1; i < R[1]; i++) {
        var pl = (leader + i) % R[1];
        if (pl !== 0 && played[pl] < 0) aiPlay(pl);
      }
      resolve();
    },
    draw: function (t) {
      var i;
      t.header('TRICK TAKING', R[0] + (phase === 'bid'
        ? ' — up and down set your bid of ' + myBid + ', Enter confirms'
        : ' — trick ' + (trickno + 1) + ' of ' + hs + ', arrows pick, Enter plays'));
      t.text(5, 3, 'trump ' + (trump < 0 ? 'none' : SUIT[trump]) + '   led ' +
             (led < 0 ? '-' : SUIT[led]), C.grey);
      for (i = 0; i < R[1]; i++)
        if (played[i] >= 0) { t.text(5 + i * 12, 5, 'P' + (i + 1), C.grey); card(t, 9 + i * 12, 5, played[i]); }
      t.text(5, 8, 'your hand (grey cards are not legal):', C.grey);
      for (i = 0; i < hand[0].length && i < 14; i++)
        card(t, 5 + i * 5, 9, hand[0][i], i === cur, phase === 'play' && !legal(0, i));
      for (i = 0; i < R[1]; i++)
        t.text(5, 12 + i, 'P' + (i + 1) + '  tricks ' + taken[i] + '  points ' + pts[i] +
               '  total ' + (total[i] || 0) + (bids.length ? '  bid ' + bids[i] : ''), C.white);
      t.text(5, 18, msg, C.yellow);
    }
  };
}

/* Sevens and Scopa are not trick games and get their own loops. */
function sevens(host) {
  var hand = [], lo = [-1,-1,-1,-1], hi = [-1,-1,-1,-1], cur = 0, msg = '', over = false;
  function deal() {
    var d = deck52(), pl;
    hand = [];
    for (pl = 0; pl < 4; pl++) hand.push(d.slice(pl * 13, pl * 13 + 13));
    lo = [-1,-1,-1,-1]; hi = [-1,-1,-1,-1]; cur = 0; msg = ''; over = false;
  }
  function playable(c) { var s = su(c), r = rk(c); return lo[s] < 0 ? r === 6 : (r === lo[s] - 1 || r === hi[s] + 1); }
  function place(c) { var s = su(c), r = rk(c);
    if (lo[s] < 0) { lo[s] = hi[s] = 6; } else if (r === lo[s] - 1) lo[s] = r; else hi[s] = r; }
  function others() {
    for (var pl = 1; pl < 4; pl++) {
      for (var i = 0; i < hand[pl].length; i++)
        if (playable(hand[pl][i])) { place(hand[pl][i]); hand[pl].splice(i, 1); break; }
      if (!hand[pl].length) over = true;
    }
  }
  deal();
  return {
    key: function (k) {
      if (over) { host.saveScore(hand[0].length ? 100 : 500); deal(); return; }
      if (k === 'left')  { if (hand[0].length) cur = (cur + hand[0].length - 1) % hand[0].length; return; }
      if (k === 'right') { if (hand[0].length) cur = (cur + 1) % hand[0].length; return; }
      if (k === 'p') { msg = 'Passed.'; others(); return; }
      if (k !== 'enter') return;
      if (!hand[0].length || !playable(hand[0][cur])) { msg = 'That card will not go down.'; return; }
      place(hand[0][cur]);
      hand[0].splice(cur, 1);
      if (cur >= hand[0].length && cur > 0) cur--;
      if (!hand[0].length) { over = true; return; }
      msg = '';
      others();
    },
    draw: function (t) {
      var i;
      t.header('SEVENS', 'Build outwards from each seven — arrows pick, Enter plays, P passes');
      for (i = 0; i < 4; i++)
        t.text(6, 3 + i, SUIT[i] + '  ' + (lo[i] < 0 ? '(needs the seven)'
               : RANK[lo[i]] + ' .. ' + RANK[hi[i]]), lo[i] < 0 ? C.grey : C.yellow);
      for (i = 1; i < 4; i++) t.text(6, 8 + i, 'Player ' + (i + 1) + ' holds ' + hand[i].length, C.grey);
      t.text(6, 13, 'your hand:', C.grey);
      for (i = 0; i < hand[0].length && i < 14; i++)
        card(t, 6 + i * 5, 14, hand[0][i], i === cur, !playable(hand[0][i]));
      t.text(6, 16, over ? (hand[0].length ? 'Somebody else went out.' : 'You went out first.') : msg,
             over ? C.green : C.yellow);
    }
  };
}

function scopa(host) {
  var deck, pos, table, hand, mine, theirs, sweeps, cur = 0, msg = '', over = false;
  function prim(c) { return [16,12,13,14,15,18,21,10,10,10,10,10,10][rk(c)]; }
  function points(a, b, sw) {
    var i, p = sw, coins = 0, ocoins = 0, best = [0,0,0,0], obest = [0,0,0,0], m = 0, y = 0;
    for (i = 0; i < a.length; i++) {
      if (su(a[i]) === 2) coins++;
      if (a[i] === 2 * 13 + 6) p++;
      if (prim(a[i]) > best[su(a[i])]) best[su(a[i])] = prim(a[i]);
    }
    for (i = 0; i < b.length; i++) {
      if (su(b[i]) === 2) ocoins++;
      if (prim(b[i]) > obest[su(b[i])]) obest[su(b[i])] = prim(b[i]);
    }
    if (a.length > b.length) p++;
    if (coins > ocoins) p++;
    for (i = 0; i < 4; i++) { m += best[i]; y += obest[i]; }
    if (m > y) p++;
    return p;
  }
  function deal() {
    var d = [], c;
    for (c = 0; c < 52; c++) { var r = rk(c); if (!(r >= 7 && r <= 9)) d.push(c); }
    deck = shuffle(d); pos = 0;
    table = deck.slice(pos, pos + 4); pos += 4;
    mine = []; theirs = []; sweeps = [0, 0]; cur = 0; msg = ''; over = false;
    refill();
  }
  function refill() {
    hand = [deck.slice(pos, pos + 3), deck.slice(pos + 3, pos + 6)];
    pos += 6;
    if (!hand[0].length) over = true;
  }
  function capture(pl, c) {
    var i = -1, j;
    for (j = 0; j < table.length; j++) if (rk(table[j]) === rk(c)) { i = j; break; }
    if (i >= 0) {
      var pile = pl ? theirs : mine;
      pile.push(c, table[i]);
      table.splice(i, 1);
      if (!table.length) sweeps[pl]++;
    } else table.push(c);
  }
  deal();
  return {
    key: function (k) {
      if (over) { host.saveScore(points(mine, theirs, sweeps[0]) * 100 + mine.length); deal(); return; }
      if (k === 'left')  { if (hand[0].length) cur = (cur + hand[0].length - 1) % hand[0].length; return; }
      if (k === 'right') { if (hand[0].length) cur = (cur + 1) % hand[0].length; return; }
      if (k !== 'enter' || !hand[0].length) return;
      capture(0, hand[0].splice(cur, 1)[0]);
      if (cur >= hand[0].length && cur > 0) cur--;
      if (hand[1].length) capture(1, hand[1].shift());
      if (!hand[0].length && !hand[1].length) { if (pos < deck.length) refill(); else over = true; }
    },
    draw: function (t) {
      var i;
      t.header('SCOPA', 'Capture a table card of the same rank — arrows pick, Enter plays');
      t.text(6, 3, 'table:', C.grey);
      for (i = 0; i < table.length; i++) card(t, 6 + i * 5, 4, table[i]);
      t.text(6, 7, 'you have captured ' + mine.length + ', they ' + theirs.length +
             '   sweeps ' + sweeps[0] + '-' + sweeps[1], C.grey);
      t.text(6, 9, 'your hand:', C.grey);
      for (i = 0; i < hand[0].length; i++) card(t, 6 + i * 5, 10, hand[0][i], i === cur);
      if (over) {
        var a = points(mine, theirs, sweeps[0]), b = points(theirs, mine, sweeps[1]);
        t.text(6, 13, 'You ' + a + ' - ' + b + ' them (cards, coins, seven of coins, primiera, sweeps)',
               a > b ? C.green : C.red, null, true);
        t.text(6, 15, 'Press any key.', C.grey);
      } else t.text(6, 13, msg, C.yellow);
    }
  };
}

reg('tricktaking', {
  title: 'Trick taking', help: 'Arrows pick a card · Enter plays',
  start: function (host, p) {
    var v = p.variant | 0;
    if (v < 0 || v > 11) v = 0;
    if (v === 7)  return sevens(host);
    if (v === 11) return scopa(host);
    return tricks(host, v);
  }
});

/* =============================================================== shedding */
var M_RANKSUIT = 0, M_HIGHER = 1, M_ANY = 2;
var SR = [
  ['Crazy Eights',           4, 7, M_RANKSUIT,  7, 1, 0],
  ['Crazy Eights Wild Draw', 4, 7, M_RANKSUIT,  7, 2, 0],
  ['President',              4,13, M_HIGHER,   -1, 0, 0],
  ['Cheat',                  4,13, M_ANY,      -1, 0, 1],
  ['Durak',                  2, 6, M_HIGHER,   -1, 0, 0]
];

function shedMatch(host, ri) {
  var R = SR[ri], deck, pos, hand, pile, pileSuit, cur = 0, msg = '', over = false, claimed = 0;
  function deal() {
    deck = deck52(); pos = 0; hand = [];
    for (var pl = 0; pl < R[1]; pl++) { hand.push(deck.slice(pos, pos + R[2])); pos += R[2]; }
    /* Cheat and President deal the whole pack, so the pile starts empty. */
    pile = pos < 52 ? deck[pos++] : -1;
    pileSuit = pile >= 0 ? su(pile) : -1;
    cur = 0; msg = ''; over = false; claimed = 0;
  }
  function ok(c) {
    if (pile < 0) return true;
    if (R[3] === M_ANY) return true;
    if (R[4] >= 0 && rk(c) === R[4]) return true;
    if (R[3] === M_HIGHER) return rk(c) > rk(pile);
    return rk(c) === rk(pile) || su(c) === pileSuit;
  }
  function others() {
    for (var pl = 1; pl < R[1] && !over; pl++) {
      var i, done = false;
      for (i = 0; i < hand[pl].length; i++)
        if (ok(hand[pl][i])) {
          pile = hand[pl].splice(i, 1)[0];
          claimed = R[6] && rnd(100) < 30 ? rnd(13) : rk(pile);
          pileSuit = (R[4] >= 0 && rk(pile) === R[4]) ? rnd(4) : su(pile);
          done = true;
          break;
        }
      if (!done && R[5] && pos < 52) for (i = 0; i < R[5] && pos < 52; i++) hand[pl].push(deck[pos++]);
      if (!hand[pl].length) over = true;
    }
  }
  deal();
  return {
    key: function (k) {
      if (over) { host.saveScore(hand[0].length ? 100 : 500); deal(); return; }
      if (k === 'left')  { if (hand[0].length) cur = (cur + hand[0].length - 1) % hand[0].length; return; }
      if (k === 'right') { if (hand[0].length) cur = (cur + 1) % hand[0].length; return; }
      if (k === 'c' && R[6]) {
        var truthful = pile >= 0 && rk(pile) === claimed;
        if (pile >= 0) (truthful ? hand[0] : hand[1]).push(pile);
        msg = truthful ? 'They were telling the truth — you pick up.' : 'Caught! They pick up.';
        pile = -1; pileSuit = -1;
        others();
        return;
      }
      if (k === 'd') {
        for (var i = 0; i < R[5] && pos < 52; i++) hand[0].push(deck[pos++]);
        msg = R[5] ? 'Drew from the stock.' : 'Passed.';
        others();
        return;
      }
      if (k !== 'enter' || !hand[0].length) return;
      if (!R[6] && !ok(hand[0][cur])) { msg = 'That card will not go down.'; return; }
      pile = hand[0].splice(cur, 1)[0];
      claimed = rk(pile);
      pileSuit = (R[4] >= 0 && rk(pile) === R[4]) ? rnd(4) : su(pile);
      if (cur >= hand[0].length && cur > 0) cur--;
      msg = '';
      if (!hand[0].length) { over = true; return; }
      others();
    },
    draw: function (t) {
      var i;
      t.header('SHEDDING', R[0] + ' — arrows pick, Enter plays, D draws or passes' +
               (R[6] ? ', C calls a bluff' : ''));
      if (pile < 0) t.text(6, 3, 'pile: empty — anything may go down', C.grey);
      else { t.text(6, 3, 'pile:', C.grey); card(t, 12, 3, pile);
             t.text(18, 3, 'suit in play ' + SUIT[pileSuit], C.grey); }
      if (R[6]) t.text(6, 4, 'they claimed ' + RANK[claimed], C.yellow);
      for (i = 1; i < R[1]; i++) t.text(6, 6 + i, 'Player ' + (i + 1) + ' holds ' + hand[i].length, C.grey);
      t.text(6, 11, 'your hand:', C.grey);
      for (i = 0; i < hand[0].length && i < 16; i++)
        card(t, 5 + i * 5, 12, hand[0][i], i === cur, !R[6] && !ok(hand[0][i]));
      t.text(6, 15, over ? (hand[0].length ? 'Somebody else went out.' : 'You went out first.') : msg,
             over ? C.green : C.yellow);
    }
  };
}

function shedPair(host, v) {
  var name = v === 5 ? 'OLD MAID' : v === 10 ? 'GO FISH' : 'RUMMY';
  var players = v === 11 ? 2 : 3;
  var deck, pos, hand, books, cur = 0, msg = '', over = false;
  function dropPairs(pl) {
    var i, j;
    for (i = 0; i < hand[pl].length; i++)
      for (j = i + 1; j < hand[pl].length; j++)
        if (rk(hand[pl][i]) === rk(hand[pl][j])) {
          hand[pl].splice(j, 1); hand[pl].splice(i, 1);
          books[pl]++;
          i = -1;
          break;
        }
  }
  function deal() {
    var i;
    deck = deck52(); pos = 0; hand = []; books = [];
    for (i = 0; i < players; i++) { hand.push([]); books.push(0); }
    if (v === 5) {
      deck.splice(deck.indexOf(11), 1);            /* one queen removed */
      for (i = 0; i < deck.length; i++) hand[i % players].push(deck[i]);
      pos = 52;
      for (i = 0; i < players; i++) dropPairs(i);
    } else {
      var per = v === 10 ? 7 : 10;
      for (i = 0; i < players; i++) { hand[i] = deck.slice(pos, pos + per); pos += per; }
    }
    cur = 0; msg = ''; over = false;
  }
  function others() {
    for (var pl = 1; pl < players && !over; pl++) {
      if (v === 5) {
        var src = (pl + 1) % players;
        if (hand[src].length) hand[pl].push(hand[src].splice(rnd(hand[src].length), 1)[0]);
      } else if (v === 10 && hand[pl].length) {
        var want = rk(hand[pl][rnd(hand[pl].length)]), got = 0, i, j;
        for (i = 0; i < players; i++) {
          if (i === pl) continue;
          for (j = hand[i].length - 1; j >= 0; j--)
            if (rk(hand[i][j]) === want) { hand[pl].push(hand[i].splice(j, 1)[0]); got++; }
        }
        if (!got && pos < 52) hand[pl].push(deck[pos++]);
      } else if (v === 11) {
        if (pos < 52) hand[pl].push(deck[pos++]);
        for (var r = 0; r < 13; r++) {
          var c2 = hand[pl].filter(function (c) { return rk(c) === r; }).length;
          if (c2 >= 3) {
            hand[pl] = hand[pl].filter(function (c) { return rk(c) !== r; });
            books[pl]++;
            break;
          }
        }
      }
      if (v !== 11) dropPairs(pl);
      if (!hand[pl].length) over = true;
    }
  }
  deal();
  return {
    key: function (k) {
      if (over) {
        if (v === 5) {
          var loser = 0, i;
          for (i = 0; i < players; i++) if (hand[i].length > hand[loser].length) loser = i;
          host.saveScore(loser === 0 ? 100 : 500);
        } else host.saveScore(books[0] * 100);
        deal();
        return;
      }
      if (k === 'left')  { if (hand[0].length) cur = (cur + hand[0].length - 1) % hand[0].length; return; }
      if (k === 'right') { if (hand[0].length) cur = (cur + 1) % hand[0].length; return; }
      if (k === 'd' && v === 11) { if (pos < 52) hand[0].push(deck[pos++]); others(); return; }
      if (k !== 'enter') return;
      if (v === 5) {
        if (hand[1].length) { hand[0].push(hand[1].splice(rnd(hand[1].length), 1)[0]); msg = 'Took a card.'; }
      } else if (v === 10) {
        if (!hand[0].length) return;
        var want = rk(hand[0][cur]), got = 0, i, j;
        for (i = 1; i < players; i++)
          for (j = hand[i].length - 1; j >= 0; j--)
            if (rk(hand[i][j]) === want) { hand[0].push(hand[i].splice(j, 1)[0]); got++; }
        if (!got) { if (pos < 52) hand[0].push(deck[pos++]); msg = 'Go fish.'; }
        else msg = 'They handed them over.';
      } else {
        if (!hand[0].length) return;
        var r2 = rk(hand[0][cur]);
        if (hand[0].filter(function (c) { return rk(c) === r2; }).length < 3) {
          msg = 'You need three of a rank to meld.';
          return;
        }
        hand[0] = hand[0].filter(function (c) { return rk(c) !== r2; });
        books[0]++;
        msg = 'Melded.';
      }
      if (v !== 11) dropPairs(0);
      if (cur >= hand[0].length && cur > 0) cur--;
      if (!hand[0].length) { over = true; return; }
      others();
    },
    draw: function (t) {
      var i;
      t.header(name, v === 5 ? 'Enter takes a card from the next player'
             : v === 10 ? 'Arrows pick a rank to ask for, Enter asks'
                        : 'Arrows pick, Enter melds a set of three, D draws');
      for (i = 1; i < players; i++)
        t.text(6, 3 + i, 'Player ' + (i + 1) + ' holds ' + hand[i].length + '  books ' + books[i], C.grey);
      t.text(6, 8, 'your books: ' + books[0], C.white);
      t.text(6, 10, 'your hand:', C.grey);
      for (i = 0; i < hand[0].length && i < 16; i++) card(t, 5 + i * 5, 11, hand[0][i], i === cur);
      t.text(6, 14, over ? 'Hand over — press any key.' : msg, over ? C.green : C.yellow);
    }
  };
}

function shedReflex(host, v) {
  var name = v === 6 ? 'SNAP' : v === 7 ? 'SLAPJACK' : v === 8 ? 'BEGGAR MY NEIGHBOUR' : 'EGYPTIAN RATSCREW';
  var deck, pile, mine, theirs, live = false, pay = 0, shown = 0, turn = 0, score = 0, best = 0, over = false;
  function reset() {
    deck = deck52(); pile = []; mine = 26; theirs = 26;
    live = false; pay = 0; shown = Date.now(); turn = 0; score = 0; over = false;
  }
  reset();
  return {
    tick: function () {
      if (over) return;
      if (!live && Date.now() - shown > 700) {
        var who = turn % 2, c;
        if ((who === 0 && !mine) || (who === 1 && !theirs)) { over = true; return; }
        c = deck[(turn * 7) % 52];
        if (who === 0) mine--; else theirs--;
        pile.push(c);
        shown = Date.now();
        turn++;
        if (v === 7)      live = rk(c) === 10;
        else if (v === 6) live = pile.length >= 2 && rk(pile[pile.length-1]) === rk(pile[pile.length-2]);
        else if (v === 9) live = (pile.length >= 2 && rk(pile[pile.length-1]) === rk(pile[pile.length-2])) ||
                                 (pile.length >= 3 && rk(pile[pile.length-1]) === rk(pile[pile.length-3]));
        else { var r = rk(c); pay = r === 10 ? 1 : r === 11 ? 2 : r === 12 ? 3 : r === 0 ? 4 : 0; live = pay > 0; }
      }
      /* The opponent slaps too, a little slower than a person. */
      if (live && Date.now() - shown > 520) { theirs += pile.length; pile = []; live = false; pay = 0; shown = Date.now(); }
      if (mine <= 0 || theirs <= 0) over = true;
    },
    key: function (k) {
      if (over) { host.saveScore(best + mine * 10); reset(); return; }
      if (k !== 'space' && k !== 'enter') return;
      if (live) {
        mine += pile.length; pile = []; live = false; pay = 0;
        score += 50;
        if (score > best) best = score;
      } else {
        /* A false slap costs a card, which is the real rule. */
        if (mine > 0) { mine--; theirs++; }
        score = Math.max(0, score - 20);
      }
      shown = Date.now();
    },
    draw: function (t) {
      t.header(name, 'Press Space when the pile is slappable');
      if (pile.length) card(t, 30, 5, pile[pile.length - 1], true);
      t.text(8, 8,  'your stock ' + mine + '    their stock ' + theirs + '   pile ' + pile.length, C.grey);
      t.text(8, 10, 'score ' + score + '  (best ' + best + ')', C.white);
      if (v === 8 && pay) t.text(8, 12, 'a forfeit of ' + pay + ' card' + (pay === 1 ? '' : 's') + ' is owed', C.yellow);
      else if (live) t.text(8, 12, 'SLAP!', C.green, null, true);
      if (over) t.text(8, 14, mine > theirs ? 'You took most of the pack.' : 'They took most of the pack.',
                       mine > theirs ? C.green : C.red);
    }
  };
}

reg('shedding', {
  title: 'Shedding', help: 'Arrows pick · Enter plays · D draws · Space slaps',
  start: function (host, p) {
    var v = p.variant | 0;
    if (v < 0 || v > 11) v = 0;
    if (v <= 4) return shedMatch(host, v);
    if (v === 5 || v === 10 || v === 11) return shedPair(host, v);
    return shedReflex(host, v);
  }
});

/* =============================================================== cardmisc */
var POKER_NAME = ['high card','a pair','two pair','three of a kind','a straight',
                  'a flush','a full house','four of a kind','a straight flush','a royal flush'];

function pokerRank(cards) {
  var cnt = [], suits = [0,0,0,0], i, pairs = 0, three = 0, four = 0;
  var n = cards.length, flush = false, straight = false, hi = -1, run = 0;
  for (i = 0; i < 13; i++) cnt.push(0);
  for (i = 0; i < n; i++) { cnt[rk(cards[i])]++; suits[su(cards[i])]++; }
  for (i = 0; i < 13; i++) {
    if (cnt[i] === 2) pairs++;
    if (cnt[i] === 3) three++;
    if (cnt[i] === 4) four++;
    if (cnt[i] && i > hi) hi = i;
  }
  for (i = 0; i < 4; i++) if (suits[i] === n) flush = true;
  for (i = 0; i < 13; i++) { if (cnt[i]) { run++; if (run >= n) straight = true; } else run = 0; }
  if (!straight && n === 5 && cnt[0] && cnt[1] && cnt[2] && cnt[3] && cnt[12]) { straight = true; hi = 3; }
  if (!straight && n === 3 && cnt[0] && cnt[1] && cnt[2]) straight = true;
  var cat = (straight && flush) ? (hi === 12 ? 9 : 8) : four ? 7 : (three && pairs) ? 6 :
            flush ? 5 : straight ? 4 : three ? 3 : pairs >= 2 ? 2 : pairs ? 1 : 0;
  return { cat: cat, kick: hi };
}

function cribbageScore(hand4, turn) {
  var c = hand4.concat([turn]), i, j, m, fifteens = 0, pairs = 0, runs = 0, flush = 0, best = 0;
  var cnt = [];
  for (i = 0; i < 13; i++) cnt.push(0);
  for (i = 0; i < 5; i++) cnt[rk(c[i])]++;
  for (m = 1; m < 32; m++) {
    var sum = 0;
    for (i = 0; i < 5; i++) if (m & (1 << i)) { var r = rk(c[i]); sum += r >= 9 ? 10 : r + 1; }
    if (sum === 15) fifteens++;
  }
  for (i = 0; i < 5; i++) for (j = i + 1; j < 5; j++) if (rk(c[i]) === rk(c[j])) pairs++;
  for (i = 0; i < 13; i++) {
    var len = 0, mult = 1;
    for (j = i; j < 13 && cnt[j]; j++) { len++; mult *= cnt[j]; }
    if (len >= 3 && len > best) { best = len; runs = len * mult; i = j; }
  }
  if (hand4.every(function (x) { return su(x) === su(hand4[0]); }))
    flush = su(turn) === su(hand4[0]) ? 5 : 4;
  var nob = hand4.some(function (x) { return rk(x) === 10 && su(x) === su(turn); }) ? 1 : 0;
  return { total: fifteens * 2 + pairs * 2 + runs + flush + nob,
           note: fifteens + ' fifteens, ' + pairs + ' pairs, run ' + runs + ', flush ' + flush };
}

function meldValue(h) {
  var cnt = [], i, melded = 0, dead = 0, s, r, run;
  for (i = 0; i < 13; i++) cnt.push(0);
  for (i = 0; i < h.length; i++) cnt[rk(h[i])]++;
  for (i = 0; i < 13; i++) {
    if (cnt[i] >= 3) melded += cnt[i];
    else dead += cnt[i] * (i >= 9 ? 10 : i + 1);
  }
  for (s = 0; s < 4; s++) {
    run = 0;
    for (r = 0; r < 13; r++) {
      var have = h.some(function (c) { return su(c) === s && rk(c) === r; });
      if (have) { run++; if (run === 3) melded += 3; else if (run > 3) melded += 1; }
      else run = 0;
    }
  }
  return { melded: melded, deadwood: dead };
}

reg('cardmisc', {
  title: 'Card games', help: 'Arrows choose · Enter plays · +/- stakes',
  start: function (host, p) {
    var v = p.variant | 0;
    if (v < 0 || v > 9) v = 0;

    if (v === 0) {                                   /* Baccarat */
      var purse = 100, bet = 10, choice = 0, best = 100, ph = [], bh = [], msg = '';
      var RES = ['Player', 'Banker', 'Tie'];
      function total(h) { return h.reduce(function (a, c) { var r = rk(c); return a + (r >= 9 ? 0 : r + 1); }, 0) % 10; }
      return {
        key: function (k) {
          if (k === 'up')   { choice = (choice + 2) % 3; return; }
          if (k === 'down') { choice = (choice + 1) % 3; return; }
          if (k === '+' || k === '=') { if (bet + 5 <= purse) bet += 5; return; }
          if (k === '-')             { if (bet > 5) bet -= 5; return; }
          if (k !== 'enter') return;
          var d = deck52(), pos = 0;
          ph = [d[pos++], d[pos++]]; bh = [d[pos++], d[pos++]];
          if (total(ph) <= 5 && total(ph) < 8 && total(bh) < 8) ph.push(d[pos++]);
          if (total(bh) <= 5 && total(ph) < 8 && total(bh) < 8) bh.push(d[pos++]);
          var pv = total(ph), bv = total(bh);
          var win = pv > bv ? 0 : bv > pv ? 1 : 2;
          if (win === choice) purse += choice === 2 ? bet * 8 : choice === 1 ? Math.floor(bet * 19 / 20) : bet;
          else purse -= bet;
          if (purse > best) best = purse;
          msg = RES[win] + ' wins (' + pv + ' to ' + bv + ')';
          if (purse <= 0) { purse = 100; bet = 10; }
          if (bet > purse) bet = purse;
          host.saveScore(best);
        },
        draw: function (t) {
          t.header('BACCARAT', 'Purse ' + purse + ' — arrows choose, +/- stakes ' + bet + ', Enter deals');
          for (var i = 0; i < 3; i++)
            t.text(10, 4 + i, 'bet on ' + (RES[i] + '        ').slice(0, 8) + ' pays ' +
                   (i === 2 ? '8 to 1' : i === 1 ? '19 to 20' : 'even'), C.white,
                   i === choice ? '#2b4a6b' : null);
          for (i = 0; i < ph.length; i++) card(t, 10 + i * 5, 9, ph[i]);
          for (i = 0; i < bh.length; i++) card(t, 10 + i * 5, 11, bh[i]);
          if (ph.length) { t.text(30, 9, 'player ' + total(ph), C.white); t.text(30, 11, 'banker ' + total(bh), C.white); }
          t.text(8, 14, msg, C.yellow);
          t.text(8, 16, 'best purse ' + best, C.grey);
        }
      };
    }

    if (v === 1) {                                   /* Cribbage */
      var deck, mine, turn, hold, cur = 0, mypeg = 0, theirpeg = 0, msg = '', phase = 'pick';
      function deal() {
        deck = deck52();
        mine = deck.slice(0, 6);
        turn = deck[12];
        hold = [false,false,false,false,false,false];
        cur = 0; phase = 'pick'; msg = '';
      }
      deal();
      return {
        key: function (k) {
          if (phase === 'done') { deal(); return; }
          if (k === 'left')  { cur = (cur + 5) % 6; return; }
          if (k === 'right') { cur = (cur + 1) % 6; return; }
          if (k === 'space') {
            var n = hold.filter(Boolean).length;
            if (hold[cur] || n < 4) hold[cur] = !hold[cur];
            return;
          }
          if (k !== 'enter' || hold.filter(Boolean).length !== 4) return;
          var keep = mine.filter(function (c, i) { return hold[i]; });
          var a = cribbageScore(keep, turn), b = cribbageScore(deck.slice(6, 10), turn);
          mypeg += a.total; theirpeg += b.total;
          msg = 'Your hand scores ' + a.total + ' — ' + a.note + '.  Theirs scores ' + b.total + '.';
          phase = 'done';
          host.saveScore(mypeg);
          if (mypeg >= 121 || theirpeg >= 121) { mypeg = 0; theirpeg = 0; }
        },
        draw: function (t) {
          t.header('CRIBBAGE', 'Keep four — arrows pick, Space keeps (' +
                   hold.filter(Boolean).length + ' of 4), Enter confirms');
          for (var i = 0; i < 6; i++) {
            card(t, 10 + i * 6, 5, mine[i], i === cur);
            t.text(10 + i * 6, 6, hold[i] ? 'keep' : '    ', C.green);
          }
          t.text(8, 8, 'turn-up:', C.grey);
          card(t, 18, 8, turn);
          t.text(8, 10, 'you ' + mypeg + ' — them ' + theirpeg + ' (first to 121)', C.white);
          t.text(8, 12, msg, C.yellow);
        }
      };
    }

    if (v === 2 || v === 3 || v === 4) {             /* Gin Rummy, Canasta, Pinochle */
      var canasta = v !== 2;
      var deck2, pos2, mine2, theirs2, cur2 = 0, my = 0, their = 0, over2 = false;
      function deal2() {
        deck2 = deck52(); pos2 = 0;
        var per = canasta ? 13 : 10;
        mine2 = deck2.slice(0, per); theirs2 = deck2.slice(per, per * 2);
        pos2 = per * 2; cur2 = 0; over2 = false;
      }
      deal2();
      return {
        key: function (k) {
          if (over2) { host.saveScore(my); deal2(); return; }
          if (k === 'left')  { if (mine2.length) cur2 = (cur2 + mine2.length - 1) % mine2.length; return; }
          if (k === 'right') { if (mine2.length) cur2 = (cur2 + 1) % mine2.length; return; }
          if (k === 'd') {
            if (pos2 < 52 && mine2.length < 20) mine2.push(deck2[pos2++]);
            if (pos2 < 52 && theirs2.length < 20) theirs2.push(deck2[pos2++]);
            if (pos2 >= 52) over2 = true;
            return;
          }
          if (k === 'space') {
            if (mine2.length > 1) { mine2.splice(cur2, 1); if (cur2 >= mine2.length && cur2 > 0) cur2--; }
            return;
          }
          if (k === 'k') {
            var a = meldValue(mine2), b = meldValue(theirs2);
            if (a.deadwood > (canasta ? 20 : 10)) return;
            my += Math.max(0, b.deadwood - a.deadwood);
            their += Math.max(0, a.deadwood - b.deadwood);
            over2 = true;
          }
        },
        draw: function (t) {
          var info = meldValue(mine2), i;
          t.header(canasta ? 'CANASTA' : 'GIN RUMMY',
                   'Arrows pick, D draws, Space discards, K knocks (deadwood ' + info.deadwood + ')');
          for (i = 0; i < mine2.length && i < 14; i++) card(t, 5 + i * 5, 5, mine2[i], i === cur2);
          t.text(5, 7, 'melded ' + info.melded + ' cards, deadwood ' + info.deadwood, C.grey);
          t.text(5, 9, 'opponent holds ' + theirs2.length + '   stock ' + (52 - pos2), C.grey);
          t.text(5, 11, 'you ' + my + ' — them ' + their, C.white);
          if (over2) t.text(5, 13, my > their ? 'You are ahead — press any key.' : 'They are ahead — press any key.',
                            my > their ? C.green : C.red);
        }
      };
    }

    if (v === 5 || v === 6) {                        /* Casino, Golf card game */
      var golf = v === 6;
      var deck3, pos3, table3, hand3, cur3 = 0, captured = 0, shownFaces = [], best3 = 999, over3 = false;
      function deal3() {
        deck3 = deck52(); pos3 = 0; captured = 0; cur3 = 0; over3 = false;
        if (golf) { hand3 = deck3.slice(0, 6); pos3 = 6; shownFaces = [true,true,false,false,false,false]; }
        else { table3 = deck3.slice(0, 4); hand3 = deck3.slice(4, 8); pos3 = 8; }
      }
      deal3();
      return {
        key: function (k) {
          if (over3) {
            if (golf) {
              var total3 = hand3.reduce(function (a, c) { var r = rk(c); return a + (r === 12 ? 0 : r >= 9 ? 10 : r + 1); }, 0);
              if (total3 < best3) best3 = total3;
              host.saveScore(200 - total3);
            } else host.saveScore(captured * 10);
            deal3();
            return;
          }
          var n = golf ? 6 : hand3.length;
          if (k === 'left')  { if (n) cur3 = (cur3 + n - 1) % n; return; }
          if (k === 'right') { if (n) cur3 = (cur3 + 1) % n; return; }
          if (k === 'p') { if (pos3 < 52) pos3++; if (pos3 >= 52) over3 = true; return; }
          if (k !== 'enter') return;
          if (golf) {
            if (pos3 >= 52) { over3 = true; return; }
            hand3[cur3] = deck3[pos3++];
            shownFaces[cur3] = true;
            if (shownFaces.every(Boolean) || pos3 >= 52) over3 = true;
          } else {
            if (!hand3.length) { over3 = true; return; }
            var hit = -1, i;
            for (i = 0; i < table3.length; i++) if (rk(table3[i]) === rk(hand3[cur3])) { hit = i; break; }
            if (hit >= 0) { captured += 2; table3.splice(hit, 1); }
            else table3.push(hand3[cur3]);
            hand3.splice(cur3, 1);
            if (cur3 >= hand3.length && cur3 > 0) cur3--;
            if (!hand3.length) {
              if (pos3 + 4 <= 52) { hand3 = deck3.slice(pos3, pos3 + 4); pos3 += 4; }
              else over3 = true;
            }
          }
        },
        draw: function (t) {
          var i;
          t.header(golf ? 'GOLF CARD GAME' : 'CASINO',
                   golf ? 'Swap drawn cards for high ones — arrows pick, Enter swaps, P passes'
                        : 'Capture table cards of the same rank — arrows pick, Enter plays');
          if (golf) {
            for (i = 0; i < 6; i++) {
              if (shownFaces[i]) card(t, 14 + (i % 3) * 6, 5 + Math.floor(i / 3) * 3, hand3[i], i === cur3);
              else t.text(14 + (i % 3) * 6, 5 + Math.floor(i / 3) * 3, '[##]', C.grey, i === cur3 ? '#2b4a6b' : null);
            }
            t.text(10, 11, 'stock ' + (52 - pos3) + ' — lowest total wins', C.grey);
            if (pos3 < 52) { t.text(10, 13, 'drawn:', C.grey); card(t, 18, 13, deck3[pos3]); }
            if (over3) {
              var tot = hand3.reduce(function (a, c) { var r = rk(c); return a + (r === 12 ? 0 : r >= 9 ? 10 : r + 1); }, 0);
              t.text(10, 15, 'Your six total ' + tot + ' (lower is better) — press any key.', C.green);
            }
          } else {
            t.text(8, 3, 'table:', C.grey);
            for (i = 0; i < table3.length; i++) card(t, 8 + i * 5, 4, table3[i]);
            t.text(8, 7, 'captured ' + captured, C.grey);
            t.text(8, 9, 'your hand:', C.grey);
            for (i = 0; i < hand3.length; i++) card(t, 8 + i * 5, 10, hand3[i], i === cur3);
            if (over3) t.text(8, 13, 'You captured ' + captured + ' cards — press any key.', C.green);
          }
        }
      };
    }

    if (v === 7) {                                   /* Blackjack Switch */
      var deck4, pos4, h = [[], []], dealer = [], purse4 = 100, bet4 = 10, best4 = 100;
      var swapped = false, done = false, msg4 = '';
      function val(hand) {
        var v2 = 0, aces = 0, i;
        for (i = 0; i < hand.length; i++) {
          var r = rk(hand[i]);
          if (r === 0) { aces++; v2 += 11; } else v2 += r >= 9 ? 10 : r + 1;
        }
        while (v2 > 21 && aces) { v2 -= 10; aces--; }
        return v2;
      }
      function deal4() {
        deck4 = deck52(); pos4 = 0;
        h = [[deck4[pos4++], deck4[pos4++]], [deck4[pos4++], deck4[pos4++]]];
        dealer = [deck4[pos4++], deck4[pos4++]];
        swapped = false; done = false; msg4 = '';
      }
      deal4();
      return {
        key: function (k) {
          if (done) { deal4(); return; }
          if (k === 's') { if (!swapped) { var tmp = h[0][1]; h[0][1] = h[1][1]; h[1][1] = tmp; swapped = true; } return; }
          if (k === '+' || k === '=') { if (bet4 + 5 <= purse4) bet4 += 5; return; }
          if (k === '-')             { if (bet4 > 5) bet4 -= 5; return; }
          if (k === '1' || k === '2') { var t2 = +k - 1; if (h[t2].length < 8 && val(h[t2]) < 21) h[t2].push(deck4[pos4++]); return; }
          if (k !== 'enter') return;
          while (val(dealer) < 17) dealer.push(deck4[pos4++]);
          var dv = val(dealer), net = 0, j;
          for (j = 0; j < 2; j++) {
            var hv = val(h[j]);
            /* In Switch a dealer 22 pushes rather than busts. */
            if (hv > 21) net -= bet4 / 2;
            else if (dv === 22) net += 0;
            else if (dv > 21 || hv > dv) net += bet4 / 2;
            else if (hv < dv) net -= bet4 / 2;
          }
          purse4 += net;
          if (purse4 > best4) best4 = purse4;
          msg4 = 'dealer ' + dv + ' — you ' + (net >= 0 ? 'win ' : 'lose ') + Math.abs(net);
          done = true;
          if (purse4 <= 0) { purse4 = 100; bet4 = 10; }
          if (bet4 > purse4) bet4 = purse4;
          host.saveScore(best4);
        },
        draw: function (t) {
          var i, j;
          t.header('BLACKJACK SWITCH', 'Purse ' + purse4 + ', stake ' + bet4 +
                   ' — S switches the second cards, 1/2 hits, Enter stands');
          t.text(8, 3, 'dealer:', C.grey);
          for (i = 0; i < dealer.length; i++)
            if (i === 1 && !done) t.text(8 + i * 5, 4, '[##]', C.grey);
            else card(t, 8 + i * 5, 4, dealer[i]);
          for (j = 0; j < 2; j++) {
            t.text(8, 7 + j * 3, 'hand ' + (j + 1) + ' (' + val(h[j]) + ')', C.white);
            for (i = 0; i < h[j].length; i++) card(t, 22 + i * 5, 7 + j * 3, h[j][i]);
          }
          t.text(8, 14, swapped ? 'switched' : 'not switched', C.grey);
          t.text(8, 16, msg4, C.yellow);
        }
      };
    }

    /* Three Card Poker and Five Card Draw */
    var five = v === 9, purse5 = 100, bet5 = 10, best5 = 100;
    var deck5, pos5, mine5, theirs5, hold5, cur5 = 0, phase5 = 'play', msg5 = '';
    function deal5() {
      var n = five ? 5 : 3, i;
      deck5 = deck52(); pos5 = 0;
      mine5 = []; theirs5 = []; hold5 = [];
      for (i = 0; i < n; i++) { mine5.push(deck5[pos5++]); theirs5.push(deck5[pos5++]); hold5.push(false); }
      cur5 = 0; phase5 = 'play'; msg5 = '';
    }
    function showdown() {
      var a = pokerRank(mine5), b = pokerRank(theirs5);
      var win = a.cat > b.cat || (a.cat === b.cat && a.kick > b.kick);
      var mult = a.cat >= 8 ? 40 : a.cat >= 6 ? 8 : a.cat >= 4 ? 4 : a.cat >= 1 ? 2 : 1;
      if (win) purse5 += bet5 * mult; else purse5 -= bet5;
      if (purse5 > best5) best5 = purse5;
      msg5 = 'you: ' + POKER_NAME[a.cat] + '   them: ' + POKER_NAME[b.cat] + ' — ' +
             (win ? 'you take it' : 'they take it');
      phase5 = 'done';
      if (purse5 <= 0) { purse5 = 100; bet5 = 10; }
      if (bet5 > purse5) bet5 = purse5;
      host.saveScore(best5);
    }
    deal5();
    return {
      key: function (k) {
        if (phase5 === 'done') { deal5(); return; }
        var n = five ? 5 : 3;
        if (k === 'left')  { cur5 = (cur5 + n - 1) % n; return; }
        if (k === 'right') { cur5 = (cur5 + 1) % n; return; }
        if (k === '+' || k === '=') { if (bet5 + 5 <= purse5) bet5 += 5; return; }
        if (k === '-')             { if (bet5 > 5) bet5 -= 5; return; }
        if (k === 'space' && five) { hold5[cur5] = !hold5[cur5]; return; }
        if (k === 'f' && !five) { purse5 -= Math.floor(bet5 / 2); phase5 = 'done'; msg5 = 'Folded.'; return; }
        if (k !== 'enter') return;
        if (five) for (var i = 0; i < 5; i++) if (!hold5[i]) mine5[i] = deck5[pos5++];
        showdown();
      },
      draw: function (t) {
        var i, a = pokerRank(mine5);
        t.header(five ? 'FIVE CARD DRAW' : 'THREE CARD POKER',
                 'Purse ' + purse5 + ', stake ' + bet5 + ' — ' +
                 (five ? 'Space holds, Enter draws' : 'Enter plays, F folds'));
        for (i = 0; i < mine5.length; i++) {
          card(t, 12 + i * 6, 5, mine5[i], i === cur5);
          if (five) t.text(12 + i * 6, 6, hold5[i] ? 'held' : '    ', C.green);
        }
        t.text(10, 8, 'you hold ' + POKER_NAME[a.cat], C.grey);
        if (phase5 === 'done')
          for (i = 0; i < theirs5.length; i++) card(t, 12 + i * 6, 10, theirs5[i]);
        t.text(10, 13, msg5, C.yellow);
        t.text(10, 15, 'best purse ' + best5, C.grey);
      }
    };
  }
});

})();
