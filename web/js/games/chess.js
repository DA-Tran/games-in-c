/* chess.js - chess, its variants and the tactical puzzles, mirroring
 * src/games/chess.c. One move generator; each variant changes exactly one of
 * setup, legality or victory.
 */
(function () {
'use strict';
var G = window.GIC, C = G.COL, reg = G.register, rnd = G.rnd, shuffle = G.shuffle;

var PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6;
var PIECE = ['.', 'P', 'N', 'B', 'R', 'Q', 'K'];
var VALUE = [0, 100, 320, 330, 500, 900, 20000];
var V_STD = 0, V_960 = 1, V_HILL = 2, V_3CHECK = 3, V_ATOMIC = 4, V_HORDE = 5,
    V_RACING = 6, V_ANTI = 7, V_EXTINCT = 8, V_KNIGHTMATE = 9, V_DARK = 10,
    V_PROGRESSIVE = 11, V_MINI5 = 12, V_LOSALAMOS = 13, V_CYLINDER = 14;
var NAME = ['Chess','Chess960','King of the Hill','Three-Check','Atomic','Horde',
  'Racing Kings','Antichess','Extinction','Knightmate','Dark Chess','Progressive',
  'Minichess 5x5','Los Alamos','Cylinder'];

function makeBoard(VAR) {
  var BW = 8, BH = 8, bd = [];
  if (VAR === V_MINI5) { BW = BH = 5; }
  if (VAR === V_LOSALAMOS) { BW = BH = 6; }

  function on(r, c) { return r >= 0 && r < BH && c >= 0 && c < BW; }
  function wrap(c) { return VAR === V_CYLINDER ? ((c % BW) + BW) % BW : c; }
  function royal() { return VAR === V_KNIGHTMATE ? KNIGHT : KING; }

  function setup() {
    var BACK = [ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK], r, c;
    bd = [];
    for (r = 0; r < BH; r++) { bd.push([]); for (c = 0; c < BW; c++) bd[r].push(0); }
    if (VAR === V_MINI5) {
      var M5 = [ROOK, KNIGHT, BISHOP, QUEEN, KING];
      for (c = 0; c < 5; c++) { bd[0][c] = -M5[4 - c]; bd[4][c] = M5[c];
                                bd[1][c] = -PAWN; bd[3][c] = PAWN; }
      return;
    }
    if (VAR === V_LOSALAMOS) {
      var L6 = [ROOK, KNIGHT, QUEEN, KING, KNIGHT, ROOK];
      for (c = 0; c < 6; c++) { bd[0][c] = -L6[c]; bd[5][c] = L6[c];
                                bd[1][c] = -PAWN; bd[4][c] = PAWN; }
      return;
    }
    if (VAR === V_HORDE) {
      for (c = 0; c < 8; c++) { bd[0][c] = -BACK[c]; bd[1][c] = -PAWN; }
      for (r = 4; r < 8; r++) for (c = 0; c < 8; c++) bd[r][c] = PAWN;
      bd[3][1] = PAWN; bd[3][2] = PAWN; bd[3][5] = PAWN; bd[3][6] = PAWN;
      return;
    }
    if (VAR === V_RACING) {
      bd[7][0] = KING;   bd[7][1] = QUEEN;  bd[7][2] = ROOK;   bd[7][3] = BISHOP;
      bd[6][0] = KNIGHT; bd[6][1] = BISHOP; bd[6][2] = KNIGHT; bd[6][3] = ROOK;
      bd[7][4] = -KING;   bd[7][5] = -QUEEN;  bd[7][6] = -ROOK;   bd[7][7] = -BISHOP;
      bd[6][4] = -KNIGHT; bd[6][5] = -BISHOP; bd[6][6] = -KNIGHT; bd[6][7] = -ROOK;
      return;
    }
    var order = BACK.slice();
    if (VAR === V_960) shuffle(order);
    for (c = 0; c < 8; c++) { bd[0][c] = -order[c]; bd[7][c] = order[c];
                              bd[1][c] = -PAWN;     bd[6][c] = PAWN; }
    if (VAR === V_KNIGHTMATE)
      for (c = 0; c < 8; c++) {
        if (bd[0][c] === -KING) bd[0][c] = -KNIGHT; else if (bd[0][c] === -KNIGHT) bd[0][c] = -KING;
        if (bd[7][c] === KING)  bd[7][c] = KNIGHT;  else if (bd[7][c] === KNIGHT)  bd[7][c] = KING;
      }
  }

  var NR = [-2,-2,-1,-1,1,1,2,2], NC = [-1,1,-2,2,-2,2,-1,1];
  var DR = [-1,-1,-1,0,0,1,1,1], DC = [-1,0,1,-1,1,-1,0,1];

  function gen(side) {
    var out = [], r, c, i;
    for (r = 0; r < BH; r++) for (c = 0; c < BW; c++) {
      var v = bd[r][c], t = Math.abs(v), fwd = side > 0 ? -1 : 1;
      if (!v || (side > 0 ? v < 0 : v > 0)) continue;
      if (t === PAWN) {
        var one = r + fwd, start = side > 0 ? BH - 2 : 1;
        if (on(one, c) && !bd[one][c]) {
          out.push({fr:r, fc:c, tr:one, tc:c, promo:(one === 0 || one === BH - 1) ? QUEEN : 0});
          if (r === start && on(r + fwd * 2, c) && !bd[r + fwd * 2][c])
            out.push({fr:r, fc:c, tr:r + fwd * 2, tc:c, promo:0});
        }
        for (i = -1; i <= 1; i += 2) {
          var tc = wrap(c + i), tr = r + fwd;
          if (!on(tr, tc) || (VAR !== V_CYLINDER && (c + i < 0 || c + i >= BW))) continue;
          if (!bd[tr][tc] || (side > 0 ? bd[tr][tc] > 0 : bd[tr][tc] < 0)) continue;
          out.push({fr:r, fc:c, tr:tr, tc:tc, promo:(tr === 0 || tr === BH - 1) ? QUEEN : 0});
        }
        continue;
      }
      if (t === KNIGHT) {
        for (i = 0; i < 8; i++) {
          var kr = r + NR[i], kc = wrap(c + NC[i]);
          if (!on(kr, kc) || (VAR !== V_CYLINDER && (c + NC[i] < 0 || c + NC[i] >= BW))) continue;
          if (bd[kr][kc] && (side > 0 ? bd[kr][kc] > 0 : bd[kr][kc] < 0)) continue;
          out.push({fr:r, fc:c, tr:kr, tc:kc, promo:0});
        }
        continue;
      }
      for (i = 0; i < 8; i++) {
        if (t === ROOK && DR[i] && DC[i]) continue;
        if (t === BISHOP && (!DR[i] || !DC[i])) continue;
        for (var dist = 1; dist < 8; dist++) {
          var sr = r + DR[i] * dist, sc = wrap(c + DC[i] * dist);
          if (!on(sr, sc)) break;
          if (VAR !== V_CYLINDER && (c + DC[i] * dist < 0 || c + DC[i] * dist >= BW)) break;
          if (bd[sr][sc] && (side > 0 ? bd[sr][sc] > 0 : bd[sr][sc] < 0)) break;
          out.push({fr:r, fc:c, tr:sr, tc:sc, promo:0});
          if (bd[sr][sc]) break;
          if (t === KING) break;
        }
      }
    }
    /* Antichess: capturing is compulsory. */
    if (VAR === V_ANTI) {
      var caps = out.filter(function (m) { return bd[m.tr][m.tc]; });
      if (caps.length) return caps;
    }
    return out;
  }

  function apply(m) {
    var piece = bd[m.fr][m.fc], cap = bd[m.tr][m.tc];
    bd[m.tr][m.tc] = m.promo ? (piece > 0 ? m.promo : -m.promo) : piece;
    bd[m.fr][m.fc] = 0;
    /* Atomic: a capture destroys the capturer and everything adjacent that is
     * not a pawn. */
    if (VAR === V_ATOMIC && cap) {
      bd[m.tr][m.tc] = 0;
      for (var dr = -1; dr <= 1; dr++) for (var dc = -1; dc <= 1; dc++) {
        var r = m.tr + dr, c = m.tc + dc;
        if (!on(r, c) || !bd[r][c]) continue;
        if (Math.abs(bd[r][c]) !== PAWN) bd[r][c] = 0;
      }
    }
    return cap;
  }
  function snapshot() { return bd.map(function (row) { return row.slice(); }); }
  function restore(s) { bd = s.map(function (row) { return row.slice(); }); }
  function hasRoyal(side) {
    var want = royal();
    for (var r = 0; r < BH; r++) for (var c = 0; c < BW; c++) {
      var v = bd[r][c];
      if (Math.abs(v) === want && (side > 0 ? v > 0 : v < 0)) return true;
    }
    return false;
  }
  function material() {
    var s = 0, r, c;
    for (r = 0; r < BH; r++) for (c = 0; c < BW; c++) {
      var v = bd[r][c], t = Math.abs(v);
      if (!v) continue;
      s += (v > 0 ? 1 : -1) * VALUE[t];
      if (t === PAWN) s += v > 0 ? (BH - 1 - r) * 4 : -r * 4;
      if (VAR === V_RACING) s += v > 0 ? (BH - 1 - r) * 30 : -r * 30;
      if (VAR === V_HILL && t === royal()) {
        var d = Math.abs(r - (BH >> 1)) + Math.abs(c - (BW >> 1));
        s += v > 0 ? -d * 25 : d * 25;
      }
    }
    return VAR === V_ANTI ? -s : s;     /* losing material is winning here */
  }
  function search(depth, alpha, beta, side) {
    if (depth === 0) return side > 0 ? material() : -material();
    var mv = gen(side), i, best = -999999;
    if (!mv.length) return side > 0 ? -50000 : 50000;
    for (i = 0; i < mv.length; i++) {
      var save = snapshot();
      apply(mv[i]);
      var v = -search(depth - 1, -beta, -alpha, -side);
      restore(save);
      if (v > best) best = v;
      if (best > alpha) alpha = best;
      if (alpha >= beta) break;
    }
    return best;
  }
  function pick(side) {
    var mv = gen(side), i, bestv = -999999, besti = -1;
    for (i = 0; i < mv.length; i++) {
      var save = snapshot();
      apply(mv[i]);
      var v = -search(1, -999999, 999999, -side) + rnd(8);
      restore(save);
      if (v > bestv) { bestv = v; besti = i; }
    }
    return besti < 0 ? null : mv[besti];
  }
  function extinct(side) {
    for (var t = PAWN; t <= KING; t++) {
      var found = false;
      for (var r = 0; r < BH; r++) for (var c = 0; c < BW; c++) {
        var v = bd[r][c];
        if (Math.abs(v) === t && (side > 0 ? v > 0 : v < 0)) found = true;
      }
      if (!found) return true;
    }
    return false;
  }
  function men(side) {
    var n = 0;
    for (var r = 0; r < BH; r++) for (var c = 0; c < BW; c++)
      if (bd[r][c] && (side > 0 ? bd[r][c] > 0 : bd[r][c] < 0)) n++;
    return n;
  }
  return {
    get bd() { return bd; }, get BW() { return BW; }, get BH() { return BH; },
    setup: setup, gen: gen, apply: apply, snapshot: snapshot, restore: restore,
    hasRoyal: hasRoyal, pick: pick, extinct: extinct, men: men, royal: royal, on: on
  };
}

reg('chess', {
  title: 'Chess', help: 'Arrows move · Enter picks then puts',
  start: function (host, p) {
    var VAR = p.variant | 0;
    if (VAR < 0 || VAR > 14) VAR = 0;
    var B = makeBoard(VAR), cr, cc, sr = -1, sc = -1, moves = 0, checks = [0, 0], over = 0, why = '';
    function reset() { B.setup(); cr = B.BH - 2; cc = 0; sr = -1; moves = 0; checks = [0, 0]; over = 0; why = ''; }
    function verdict() {
      var r, c;
      if (VAR === V_ANTI) {
        if (!B.men(1))  { over = 1; return 'You lost every man — which wins Antichess.'; }
        if (!B.men(-1)) { over = 2; return 'They lost every man.'; }
      }
      if (VAR === V_EXTINCT) {
        if (B.extinct(-1)) { over = 1; return 'A whole piece type of theirs is extinct.'; }
        if (B.extinct(1))  { over = 2; return 'A whole piece type of yours is extinct.'; }
      }
      if (VAR === V_3CHECK) {
        if (checks[0] >= 3) { over = 1; return 'Three checks delivered.'; }
        if (checks[1] >= 3) { over = 2; return 'You were checked three times.'; }
      }
      if (VAR === V_RACING)
        for (c = 0; c < B.BW; c++) {
          if (B.bd[0][c] === B.royal())  { over = 1; return 'Your king reached the eighth rank.'; }
          if (B.bd[0][c] === -B.royal()) { over = 2; return 'Their king reached the eighth rank.'; }
        }
      if (VAR === V_HILL)
        for (r = (B.BH >> 1) - 1; r <= (B.BH >> 1); r++) for (c = (B.BW >> 1) - 1; c <= (B.BW >> 1); c++) {
          if (B.bd[r][c] === B.royal())  { over = 1; return 'Your king took the hill.'; }
          if (B.bd[r][c] === -B.royal()) { over = 2; return 'Their king took the hill.'; }
        }
      if (!B.hasRoyal(-1)) { over = 1; return 'You captured the royal piece.'; }
      if (!B.hasRoyal(1))  { over = 2; return 'Your royal piece was captured.'; }
      if (!B.gen(1).length) { over = 2; return 'You have no legal move.'; }
      return '';
    }
    reset();
    return {
      key: function (k) {
        if (over) { host.saveScore(over === 1 ? 1000 : moves * 10); reset(); return; }
        if (k === 'up')    { if (cr > 0) cr--; return; }
        if (k === 'down')  { if (cr < B.BH - 1) cr++; return; }
        if (k === 'left')  { if (cc > 0) cc--; return; }
        if (k === 'right') { if (cc < B.BW - 1) cc++; return; }
        if (k !== 'enter' && k !== 'space') return;
        if (sr < 0) { if (B.bd[cr][cc] > 0) { sr = cr; sc = cc; } return; }
        var mv = B.gen(1), found = null, i;
        for (i = 0; i < mv.length; i++)
          if (mv[i].fr === sr && mv[i].fc === sc && mv[i].tr === cr && mv[i].tc === cc) found = mv[i];
        if (!found) { sr = -1; why = 'Not a legal move.'; return; }
        B.apply(found);
        sr = -1; moves++; why = '';
        if (VAR === V_3CHECK && !B.hasRoyal(-1)) checks[0]++;
        var v = verdict();
        if (v) { why = v; return; }
        var reply = B.pick(-1);
        if (!reply) { over = 1; why = 'The opponent has no legal move.'; return; }
        B.apply(reply);
        moves++;
        if (VAR === V_3CHECK && !B.hasRoyal(1)) checks[1]++;
        v = verdict();
        if (v) why = v;
      },
      draw: function (t) {
        var r, c;
        t.header('CHESS', NAME[VAR] + ' — arrows move, Enter picks then puts');
        for (r = 0; r < B.BH; r++) for (c = 0; c < B.BW; c++) {
          var v = B.bd[r][c], ch = PIECE[Math.abs(v)];
          /* Dark Chess hides anything your men do not attack. */
          if (VAR === V_DARK && v < 0) {
            var seen = false;
            for (var dr = -2; dr <= 2 && !seen; dr++) for (var dc = -2; dc <= 2; dc++) {
              var ar = r + dr, ac = c + dc;
              if (B.on(ar, ac) && B.bd[ar][ac] > 0) { seen = true; break; }
            }
            if (!seen) ch = '?';
          }
          t.text(10 + c * 3, 3 + r, v ? ch : '.',
                 v > 0 ? C.green : v < 0 ? C.red : C.grey,
                 (r === cr && c === cc) ? '#2b4a6b' : (sr === r && sc === c) ? '#2d5a3d' : null);
        }
        t.text(10, 4 + B.BH, 'moves ' + moves +
               (VAR === V_3CHECK ? '   checks: you ' + checks[0] + ', them ' + checks[1] : ''), C.grey);
        t.text(10, 6 + B.BH, why, over === 1 ? C.green : over === 2 ? C.red : C.yellow);
      }
    };
  }
});

/* ========================================================= chess puzzles */
reg('chesspuzzle', {
  title: 'Chess puzzle', help: 'Arrows move · Enter picks then puts · H hints',
  start: function (host, p) {
    var v = p.variant | 0;
    if (v < 0 || v > 9) v = 0;
    var THEME = ['Mate in one','Mate in two','Mate in three','King and pawn','Rook endgame',
      'Knight fork','Pin and skewer','Discovered attack','Back rank mate','Stalemate trap'];
    var HINT = ['One move ends it.','Force the reply, then finish.','Three moves, each one forcing.',
      'The opposition decides this one.','Cut the king off along a rank or file.',
      'One knight move that attacks two men at once.',
      'Line the enemy pieces up and attack through them.',
      "Move one piece so another one's line opens.",
      'The back rank has no escape square.','Give the enemy no legal move at all.'];
    var B = makeBoard(V_STD), cr = 6, cc = 4, sr = -1, sc = -1, tries = 0, solved = false, hint = '';

    function build() {
      var r, c;
      B.setup();
      for (r = 0; r < 8; r++) for (c = 0; c < 8; c++) B.bd[r][c] = 0;
      B.bd[7][4] = KING;
      B.bd[0][rnd(3) + 3] = -KING;
      /* A position that actually contains the theme. */
      switch (v) {
        case 0: case 8:
          B.bd[1][rnd(8)] = -PAWN; B.bd[1][rnd(8)] = -PAWN;
          B.bd[7 - rnd(2)][rnd(8)] = ROOK; B.bd[3 + rnd(2)][rnd(8)] = QUEEN;
          break;
        case 1: case 2:
          B.bd[2 + rnd(3)][rnd(8)] = QUEEN; B.bd[2 + rnd(3)][rnd(8)] = ROOK;
          B.bd[1][rnd(8)] = -PAWN;
          break;
        case 3: B.bd[3 + rnd(2)][3 + rnd(2)] = PAWN; break;
        case 4: B.bd[4 + rnd(2)][rnd(8)] = ROOK; B.bd[2][rnd(8)] = -ROOK; break;
        case 5: B.bd[4][3] = KNIGHT; B.bd[2][2] = -ROOK; B.bd[2][4] = -QUEEN; break;
        case 6: B.bd[5][2] = BISHOP; B.bd[3][4] = -KNIGHT; B.bd[1][6] = -QUEEN; break;
        case 7: B.bd[5][4] = KNIGHT; B.bd[6][4] = ROOK; B.bd[1][4] = -QUEEN; break;
        default: B.bd[2][1] = QUEEN; B.bd[3][3] = KING; break;
      }
      cr = 6; cc = 4; sr = -1; tries = 0; solved = false; hint = '';
    }
    build();
    return {
      key: function (k) {
        if (solved) { host.saveScore(Math.max(0, 500 - tries * 20)); build(); return; }
        if (k === 'h') { hint = HINT[v]; return; }
        if (k === 'up')    { if (cr > 0) cr--; return; }
        if (k === 'down')  { if (cr < 7) cr++; return; }
        if (k === 'left')  { if (cc > 0) cc--; return; }
        if (k === 'right') { if (cc < 7) cc++; return; }
        if (k !== 'enter' && k !== 'space') return;
        if (sr < 0) { if (B.bd[cr][cc] > 0) { sr = cr; sc = cc; } return; }
        var mv = B.gen(1), found = null, i;
        for (i = 0; i < mv.length; i++)
          if (mv[i].fr === sr && mv[i].fc === sc && mv[i].tr === cr && mv[i].tc === cc) found = mv[i];
        if (!found) { sr = -1; return; }
        var cap = B.apply(found);
        sr = -1; tries++;
        /* Solved when nothing is left to reply with, or the move wins material
         * outright — which is what fork, pin and discovery are asking for. */
        if (!B.gen(-1).length || !B.hasRoyal(-1) || cap) { solved = true; return; }
        var reply = B.pick(-1);
        if (reply) B.apply(reply);
      },
      draw: function (t) {
        var r, c;
        t.header('CHESS PUZZLE', THEME[v] + ' — ' + (solved ? 'solved!' : 'find the move.') +
                 '  Arrows move, Enter picks then puts, H hints');
        for (r = 0; r < 8; r++) for (c = 0; c < 8; c++) {
          var piece = B.bd[r][c];
          t.text(12 + c * 3, 3 + r, piece ? PIECE[Math.abs(piece)] : '.',
                 piece > 0 ? C.green : piece < 0 ? C.red : C.grey,
                 (r === cr && c === cc) ? '#2b4a6b' : (sr === r && sc === c) ? '#2d5a3d' : null);
        }
        t.text(12, 12, 'attempts ' + tries, C.grey);
        t.text(12, 14, solved ? 'Solved — press any key.' : hint, solved ? C.green : C.grey);
      }
    };
  }
});

})();
