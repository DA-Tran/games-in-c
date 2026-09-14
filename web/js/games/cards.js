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
  title: 'Blackjack', help: 'H hit · S stand · D double · Q quits',
  start: function (host, p) {
    /* decks, holeUp, hitSoft17, bjNum, bjDen, tiesDealer, noTens, fiveCard,
     * surrender, name — these move the edge, not the wallpaper. */
    var RULES = [
      [2,0,0,3,2,0,0,0,0,'Blackjack'],
      [1,0,1,3,2,0,0,0,0,'Single Deck'],
      [6,0,0,3,2,0,0,0,0,'Six Deck'],
      [8,1,1,1,1,1,0,0,0,'Double Exposure'],
      [6,0,1,3,2,0,1,1,0,'Spanish 21'],
      [4,0,0,3,2,0,0,0,0,'Vegas Strip'],
      [8,0,0,3,2,0,0,0,1,'Atlantic City'],
      [8,1,0,1,1,1,0,0,0,'Face Up 21'],
      [1,0,1,2,1,1,0,1,0,'Pontoon']];
    var R = RULES[(p.variant >= 0 && p.variant <= 8) ? p.variant : 0];
    var DECKS=R[0], HOLEUP=R[1], HITS17=R[2], BJN=R[3], BJD=R[4],
        TIES=R[5], NOTENS=R[6], FIVE=R[7], SURR=R[8], RNAME=R[9];

    var deck, top, ph, dh, chips, bet, phase, msg;

    function shoe() {
      deck = [];
      for (var d = 0; d < DECKS; d++) for (var s = 0; s < 4; s++) for (var r = 0; r < 13; r++) {
        if (NOTENS && r === 9) continue;       /* Spanish 21 strips the tens */
        deck.push(s * 13 + r);
      }
      deck = shuffle(deck);
      top = 0;
    }
    function deal() { if (top >= deck.length) shoe(); return deck[top++]; }
    function value(h) {
      var total = 0, aces = 0;
      for (var i = 0; i < h.length; i++) {
        var r = h[i] % 13;
        if (r === 0) { total += 11; aces++; }
        else if (r >= 9) total += 10;
        else total += r + 1;
      }
      while (total > 21 && aces > 0) { total -= 10; aces--; }
      return total;
    }
    function soft(h) {
      var total = 0, aces = 0;
      for (var i = 0; i < h.length; i++) {
        var r = h[i] % 13;
        if (r === 0) { total += 11; aces++; }
        else if (r >= 9) total += 10;
        else total += r + 1;
      }
      while (total > 21 && aces > 0) { total -= 10; aces--; }
      return aces > 0;
    }
    function natural(h) { return h.length === 2 && value(h) === 21; }

    function newHand() {
      ph = [deal(), deal()];
      dh = [deal(), deal()];
      phase = 'play'; msg = null;
      if (natural(ph)) {
        if (HOLEUP && natural(dh)) { msg = 'Both naturals — dealer takes the tie.'; chips -= bet; }
        else { msg = 'Natural! Pays ' + BJN + ':' + BJD + '.'; chips += Math.floor(bet * BJN / BJD); }
        phase = 'over';
        host.saveScore(chips);
      }
    }
    function dealerPlay() {
      for (var guard = 0; guard < 20; guard++) {
        var dv = value(dh);
        if (dv < 17) { dh.push(deal()); continue; }
        if (dv === 17 && soft(dh) && HITS17) { dh.push(deal()); continue; }
        break;
      }
      var pv = value(ph), dv2 = value(dh);
      if (dv2 > 21) { msg = 'Dealer busts — you win!'; chips += bet; }
      else if (dv2 > pv) { msg = 'Dealer wins.'; chips -= bet; }
      else if (dv2 < pv) { msg = 'You win!'; chips += bet; }
      else if (TIES) { msg = 'Tie — dealer takes it.'; chips -= bet; }
      else msg = 'Push.';
      phase = 'over';
      host.saveScore(chips);
    }
    function reset() { chips = 100; bet = 10; shoe(); phase = 'bet'; msg = null; ph = []; dh = []; }
    reset();
    return {
      key: function (k) {
        if (chips <= 0 && phase === 'over') { reset(); return; }
        if (phase === 'bet') {
          if (k === 'up' && bet + 10 <= chips) bet += 10;
          if (k === 'down' && bet > 10) bet -= 10;
          if (k === 'enter' || k === 'space') newHand();
          return;
        }
        if (phase === 'over') { if (chips > 0) { if (bet > chips) bet = chips; phase = 'bet'; } return; }
        if (k === 'h') {
          ph.push(deal());
          if (value(ph) > 21) { msg = 'Bust — you lose.'; chips -= bet; phase = 'over'; host.saveScore(chips); }
          else if (FIVE && ph.length >= 5) { msg = 'Five-card trick — you win!'; chips += bet; phase = 'over'; host.saveScore(chips); }
          return;
        }
        if (k === 's') { dealerPlay(); return; }
        if (k === 'r' && SURR && ph.length === 2) {
          msg = 'Surrendered — half the stake back.';
          chips -= Math.floor(bet / 2);
          phase = 'over'; host.saveScore(chips);
          return;
        }
        if (k === 'd' && ph.length === 2 && chips >= bet * 2) {
          bet *= 2; ph.push(deal());
          if (value(ph) > 21) { msg = 'Bust — you lose.'; chips -= bet; phase = 'over'; host.saveScore(chips); }
          else dealerPlay();
        }
      },
      draw: function (t) {
        t.header('TWENTY-ONE', RNAME + ' — ' + DECKS + ' deck' + (DECKS === 1 ? '' : 's') +
                 ', dealer ' + (HITS17 ? 'hits' : 'stands') + ' soft 17, natural pays ' +
                 BJN + ':' + BJD + (NOTENS ? ', no tens' : '') + (FIVE ? ', five-card trick' : ''));
        if (phase === 'bet') {
          t.text(28, 8, 'Chips: ' + chips + '     ', C.fg);
          t.text(28, 10, 'Bet:   ' + bet + '     ', C.yellow, null, true);
          t.text(28, 12, 'Up/Down change bet, Enter deals', C.dim);
          return;
        }
        var i;
        t.text(14, 4, 'Dealer', C.red, null, true);
        for (i = 0; i < dh.length; i++) drawCard(t, 14 + i * 7, 5, dh[i], HOLEUP || phase === 'over' || i !== 1);
        t.text(14, 8, 'Total: ' + ((HOLEUP || phase === 'over') ? value(dh) : '?') + '    ', C.fg);
        t.text(14, 11, 'You', C.cyan, null, true);
        for (i = 0; i < ph.length; i++) drawCard(t, 14 + i * 7, 12, ph[i], true);
        t.text(14, 15, 'Total: ' + value(ph) + (soft(ph) ? ' (soft)' : '      '), C.fg);
        t.text(14, 17, 'Chips: ' + chips + '   Bet: ' + bet + '    ', C.fg);
        t.text(14, 19, 'H hit, S stand, D double' + (SURR ? ', R surrender' : ''), C.dim);
        if (msg) t.text(14, 21, msg + '   Press a key.', C.white, null, true);
      }
    };
  }
});

