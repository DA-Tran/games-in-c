/* cards.js - ports of src/games/{blackjack,video_poker,war,go_fish,
 * yahtzee,pig,slots,higher_lower}.c
 *
 * Card encoding matches include/cards.h: suit = card/13, rank = card%13.
 */
(function () {
'use strict';
var G = window.GIC, C = G.COL, reg = G.register, rnd = G.rnd, shuffle = G.shuffle;

var RANK = ['A','2','3','4','5','6','7','8','9','10','J','Q','K'];
var SUIT = ['♠','♥','♦','♣'];
function suitColour(card) { var s = card / 13 | 0; return (s === 1 || s === 2) ? C.red : C.white; }
function cardName(card) { return RANK[card % 13] + SUIT[card / 13 | 0]; }
function newDeck() { return shuffle(Array.from({ length: 52 }, function (_, i) { return i; })); }
function drawCard(t, x, y, card, up) {
  t.text(x, y, '┌────┐', C.grey);
  if (up) {
    t.text(x, y + 1, '│', C.grey);
    t.text(x + 1, y + 1, (cardName(card) + '   ').slice(0, 4), suitColour(card), null, true);
    t.text(x + 5, y + 1, '│', C.grey);
  } else t.text(x, y + 1, '│▒▒▒▒│', C.blue);
  t.text(x, y + 2, '└────┘', C.grey);
}

/* -------------------------------------------------------------- blackjack */
reg('blackjack', {
  title: 'Blackjack', help: 'H hit · S stand · D double · Enter deals · Q quits',
  start: function (host, p) {
    var deck, top, phand, dhand, chips, bet, phase, msg;
    function deal() { if (top >= 52) { deck = newDeck(); top = 0; } return deck[top++]; }
    function value(h) {
      var total = 0, aces = 0;
      h.forEach(function (c) {
        var r = c % 13;
        if (r === 0) { total += 11; aces++; }
        else if (r >= 9) total += 10;
        else total += r + 1;
      });
      while (total > 21 && aces > 0) { total -= 10; aces--; }
      return total;
    }
    function reset() { deck = newDeck(); top = 0; chips = 100; bet = 10; phase = 'bet'; msg = null; }
    function start() {
      phand = [deal(), deal()];
      dhand = [deal(), deal()];
      phase = 'play';
      msg = null;
      if (value(phand) === 21) { chips += Math.floor(bet * 1.5); msg = 'Blackjack! Pays 3:2.'; phase = 'done'; }
    }
    function settle() {
      while (value(dhand) < 17 && dhand.length < 11) dhand.push(deal());
      var pv = value(phand), dv = value(dhand);
      if (pv > 21)      { msg = 'Bust — you lose.'; chips -= bet; }
      else if (dv > 21) { msg = 'Dealer busts — you win!'; chips += bet; }
      else if (dv > pv) { msg = 'Dealer wins.'; chips -= bet; }
      else if (dv < pv) { msg = 'You win!'; chips += bet; }
      else msg = 'Push.';
      phase = 'done';
      host.saveScore(chips);
    }
    reset();
    return {
      key: function (k) {
        if (phase === 'bet') {
          if (k === 'up'   && bet + 10 <= chips) bet += 10;
          if (k === 'down' && bet > 10) bet -= 10;
          if (k === 'enter' || k === 'space') start();
          return;
        }
        if (phase === 'done') {
          if (chips <= 0) { reset(); return; }
          phase = 'bet';
          bet = Math.min(bet, chips);
          msg = null;
          return;
        }
        if (k === 'h') {
          phand.push(deal());
          if (value(phand) > 21) settle();
          return;
        }
        if (k === 's') { settle(); return; }
        if (k === 'd' && phand.length === 2 && chips >= bet * 2) {
          bet *= 2; phand.push(deal()); settle();
        }
      },
      draw: function (t) {
        t.header('BLACKJACK', 'H hit · S stand · D double · Enter deals · Q quits');
        if (phase === 'bet') {
          t.text(30, 8, 'Chips: ' + chips, C.fg);
          t.text(30, 10, 'Bet:   ' + bet, C.yellow, null, true);
          t.text(30, 13, 'Up/Down changes the bet, Enter deals.', C.dim);
          return;
        }
        t.text(18, 4, 'Dealer', C.red, null, true);
        dhand.forEach(function (c, i) { drawCard(t, 18 + i * 7, 5, c, phase !== 'play' || i !== 1); });
        t.text(18, 8, phase === 'play' ? 'Total: ?  ' : 'Total: ' + value(dhand) + '  ', C.fg);
        t.text(18, 11, 'You', C.cyan, null, true);
        phand.forEach(function (c, i) { drawCard(t, 18 + i * 7, 12, c, true); });
        t.text(18, 15, 'Total: ' + value(phand) + '  ', C.fg);
        t.text(18, 18, 'Chips: ' + chips + '   Bet: ' + bet + '   ', C.fg);
        if (msg) t.text(18, 20, msg + (phase === 'done' ? '  (press any key)' : ''), C.white, null, true);
      }
    };
  }
});

/* ------------------------------------------------------------ video poker */
reg('videopoker', {
  title: 'Video Poker', help: 'Left/Right select · Space holds · Enter draws · Q quits',
  start: function (host, p) {
    var PAY_NAME = ['Royal Flush','Straight Flush','Four of a Kind','Full House',
                    'Flush','Straight','Three of a Kind','Two Pair','Jacks or Better'];
    var PAYOUT = [800,50,25,9,6,4,3,2,1];
    var deck, top, hand, hold, credits, cur, phase, msg;
    function evaluate() {
      var counts = new Array(13).fill(0), suits = [0,0,0,0];
      hand.forEach(function (c) { counts[c % 13]++; suits[c / 13 | 0]++; });
      var pairs = 0, three = false, four = false, jacks = false;
      var flush = suits.some(function (s) { return s === 5; });
      var lo = 13, hi = -1, distinct = 0;
      counts.forEach(function (n, i) {
        if (n === 2) { pairs++; if (i === 0 || i >= 10) jacks = true; }
        if (n === 3) three = true;
        if (n === 4) four = true;
        if (n) { distinct++; lo = Math.min(lo, i); hi = Math.max(hi, i); }
      });
      var straight = 0;
      if (distinct === 5) {
        if (hi - lo === 4) straight = 1;
        if (counts[0] && counts[9] && counts[10] && counts[11] && counts[12]) straight = 2;
      }
      if (straight === 2 && flush) return 0;
      if (straight && flush) return 1;
      if (four) return 2;
      if (three && pairs === 1) return 3;
      if (flush) return 4;
      if (straight) return 5;
      if (three) return 6;
      if (pairs === 2) return 7;
      if (pairs === 1 && jacks) return 8;
      return -1;
    }
    function newHand() {
      if (top > 40) { deck = newDeck(); top = 0; }
      hand = []; hold = [false,false,false,false,false];
      for (var i = 0; i < 5; i++) hand.push(deck[top++]);
      credits -= 5;
      phase = 'hold';
      msg = null;
    }
    function reset() { deck = newDeck(); top = 0; credits = 100; cur = 0; newHand(); }
    reset();
    return {
      key: function (k) {
        if (phase === 'draw') {
          if (credits <= 0) { reset(); return; }
          newHand();
          return;
        }
        if (k === 'left')  cur = (cur + 4) % 5;
        if (k === 'right') cur = (cur + 1) % 5;
        if (k === 'space') hold[cur] = !hold[cur];
        if (k !== 'enter') return;
        for (var i = 0; i < 5; i++) {
          if (hold[i]) continue;
          if (top >= 52) { deck = newDeck(); top = 0; }
          hand[i] = deck[top++];
        }
        var r = evaluate();
        if (r >= 0) {
          credits += PAYOUT[r] * 5;
          msg = PAY_NAME[r] + ' — pays ' + (PAYOUT[r] * 5) + '!';
        } else msg = 'No win.';
        phase = 'draw';
        host.saveScore(credits);
      },
      draw: function (t) {
        t.header('VIDEO POKER', 'Left/Right select · Space holds · Enter draws · Q quits');
        PAY_NAME.forEach(function (n, i) {
          t.text(52, 4 + i, (n + '                ').slice(0, 17) + String(PAYOUT[i]).padStart(4, ' '), C.dim);
        });
        hand.forEach(function (c, i) {
          drawCard(t, 12 + i * 7, 7, c, true);
          t.text(12 + i * 7, 10, hold[i] ? ' HOLD ' : '      ', C.yellow, null, true);
          t.text(12 + i * 7, 11, i === cur ? '  ^^  ' : '      ', C.cyan, null, true);
        });
        t.text(12, 14, 'Credits: ' + credits + '   ' + (phase === 'hold' ? 'Hold phase' : 'Draw phase'), C.fg);
        if (msg) t.text(12, 16, msg + '  (press any key)', C.white, null, true);
      }
    };
  }
});

/* -------------------------------------------------------------------- war */
reg('war', {
  title: 'War', help: 'Enter plays the next round · Q quits',
  start: function (host, p) {
    var p, c, pot, pc_, cc_, msg, rounds, over;
    function rankOf(card) { var r = card % 13; return r === 0 ? 13 : r; }
    function reset() {
      var d = newDeck();
      p = d.slice(0, 26); c = d.slice(26);
      pot = []; pc_ = cc_ = null; msg = 'Press Enter to play.'; rounds = 0; over = null;
    }
    function round() {
      if (!p.length || !c.length) return;
      rounds++;
      pot = [];
      pc_ = p.shift(); cc_ = c.shift();
      pot.push(pc_, cc_);
      while (rankOf(pc_) === rankOf(cc_) && p.length > 3 && c.length > 3) {
        for (var i = 0; i < 3; i++) { pot.push(p.shift()); pot.push(c.shift()); }
        pc_ = p.shift(); cc_ = c.shift();
        pot.push(pc_, cc_);
      }
      if (rankOf(pc_) > rankOf(cc_)) { p = p.concat(pot); msg = 'You take ' + pot.length + ' cards.'; }
      else if (rankOf(cc_) > rankOf(pc_)) { c = c.concat(pot); msg = 'Computer takes ' + pot.length + ' cards.'; }
      else msg = 'Tie held.';
      if (!p.length || !c.length || rounds > 600) {
        over = p.length > c.length ? 'You win the war!' : p.length < c.length ? 'Computer wins.' : 'A draw.';
        host.saveScore(p.length);
      }
    }
    reset();
    return {
      key: function (k) {
        if (over) { reset(); return; }
        if (k === 'enter' || k === 'space') round();
      },
      draw: function (t) {
        t.header('WAR', 'Enter plays the next round · Q quits');
        t.text(30, 5, 'You', C.cyan, null, true);
        if (pc_ !== null) drawCard(t, 30, 6, pc_, true);
        t.text(44, 5, 'CPU', C.red, null, true);
        if (cc_ !== null) drawCard(t, 44, 6, cc_, true);
        t.text(30, 11, msg + '                        ', C.white);
        t.text(30, 13, 'Your cards: ' + p.length + '   CPU cards: ' + c.length +
                       '   Round ' + rounds + '   ', C.fg);
        if (over) t.center(16, over + ' Press any key.', C.white, true);
      }
    };
  }
});

/* ---------------------------------------------------------------- go fish */
reg('gofish', {
  title: 'Go Fish', help: 'Left/Right pick a rank · Enter asks · Q quits',
  start: function (host, p) {
    var deck, top, phand, chand, pbooks, cbooks, knows, cur, msg, over;
    function countRank(h, r) { return h.filter(function (c) { return c % 13 === r; }).length; }
    function pull(h, r) {
      var out = [];
      for (var i = h.length - 1; i >= 0; i--)
        if (h[i] % 13 === r) out.push(h.splice(i, 1)[0]);
      return out;
    }
    function books(h) {
      var made = 0;
      for (var r = 0; r < 13; r++) if (countRank(h, r) === 4) { pull(h, r); made++; }
      return made;
    }
    function reset() {
      deck = newDeck(); top = 0;
      phand = []; chand = [];
      for (var i = 0; i < 7; i++) { phand.push(deck[top++]); chand.push(deck[top++]); }
      pbooks = books(phand); cbooks = books(chand);
      knows = new Array(13).fill(0);
      cur = 0; msg = 'Pick a rank and ask.'; over = null;
    }
    function ranks() {
      var out = [];
      for (var r = 0; r < 13; r++) if (countRank(phand, r)) out.push(r);
      return out;
    }
    reset();
    return {
      key: function (k) {
        if (over) { reset(); return; }
        var rs = ranks();
        if (!rs.length) { if (top < 52) phand.push(deck[top++]); return; }
        cur = Math.min(cur, rs.length - 1);
        if (k === 'left')  cur = (cur + rs.length - 1) % rs.length;
        if (k === 'right') cur = (cur + 1) % rs.length;
        if (k !== 'enter' && k !== 'space') return;

        var ask = rs[cur];
        knows[ask] = 1;
        var got = pull(chand, ask);
        if (got.length) {
          phand = phand.concat(got);
          msg = 'You got ' + got.length + ' ' + RANK[ask] + (got.length > 1 ? 's.' : '.');
        } else {
          if (top < 52) phand.push(deck[top++]);
          msg = 'Go fish! You drew a card.';
        }
        pbooks += books(phand);

        if (chand.length) {
          var want = -1, r;
          for (r = 0; r < 13; r++) if (knows[r] && countRank(chand, r)) { want = r; break; }
          if (want < 0) for (r = 0; r < 13; r++) if (countRank(chand, r)) { want = r; break; }
          if (want >= 0) {
            var taken = pull(phand, want);
            if (taken.length) {
              chand = chand.concat(taken);
              msg += '  CPU asked for ' + RANK[want] + ' and took ' + taken.length + '.';
            } else {
              if (top < 52) chand.push(deck[top++]);
              msg += '  CPU asked for ' + RANK[want] + ' — go fish.';
            }
            cbooks += books(chand);
          }
        }
        if (pbooks + cbooks >= 13 || (!phand.length && !chand.length && top >= 52)) {
          over = pbooks > cbooks ? 'You win!' : pbooks < cbooks ? 'Computer wins.' : 'Draw.';
          host.saveScore(pbooks * 100);
        }
      },
      draw: function (t) {
        t.header('GO FISH', 'Left/Right pick a rank · Enter asks · Q quits');
        var rs = ranks();
        t.text(16, 4, 'Your hand (' + phand.length + ' cards)', C.cyan, null, true);
        rs.forEach(function (r, i) {
          t.text(16 + (i % 9) * 7, 6 + ((i / 9) | 0) * 2,
                 RANK[r] + ' x' + countRank(phand, r) + '  ',
                 C.white, i === cur ? '#2b4a6b' : null, i === cur);
        });
        t.text(16, 12, 'Your books: ' + pbooks + '   CPU books: ' + cbooks +
                       '   Deck: ' + (52 - top) + '   ', C.fg);
        t.text(16, 13, 'CPU holds ' + chand.length + ' cards   ', C.dim);
        t.text(16, 15, (msg || '') + '                              ', C.white);
        if (over) t.center(18, over + ' Press any key.', C.white, true);
      }
    };
  }
});

/* ---------------------------------------------------------------- yahtzee */
reg('yahtzee', {
  title: 'Yahtzee', help: 'Space holds · R rerolls · Tab switches · Enter scores',
  start: function (host, p) {
    var CAT = ['Ones','Twos','Threes','Fours','Fives','Sixes','Three of a Kind',
               'Four of a Kind','Full House','Small Straight','Large Straight','Yahtzee','Chance'];
    var dice, keep, used, sc, rolls, cur, selecting, turn, finalScore;
    function roll() { for (var i = 0; i < 5; i++) if (!keep[i]) dice[i] = G.rndRange(1, 6); }
    function scoreFor(cat) {
      var counts = new Array(7).fill(0), f;
      dice.forEach(function (d) { counts[d]++; });
      var sum = dice.reduce(function (a, b) { return a + b; }, 0);
      var three = false, four = false, pair = false, triple = false;
      for (f = 1; f <= 6; f++) {
        if (counts[f] >= 3) three = true;
        if (counts[f] >= 4) four = true;
        if (counts[f] === 2) pair = true;
        if (counts[f] === 3) triple = true;
      }
      if (cat < 6) return counts[cat + 1] * (cat + 1);
      if (cat === 6) return three ? sum : 0;
      if (cat === 7) return four ? sum : 0;
      if (cat === 8) return (pair && triple) ? 25 : 0;
      if (cat === 9) {
        for (f = 1; f <= 3; f++) if (counts[f] && counts[f+1] && counts[f+2] && counts[f+3]) return 30;
        return 0;
      }
      if (cat === 10) {
        for (f = 1; f <= 2; f++)
          if (counts[f] && counts[f+1] && counts[f+2] && counts[f+3] && counts[f+4]) return 40;
        return 0;
      }
      if (cat === 11) { for (f = 1; f <= 6; f++) if (counts[f] === 5) return 50; return 0; }
      return sum;
    }
    function total() {
      var t = 0, upper = 0;
      sc.forEach(function (v, i) { if (used[i]) { t += v; if (i < 6) upper += v; } });
      return t + (upper >= 63 ? 35 : 0);
    }
    function nextTurn() {
      keep = [false,false,false,false,false];
      roll();
      rolls = 2;
      selecting = false;
      cur = 0;
    }
    function reset() {
      dice = [1,1,1,1,1]; used = new Array(13).fill(false); sc = new Array(13).fill(0);
      turn = 0; finalScore = null;
      nextTurn();
    }
    reset();
    return {
      key: function (k) {
        if (finalScore !== null) { reset(); return; }
        if (k === 'tab') { selecting = !selecting; cur = 0; return; }
        if (!selecting) {
          if (k === 'left')  cur = (cur + 4) % 5;
          if (k === 'right') cur = (cur + 1) % 5;
          if (k === 'space') keep[cur] = !keep[cur];
          if (k === 'r' && rolls > 0) { roll(); rolls--; }
          if (k === 'enter') { selecting = true; cur = 0; }
          return;
        }
        if (k === 'up')   cur = (cur + 12) % 13;
        if (k === 'down') cur = (cur + 1) % 13;
        if (k === 'left' || k === 'right') { selecting = false; cur = 0; return; }
        if (k !== 'enter' && k !== 'space') return;
        if (used[cur]) return;
        sc[cur] = scoreFor(cur);
        used[cur] = true;
        turn++;
        if (turn >= 13) { finalScore = total(); host.saveScore(finalScore); }
        else nextTurn();
      },
      draw: function (t) {
        t.header('YAHTZEE', 'Rolls left: ' + rolls + ' · Space holds · R rerolls · Tab switches · Enter scores');
        dice.forEach(function (d, i) {
          t.text(16 + i * 8, 4, '┌───┐', keep[i] ? C.green : C.white);
          t.text(16 + i * 8, 5, '│ ' + d + ' │', keep[i] ? C.green : C.white, null, true);
          t.text(16 + i * 8, 6, '└───┘', keep[i] ? C.green : C.white);
          t.text(16 + i * 8, 7, (!selecting && i === cur) ? ' ^^^ ' : '     ', C.cyan);
          t.text(16 + i * 8, 8, keep[i] ? 'HELD ' : '     ', C.dim);
        });
        CAT.forEach(function (n, i) {
          var shown = used[i] ? sc[i] : scoreFor(i);
          t.text(16, 10 + i, (n + '                 ').slice(0, 18) + String(shown).padStart(3, ' '),
                 used[i] ? C.dim : C.white, (selecting && i === cur) ? '#2b4a6b' : null, !used[i]);
        });
        t.text(16, 24, 'Total: ' + total() + '   ', C.yellow, null, true);
        if (finalScore !== null) t.center(26, 'Final score ' + finalScore + ' — press any key.', C.green, true);
      }
    };
  }
});

/* -------------------------------------------------------------------- pig */
reg('pig', {
  title: 'Pig', help: 'R rolls · H holds · first to 100 · Q quits',
  start: function (host, p) {
    var you, cpu, turnTotal, die, msg, over;
    function reset() { you = cpu = 0; turnTotal = 0; die = 0; msg = 'Your turn.'; over = null; }
    function cpuTurn() {
      var t = 0;
      for (;;) {
        if (t >= 20 || cpu + t >= 100) break;
        var d = G.rndRange(1, 6);
        if (d === 1) { t = 0; msg = 'Computer rolled a 1 and loses its turn.'; break; }
        t += d;
        msg = 'Computer rolled ' + d + ' (turn total ' + t + ').';
      }
      cpu += t;
      if (cpu >= 100) { over = 'Computer reaches 100.'; host.saveScore(you); }
    }
    reset();
    return {
      key: function (k) {
        if (over) { reset(); return; }
        if (k === 'r' || k === 'space') {
          die = G.rndRange(1, 6);
          if (die === 1) {
            turnTotal = 0;
            msg = 'You rolled a 1 — turn lost.';
            cpuTurn();
          } else {
            turnTotal += die;
            msg = 'You rolled ' + die + '.';
            if (you + turnTotal >= 100) {
              you += turnTotal; turnTotal = 0;
              over = 'You reach 100 — you win!';
              host.saveScore(you);
            }
          }
          return;
        }
        if (k === 'h') {
          you += turnTotal;
          turnTotal = 0;
          msg = 'You hold.';
          if (you >= 100) { over = 'You reach 100 — you win!'; host.saveScore(you); return; }
          cpuTurn();
        }
      },
      draw: function (t) {
        t.header('PIG', 'R rolls · H holds · first to 100 · Q quits');
        t.text(28, 5, 'You  ' + String(you).padStart(3, ' '), C.cyan, null, true);
        t.text(28, 6, 'CPU  ' + String(cpu).padStart(3, ' '), C.red, null, true);
        t.text(28, 9, 'Turn total: ' + turnTotal + '   ', C.yellow);
        if (die) {
          t.text(28, 11, '┌───┐', C.white);
          t.text(28, 12, '│ ' + die + ' │', C.white, null, true);
          t.text(28, 13, '└───┘', C.white);
        }
        t.text(28, 15, (msg || '') + '                                   ', C.fg);
        if (over) t.center(18, over + ' Press any key.', C.white, true);
      }
    };
  }
});

/* ----------------------------------------------------------------- slots */
reg('slots', {
  title: 'Slot Machine', help: 'Enter spins · Up/Down changes bet · Q quits',
  start: function (host, p) {
    /* Weights and payouts are constant across themes, so the published
     * return-to-player is identical whichever set you play. */
    var TNAME = ['Fruit','Egypt','Space','Pirate','Jungle','Diamond',
                 'Western','Aztec','Neon','Deep Sea','Classic'];
    var TSYM = [
      ['Ch','Lm','Bl','St','Dm','77'], ['An','Sc','Ey','Ra','Ok','Ph'],
      ['Cm','Mn','St','Ro','Al','Bh'], ['Sk','Mp','Rm','Cn','Pr','Cx'],
      ['Ln','Mk','Sn','Pr','Tg','Id'], ['Cl','Sp','Em','Rb','Dm','Cr'],
      ['Ht','Bt','Hs','Cl','Sh','Go'], ['Sn','Jg','Ma','Te','Gd','Cd'],
      ['Ar','Cr','Sq','Tr','Cb','Nx'], ['Fs','Cr','Sh','Oc','Pl','Tr'],
      ['Ch','Lm','Bl','St','Dm','77']];
    var ti = TNAME.indexOf(p.theme || '');
    if (ti < 0) ti = (p.variant >= 0 && p.variant < TNAME.length) ? p.variant : 0;
    var SYM = TSYM[ti];
    var WEIGHT = [30,25,20,13,8,4], PAY3 = [10,20,40,80,250,1000], PAY2 = [1,2,3,5,12,40];
    var reels = [0,0,0], credits = 100, bet = 5, msg = 'Press Enter to spin.';
    function spin1() {
      var total = WEIGHT.reduce(function (a, b) { return a + b; }, 0), r = rnd(total);
      for (var i = 0; i < WEIGHT.length; i++) { if (r < WEIGHT[i]) return i; r -= WEIGHT[i]; }
      return WEIGHT.length - 1;
    }
    return {
      key: function (k) {
        if (credits <= 0) { credits = 100; bet = 5; msg = 'New stake of 100 credits.'; return; }
        if (k === 'up'   && bet + 5 <= credits && bet < 50) bet += 5;
        if (k === 'down' && bet > 5) bet -= 5;
        if (k !== 'enter' && k !== 'space') return;
        credits -= bet;
        reels = [spin1(), spin1(), spin1()];
        var a = reels[0], b = reels[1], c = reels[2], win = 0;
        if (a === b && b === c) win = PAY3[a] * bet / 5;
        else if (a === b) win = PAY2[a] * bet / 5;
        else if (b === c) win = PAY2[b] * bet / 5;
        else if (a === c) win = PAY2[a] * bet / 5;
        win = Math.floor(win);
        credits += win;
        msg = win ? 'You win ' + win + ' credits!' : 'No win.';
        host.saveScore(credits);
      },
      draw: function (t) {
        t.header('SLOT MACHINE', TNAME[ti] + ' reels · Enter spins · Up/Down changes bet');
        t.box(28, 5, 22, 5, C.yellow);
        t.text(33, 7, ' ' + SYM[reels[0]] + '   ' + SYM[reels[1]] + '   ' + SYM[reels[2]] + ' ', C.white, null, true);
        t.text(28, 11, 'Credits: ' + credits + '   Bet: ' + bet + '   ', C.fg);
        t.text(28, 13, msg + '                          ', C.white, null, true);
        t.text(54, 4, 'Paytable (per 5 bet)', C.dim);
        SYM.forEach(function (s, i) {
          t.text(54, 5 + i, s + '  x3 ' + String(PAY3[i]).padStart(4, ' ') +
                            '   x2 ' + String(PAY2[i]).padStart(3, ' '), C.dim);
        });
      }
    };
  }
});

/* ----------------------------------------------------------- higher/lower */
reg('higherlower', {
  title: 'Higher or Lower', help: 'H higher · L lower · Q quits',
  start: function (host, p) {
    var deck, top, streak, best, chips, cur, nxt, msg, over;
    function rankOf(card) { var r = card % 13; return r === 0 ? 14 : r + 1; }
    function reset() {
      deck = newDeck(); top = 0; streak = 0; best = 0; chips = 50;
      cur = deck[top]; nxt = null; msg = 'Higher or lower?'; over = null;
    }
    reset();
    return {
      key: function (k) {
        if (over) { reset(); return; }
        if (nxt !== null) { cur = nxt; nxt = null; msg = 'Higher or lower?'; return; }
        var guessHigh;
        if (k === 'h' || k === 'up') guessHigh = true;
        else if (k === 'l' || k === 'down') guessHigh = false;
        else return;
        top++;
        if (top >= 52) { over = 'Deck exhausted.'; host.saveScore(best); return; }
        nxt = deck[top];
        if (rankOf(nxt) === rankOf(cur)) msg = 'Equal rank — push.';
        else if ((rankOf(nxt) > rankOf(cur)) === guessHigh) {
          streak++;
          chips += 5 + streak;
          best = Math.max(best, streak);
          msg = 'Correct! Streak ' + streak + ', +' + (5 + streak) + ' chips.';
        } else {
          chips -= 10;
          streak = 0;
          msg = 'Wrong — streak lost, -10 chips.';
          if (chips <= 0) { over = 'Out of chips.'; host.saveScore(best); }
        }
      },
      draw: function (t) {
        t.header('HIGHER OR LOWER', 'H higher · L lower · Q quits');
        drawCard(t, 34, 6, cur, true);
        if (nxt !== null) drawCard(t, 44, 6, nxt, true);
        t.text(28, 11, 'Streak: ' + streak + '  Best: ' + best + '  Chips: ' + chips + '   ', C.fg);
        t.text(28, 13, msg + '                                  ', C.white, null, true);
        if (nxt !== null && !over) t.text(28, 15, 'Press any key to continue.', C.dim);
        if (over) t.center(17, over + ' Press any key.', C.white, true);
      }
    };
  }
});

}());
