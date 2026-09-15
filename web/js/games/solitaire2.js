/* solitaire2.js - the eleven patiences that are not tableau games,
 * mirroring src/games/solitaire2.c. Reached through G.solitaireOther(),
 * the same way the C build reaches solitaire_other().
 */
(function () {
'use strict';
var G = window.GIC, C = G.COL, rnd = G.rnd, shuffle = G.shuffle;

function rankOf(c) { return c % 13; }
function suitOf(c) { return Math.floor(c / 13) % 4; }
function redOf(c)  { var s = suitOf(c); return s === 1 || s === 2; }
var RANK = ['A','2','3','4','5','6','7','8','9','10','J','Q','K'];
var SUIT = ['♠','♥','♦','♣'];
function nameOf(c) { return (RANK[rankOf(c)] + SUIT[suitOf(c)] + '   ').slice(0, 4); }
function colourOf(c) { return redOf(c) ? C.red : C.white; }
function freshDeck() { var d = [], i; for (i = 0; i < 52; i++) d.push(i); return shuffle(d); }
function card(t, x, y, c, hi) {
  if (c < 0 || c === undefined) { t.text(x, y, '[  ]', C.grey, hi ? '#2b4a6b' : null); return; }
  t.text(x, y, nameOf(c), colourOf(c), hi ? '#2b4a6b' : null);
}

/* ---------------------------------------------------------------- Pyramid */
function pyramid(host) {
  var pyr, gone, stock, waste, cur, sel, cleared, redeals, msg;
  function deal() {
    var d = freshDeck(), i;
    pyr = d.slice(0, 28); gone = []; for (i = 0; i < 28; i++) gone.push(false);
    stock = d.slice(28); waste = [];
    cur = 27; sel = -1; cleared = 0; redeals = 0; msg = '';
  }
  function exposed(i) {
    if (i >= 28) return waste.length > 0;
    if (gone[i]) return false;
    var r = 0; while ((r + 1) * (r + 2) / 2 <= i) r++;
    if (r === 6) return true;
    var c = i - r * (r + 1) / 2, b = (r + 1) * (r + 2) / 2;
    return gone[b + c] && gone[b + c + 1];
  }
  deal();
  return {
    key: function (k) {
      if (cleared === 28) { host.saveScore(1000 - redeals * 100); deal(); return; }
      if (k === 'left')  { cur = (cur + 28) % 29; return; }
      if (k === 'right') { cur = (cur + 1) % 29; return; }
      if (k === 'up')    { cur = cur >= 28 ? 27 : (cur > 0 ? ((cur - 1) >> 1) : 28); return; }
      if (k === 'down')  { cur = cur < 28 ? 28 : 27; return; }
      if (k === 'd' || k === 'space') {
        if (stock.length) waste.push(stock.pop());
        else if (waste.length && redeals < 2) { while (waste.length) stock.push(waste.pop()); redeals++; }
        else msg = 'No draws left.';
        return;
      }
      if (k !== 'enter') return;
      if (!exposed(cur)) { msg = 'That card is still covered.'; sel = -1; return; }
      var c = cur < 28 ? pyr[cur] : waste[waste.length - 1];
      if (rankOf(c) === 12) { if (cur < 28) { gone[cur] = true; cleared++; } else waste.pop(); sel = -1; return; }
      if (sel < 0) { sel = cur; msg = 'Now pick its partner.'; return; }
      if (sel === cur) { sel = -1; return; }
      var o = sel < 28 ? pyr[sel] : waste[waste.length - 1];
      if (rankOf(c) + rankOf(o) + 2 !== 13) { msg = 'Those two do not total thirteen.'; sel = -1; return; }
      if (sel < 28) { gone[sel] = true; cleared++; } else waste.pop();
      if (cur < 28) { gone[cur] = true; cleared++; } else waste.pop();
      sel = -1; msg = '';
    },
    draw: function (t) {
      t.header('PYRAMID SOLITAIRE', 'Pairs totalling thirteen — arrows move, Enter selects, D draws');
      for (var r = 0; r < 7; r++) {
        var base = r * (r + 1) / 2;
        for (var c = 0; c <= r; c++) {
          var i = base + c;
          if (!gone[i]) card(t, 24 - r * 3 + c * 6, 3 + r * 2, pyr[i], i === cur || i === sel);
        }
      }
      t.text(6, 18, 'stock ' + stock.length, C.grey);
      card(t, 18, 18, waste.length ? waste[waste.length - 1] : -1, cur === 28 || sel === 28);
      t.text(28, 18, 'cleared ' + cleared + '/28', C.grey);
      t.text(6, 20, cleared === 28 ? 'Pyramid cleared! Press any key.' : msg,
             cleared === 28 ? C.green : C.yellow);
    }
  };
}

/* -------------------------------------------------------- TriPeaks / Golf */
function ladder(host, tri) {
  var lay, gone, pile, stock, waste, cur, cleared, streak, best, score, msg;
  var ROWLEN = [3, 6, 9, 10];
  function deal() {
    var d = freshDeck(), i, j, pos = 0;
    cleared = 0; streak = 0; best = 0; score = 0; cur = 0; msg = ''; waste = [];
    if (tri) { lay = d.slice(0, 28); gone = []; for (i = 0; i < 28; i++) gone.push(false); stock = d.slice(28); }
    else {
      pile = [];
      for (i = 0; i < 7; i++) { var q = []; for (j = 0; j < 5; j++) q.push(d[pos++]); pile.push(q); }
      stock = d.slice(pos);
    }
    waste.push(stock.pop());
  }
  function rowOf(i) { return i >= 18 ? 3 : i >= 9 ? 2 : i >= 3 ? 1 : 0; }
  function uncovered(i) {
    if (gone[i]) return false;
    var r = rowOf(i), c = i - [0, 3, 9, 18][r], b;
    if (r === 3) return true;
    if (r === 0) b = 3 + 2 * c;
    else if (r === 1) b = 9 + 3 * (c >> 1) + (c & 1);
    else b = 18 + c;
    return gone[b] && gone[b + 1];
  }
  function total() { return tri ? 28 : 35; }
  deal();
  return {
    key: function (k) {
      if (cleared === total()) { host.saveScore(score + 500); deal(); return; }
      var n = tri ? 28 : 7;
      if (k === 'left')  { cur = (cur + n - 1) % n; return; }
      if (k === 'right') { cur = (cur + 1) % n; return; }
      if (tri && k === 'up')   { if (cur >= 10) cur -= 10; return; }
      if (tri && k === 'down') { if (cur + 10 < 28) cur += 10; return; }
      if (k === 'd' || k === 'space') {
        if (!stock.length) { msg = 'The stock is empty.'; return; }
        waste.push(stock.pop()); streak = 0; return;
      }
      if (k !== 'enter') return;
      var c;
      if (tri) {
        if (!uncovered(cur)) { msg = 'That card is still covered.'; return; }
        c = lay[cur];
      } else {
        if (!pile[cur].length) { msg = 'That pile is empty.'; return; }
        c = pile[cur][pile[cur].length - 1];
      }
      var w = waste[waste.length - 1];
      if (tri) {
        var dd = (rankOf(c) - rankOf(w) + 13) % 13;
        if (dd !== 1 && dd !== 12) { msg = 'Not one rank away.'; return; }
      } else {
        var d2 = rankOf(c) - rankOf(w);
        if (d2 !== 1 && d2 !== -1) { msg = 'Not one rank away (Golf does not wrap).'; return; }
      }
      waste.push(c);
      if (tri) gone[cur] = true; else pile[cur].pop();
      cleared++; streak++;
      if (streak > best) best = streak;
      score += streak * 10;
      msg = '';
    },
    draw: function (t) {
      t.header(tri ? 'TRIPEAKS' : 'GOLF SOLITAIRE',
        tri ? 'One rank either way, aces wrap — arrows move, Enter takes, D deals'
            : 'One rank either way, no wrap — arrows move, Enter takes, D deals');
      if (tri) {
        var base = 0;
        for (var r = 0; r < 4; r++) {
          for (var c = 0; c < ROWLEN[r]; c++) {
            var i = base + c, col;
            if (r === 0) col = 10 + c * 20;
            else if (r === 1) col = 6 + (c >> 1) * 20 + (c & 1) * 8;
            else if (r === 2) col = 4 + Math.floor(c / 3) * 20 + (c % 3) * 6;
            else col = 2 + c * 6;
            if (!gone[i]) card(t, col, 3 + r * 2, lay[i], i === cur);
          }
          base += ROWLEN[r];
        }
      } else {
        for (var p = 0; p < 7; p++) {
          t.text(4 + p * 7, 3, ' ' + (p + 1) + ' ', C.yellow, p === cur ? '#2b4a6b' : null, true);
          for (var j = 0; j < pile[p].length; j++)
            card(t, 4 + p * 7, 4 + j, pile[p][j], p === cur && j === pile[p].length - 1);
          if (!pile[p].length) card(t, 4 + p * 7, 4, -1, p === cur);
        }
      }
      t.text(4, 13, 'stock ' + stock.length, C.grey);
      card(t, 16, 13, waste.length ? waste[waste.length - 1] : -1, false);
      t.text(24, 13, 'cleared ' + cleared + '/' + total() + '  streak ' + streak +
                     ' (best ' + best + ')  score ' + score, C.grey);
      t.text(4, 15, cleared === total() ? 'Board cleared! Press any key.' : msg,
             cleared === total() ? C.green : C.yellow);
    }
  };
}

/* ---------------------------------------------------------------- Aces Up */
function acesUp(host) {
  var deck, pile, pos, cur, msg;
  function deal() {
    deck = freshDeck(); pile = [[], [], [], []]; pos = 0; cur = 0; msg = '';
    for (var i = 0; i < 4; i++) pile[i].push(deck[pos++]);
  }
  function left() { return pile[0].length + pile[1].length + pile[2].length + pile[3].length; }
  function beats(hi, lo) { /* aces are high */
    if (rankOf(hi) === 0) return rankOf(lo) !== 0;
    return rankOf(lo) !== 0 && rankOf(hi) > rankOf(lo);
  }
  deal();
  return {
    key: function (k) {
      var i;
      if (left() === 4 && pos >= 52) {
        var aces = 0;
        for (i = 0; i < 4; i++) if (pile[i].length === 1 && rankOf(pile[i][0]) === 0) aces++;
        if (aces === 4) { host.saveScore(1000); deal(); return; }
      }
      if (k === 'left')  { cur = (cur + 3) % 4; return; }
      if (k === 'right') { cur = (cur + 1) % 4; return; }
      if (k === 'd' || k === 'space') {
        if (pos >= 52) { msg = 'The stock is empty.'; return; }
        for (i = 0; i < 4 && pos < 52; i++) pile[i].push(deck[pos++]);
        return;
      }
      if (k === 'm') {
        if (!pile[cur].length) { msg = 'Nothing to move.'; return; }
        var tgt = -1;
        for (i = 0; i < 4; i++) if (!pile[i].length) tgt = i;
        if (tgt < 0) { msg = 'No empty pile to move into.'; return; }
        pile[tgt].push(pile[cur].pop());
        return;
      }
      if (k !== 'enter') return;
      if (!pile[cur].length) { msg = 'That pile is empty.'; return; }
      var c = pile[cur][pile[cur].length - 1], ok = false;
      for (i = 0; i < 4; i++)
        if (i !== cur && pile[i].length && suitOf(pile[i][pile[i].length - 1]) === suitOf(c) &&
            beats(pile[i][pile[i].length - 1], c)) ok = true;
      if (!ok) { msg = 'Nothing of that suit beats it.'; return; }
      pile[cur].pop(); msg = '';
    },
    draw: function (t) {
      t.header('ACES UP', 'Discard any card beaten by its own suit — Enter discards, M moves to a space, D deals');
      for (var i = 0; i < 4; i++) {
        t.text(10 + i * 10, 3, ' pile ' + (i + 1) + ' ', C.yellow, i === cur ? '#2b4a6b' : null, true);
        for (var j = 0; j < pile[i].length; j++)
          card(t, 10 + i * 10, 4 + j, pile[i][j], i === cur && j === pile[i].length - 1);
        if (!pile[i].length) card(t, 10 + i * 10, 4, -1, i === cur);
      }
      t.text(6, 20, 'stock ' + (52 - pos) + '   cards left ' + left(), C.grey);
      t.text(6, 22, msg, C.yellow);
    }
  };
}

/* -------------------------------------------------------------- Accordion */
function accordion(host) {
  var top, size, cur, msg;
  function deal() { top = freshDeck(); size = top.map(function () { return 1; }); cur = 0; msg = ''; }
  function ok(a, b) { return suitOf(top[a]) === suitOf(top[b]) || rankOf(top[a]) === rankOf(top[b]); }
  deal();
  return {
    key: function (k) {
      if (top.length === 1) { host.saveScore(2000); deal(); return; }
      if (k === 'left')  { cur = (cur + top.length - 1) % top.length; return; }
      if (k === 'right') { cur = (cur + 1) % top.length; return; }
      if (k === 'up')    { if (cur >= 13) cur -= 13; return; }
      if (k === 'down')  { if (cur + 13 < top.length) cur += 13; return; }
      if (k !== 'enter' && k !== '1' && k !== '3') return;
      var back = (k === '3') ? 3 : 1, dst = cur - back;
      if (dst < 0) { msg = 'Nothing that far back.'; return; }
      if (!ok(cur, dst)) { msg = 'They share neither suit nor rank.'; return; }
      size[dst] += size[cur]; top[dst] = top[cur];
      top.splice(cur, 1); size.splice(cur, 1);
      if (cur >= top.length) cur = top.length - 1;
      msg = '';
    },
    draw: function (t) {
      t.header('ACCORDION', 'Fold onto the pile beside it or three back — Enter folds left, 3 folds three back');
      for (var i = 0; i < top.length; i++) {
        var y = 3 + Math.floor(i / 13) * 3, x = 4 + (i % 13) * 5;
        card(t, x, y, top[i], i === cur);
        if (size[i] > 1) t.text(x, y + 1, 'x' + size[i], C.grey);
      }
      t.text(4, 16, 'piles ' + top.length + ' (one pile wins)', C.grey);
      t.text(4, 18, top.length === 1 ? 'One pile! Press any key.' : msg,
             top.length === 1 ? C.green : C.yellow);
    }
  };
}

/* --------------------------------------------------------- Clock Patience */
function clockPatience(host) {
  var pile, up, placed, kings, cur;
  var CX = [40,50,57,60,57,50,40,30,23,20,23,30,40];
  var CY = [3,4,6,8,10,11,10,8,6,4,3,2,7];
  function deal() {
    var d = freshDeck(), i;
    pile = []; up = [];
    for (i = 0; i < 13; i++) { pile.push([]); up.push([]); }
    for (i = 0; i < 52; i++) pile[i % 13].push(d[i]);
    placed = 0; kings = 0; cur = 12;
  }
  deal();
  return {
    key: function (k) {
      if (kings === 4 || placed === 52) { host.saveScore(placed * 10); deal(); return; }
      if (k !== 'space' && k !== 'enter') return;
      if (!pile[cur].length) { cur = (cur + 1) % 13; return; }
      var c = pile[cur].pop(), home = rankOf(c);
      if (up[home].length < 4) up[home].push(c);
      placed++;
      if (home === 12) kings++;
      cur = home;
    },
    draw: function (t) {
      t.header('CLOCK PATIENCE', 'The cards decide — press Space to turn the next');
      for (var i = 0; i < 13; i++) {
        t.text(CX[i], CY[i], (i === 12 ? '13' : String(i + 1)), C.grey, i === cur ? '#2b4a6b' : null);
        for (var j = 0; j < up[i].length; j++) card(t, CX[i] + 3 + j * 5, CY[i], up[i][j], false);
        if (!up[i].length) t.text(CX[i] + 3, CY[i], '[' + pile[i].length + ']', C.grey);
      }
      t.text(6, 14, 'placed ' + placed + '/52   kings shown ' + kings + '/4', C.grey);
      if (kings === 4 || placed === 52)
        t.text(6, 16, placed === 52 ? 'Every card home — a one-in-eighty win.'
                                    : 'The fourth king came up too soon.',
               placed === 52 ? C.green : C.red);
    }
  };
}

/* ------------------------------------------------------------ Calculation */
function calculation(host) {
  var found, waste, stock, cur, placed, msg;
  function deal() {
    var d = freshDeck(), i, j;
    found = []; waste = [[], [], [], []];
    for (i = 0; i < 4; i++) {
      for (j = 0; j < d.length; j++) if (rankOf(d[j]) === i) break;
      found.push([d.splice(j, 1)[0]]);
    }
    stock = d; cur = 0; placed = 4; msg = '';
  }
  function wants(i) { return (rankOf(found[i][found[i].length - 1]) + i + 1) % 13; }
  function cascade() {
    var again = true, i, j;
    while (again) {
      again = false;
      for (i = 0; i < 4; i++) {
        if (!waste[i].length) continue;
        for (j = 0; j < 4; j++)
          if (found[j].length < 13 && rankOf(waste[i][waste[i].length - 1]) === wants(j)) {
            found[j].push(waste[i].pop()); placed++; again = true; break;
          }
      }
    }
  }
  deal();
  return {
    key: function (k) {
      if (placed === 52) { host.saveScore(2000); deal(); return; }
      if (k === 'left')  { cur = (cur + 7) % 8; return; }
      if (k === 'right') { cur = (cur + 1) % 8; return; }
      if (k === 'up')    { if (cur >= 4) cur -= 4; return; }
      if (k === 'down')  { if (cur < 4) cur += 4; return; }
      if (k !== 'enter') return;
      if (!stock.length) { msg = 'The stock is empty.'; return; }
      var c = stock[stock.length - 1];
      if (cur < 4) {
        if (found[cur].length >= 13) { msg = 'That foundation is finished.'; return; }
        if (rankOf(c) !== wants(cur)) { msg = 'Not the rank that foundation wants.'; return; }
        found[cur].push(stock.pop()); placed++;
      } else waste[cur - 4].push(stock.pop());
      cascade(); msg = '';
    },
    draw: function (t) {
      t.header('CALCULATION', 'Foundations climb by one, two, three and four — arrows move, Enter places');
      for (var i = 0; i < 4; i++) {
        t.text(6 + i * 16, 3, 'step +' + (i + 1), C.grey);
        card(t, 6 + i * 16, 4, found[i][found[i].length - 1], cur === i);
        t.text(12 + i * 16, 4, found[i].length + '/13 next ' + RANK[wants(i)], C.grey);
        t.text(6 + i * 16, 7, 'waste ' + (i + 1) + ' (' + waste[i].length + ')', C.grey);
        card(t, 6 + i * 16, 8, waste[i].length ? waste[i][waste[i].length - 1] : -1, cur === 4 + i);
      }
      t.text(6, 11, 'stock ' + stock.length, C.grey);
      if (stock.length) {
        t.text(18, 11, 'in hand:', C.grey);
        card(t, 27, 11, stock[stock.length - 1], false);
      }
      t.text(6, 13, 'placed ' + placed + '/52', C.grey);
      t.text(6, 15, placed === 52 ? 'All fifty-two placed! Press any key.' : msg,
             placed === 52 ? C.green : C.yellow);
    }
  };
}

/* ---------------------------------------------------------------- Garbage */
function garbage(host) {
  var deck, slot, pos, hand, round, best, msg;
  function deal() {
    deck = freshDeck(); slot = []; for (var i = 0; i < 10; i++) slot.push(-1);
    pos = 10; hand = -1; round = 1; best = 0; msg = '';
  }
  function filled() { return slot.filter(function (s) { return s >= 0; }).length; }
  deal();
  return {
    key: function (k) {
      if (filled() === 10) { host.saveScore(Math.max(100, 1000 - round * 50)); deal(); return; }
      if (52 - pos <= 0 && hand < 0) { host.saveScore(best * 60); deal(); return; }
      if (k !== 'space' && k !== 'enter') return;
      if (hand < 0) { if (pos >= 52) return; hand = deck[pos++]; }
      var r = rankOf(hand), i;
      if (r === 12) {
        for (i = 0; i < 10; i++) if (slot[i] < 0) break;
        if (i === 10) { hand = -1; return; }
        slot[i] = hand; hand = -1; msg = 'King is wild.';
      } else if (r >= 10) {
        msg = 'Garbage — the run ends.';
        hand = -1; round++;
        slot = slot.map(function () { return -1; });
        deck = freshDeck(); pos = 0;
      } else if (slot[r] >= 0) {
        msg = 'Slot ' + (r + 1) + ' is taken — the run ends.';
        hand = -1; round++;
      } else { slot[r] = hand; hand = -1; msg = ''; }
      if (filled() > best) best = filled();
    },
    draw: function (t) {
      t.header('GARBAGE', 'Cards find their own slot; a jack or queen ends the run — Space draws');
      for (var i = 0; i < 10; i++) {
        t.text(5 + i * 7, 4, String(i + 1), C.grey);
        if (slot[i] >= 0) card(t, 5 + i * 7, 5, slot[i], false);
        else t.text(5 + i * 7, 5, '[##]', C.grey);
      }
      t.text(5, 8, 'stock ' + (52 - pos) + '   round ' + round + '   filled ' + filled() + '/10', C.grey);
      if (hand >= 0) { t.text(5, 10, 'in hand', C.grey); card(t, 14, 10, hand, false); }
      t.text(5, 12, filled() === 10 ? 'All ten filled! Press any key.' : msg,
             filled() === 10 ? C.green : C.yellow);
    }
  };
}

/* ----------------------------------------------------------- Kings Corner */
function kingsCorner(host) {
  var deck, pos, pile, hand, cur, msg, over;
  function deal() {
    deck = freshDeck(); pos = 0; pile = []; hand = [[], []];
    for (var i = 0; i < 8; i++) pile.push([]);
    for (i = 0; i < 7; i++) { hand[0].push(deck[pos++]); hand[1].push(deck[pos++]); }
    for (i = 0; i < 4; i++) pile[i].push(deck[pos++]);
    cur = 0; msg = 'Your turn — 1-8 plays the selected card, D draws and ends the turn.'; over = 0;
  }
  function fits(t, c) {
    if (pile[t].length) {
      var top = pile[t][pile[t].length - 1];
      return rankOf(top) === rankOf(c) + 1 && redOf(top) !== redOf(c);
    }
    return t >= 4 ? rankOf(c) === 12 : true;
  }
  function opponent() {
    var again = true, i, t;
    while (again) {
      again = false;
      for (i = 0; i < hand[1].length && !again; i++)
        for (t = 0; t < 8; t++) {
          if (!fits(t, hand[1][i])) continue;
          pile[t].push(hand[1].splice(i, 1)[0]);
          again = true;
          break;
        }
    }
    if (pos < 52 && hand[1].length) hand[1].push(deck[pos++]);
  }
  deal();
  return {
    key: function (k) {
      if (over) { host.saveScore(over === 1 ? 500 : 100); deal(); return; }
      if (k === 'left')  { if (hand[0].length) cur = (cur + hand[0].length - 1) % hand[0].length; return; }
      if (k === 'right') { if (hand[0].length) cur = (cur + 1) % hand[0].length; return; }
      if (k === 'd') {
        if (pos < 52) hand[0].push(deck[pos++]);
        opponent();
        if (!hand[1].length) over = 2;
        msg = 'Your turn.';
        return;
      }
      if (!/^[1-8]$/.test(k)) return;
      var t = +k - 1;
      if (cur >= hand[0].length) { msg = 'No card selected.'; return; }
      var c = hand[0][cur];
      if (!fits(t, c)) {
        msg = pile[t].length ? 'Must be one lower and the other colour.'
                             : 'Only a king may open a corner.';
        return;
      }
      pile[t].push(c);
      hand[0].splice(cur, 1);
      if (cur >= hand[0].length && cur > 0) cur--;
      if (!hand[0].length) { over = 1; return; }
      msg = 'Played — keep going or press D to end your turn.';
    },
    draw: function (t) {
      t.header('KINGS CORNER', 'Build down in alternating colours; corners need a king — 1-8 plays, D draws');
      for (var i = 0; i < 8; i++) {
        var x = 6 + (i % 4) * 14, y = 3 + Math.floor(i / 4) * 4;
        t.text(x, y, (i < 4 ? 'side ' : 'corner ') + (i + 1), C.grey);
        card(t, x, y + 1, pile[i].length ? pile[i][pile[i].length - 1] : -1, false);
        if (pile[i].length > 1) t.text(x + 5, y + 1, 'x' + pile[i].length, C.grey);
      }
      t.text(6, 12, 'opponent holds ' + hand[1].length + ' cards   stock ' + (52 - pos), C.grey);
      t.text(6, 14, 'your hand:', C.white, null, true);
      for (i = 0; i < hand[0].length; i++) card(t, 6 + i * 5, 15, hand[0][i], i === cur);
      t.text(6, 17, over ? (over === 1 ? 'Your hand is empty — you win.' : 'The opponent went out first.') : msg,
             over === 1 ? C.green : over === 2 ? C.red : C.yellow);
    }
  };
}

/* -------------------------------------------------------- Nertz and Spit */
function race(host, spit) {
  var deck, pos, stockPile, work, centre, hand, foes, cur, score, t0, lastFoe, msg, over;
  var LIMIT = 180000;
  function deal() {
    deck = freshDeck(); pos = 0; stockPile = []; work = [[], [], [], []]; centre = [[], []]; hand = [];
    for (var i = 0; i < (spit ? 10 : 13); i++) stockPile.push(deck[pos++]);
    for (i = 0; i < 4; i++) work[i].push(deck[pos++]);
    for (i = 0; i < 2; i++) centre[i].push(deck[pos++]);
    while (pos < 52) hand.push(deck[pos++]);
    foes = spit ? 12 : 0; cur = 0; score = 0; t0 = Date.now(); lastFoe = Date.now();
    msg = ''; over = 0;
  }
  deal();
  return {
    tick: function () {
      if (over) return;
      if (spit) {
        if (Date.now() - lastFoe > 1400) {
          lastFoe = Date.now();
          if (foes > 0 && rnd(100) < 55) {
            var t = rnd(2), c = rnd(52);
            var d = (rankOf(c) - rankOf(centre[t][centre[t].length - 1]) + 13) % 13;
            if (d === 1 || d === 12) { centre[t].push(c); foes--; }
          }
        }
        if (!foes) over = 2;
      } else if (Date.now() - t0 >= LIMIT) over = 2;
      if (!stockPile.length) over = 1;
    },
    key: function (k) {
      if (over) { host.saveScore(score + (over === 1 ? 500 : 0)); deal(); return; }
      if (k === 'left')  { cur = (cur + 4) % 5; return; }
      if (k === 'right') { cur = (cur + 1) % 5; return; }
      if (k === 'd') {
        if (hand.length) work[rnd(4)].push(hand.pop());
        else msg = 'Your hand is empty.';
        return;
      }
      if (k !== '1' && k !== '2' && k !== 'enter') return;
      var t = (k === '2') ? 1 : 0;
      var c = cur === 0 ? (stockPile.length ? stockPile[stockPile.length - 1] : -1)
                        : (work[cur - 1].length ? work[cur - 1][work[cur - 1].length - 1] : -1);
      if (c < 0) { msg = 'Nothing there to play.'; return; }
      if (!centre[t].length) { msg = 'That centre pile is empty.'; return; }
      var d = (rankOf(c) - rankOf(centre[t][centre[t].length - 1]) + 13) % 13;
      if (d !== 1 && d !== 12) { msg = 'Not one rank away.'; return; }
      centre[t].push(c);
      if (cur === 0) stockPile.pop(); else work[cur - 1].pop();
      score += 25; msg = '';
    },
    draw: function (t) {
      t.header(spit ? 'SPIT' : 'NERTZ',
        spit ? 'Slap cards one rank either way onto the centre — 1 and 2 play, D flips'
             : 'Beat the clock — 1 and 2 play, D flips');
      t.text(6, 3, (spit ? 'your stock ' : 'nertz ') + stockPile.length, C.white, null, true);
      card(t, 6, 4, stockPile.length ? stockPile[stockPile.length - 1] : -1, cur === 0);
      for (var i = 0; i < 4; i++) {
        t.text(22 + i * 10, 3, 'work ' + (i + 1), C.grey);
        card(t, 22 + i * 10, 4, work[i].length ? work[i][work[i].length - 1] : -1, cur === 1 + i);
        if (work[i].length > 1) t.text(27 + i * 10, 4, 'x' + work[i].length, C.grey);
      }
      for (i = 0; i < 2; i++) {
        t.text(22 + i * 16, 7, 'centre ' + (i + 1), C.yellow);
        card(t, 22 + i * 16, 8, centre[i].length ? centre[i][centre[i].length - 1] : -1, false);
      }
      t.text(6, 10, 'hand ' + hand.length + '   score ' + score, C.grey);
      t.text(32, 10, spit ? 'opponent stock ' + foes
                          : 'time left ' + Math.max(0, Math.ceil((LIMIT - (Date.now() - t0)) / 1000)) + ' s', C.grey);
      t.text(6, 12, over ? (over === 1 ? 'Stock cleared — you win.'
                                       : (spit ? 'The opponent went out first.'
                                               : 'Time — ' + stockPile.length + ' cards left.'))
                         : msg,
             over === 1 ? C.green : over === 2 ? C.red : C.yellow);
    }
  };
}

G.solitaireOther = function (host, p, v) {
  switch (v) {
    case  9: return pyramid(host);
    case 10: return ladder(host, 1);
    case 11: return ladder(host, 0);
    case 17: return acesUp(host);
    case 18: return accordion(host);
    case 21: return clockPatience(host);
    case 23: return calculation(host);
    case 25: return garbage(host);
    case 26: return kingsCorner(host);
    case 27: return race(host, 0);
    case 28: return race(host, 1);
    default: return pyramid(host);
  }
};

})();