/* ------------------------------------------------------------ video poker */
reg('videopoker', {
  title: 'Video Poker', help: 'Left/Right select · Space holds · Enter draws · Q quits',
  start: function (host, p) {
    var CATN = ['Royal Flush','Four Deuces','Wild Royal','Five of a Kind','Straight Flush',
                'Four Aces + kicker','Four Aces','Four 2s-4s','Four J-K','Four of a Kind',
                'Full House','Flush','Straight','Three of a Kind','Two Pair','Pair'];
    var TABLES = [
      ['Jacks or Better',0,10,[800,0,0,0,50,0,0,0,0,25,9,6,4,3,2,1]],
      ['Bonus Poker',0,10,[800,0,0,0,50,0,80,40,0,25,8,5,4,3,2,1]],
      ['Double Bonus Poker',0,10,[800,0,0,0,50,0,160,80,0,50,9,7,5,3,1,1]],
      ['Double Double Bonus',0,10,[800,0,0,0,50,400,160,80,0,50,9,6,4,3,1,1]],
      ['Deuces Wild',1,0,[800,200,25,15,9,0,0,0,0,5,3,2,2,1,0,0]],
      ['Joker Poker',2,11,[800,0,100,200,50,0,0,0,0,20,7,5,3,2,1,1]],
      ['Aces and Faces',0,10,[800,0,0,0,50,0,80,0,40,25,8,5,4,3,2,1]],
      ['Tens or Better',0,9,[800,0,0,0,50,0,0,0,0,25,6,5,4,3,2,1]],
      ['All American',0,10,[800,0,0,0,200,0,0,0,0,40,8,8,8,3,1,1]]];
    var T = TABLES[(p.level >= 0 && p.level <= 8) ? p.level : 0];
    var TNAME = T[0], WILD = T[1], QUAL = T[2], PAY = T[3];
    var deck, top, hand, hold, credits, cur, phase, msg;

    function isWild(c) { return WILD === 1 ? (c % 13 === 1) : WILD === 2 ? (c >= 52) : false; }
    function shuffleDeck() {
      var d = [];
      for (var i = 0; i < 52; i++) d.push(i);
      if (WILD === 2) d.push(52);              /* joker poker deals 53 cards */
      deck = shuffle(d); top = 0;
    }
    /* Best category the hand reaches, wilds filling whatever helps most. */
    function evaluate() {
      var counts = new Array(13).fill(0), suits = [0,0,0,0], wilds = 0, ranks = [], i;
      for (i = 0; i < 5; i++) {
        if (isWild(hand[i])) { wilds++; continue; }
        counts[hand[i] % 13]++; suits[hand[i] / 13 | 0]++; ranks.push(hand[i] % 13);
      }
      var maxc = 0, distinct = 0, pairs = 0, three = false;
      for (i = 0; i < 13; i++) {
        if (counts[i] > maxc) maxc = counts[i];
        if (counts[i]) distinct++;
        if (counts[i] === 2) pairs++;
        if (counts[i] >= 3) three = true;
      }
      var flush = false, fsuit = -1;
      for (i = 0; i < 4; i++) if (suits[i] + wilds >= 5) { flush = true; fsuit = i; }
      var straight = 0;
      if (distinct + wilds >= 5 && distinct === ranks.length) {
        for (var lo = 0; lo + 4 <= 12 && !straight; lo++) {
          var need = 0;
          for (var k = 0; k < 5; k++) if (!counts[lo + k]) need++;
          if (need <= wilds) straight = 1;
        }
        var needA = 0;
        [0,9,10,11,12].forEach(function (r) { if (!counts[r]) needA++; });
        if (needA <= wilds) straight = 2;
      }
      var royal = false;
      if (straight === 2 && flush) {
        royal = true;
        for (i = 0; i < 5; i++) if (!isWild(hand[i]) && (hand[i] / 13 | 0) !== fsuit) royal = false;
      }
      if (royal && wilds === 0) return 0;
      if (WILD === 1 && wilds === 4) return 1;
      if (royal) return 2;
      if (maxc + wilds >= 5) return 3;
      if (straight && flush) return 4;
      if (maxc + wilds >= 4) {
        var quad = -1;
        for (i = 0; i < 13; i++) if (counts[i] + wilds >= 4) { quad = i; break; }
        if (quad === 0) {
          if (PAY[5]) for (i = 0; i < ranks.length; i++) if (ranks[i] >= 1 && ranks[i] <= 3) return 5;
          if (PAY[6]) return 6;
        }
        if (quad >= 1 && quad <= 3 && PAY[7]) return 7;
        if (quad >= 10 && quad <= 12 && PAY[8]) return 8;
        return 9;
      }
      if ((three && pairs >= 1) || (pairs === 2 && wilds >= 1)) return 10;
      if (flush) return 11;
      if (straight) return 12;
      if (maxc + wilds >= 3) return 13;
      if (pairs === 2) return 14;
      if (pairs === 1) {
        for (var r2 = 0; r2 < 13; r2++)
          if (counts[r2] === 2 && (r2 === 0 || r2 >= QUAL)) return 15;
      }
      return -1;
    }
    function newHand() {
      if (top > deck.length - 12) shuffleDeck();
      hand = []; hold = [false,false,false,false,false];
      for (var i = 0; i < 5; i++) hand.push(deck[top++]);
      credits -= 5; cur = 0; phase = 'hold'; msg = null;
    }
    function reset() { credits = 100; shuffleDeck(); newHand(); }
    reset();
    return {
      key: function (k) {
        if (phase === 'done') { if (credits > 0) newHand(); else reset(); return; }
        if (k === 'left') cur = (cur + 4) % 5;
        if (k === 'right') cur = (cur + 1) % 5;
        if (k === 'space') hold[cur] = !hold[cur];
        if (k !== 'enter') return;
        for (var i = 0; i < 5; i++) if (!hold[i]) { if (top >= deck.length) shuffleDeck(); hand[i] = deck[top++]; }
        var res = evaluate();
        if (res >= 0 && PAY[res]) { credits += PAY[res] * 5; msg = CATN[res] + ' — pays ' + PAY[res] * 5 + '!'; }
        else msg = 'No win.';
        phase = 'done';
        host.saveScore(credits);
      },
      draw: function (t) {
        t.header('VIDEO POKER', TNAME + (WILD === 1 ? ' (deuces are wild)' : WILD === 2 ? ' (joker is wild)' : ''));
        var row = 3, i;
        for (i = 0; i < 16; i++) {
          if (!PAY[i]) continue;
          t.text(50, row++, (CATN[i] + '                    ').slice(0, 20) + String(PAY[i]).padStart(4, ' '), C.dim);
        }
        for (i = 0; i < 5; i++) {
          drawCard(t, 10 + i * 7, 7, hand[i], true);
          t.text(10 + i * 7, 10, hold[i] ? ' HOLD ' : '      ', hold[i] ? C.yellow : C.grey, null, hold[i]);
          t.text(10 + i * 7, 11, i === cur ? '  ^^  ' : '      ', C.white, i === cur ? '#2b4a6b' : null);
        }
        t.text(10, 14, 'Credits: ' + credits + '   ' + (phase === 'done' ? 'Draw phase ' : 'Hold phase '), C.fg);
        if (msg) t.text(10, 16, msg + '   Press a key.', C.white, null, true);
      }
    };
  }
});

