/* solitaire.js - the tableau patience family, mirroring src/games/solitaire.c.
 *
 * The same Rules table drives the same eighteen games; the eleven that are not
 * tableau games live in solitaire2.js and are reached through the same entry
 * point, exactly as the C build reaches solitaire_other().
 */
(function () {
'use strict';
var G = window.GIC, C = G.COL, reg = G.register, rnd = G.rnd, shuffle = G.shuffle;

var B_ALT = 0, B_SUIT = 1, B_ANY = 2;
var S_NONE = 0, S_W1 = 1, S_W3 = 2, S_ROW = 3;
var M_SEQ = 0, M_ANY = 1, M_ONE = 2;
var F_ANY = 0, F_KING = 1, F_NONE = 2;
var FD_SUIT = 0, FD_SPIDER = 1;

/* name, decks, suits, tpiles, cells, build, stock, movegrp, fill, nfound, fmode, reserve */
var RULES = [
  ['Klondike Draw One',     1,4, 7,0, B_ALT,  S_W1,   M_SEQ, F_KING, 4, FD_SUIT,   0],
  ['Klondike Draw Three',   1,4, 7,0, B_ALT,  S_W3,   M_SEQ, F_KING, 4, FD_SUIT,   0],
  ['Spider One Suit',       2,1,10,0, B_SUIT, S_ROW,  M_SEQ, F_ANY,  8, FD_SPIDER, 0],
  ['Spider Two Suit',       2,2,10,0, B_SUIT, S_ROW,  M_SEQ, F_ANY,  8, FD_SPIDER, 0],
  ['Spider Four Suit',      2,4,10,0, B_SUIT, S_ROW,  M_SEQ, F_ANY,  8, FD_SPIDER, 0],
  ['FreeCell',              1,4, 8,4, B_ALT,  S_NONE, M_SEQ, F_ANY,  4, FD_SUIT,   0],
  ['Eight Off',             1,4, 8,8, B_SUIT, S_NONE, M_SEQ, F_KING, 4, FD_SUIT,   0],
  ['Seahaven Towers',       1,4,10,4, B_SUIT, S_NONE, M_SEQ, F_KING, 4, FD_SUIT,   0],
  ['Penguin',               1,4, 7,7, B_SUIT, S_NONE, M_SEQ, F_ANY,  4, FD_SUIT,   0],
  ['Yukon',                 1,4, 7,0, B_ALT,  S_NONE, M_ANY, F_KING, 4, FD_SUIT,   0],
  ['Russian Solitaire',     1,4, 7,0, B_SUIT, S_NONE, M_ANY, F_KING, 4, FD_SUIT,   0],
  ['Canfield',              1,4, 4,0, B_ALT,  S_W3,   M_SEQ, F_ANY,  4, FD_SUIT,  13],
  ['Scorpion',              1,4, 7,0, B_SUIT, S_NONE, M_ANY, F_KING, 4, FD_SPIDER, 0],
  ['Forty Thieves',         2,4,10,0, B_SUIT, S_W1,   M_ONE, F_ANY,  8, FD_SUIT,   0],
  ["Baker's Dozen",         1,4,13,0, B_ANY,  S_NONE, M_ONE, F_NONE, 4, FD_SUIT,   0],
  ['Beleaguered Castle',    1,4, 8,0, B_ANY,  S_NONE, M_ONE, F_ANY,  4, FD_SUIT,   0],
  ['La Belle Lucie',        1,4,18,0, B_SUIT, S_NONE, M_ONE, F_NONE, 4, FD_SUIT,   0],
  ['Napoleon at St Helena', 2,4,10,0, B_SUIT, S_W1,   M_ONE, F_ANY,  8, FD_SUIT,   0]
];

var VMAP = [0,1,2,3,4,5,6,7,8,-1,-1,-1,9,10,11,12,13,-1,-1,14,15,-1,16,-1,17,-1,-1,-1,-1];

var RANK = ['A','2','3','4','5','6','7','8','9','10','J','Q','K'];
var SUIT = ['♠','♥','♦','♣'];
function rankOf(c) { return c % 13; }
function suitOf(c) { return Math.floor(c / 13) % 4; }
function redOf(c)  { var s = suitOf(c); return s === 1 || s === 2; }
function nameOf(c) { return RANK[rankOf(c)] + SUIT[suitOf(c)]; }
function colourOf(c) { return redOf(c) ? C.red : C.white; }

function makeGame(host, ri, title) {
  var R = {
    name: RULES[ri][0], idx: ri, decks: RULES[ri][1], suits: RULES[ri][2],
    tpiles: RULES[ri][3], cells: RULES[ri][4], build: RULES[ri][5],
    stock: RULES[ri][6], movegrp: RULES[ri][7], fill: RULES[ri][8],
    nfound: RULES[ri][9], fmode: RULES[ri][10], reserve: RULES[ri][11]
  };
  var tab, tup, cell, found, stock, waste, reserve, redeals, spiderDone, moves;
  var cur = 0, sel = -1, nsel = 0, want = 0, msg = '';

  function canStack(lower, upper) {
    if (rankOf(upper) !== rankOf(lower) + 1) return false;
    if (R.build === B_ALT)  return redOf(upper) !== redOf(lower);
    if (R.build === B_SUIT) return suitOf(upper) === suitOf(lower);
    return true;
  }
  function mayFill(card) {
    if (R.fill === F_NONE) return false;
    if (R.fill === F_KING) return rankOf(card) === 12;
    return true;
  }
  function liftable(p) {
    var n = tab[p].length, up = tup[p], i;
    if (n === 0) return 0;
    if (R.movegrp === M_ONE) return 1;
    if (R.movegrp === M_ANY) return up;
    for (i = n - 1; i > n - up; i--) if (!canStack(tab[p][i], tab[p][i - 1])) break;
    return n - i;
  }
  function moveLimit(toEmpty) {
    var i, free = 0, empt = 0;
    for (i = 0; i < R.cells; i++) if (cell[i] < 0) free++;
    for (i = 0; i < R.tpiles; i++) if (tab[i].length === 0) empt++;
    if (toEmpty && empt > 0) empt--;
    return (free + 1) * (empt + 1);
  }
  function pileTarget(i) {
    switch (R.idx) {
      case 0: case 1: return i + 1;
      case 2: case 3: case 4: return i < 4 ? 6 : 5;
      case 5: return i < 4 ? 7 : 6;
      case 6: return 6;
      case 7: return 5;
      case 8: return 7;
      case 9: case 10: return i === 0 ? 1 : i + 5;
      case 11: return 1;
      case 12: return 7;
      case 16: return i < 17 ? 3 : 1;
      case 14: return 4;
      case 15: return 6;
      default: return 4;
    }
  }
  function hiddenTarget(i) {
    switch (R.idx) {
      case 0: case 1: return i;
      case 2: case 3: case 4: return pileTarget(i) - 1;
      case 9: case 10: return i === 0 ? 0 : i;
      case 12: return i < 3 ? 3 : 0;
      default: return 0;
    }
  }
  function deal() {
    var deck = [], d, i, j, k, pos = 0;
    for (d = 0; d < R.decks; d++)
      for (i = 0; i < 4; i++)
        for (j = 0; j < 13; j++) deck.push((i % R.suits) * 13 + j);
    shuffle(deck);
    tab = []; tup = []; cell = []; found = []; stock = []; waste = []; reserve = [];
    redeals = 0; spiderDone = 0; moves = 0; cur = 0; sel = -1; nsel = 0; want = 0;
    for (i = 0; i < R.cells; i++) cell.push(-1);
    for (i = 0; i < R.nfound; i++) found.push([]);
    for (i = 0; i < R.reserve; i++) reserve.push(deck[pos++]);
    for (i = 0; i < R.tpiles; i++) {
      var want2 = pileTarget(i), hide = hiddenTarget(i), pile = [];
      for (k = 0; k < want2 && pos < deck.length; k++) pile.push(deck[pos++]);
      tab.push(pile);
      tup.push(Math.max(pile.length ? 1 : 0, pile.length - hide));
    }
    if (R.idx === 6) for (i = 0; i < 4 && pos < deck.length; i++) cell[i] = deck[pos++];
    if (R.idx === 7) for (i = 0; i < 2 && pos < deck.length; i++) cell[i] = deck[pos++];
    if (R.idx === 8) for (i = 0; i < 3 && pos < deck.length; i++) cell[i] = deck[pos++];
    while (pos < deck.length) stock.push(deck[pos++]);

    if (R.idx === 15) {                      /* Beleaguered Castle: aces up */
      for (i = 0; i < R.tpiles; i++)
        for (j = 0; j < tab[i].length; j++)
          if (rankOf(tab[i][j]) === 0) { found[suitOf(tab[i][j])].push(tab[i].splice(j, 1)[0]); j--; }
      for (i = 0; i < R.tpiles; i++) tup[i] = tab[i].length;
    }
    if (R.idx === 14)                        /* Baker's Dozen: kings sink */
      for (i = 0; i < R.tpiles; i++)
        for (j = tab[i].length - 1; j > 0; j--)
          if (rankOf(tab[i][j]) === 12) tab[i].unshift(tab[i].splice(j, 1)[0]);
    if (R.idx === 12)                        /* Scorpion's last three */
      for (i = 0; i < 3 && stock.length; i++) { tab[i].push(stock.pop()); tup[i]++; }
    for (i = 0; i < R.tpiles; i++) tup[i] = Math.min(tup[i], tab[i].length);
  }
  function foundAccepts(f, card) {
    if (R.fmode !== FD_SUIT) return false;
    if (found[f].length === 0) return suitOf(card) === f % 4 && rankOf(card) === 0;
    return suitOf(card) === suitOf(found[f][0]) &&
           rankOf(card) === rankOf(found[f][found[f].length - 1]) + 1;
  }
  function harvest() {
    var p, i;
    if (R.fmode !== FD_SPIDER) return;
    for (p = 0; p < R.tpiles; p++) {
      var n = tab[p].length;
      if (n < 13 || tup[p] < 13) continue;
      if (rankOf(tab[p][n - 1]) !== 0 || rankOf(tab[p][n - 13]) !== 12) continue;
      for (i = n - 12; i < n; i++) if (!canStack(tab[p][i], tab[p][i - 1])) break;
      if (i !== n) continue;
      tab[p].length = n - 13;
      tup[p] = Math.max(tab[p].length ? 1 : 0, tup[p] - 13);
      spiderDone++;
    }
  }
  function won() {
    var f, total = 0;
    if (R.fmode === FD_SPIDER) return spiderDone >= R.nfound;
    for (f = 0; f < R.nfound; f++) total += found[f].length;
    return total >= 52 * R.decks;
  }
  function autoplay() {
    var done = 0, again = true, p, f, i;
    if (R.fmode === FD_SPIDER) { harvest(); return 0; }
    while (again) {
      again = false;
      for (p = 0; p < R.tpiles; p++) {
        if (!tab[p].length) continue;
        for (f = 0; f < R.nfound; f++)
          if (foundAccepts(f, tab[p][tab[p].length - 1])) {
            found[f].push(tab[p].pop());
            tup[p] = Math.max(tab[p].length ? 1 : 0, Math.min(tup[p] - 1, tab[p].length));
            again = true; done++; moves++;
            break;
          }
      }
      for (i = 0; i < R.cells; i++) {
        if (cell[i] < 0) continue;
        for (f = 0; f < R.nfound; f++)
          if (foundAccepts(f, cell[i])) { found[f].push(cell[i]); cell[i] = -1; again = true; done++; moves++; break; }
      }
      if (waste.length)
        for (f = 0; f < R.nfound; f++)
          if (foundAccepts(f, waste[waste.length - 1])) { found[f].push(waste.pop()); again = true; done++; moves++; break; }
    }
    return done;
  }
  function drawStock() {
    var i, n;
    if (R.stock === S_NONE) return 'This game has no stock.';
    if (R.stock === S_ROW) {
      for (i = 0; i < R.tpiles; i++) if (!tab[i].length) return 'Fill every empty pile first.';
      if (!stock.length) return 'The stock is empty.';
      for (i = 0; i < R.tpiles && stock.length; i++) { tab[i].push(stock.pop()); tup[i]++; }
      harvest(); moves++; return '';
    }
    if (!stock.length) {
      if (!waste.length) return 'Stock and waste are both empty.';
      if (R.stock === S_W1 && redeals >= 2) return 'No redeals left.';
      while (waste.length) stock.push(waste.pop());
      redeals++; return '';
    }
    n = (R.stock === S_W3) ? 3 : 1;
    for (i = 0; i < n && stock.length; i++) waste.push(stock.pop());
    moves++; return '';
  }

  function slotCells() { return R.cells; }
  function slotFound() { return slotCells() + R.nfound; }
  function slotStock() { return slotFound() + (R.stock === S_NONE ? 0 : 1); }
  function slotRes()   { return slotStock() + (R.reserve ? 1 : 0); }
  function nslots()    { return slotRes() + R.tpiles; }

  function peekTake(s, wantN) {
    var p, avail, i, out = [];
    if (s < slotCells()) return cell[s] < 0 ? [] : [cell[s]];
    if (s < slotFound()) { var f = s - slotCells(); return found[f].length ? [found[f][found[f].length - 1]] : []; }
    if (s < slotStock()) return waste.length ? [waste[waste.length - 1]] : [];
    if (s < slotRes())   return reserve.length ? [reserve[reserve.length - 1]] : [];
    p = s - slotRes();
    avail = liftable(p);
    if (!avail) return [];
    if (wantN > 0 && wantN < avail) avail = wantN;
    for (i = 0; i < avail; i++) out.push(tab[p][tab[p].length - avail + i]);
    return out;
  }
  function commitTake(s, k) {
    if (s < slotCells()) { cell[s] = -1; return; }
    if (s < slotFound()) { found[s - slotCells()].pop(); return; }
    if (s < slotStock()) { waste.pop(); return; }
    if (s < slotRes())   { reserve.pop(); return; }
    var p = s - slotRes();
    tab[p].length -= k;
    tup[p] = Math.max(tab[p].length ? 1 : 0, tup[p] - k);
  }
  function tryMove(from, to) {
    var buf, k, i;
    if (from === to) return '';
    buf = peekTake(from, want);
    k = buf.length;
    if (!k) return 'Nothing to move from there.';

    if (to < slotCells()) {
      buf = peekTake(from, 1);
      if (buf.length !== 1) return 'Only one card fits a cell.';
      if (cell[to] >= 0) return 'That cell is occupied.';
      commitTake(from, 1); cell[to] = buf[0]; moves++; harvest(); return '';
    }
    if (to < slotFound()) {
      var f = to - slotCells();
      if (R.fmode === FD_SPIDER) return 'Complete a king-to-ace run in the tableau instead.';
      buf = peekTake(from, 1);
      if (buf.length !== 1) return 'One card at a time.';
      if (!foundAccepts(f, buf[0])) return 'That card will not go up there.';
      commitTake(from, 1); found[f].push(buf[0]); moves++; return '';
    }
    if (to < slotRes()) return 'You cannot move cards onto the stock.';

    var p = to - slotRes();
    while (k > 0) {
      var ok = true;
      if (!tab[p].length) { if (!mayFill(buf[0])) ok = false; }
      else if (!canStack(buf[0], tab[p][tab[p].length - 1])) ok = false;
      if (ok && R.movegrp === M_SEQ && k > moveLimit(tab[p].length === 0)) ok = false;
      if (ok) break;
      k--;
      if (k > 0) { buf = peekTake(from, k); k = buf.length; }
    }
    if (!k) {
      if (!tab[p].length)
        return R.fill === F_NONE ? 'Empty piles stay empty in this game.'
                                 : 'Only a king may start an empty pile.';
      return 'That card will not sit there.';
    }
    commitTake(from, k);
    for (i = 0; i < k; i++) tab[p].push(buf[i]);
    tup[p] = Math.min(tab[p].length, tup[p] + k);
    moves++; harvest();
    return '';
  }

  deal();

  function drawSlot(t, x, y, s) {
    var c = s < slotCells() ? cell[s]
          : s < slotFound() ? (found[s - slotCells()].length ? found[s - slotCells()][found[s - slotCells()].length - 1] : -1)
          : s < slotStock() ? (waste.length ? waste[waste.length - 1] : -1)
          : s < slotRes()   ? (reserve.length ? reserve[reserve.length - 1] : -1)
          : (tab[s - slotRes()].length ? tab[s - slotRes()][tab[s - slotRes()].length - 1] : -1);
    var bg = cur === s ? '#2b4a6b' : (sel === s ? '#2d5a3d' : null);
    if (c < 0) t.text(x, y, '[  ]', C.grey, bg);
    else t.text(x, y, (nameOf(c) + '   ').slice(0, 4), colourOf(c), bg);
  }

  return {
    key: function (k) {
      if (won()) { host.saveScore(Math.max(0, 10000 - moves)); deal(); return; }
      if (k === 'left')  { cur = (cur + nslots() - 1) % nslots(); return; }
      if (k === 'right') { cur = (cur + 1) % nslots(); return; }
      if (k === 'up')    { cur = cur >= slotRes() ? 0 : slotRes(); return; }
      if (k === 'down')  { cur = cur < slotRes() ? slotRes() : 0; return; }
      if (/^[1-9]$/.test(k)) { want = +k; msg = 'Group size set.'; return; }
      if (k === 'd' || k === 'space') { msg = drawStock(); return; }
      if (k === 'a') { msg = autoplay() ? '' : 'Nothing to send up.'; return; }
      if (k === 'n') { deal(); return; }
      if (k === 'x') { sel = -1; nsel = 0; want = 0; return; }
      if (k === 'enter') {
        if (sel < 0) {
          nsel = peekTake(cur, want).length;
          if (!nsel) { msg = 'Nothing to pick up there.'; return; }
          sel = cur;
        } else {
          msg = tryMove(sel, cur);
          sel = -1; nsel = 0; want = 0;
        }
      }
    },
    draw: function (t) {
      var i, p;
      t.header('PATIENCE', R.name + ' — arrows move, Enter picks up and drops, D draws, A auto-plays, N new deal');
      for (i = 0; i < R.cells; i++) drawSlot(t, 4 + i * 5, 3, i);
      for (i = 0; i < R.nfound; i++) {
        if (R.fmode === FD_SPIDER)
          t.text(48 + i * 5, 3, i < spiderDone ? '[##]' : '[  ]', i < spiderDone ? C.green : C.grey);
        else drawSlot(t, 48 + i * 5, 3, slotCells() + i);
      }
      if (R.stock !== S_NONE && R.stock !== S_ROW) {
        t.text(4, 5, 'stock ' + stock.length, C.grey);
        drawSlot(t, 15, 5, slotFound());
      } else if (R.stock === S_ROW) {
        t.text(4, 5, 'stock ' + stock.length + ' (deals a row)', C.grey);
      }
      if (R.reserve) { t.text(28, 5, 'reserve ' + reserve.length, C.grey); drawSlot(t, 42, 5, slotStock()); }

      for (p = 0; p < R.tpiles; p++) {
        var s = slotRes() + p, col = 3 + p * 5;
        t.text(col, 7, ' ' + (p < 9 ? (p + 1) : String.fromCharCode(97 + p - 9)) + ' ',
               C.yellow, cur === s ? '#2b4a6b' : null, true);
        if (!tab[p].length) { t.text(col, 8, '[  ]', C.grey, cur === s ? '#2b4a6b' : null); continue; }
        var lift = (sel === s) ? nsel : 0;
        for (i = 0; i < tab[p].length && i < 16; i++) {
          var c = tab[p][i], down = i < tab[p].length - tup[p];
          if (down) { t.text(col, 8 + i, '##  ', C.grey); continue; }
          t.text(col, 8 + i, (nameOf(c) + '   ').slice(0, 4), colourOf(c),
                 (lift && i >= tab[p].length - lift) ? '#2d5a3d' : null);
        }
        if (tab[p].length > 16) t.text(col, 24, '+' + (tab[p].length - 16), C.grey);
      }
      t.text(3, 26, 'Moves ' + moves + '   ' + (won() ? 'Solved! Press any key.' : msg),
             won() ? C.green : C.yellow);
    }
  };
}

reg('solitaire', {
  title: 'Patience', help: 'Arrows move · Enter picks up and drops · D draws · A auto-plays',
  start: function (host, p) {
    var v = (p.variant | 0);
    if (v < 0 || v > 28) v = 0;
    if (VMAP[v] >= 0) return makeGame(host, VMAP[v], p.title);
    return G.solitaireOther(host, p, v);
  }
});

/* solitaire2.js fills this in; exported here so both files can find it. */
G.solitaireRankOf = rankOf;
G.solitaireSuitOf = suitOf;
G.solitaireRedOf  = redOf;
G.solitaireNameOf = nameOf;
G.solitaireColour = colourOf;

})();