/* -------------------------------------------------------------------- war */
reg('war', {
  title: 'War', help: 'Enter plays · Q quits',
  start: function (host, p) {
    var VAR = (p.variant >= 0 && p.variant <= 2) ? p.variant : 0;
    function rankOf(c) { var r = c % 13; return r === 0 ? 13 : r; }

    /* ---- classic attrition war ---- */
    if (VAR === 0) {
      var pq, cq, a, b, pot, msg, note;
      function reset() {
        var d = newDeck();
        pq = d.slice(0, 26); cq = d.slice(26);
        a = b = null; pot = 0; msg = null; note = '';
      }
      reset();
      return {
        key: function (k) {
          if (msg) { reset(); return; }
          if (k !== 'enter' && k !== 'space') return;
          if (!pq.length || !cq.length) return;
          var stack = [];
          a = pq.shift(); b = cq.shift();
          stack.push(a, b);
          var guard = 0;
          while (rankOf(a) === rankOf(b) && pq.length > 1 && cq.length > 1 && guard++ < 6) {
            for (var i = 0; i < 3 && pq.length > 1 && cq.length > 1; i++) { stack.push(pq.shift()); stack.push(cq.shift()); }
            a = pq.shift(); b = cq.shift();
            stack.push(a, b);
            note = 'WAR! Three down, one up.';
          }
          pot = stack.length;
          if (rankOf(a) > rankOf(b)) { pq = pq.concat(shuffle(stack)); note = 'You take ' + pot + ' cards.'; }
          else { cq = cq.concat(shuffle(stack)); note = 'Computer takes ' + pot + ' cards.'; }
          if (!pq.length || !cq.length) {
            msg = pq.length > cq.length ? 'You hold the deck — you win!' : 'The computer takes the deck.';
            host.saveScore(pq.length);
          }
        },
        draw: function (t) {
          t.header('WAR', 'Enter plays a card · Q quits');
          t.text(20, 5, 'You: ' + pq.length + ' cards     ', C.cyan, null, true);
          t.text(42, 5, 'Computer: ' + cq.length + ' cards   ', C.red, null, true);
          if (a !== null) { drawCard(t, 20, 8, a, true); drawCard(t, 42, 8, b, true); }
          t.text(20, 13, note + '                                ', C.white);
          if (msg) t.center(16, msg + ' Press any key.', C.white, true);
        }
      };
    }

    /* ---- casino war: one hand, and the tie is the whole game ---- */
    if (VAR === 1) {
      var deck1, top1, chips1 = 100, bet1 = 10, ca, cb, phase1 = 'bet', note1 = '';
      function fresh() { deck1 = newDeck(); top1 = 0; }
      fresh();
      return {
        key: function (k) {
          if (phase1 === 'bet') {
            if (k === 'up' && bet1 + 10 <= chips1) bet1 += 10;
            if (k === 'down' && bet1 > 10) bet1 -= 10;
            if (k === 'enter' || k === 'space') {
              if (top1 > 46) fresh();
              ca = deck1[top1++]; cb = deck1[top1++];
              if (rankOf(ca) > rankOf(cb)) { chips1 += bet1; note1 = 'Your card is higher — you win ' + bet1 + '.'; phase1 = 'done'; }
              else if (rankOf(ca) < rankOf(cb)) { chips1 -= bet1; note1 = 'Dealer takes it.'; phase1 = 'done'; }
              else { note1 = 'TIE. W goes to war (double), S surrenders half.'; phase1 = 'tie'; }
              host.saveScore(chips1);
            }
            return;
          }
          if (phase1 === 'tie') {
            if (k === 's') { chips1 -= Math.floor(bet1 / 2); note1 = 'Surrendered half.'; phase1 = 'done'; }
            else if (k === 'w') {
              if (chips1 < bet1 * 2) { chips1 -= Math.floor(bet1 / 2); note1 = 'Not enough to go to war.'; }
              else {
                if (top1 > 48) fresh();
                ca = deck1[top1++]; cb = deck1[top1++];
                if (rankOf(ca) >= rankOf(cb)) { chips1 += bet1; note1 = 'You win the war — ' + bet1 + '.'; }
                else { chips1 -= bet1 * 2; note1 = 'You lose the war — ' + bet1 * 2 + '.'; }
              }
              phase1 = 'done';
            }
            host.saveScore(chips1);
            return;
          }
          if (chips1 <= 0) { chips1 = 100; bet1 = 10; }
          if (bet1 > chips1) bet1 = chips1;
          phase1 = 'bet';
        },
        draw: function (t) {
          t.header('CASINO WAR', 'Up/Down change bet · Enter deals · ties double or surrender');
          t.text(26, 6, 'Chips: ' + chips1 + '     ', C.fg);
          t.text(26, 8, 'Bet:   ' + bet1 + '     ', C.yellow, null, true);
          if (ca !== undefined) { drawCard(t, 24, 11, ca, true); drawCard(t, 40, 11, cb, true); }
          t.text(24, 15, note1 + '                                     ', C.white, null, true);
        }
      };
    }

    /* ---- red dog: bet on the spread between two cards ---- */
    var deck2, top2, chips2 = 100, bet2 = 10, ra, rb, rc, phase2 = 'bet', note2 = '', gap = 0;
    var SPREAD_PAY = [5, 4, 2, 1];
    function fresh2() { deck2 = newDeck(); top2 = 0; }
    fresh2();
    return {
      key: function (k) {
        if (phase2 === 'bet') {
          if (k === 'up' && bet2 + 10 <= chips2) bet2 += 10;
          if (k === 'down' && bet2 > 10) bet2 -= 10;
          if (k === 'enter' || k === 'space') {
            if (top2 > 46) fresh2();
            ra = deck2[top2++]; rb = deck2[top2++]; rc = undefined;
            var lo = Math.min(rankOf(ra), rankOf(rb)), hi = Math.max(rankOf(ra), rankOf(rb));
            gap = hi - lo - 1;
            if (hi === lo) {
              rc = deck2[top2++];
              if (rankOf(rc) === lo) { chips2 += bet2 * 11; note2 = 'Three of a kind — pays 11:1!'; }
              else note2 = 'Pair, no third match — push.';
              phase2 = 'done';
            } else if (gap <= 0) { note2 = 'Consecutive cards — push.'; phase2 = 'done'; }
            else { note2 = 'Spread of ' + gap + '. Enter draws.'; phase2 = 'draw'; }
            host.saveScore(chips2);
          }
          return;
        }
        if (phase2 === 'draw') {
          if (k !== 'enter' && k !== 'space') return;
          rc = deck2[top2++];
          var lo2 = Math.min(rankOf(ra), rankOf(rb)), hi2 = Math.max(rankOf(ra), rankOf(rb));
          var pay = SPREAD_PAY[gap > 4 ? 3 : gap - 1];
          if (rankOf(rc) > lo2 && rankOf(rc) < hi2) { chips2 += bet2 * pay; note2 = 'Inside! Pays ' + pay + ':1 — ' + bet2 * pay + '.'; }
          else { chips2 -= bet2; note2 = 'Outside the spread — you lose.'; }
          phase2 = 'done';
          host.saveScore(chips2);
          return;
        }
        if (chips2 <= 0) { chips2 = 100; bet2 = 10; }
        if (bet2 > chips2) bet2 = chips2;
        phase2 = 'bet';
      },
      draw: function (t) {
        t.header('RED DOG', 'Gap 1 pays 5:1, 2 pays 4:1, 3 pays 2:1, 4+ pays 1:1');
        t.text(24, 6, 'Chips: ' + chips2 + '     ', C.fg);
        t.text(24, 8, 'Bet:   ' + bet2 + '     ', C.yellow, null, true);
        if (ra !== undefined) {
          drawCard(t, 22, 11, ra, true);
          drawCard(t, 36, 11, rb, true);
          if (rc !== undefined) drawCard(t, 50, 11, rc, true);
        }
        t.text(22, 15, note2 + '                                    ', C.white, null, true);
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
  title: 'Yahtzee', help: 'Space holds · R rerolls · Tab switches · Enter scores · Q quits',
  start: function (host, p) {
    var CATS = ['Ones','Twos','Threes','Fours','Fives','Sixes','Three of a Kind',
                'Four of a Kind','Full House','Small Straight','Large Straight',
                'Yahtzee','Chance'];
    var V = (p.variant >= 0 && p.variant <= 5) ? p.variant : 0;
    var ND = V === 2 ? 6 : 5, NROLL = V === 4 ? 2 : 3, NCOL = V === 1 ? 3 : 1;
    var DUP = V === 3, TARGET = V === 5 ? 220 : 0;
    var dice, keep, used, sc, cpuUsed, cpuSc, col, turn, rolls, cur, selecting, msg;

    function countFace(f) { var n = 0; for (var i = 0; i < ND; i++) if (dice[i] === f) n++; return n; }
    /* Sum of the best five, so a sixth die does not inflate the set boxes. */
    function sumBest(n) {
      var s = dice.slice().sort(function (a, b) { return b - a; }), t = 0;
      for (var i = 0; i < n && i < s.length; i++) t += s[i];
      return t;
    }
    function scoreFor(cat) {
      var counts = [0,0,0,0,0,0,0], i, f, three = false, four = false, five = false, pair = false, triple = false;
      for (i = 0; i < ND; i++) counts[dice[i]]++;
      for (f = 1; f <= 6; f++) {
        if (counts[f] >= 3) three = true;
        if (counts[f] >= 4) four = true;
        if (counts[f] >= 5) five = true;
        if (counts[f] === 2) pair = true;
        if (counts[f] === 3) triple = true;
      }
      if (cat < 6) return countFace(cat + 1) * (cat + 1);
      if (cat === 6) return three ? sumBest(5) : 0;
      if (cat === 7) return four ? sumBest(5) : 0;
      if (cat === 8) return (pair && triple) ? 25 : 0;
      if (cat === 9) { for (f = 1; f <= 3; f++) if (counts[f] && counts[f+1] && counts[f+2] && counts[f+3]) return 30; return 0; }
      if (cat === 10) { for (f = 1; f <= 2; f++) if (counts[f] && counts[f+1] && counts[f+2] && counts[f+3] && counts[f+4]) return 40; return 0; }
      if (cat === 11) return five ? 50 : 0;
      return sumBest(5);
    }
    function roll() { for (var i = 0; i < ND; i++) if (!keep[i]) dice[i] = 1 + rnd(6); }
    function colTotal(c) {
      var upper = 0, total = 0, i;
      for (i = 0; i < 13; i++) { if (!used[c][i]) continue; total += sc[c][i]; if (i < 6) upper += sc[c][i]; }
      if (upper >= 63) total += 35;
      return total * (NCOL > 1 ? c + 1 : 1);
    }
    function grand() { var t = 0; for (var c = 0; c < NCOL; c++) t += colTotal(c); return t; }
    function cpuTotal() {
      var t = 0, u = 0;
      for (var i = 0; i < 13; i++) if (cpuUsed[i]) { t += cpuSc[i]; if (i < 6) u += cpuSc[i]; }
      return u >= 63 ? t + 35 : t;
    }
    /* The duplicate opponent plays the same dice, taking its best open box. */
    function cpuPlace() {
      var best = -1, bi = -1;
      for (var i = 0; i < 13; i++) {
        if (cpuUsed[i]) continue;
        var v = scoreFor(i) + (i >= 6 ? 2 : 0);
        if (v > best) { best = v; bi = i; }
      }
      if (bi >= 0) { cpuSc[bi] = scoreFor(bi); cpuUsed[bi] = 1; }
    }
    function newTurn() {
      keep = new Array(ND).fill(false);
      roll();
      rolls = NROLL - 1;
      cur = 0; selecting = false;
    }
    function reset() {
      dice = new Array(ND).fill(1);
      used = []; sc = [];
      for (var c = 0; c < NCOL; c++) { used.push(new Array(13).fill(0)); sc.push(new Array(13).fill(0)); }
      cpuUsed = new Array(13).fill(0); cpuSc = new Array(13).fill(0);
      col = 0; turn = 0; msg = null;
      newTurn();
    }
    reset();
    return {
      key: function (k) {
        if (msg) { reset(); return; }
        if (k === 'tab') { selecting = !selecting; cur = 0; return; }
        if (!selecting) {
          if (k === 'left') cur = (cur + ND - 1) % ND;
          if (k === 'right') cur = (cur + 1) % ND;
          if (k === 'space') keep[cur] = !keep[cur];
          if (k === 'r' && rolls > 0) { roll(); rolls--; }
          if (k === 'enter') { selecting = true; cur = 0; }
          return;
        }
        if (k === 'up') cur = (cur + 12) % 13;
        if (k === 'down') cur = (cur + 1) % 13;
        if (k === 'left' || k === 'right') { selecting = false; cur = 0; return; }
        if (k !== 'enter' && k !== 'space') return;
        if (used[col][cur]) return;
        sc[col][cur] = scoreFor(cur);
        used[col][cur] = 1;
        if (DUP) cpuPlace();
        turn++;
        if (turn >= 13) { turn = 0; col++; }
        if (col >= NCOL) {
          var total = grand();
          if (DUP) {
            var ct = cpuTotal();
            msg = 'You ' + total + ', computer ' + ct + ' — ' +
                  (total > ct ? 'you win!' : total < ct ? 'computer wins.' : 'a tie.');
          } else if (TARGET) {
            msg = 'Final ' + total + ' — target ' + TARGET + ': ' + (total >= TARGET ? 'beaten!' : 'missed.');
          } else msg = 'Final score: ' + total;
          host.saveScore(total);
          return;
        }
        newTurn();
      },
      draw: function (t) {
        t.header('YAHTZEE', ND + ' dice, ' + NROLL + ' rolls/turn' +
                 (NCOL > 1 ? ', 3 columns (x1 x2 x3)' : '') + (DUP ? ', shared dice' : '') +
                 ' — Space holds, R rerolls, Tab switches');
        var i, c;
        for (i = 0; i < ND; i++) {
          var hl = keep[i] ? '#2f6b2f' : null;
          t.text(12 + i * 8, 4, '+---+', C.white, hl);
          t.text(12 + i * 8, 5, '| ' + dice[i] + ' |', C.white, hl, true);
          t.text(12 + i * 8, 6, '+---+', C.white, hl);
          t.text(12 + i * 8, 7, (!selecting && i === cur) ? ' ^^^ ' : '     ', C.cyan,
                 (!selecting && i === cur) ? '#2b4a6b' : null);
          t.text(12 + i * 8, 8, keep[i] ? 'HELD ' : '     ', C.grey);
        }
        t.text(12, 9, 'Rolls left: ' + rolls + '   ', C.fg);
        for (i = 0; i < 13; i++) {
          t.text(12, 11 + i, (CATS[i] + '                 ').slice(0, 18),
                 used[col][i] ? C.grey : C.white,
                 (selecting && i === cur) ? '#2b4a6b' : null);
          for (c = 0; c < NCOL; c++) {
            var shown = used[c][i] ? sc[c][i] : (c === col ? scoreFor(i) : 0);
            t.text(31 + c * 5, 11 + i, String(shown).padStart(3, ' '),
                   used[c][i] ? C.grey : (c === col ? C.green : C.grey));
          }
          if (DUP) t.text(31 + NCOL * 5 + 4, 11 + i, String(cpuUsed[i] ? cpuSc[i] : 0).padStart(3, ' '), C.magenta);
        }
        t.text(12, 25, 'Total: ' + grand() + (DUP ? '    CPU: ' + cpuTotal() : '') +
               (TARGET ? '    Target: ' + TARGET : '') + '     ', C.fg, null, true);
        if (msg) t.center(27, msg + ' Press any key.', C.white, true);
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
