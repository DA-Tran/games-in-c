/* board.js - board and strategy ports of src/games/{tictactoe,connect4,
 * reversi,gomoku,checkers,mancala,nim,dots_boxes,battleship,mastermind}.c
 */
(function () {
'use strict';
var G = window.GIC, C = G.COL, reg = G.register, rnd = G.rnd;

/* ------------------------------------------------------------ tic tac toe */
reg('tictactoe', {
  title: 'Tic Tac Toe', help: 'Arrows move · Enter places · Q quits',
  start: function (host, p) {
    var bd, cur = 4, msg = null;
    var LINES = [[0,1,2],[3,4,5],[6,7,8],[0,3,6],[1,4,7],[2,5,8],[0,4,8],[2,4,6]];
    function reset() { bd = '         '.split(''); msg = null; }
    function winner(b) {
      for (var i = 0; i < 8; i++) {
        var L = LINES[i];
        if (b[L[0]] !== ' ' && b[L[0]] === b[L[1]] && b[L[1]] === b[L[2]]) return b[L[0]];
      }
      return b.indexOf(' ') < 0 ? 'D' : 0;
    }
    function minimax(b, ai, depth) {
      var w = winner(b), best, i, v;
      if (w === 'O') return 10 - depth;
      if (w === 'X') return depth - 10;
      if (w === 'D') return 0;
      best = ai ? -100 : 100;
      for (i = 0; i < 9; i++) {
        if (b[i] !== ' ') continue;
        b[i] = ai ? 'O' : 'X';
        v = minimax(b, !ai, depth + 1);
        b[i] = ' ';
        if (ai) { if (v > best) best = v; } else if (v < best) best = v;
      }
      return best;
    }
    reset();
    return {
      key: function (k) {
        if (msg) { reset(); return; }
        if (k === 'left')  cur = (cur % 3 === 0) ? cur + 2 : cur - 1;
        if (k === 'right') cur = (cur % 3 === 2) ? cur - 2 : cur + 1;
        if (k === 'up')    cur = (cur < 3) ? cur + 6 : cur - 3;
        if (k === 'down')  cur = (cur > 5) ? cur - 6 : cur + 3;
        if (k !== 'enter' && k !== 'space') return;
        if (bd[cur] !== ' ') return;
        bd[cur] = 'X';
        var w = winner(bd);
        if (!w) {
          var best = -100, mv = -1;
          for (var i = 0; i < 9; i++) {
            if (bd[i] !== ' ') continue;
            bd[i] = 'O';
            var v = minimax(bd, false, 0);
            bd[i] = ' ';
            if (v > best) { best = v; mv = i; }
          }
          if (mv >= 0) bd[mv] = 'O';
          w = winner(bd);
        }
        if (w) msg = w === 'X' ? 'You win!' : w === 'O' ? 'Computer wins.' : 'Draw.';
      },
      draw: function (t) {
        t.header('TIC TAC TOE', 'Arrows move · Enter places · Q quits');
        for (var r = 0; r < 3; r++) for (var c = 0; c < 3; c++) {
          var i = r * 3 + c, ch = bd[i] === ' ' ? '·' : bd[i];
          var fg = bd[i] === 'X' ? C.cyan : bd[i] === 'O' ? C.yellow : C.grey;
          t.put(36 + c * 4, 7 + r * 2, ch, fg, i === cur ? '#2b3038' : null, true);
        }
        if (msg) t.center(15, msg, C.white, true);
        t.center(17, msg ? 'Press any key to play again' : '', C.dim);
      }
    };
  }
});

/* ----------------------------------------------------------- connect four */
reg('connect', {
  title: 'Connect Four', help: 'Left/Right aim · Enter drops · Q quits',
  start: function (host, p) {
    var W = 7, H = 6, bd, cur = 3, msg = null;
    function reset() {
      bd = [];
      for (var r = 0; r < H; r++) bd.push(new Array(W).fill(' '));
      msg = null;
    }
    function drop(col, p) {
      for (var r = H - 1; r >= 0; r--) if (bd[r][col] === ' ') { bd[r][col] = p; return r; }
      return -1;
    }
    function wins(p) {
      for (var r = 0; r < H; r++) for (var c = 0; c < W; c++) {
        if (c + 3 < W && bd[r][c] === p && bd[r][c+1] === p && bd[r][c+2] === p && bd[r][c+3] === p) return true;
        if (r + 3 < H && bd[r][c] === p && bd[r+1][c] === p && bd[r+2][c] === p && bd[r+3][c] === p) return true;
        if (r + 3 < H && c + 3 < W && bd[r][c] === p && bd[r+1][c+1] === p && bd[r+2][c+2] === p && bd[r+3][c+3] === p) return true;
        if (r + 3 < H && c - 3 >= 0 && bd[r][c] === p && bd[r+1][c-1] === p && bd[r+2][c-2] === p && bd[r+3][c-3] === p) return true;
      }
      return false;
    }
    function full() { return bd[0].every(function (v) { return v !== ' '; }); }
    function win4(a, b, c, d) {
      var v = [a,b,c,d], me = 0, op = 0, sp = 0;
      v.forEach(function (x) { if (x === 'O') me++; else if (x === 'X') op++; else sp++; });
      if (me && op) return 0;
      if (me === 3 && sp === 1) return 50;
      if (me === 2 && sp === 2) return 10;
      if (op === 3 && sp === 1) return -60;
      if (op === 2 && sp === 2) return -8;
      return 0;
    }
    function evaluate() {
      var s = 0, r, c;
      for (r = 0; r < H; r++) for (c = 0; c < W; c++) {
        if (c + 3 < W) s += win4(bd[r][c], bd[r][c+1], bd[r][c+2], bd[r][c+3]);
        if (r + 3 < H) s += win4(bd[r][c], bd[r+1][c], bd[r+2][c], bd[r+3][c]);
        if (r + 3 < H && c + 3 < W) s += win4(bd[r][c], bd[r+1][c+1], bd[r+2][c+2], bd[r+3][c+3]);
        if (r + 3 < H && c >= 3) s += win4(bd[r][c], bd[r+1][c-1], bd[r+2][c-2], bd[r+3][c-3]);
      }
      for (r = 0; r < H; r++) s += bd[r][3] === 'O' ? 6 : bd[r][3] === 'X' ? -6 : 0;
      return s;
    }
    function negamax(depth, alpha, beta, ai) {
      if (wins('O')) return 100000 + depth;
      if (wins('X')) return -100000 - depth;
      if (full() || depth === 0) return evaluate();
      var best = ai ? -1e9 : 1e9;
      for (var c = 0; c < W; c++) {
        if (bd[0][c] !== ' ') continue;
        var r = drop(c, ai ? 'O' : 'X');
        var v = negamax(depth - 1, alpha, beta, !ai);
        bd[r][c] = ' ';
        if (ai) { if (v > best) best = v; if (best > alpha) alpha = best; }
        else    { if (v < best) best = v; if (best < beta)  beta  = best; }
        if (alpha >= beta) break;
      }
      return best;
    }
    reset();
    return {
      key: function (k) {
        if (msg) { reset(); return; }
        if (k === 'left'  && cur > 0) cur--;
        if (k === 'right' && cur < W - 1) cur++;
        if (k !== 'enter' && k !== 'space') return;
        if (bd[0][cur] !== ' ') return;
        drop(cur, 'X');
        if (wins('X')) { msg = 'You win!'; return; }
        if (full())    { msg = 'Draw.';    return; }
        var order = [3,2,4,1,5,0,6], best = -1e9, mv = -1;
        for (var i = 0; i < W; i++) {
          var col = order[i];
          if (bd[0][col] !== ' ') continue;
          var r = drop(col, 'O');
          var v = negamax(4, -1e9, 1e9, false);
          bd[r][col] = ' ';
          if (v > best) { best = v; mv = col; }
        }
        if (mv >= 0) drop(mv, 'O');
        if (wins('O')) msg = 'Computer wins.';
        else if (full()) msg = 'Draw.';
      },
      draw: function (t) {
        t.header('CONNECT FOUR', 'Left/Right aim · Enter drops · Q quits');
        t.text(31 + cur * 3, 5, 'v', C.cyan, null, true);
        for (var r = 0; r < 6; r++) for (var c = 0; c < 7; c++) {
          var p = bd[r][c];
          t.put(31 + c * 3, 7 + r, p === ' ' ? '·' : '●',
                p === 'X' ? C.cyan : p === 'O' ? C.yellow : C.grey, null, p !== ' ');
        }
        if (msg) t.center(15, msg, C.white, true);
        t.center(17, msg ? 'Press any key to play again' : '', C.dim);
      }
    };
  }
});

/* ---------------------------------------------------------------- reversi */
reg('reversi', {
  title: 'Reversi', help: 'Arrows move · Enter places · Q quits',
  start: function (host, p) {
    var N = Math.max(4, Math.min(12, p.size > 0 ? p.size : 8));
    if (N % 2) N++;
    var bd, cr, cc, msg = null;
    var DX = [-1,-1,-1,0,0,1,1,1], DY = [-1,0,1,-1,1,-1,0,1];
    function weight(r, c) {
      var er = (r === 0 || r === N - 1), ec = (c === 0 || c === N - 1);
      var nr = (r === 1 || r === N - 2), nc = (c === 1 || c === N - 2);
      if (er && ec) return 120;
      if ((er && nc) || (nr && ec)) return -20;
      if (nr && nc) return -40;
      if (er || ec) return 20;
      if (nr || nc) return -5;
      return 3;
    }
    function reset() {
      bd = [];
      for (var r = 0; r < N; r++) bd.push(new Array(N).fill(' '));
      var h = N / 2;
      bd[h-1][h-1] = bd[h][h] = 'O';
      bd[h-1][h] = bd[h][h-1] = 'X';
      cr = h - 1; cc = h - 1; msg = null;
    }
    function flips(r, c, pl, apply) {
      if (bd[r][c] !== ' ') return 0;
      var o = pl === 'X' ? 'O' : 'X', total = 0;
      for (var d = 0; d < 8; d++) {
        var i = r + DX[d], j = c + DY[d], n = 0;
        while (i >= 0 && i < N && j >= 0 && j < N && bd[i][j] === o) { i += DX[d]; j += DY[d]; n++; }
        if (n && i >= 0 && i < N && j >= 0 && j < N && bd[i][j] === pl) {
          total += n;
          if (apply) {
            i = r + DX[d]; j = c + DY[d];
            while (bd[i][j] === o) { bd[i][j] = pl; i += DX[d]; j += DY[d]; }
          }
        }
      }
      if (apply && total) bd[r][c] = pl;
      return total;
    }
    function hasMove(pl) {
      for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) if (flips(r, c, pl, false)) return true;
      return false;
    }
    function counts() {
      var x = 0, o = 0;
      for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) {
        if (bd[r][c] === 'X') x++; else if (bd[r][c] === 'O') o++;
      }
      return [x, o];
    }
    function aiMove() {
      var best = -1e9, br = -1, bc = -1;
      for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) {
        var f = flips(r, c, 'O', false);
        if (!f) continue;
        var v = f + weight(r, c);
        if (v > best) { best = v; br = r; bc = c; }
      }
      if (br >= 0) flips(br, bc, 'O', true);
    }
    reset();
    return {
      key: function (k) {
        if (msg) { reset(); return; }
        if (k === 'up' && cr > 0) cr--;
        if (k === 'down' && cr < N - 1) cr++;
        if (k === 'left' && cc > 0) cc--;
        if (k === 'right' && cc < N - 1) cc++;
        if (k !== 'enter' && k !== 'space') return;
        if (!flips(cr, cc, 'X', false)) return;
        flips(cr, cc, 'X', true);
        while (hasMove('O') && !hasMove('X')) aiMove();
        if (hasMove('O')) aiMove();
        if (!hasMove('X') && !hasMove('O')) {
          var n = counts();
          msg = n[0] > n[1] ? 'You win!' : n[0] < n[1] ? 'Computer wins.' : 'Draw.';
          host.saveScore(n[0]);
        }
      },
      draw: function (t) {
        t.header('REVERSI', N + 'x' + N + ' · arrows move · Enter places');
        var left = Math.max(1, 42 - (N * 3) / 2);
        for (var r = 0; r < N; r++) {
          t.text(left - 3, 4 + r, String(r + 1).padStart(2, ' '), C.dim);
          for (var c = 0; c < N; c++) {
            var bg = (r === cr && c === cc) ? '#2b4a6b' : null, ch, fg;
            if (bd[r][c] === 'X') { ch = 'O'; fg = C.cyan; }
            else if (bd[r][c] === 'O') { ch = 'O'; fg = C.yellow; }
            else if (flips(r, c, 'X', false)) { ch = '+'; fg = C.green; }
            else { ch = '.'; fg = C.grey; }
            t.put(left + c * 3, 4 + r, ch, fg, bg, true);
          }
        }
        var n2 = counts();
        t.text(left, N + 6, 'You ' + n2[0] + '   CPU ' + n2[1] + '     ', C.fg);
        if (msg) t.center(N + 8, msg + ' Press any key.', C.white, true);
      }
    };
  }
});

/* ----------------------------------------------------------------- gomoku */
reg('gomoku', {
  title: 'Gomoku', help: 'Arrows move · Enter places · Q quits',
  start: function (host, p) {
    var N = Math.max(9, Math.min(19, p.size > 0 ? p.size : 15));
    var RENJU = p.variant ? 1 : 0;
    var D = [[0,1],[1,0],[1,1],[1,-1]];
    var VAL = [0, 1, 30, 700, 12000, 500000];
    var bd, cr, cc, msg = null, moves = 0;
    function reset() {
      bd = [];
      for (var r = 0; r < N; r++) bd.push(new Array(N).fill(' '));
      cr = cc = N >> 1; msg = null; moves = 0;
    }
    function run(r, c, dr, dc, pl) {
      var n = 0; r += dr; c += dc;
      while (r >= 0 && r < N && c >= 0 && c < N && bd[r][c] === pl) { n++; r += dr; c += dc; }
      return n;
    }
    function runLen(r, c, i, pl) {
      return 1 + run(r, c, D[i][0], D[i][1], pl) + run(r, c, -D[i][0], -D[i][1], pl);
    }
    function winsAt(r, c, pl) {
      for (var i = 0; i < 4; i++) {
        var n = runLen(r, c, i, pl);
        if (RENJU && pl === 'X') { if (n === 5) return true; }
        else if (n >= 5) return true;
      }
      return false;
    }
    /* Renju restricts black: an overline or a double four loses. */
    function forbidden(r, c) {
      if (!RENJU) return false;
      var fours = 0;
      for (var i = 0; i < 4; i++) {
        var n = runLen(r, c, i, 'X');
        if (n >= 6) return true;
        if (n === 4) fours++;
      }
      return fours >= 2;
    }
    function sq(r, c, pl) {
      var s2 = 0;
      for (var i = 0; i < 4; i++) {
        var a2 = run(r, c, D[i][0], D[i][1], pl), b2 = run(r, c, -D[i][0], -D[i][1], pl);
        var n = Math.min(5, a2 + b2 + 1);
        var ra = r + D[i][0] * (a2 + 1), ca = c + D[i][1] * (a2 + 1);
        var rb = r - D[i][0] * (b2 + 1), cb = c - D[i][1] * (b2 + 1);
        var oa = ra >= 0 && ra < N && ca >= 0 && ca < N && bd[ra][ca] === ' ';
        var ob = rb >= 0 && rb < N && cb >= 0 && cb < N && bd[rb][cb] === ' ';
        if (!oa && !ob && n < 5) continue;
        s2 += VAL[n] * (oa && ob ? 2 : 1);
      }
      return s2;
    }
    function near(r, c) {
      for (var i = -2; i <= 2; i++) for (var j = -2; j <= 2; j++) {
        var a2 = r + i, b2 = c + j;
        if (a2 >= 0 && a2 < N && b2 >= 0 && b2 < N && bd[a2][b2] !== ' ') return true;
      }
      return false;
    }
    reset();
    return {
      key: function (k) {
        if (msg) { reset(); return; }
        if (k === 'up' && cr > 0) cr--;
        if (k === 'down' && cr < N - 1) cr++;
        if (k === 'left' && cc > 0) cc--;
        if (k === 'right' && cc < N - 1) cc++;
        if (k !== 'enter' && k !== 'space') return;
        if (bd[cr][cc] !== ' ') return;
        bd[cr][cc] = 'X';
        moves++;
        if (forbidden(cr, cc)) { msg = 'Forbidden under Renju — you lose.'; return; }
        if (winsAt(cr, cc, 'X')) {
          msg = 'You win!';
          host.saveScore(Math.floor(10000 / (moves + 1)));
          return;
        }
        var best = -1, br = N >> 1, bc = N >> 1;
        for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) {
          if (bd[r][c] !== ' ' || !near(r, c)) continue;
          var v = sq(r, c, 'O') + sq(r, c, 'X') * 1.1;
          if (v > best) { best = v; br = r; bc = c; }
        }
        bd[br][bc] = 'O';
        if (winsAt(br, bc, 'O')) msg = 'Computer wins.';
      },
      draw: function (t) {
        t.header('GOMOKU', N + 'x' + N + ' ' + (RENJU ? 'Renju (black restricted)' : 'free-style'));
        var left = Math.max(1, 42 - N);
        for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) {
          var pl = bd[r][c];
          t.put(left + c * 2, 4 + r, pl === ' ' ? '.' : 'O',
                pl === 'X' ? C.cyan : pl === 'O' ? C.yellow : C.grey,
                (r === cr && c === cc) ? '#2b4a6b' : null, pl !== ' ');
        }
        if (msg) t.center(N + 6, msg + ' Press any key.', C.white, true);
      }
    };
  }
});

/* -------------------------------------------------------------------- nim */
reg('nim', {
  title: 'Nim', help: 'Up/Down heap · Left/Right amount · Enter confirms',
  start: function (host, p) {
    var VAR = (p.variant >= 0 && p.variant <= 11) ? p.variant : 0;
    var NAMES = ['Nim','Misere Nim','Subtraction',"Wythoff's Game",'Fibonacci Nim',
                 'Kayles',"Moore's Nim","Dawson's Chess",'Turning Turtles',
                 "Northcott's Game",'Mock Turtles','Chomp'];
    var HELP = ['taking the last object wins','taking the last object LOSES',
                'take 1, 2 or 3','take from one heap or equally from both',
                'take up to twice the last take','knock down one or two',
                'take from up to two heaps','split a row',
                'flip a coin','slide toward the opponent','flip one to three',
                'the corner square is poison'];
    var HEAPS = p.size > 0 ? p.size : 4;
    if (VAR === 3) HEAPS = 2;
    if (VAR === 4) HEAPS = 1;
    HEAPS = Math.max(1, Math.min(6, HEAPS));
    var heap, cur, take, lastTake, msg;

    function total() { return heap.reduce(function (a2, b2) { return a2 + b2; }, 0); }
    function genMoves(st, lastt) {
      var out = [], i, k, cap;
      if (VAR === 2 || VAR === 5 || VAR === 10) {
        var lim = VAR === 2 ? 3 : VAR === 5 ? 2 : 3;
        for (i = 0; i < HEAPS; i++) for (k = 1; k <= lim && k <= st[i]; k++) out.push([i, k]);
      } else if (VAR === 3) {
        for (k = 1; k <= st[0]; k++) out.push([0, k]);
        for (k = 1; k <= st[1]; k++) out.push([1, k]);
        for (k = 1; k <= Math.min(st[0], st[1]); k++) out.push([2, k]);
      } else if (VAR === 4) {
        cap = lastt ? lastt * 2 : st[0];
        for (k = 1; k <= Math.min(cap, st[0]); k++) out.push([0, k]);
      } else {
        for (i = 0; i < HEAPS; i++) for (k = 1; k <= st[i]; k++) out.push([i, k]);
      }
      return out;
    }
    function apply(st, m) {
      if (VAR === 3 && m[0] === 2) { st[0] -= m[1]; st[1] -= m[1]; return; }
      if (VAR === 11) {
        var w = st[m[0]] - m[1];
        st[m[0]] = w;
        for (var i = m[0] + 1; i < HEAPS; i++) if (st[i] > w) st[i] = w;
        return;
      }
      st[m[0]] -= m[1];
    }
    var memo;
    /* True when the player to move wins with perfect play. */
    function winning(st, lastt, depth) {
      var sum = st.reduce(function (a2, b2) { return a2 + b2; }, 0);
      if (sum === 0) return (VAR === 1 || VAR === 11) ? 1 : 0;
      if (depth > 22) return sum & 1;
      var key = st.join(',') + '|' + lastt;
      if (memo[key] !== undefined) return memo[key];
      memo[key] = 0;
      var mv = genMoves(st, lastt);
      for (var i = 0; i < mv.length; i++) {
        var next = st.slice();
        apply(next, mv[i]);
        if (!winning(next, VAR === 4 ? mv[i][1] : 0, depth + 1)) { memo[key] = 1; return 1; }
      }
      return 0;
    }
    function aiMove() {
      var mv = genMoves(heap, lastTake);
      if (!mv.length) return null;
      for (var i = 0; i < mv.length; i++) {
        var next = heap.slice();
        apply(next, mv[i]);
        if (!winning(next, VAR === 4 ? mv[i][1] : 0, 0)) return mv[i];
      }
      return mv[rnd(mv.length)];
    }
    function reset() {
      memo = {};
      heap = [];
      for (var i = 0; i < HEAPS; i++)
        heap.push(VAR === 11 ? HEAPS + 1 : G.rndRange(2, VAR === 4 ? 14 : 7));
      cur = 0; take = 1; lastTake = 0; msg = null;
    }
    reset();
    return {
      key: function (k) {
        if (msg) { reset(); return; }
        if (take > heap[cur]) take = heap[cur] > 0 ? heap[cur] : 1;
        if (k === 'up') { cur = (cur + HEAPS - 1) % HEAPS; take = 1; }
        if (k === 'down') { cur = (cur + 1) % HEAPS; take = 1; }
        if (k === 'left' && take > 1) take--;
        if (k === 'right' && take < heap[cur]) take++;
        if (k !== 'enter' && k !== 'space') return;
        if (heap[cur] < take || take < 1) return;
        if (VAR === 4 && lastTake && take > lastTake * 2) { msg = 'Exceeds twice the last take.'; return; }
        apply(heap, [cur, take]);
        lastTake = VAR === 4 ? take : 0;
        take = 1;
        if (total() === 0) {
          var win = (VAR === 1 || VAR === 11) ? false : true;
          msg = win ? 'You took the last — you win!' : 'You took the last — you lose.';
          host.saveScore(win ? 100 : 0);
          return;
        }
        var m = aiMove();
        if (m) {
          apply(heap, m);
          lastTake = VAR === 4 ? m[1] : 0;
          if (total() === 0) {
            var cw = (VAR === 1 || VAR === 11) ? false : true;
            msg = cw ? 'Computer took the last and wins.' : 'Computer took the last — you win!';
            host.saveScore(cw ? 0 : 100);
          } else msg = null;
        }
      },
      draw: function (t) {
        t.header('NIM FAMILY', NAMES[VAR] + ' — ' + HELP[VAR]);
        for (var i = 0; i < HEAPS; i++) {
          t.text(16, 5 + i * 2, (VAR === 11 ? 'Row ' : 'Heap ') + (i + 1),
                 i === cur ? C.white : C.dim, null, i === cur);
          var glyph = VAR === 11 ? '#' : VAR === 5 ? '|' : 'o';
          for (var j = 0; j < heap[i] && j < 30; j++)
            t.put(26 + j, 5 + i * 2, glyph, C.yellow);
          t.text(58, 5 + i * 2, '(' + heap[i] + ')   ', C.dim);
        }
        if (VAR === 4 && lastTake)
          t.text(16, 6 + HEAPS * 2, 'You may take up to ' + (lastTake * 2) + '.   ', C.dim);
        t.text(16, 7 + HEAPS * 2, 'Take ' + take + ' from ' +
               (VAR === 11 ? 'row ' : 'heap ') + (cur + 1) + '      ', C.cyan);
        if (msg) t.center(9 + HEAPS * 2, msg + ' Press any key.', C.white, true);
      }
    };
  }
});

/* -------------------------------------------------------------- mastermind */
reg('mastermind', {
  title: 'Mastermind', help: 'Left/Right peg · Up/Down colour · Enter submits',
  start: function (host, p) {
    var PEGS = Math.max(3, Math.min(6, p.count > 0 ? p.count : 4));
    var COLOURS = Math.max(4, Math.min(10, p.level > 0 ? p.level : 6));
    /* Wider codes and bigger palettes need more attempts to stay fair. */
    var TRIES = Math.min(14, 8 + (PEGS - 3) + Math.floor((COLOURS - 6) / 2));
    var NAME = 'RGBYMCWOPT';
    var CC = [C.red, C.green, C.blue, C.yellow, C.magenta,
              C.cyan, C.white, C.red, C.green, C.blue];
    var secret, guesses, work, row, cur, msg;
    function reset() {
      secret = []; for (var i = 0; i < PEGS; i++) secret.push(rnd(COLOURS));
      guesses = []; work = new Array(PEGS).fill(0); row = 0; cur = 0; msg = null;
    }
    function feedback(g) {
      var su = new Array(PEGS).fill(0), gu = new Array(PEGS).fill(0), ex = 0, pa = 0, i, j;
      for (i = 0; i < PEGS; i++) if (g[i] === secret[i]) { ex++; su[i] = gu[i] = 1; }
      for (i = 0; i < PEGS; i++) {
        if (gu[i]) continue;
        for (j = 0; j < PEGS; j++) {
          if (su[j] || g[i] !== secret[j]) continue;
          su[j] = gu[i] = 1; pa++; break;
        }
      }
      return [ex, pa];
    }
    reset();
    return {
      key: function (k) {
        if (msg) { reset(); return; }
        if (k === 'left')  cur = (cur + PEGS - 1) % PEGS;
        if (k === 'right') cur = (cur + 1) % PEGS;
        if (k === 'up')    work[cur] = (work[cur] + 1) % COLOURS;
        if (k === 'down')  work[cur] = (work[cur] + COLOURS - 1) % COLOURS;
        if (k !== 'enter' && k !== 'space') return;
        var fb = feedback(work);
        guesses.push({ g: work.slice(), ex: fb[0], pa: fb[1] });
        row++;
        if (fb[0] === PEGS) { msg = 'Code broken in ' + row + '!'; host.saveScore((TRIES - row + 1) * 10); }
        else if (row >= TRIES) msg = 'Out of guesses — code revealed.';
      },
      draw: function (t) {
        t.header('MASTERMIND', PEGS + ' pegs, ' + COLOURS + ' colours, ' + TRIES + ' guesses');
        t.text(24, 4, 'Colours: ' + NAME.slice(0, COLOURS).split('').join(' '), C.dim);
        t.text(24, 6, 'Secret: ', C.fg);
        for (var i = 0; i < PEGS; i++)
          t.put(32 + i * 3, 6, msg ? '●' : '?', msg ? CC[secret[i]] : C.grey, null, true);
        for (var r = 0; r < TRIES; r++) {
          t.text(24, 8 + r, String(r + 1).padStart(2, ' '), r === row ? C.white : C.dim);
          for (var p = 0; p < PEGS; p++) {
            if (r < guesses.length)
              t.put(28 + p * 3, 8 + r, '●', CC[guesses[r].g[p]], null, true);
            else if (r === row)
              t.put(28 + p * 3, 8 + r, NAME[work[p]], CC[work[p]], p === cur ? '#3a4048' : null, true);
            else t.put(28 + p * 3, 8 + r, '·', C.grey);
          }
          if (r < guesses.length)
            t.text(42, 8 + r, guesses[r].ex + ' exact  ' + guesses[r].pa + ' partial', C.dim);
        }
        if (msg) t.center(20, msg, C.white, true);
      }
    };
  }
});

/* ------------------------------------------------------------- battleship */
reg('battleship', {
  title: 'Battleship', help: 'Arrows aim · Enter fires · Q quits',
  start: function (host, p) {
    var N = 10, LEN = [5,4,3,3,2];
    var you, cpu, cr = 0, cc = 0, msg = '', queue = [], over = null;
    function blank() {
      var g = []; for (var r = 0; r < N; r++) g.push(new Array(N).fill(0));
      return g;
    }
    function place(g) {
      LEN.forEach(function (len) {
        for (;;) {
          var h = rnd(2), r = rnd(N), c = rnd(N), ok = true, i;
          if (h ? c + len > N : r + len > N) continue;
          for (i = 0; i < len; i++) if (g[r + (h ? 0 : i)][c + (h ? i : 0)]) ok = false;
          if (!ok) continue;
          for (i = 0; i < len; i++) g[r + (h ? 0 : i)][c + (h ? i : 0)] = 1;
          return;
        }
      });
    }
    function afloat(g) {
      var n = 0;
      for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) if (g[r][c] === 1) n++;
      return n;
    }
    function reset() { you = blank(); cpu = blank(); place(you); place(cpu); queue = []; over = null; msg = ''; }
    function push(r, c) { if (r >= 0 && r < N && c >= 0 && c < N) queue.push([r, c]); }
    function cpuShot() {
      var r, c;
      for (;;) {
        if (queue.length) { var q = queue.pop(); r = q[0]; c = q[1]; }
        else { r = rnd(N); c = rnd(N); if ((r + c) % 2) continue; }
        if (you[r][c] === 2 || you[r][c] === 3) continue;
        break;
      }
      if (you[r][c] === 1) { you[r][c] = 3; push(r-1,c); push(r+1,c); push(r,c-1); push(r,c+1); }
      else you[r][c] = 2;
    }
    function grid(t, top, left, g, hide, sr, sc) {
      t.text(left, top, '   0 1 2 3 4 5 6 7 8 9', C.dim);
      for (var r = 0; r < N; r++) {
        t.text(left, top + 1 + r, String.fromCharCode(65 + r), C.dim);
        for (var c = 0; c < N; c++) {
          var v = g[r][c], ch = '·', fg = C.blue;
          if (v === 3) { ch = 'X'; fg = C.red; }
          else if (v === 2) { ch = 'o'; fg = C.grey; }
          else if (v === 1 && !hide) { ch = '▩'; fg = C.cyan; }
          t.put(left + 3 + c * 2, top + 1 + r, ch, fg,
                (sr === r && sc === c) ? '#2b4a6b' : null, v === 3);
        }
      }
    }
    reset();
    return {
      key: function (k) {
        if (over) { reset(); return; }
        if (k === 'up'    && cr > 0) cr--;
        if (k === 'down'  && cr < N - 1) cr++;
        if (k === 'left'  && cc > 0) cc--;
        if (k === 'right' && cc < N - 1) cc++;
        if (k !== 'enter' && k !== 'space') return;
        if (cpu[cr][cc] === 2 || cpu[cr][cc] === 3) return;
        if (cpu[cr][cc] === 1) { cpu[cr][cc] = 3; msg = 'HIT!'; } else { cpu[cr][cc] = 2; msg = 'Miss.'; }
        if (afloat(cpu) === 0) { over = 'Enemy fleet destroyed — you win!'; host.saveScore(afloat(you) * 100); return; }
        cpuShot();
        if (afloat(you) === 0) over = 'Your fleet is lost — computer wins.';
      },
      draw: function (t) {
        t.header('BATTLESHIP', 'Arrows aim · Enter fires · Q quits');
        t.text(4, 4, 'YOUR FLEET', C.cyan, null, true);
        grid(t, 5, 4, you, false, -1, -1);
        t.text(46, 4, 'ENEMY WATERS', C.red, null, true);
        grid(t, 5, 46, cpu, true, cr, cc);
        t.text(4, 17, 'Ships afloat: ' + afloat(you) + '  ', C.fg);
        t.text(46, 17, 'Enemy afloat: ' + afloat(cpu) + '  ', C.fg);
        t.text(4, 19, over || msg, over ? C.white : C.yellow, null, true);
      }
    };
  }
});

/* ------------------------------------------------------------- mancala */
reg('mancala', {
  title: 'Mancala', help: 'Left/Right pick a house · Enter sows · Q quits',
  start: function (host, p) {
    var HOUSES = Math.max(4, Math.min(8, p.width > 0 ? p.width : 6));
    var SEEDS = Math.max(2, Math.min(9, p.count > 0 ? p.count : 4));
    var VAR = (p.variant >= 0 && p.variant <= 8) ? p.variant : 0;
    var NAMES = ['Kalah','Oware','Congkak','Sungka','Ayo','Dakon',
                 'Pallanguzhi','Toguz Kumalak','Bao'];
    /* The rule sets differ genuinely; each is a set of flags. */
    var storeSowing = true, freeTurn = false, relay = false, captureRule = 1, tuzdik = false;
    if (VAR === 0) { freeTurn = true; captureRule = 1; }
    else if (VAR === 1 || VAR === 4) { storeSowing = false; captureRule = 2; }
    else if (VAR === 2 || VAR === 3 || VAR === 5) { relay = true; freeTurn = true; }
    else if (VAR === 6) { captureRule = 3; relay = true; }
    else if (VAR === 7) { tuzdik = true; captureRule = 0; }
    else if (VAR === 8) { relay = true; captureRule = 1; }

    var PITS = HOUSES * 2 + 2;
    var MY = HOUSES, CPU = HOUSES * 2 + 1;
    var pit, tuz, cur, msg;

    function owns(human, idx) {
      return human ? (idx >= 0 && idx < HOUSES) : (idx > HOUSES && idx < CPU);
    }
    function sideEmpty(human) {
      var lo = human ? 0 : HOUSES + 1, s2 = 0;
      for (var i = lo; i < lo + HOUSES; i++) s2 += pit[i];
      return s2 === 0;
    }
    function sow(start, human) {
      var mine = human ? MY : CPU, theirs = human ? CPU : MY;
      var c = start, guard = 0, seeds;
      for (;;) {
        seeds = pit[c];
        pit[c] = 0;
        if (seeds === 0) return false;
        while (seeds > 0) {
          c = (c + 1) % PITS;
          if (c === theirs) continue;
          if (c === mine && !storeSowing) continue;
          if (tuzdik && c === tuz[human ? 0 : 1]) { pit[mine]++; seeds--; continue; }
          pit[c]++; seeds--;
        }
        if (c === mine) return freeTurn;
        if (relay && pit[c] > 1 && ++guard < 200) continue;
        break;
      }
      if (captureRule === 1) {
        if (pit[c] === 1 && owns(human, c)) {
          var opp = HOUSES * 2 - c;
          if (opp >= 0 && opp < PITS && opp !== c && pit[opp] > 0) {
            pit[mine] += pit[opp] + 1; pit[opp] = 0; pit[c] = 0;
          }
        }
      } else if (captureRule === 2) {
        while (!owns(human, c) && c !== mine && c !== theirs && (pit[c] === 2 || pit[c] === 3)) {
          pit[mine] += pit[c]; pit[c] = 0;
          c = (c - 1 + PITS) % PITS;
        }
      } else if (captureRule === 3) {
        for (var i = 0; i < PITS; i++) {
          if (i === mine || i === theirs) continue;
          if (pit[i] === 4) { pit[mine] += 4; pit[i] = 0; }
        }
      }
      if (tuzdik && !owns(human, c) && pit[c] === 3 && tuz[human ? 0 : 1] < 0 &&
          c !== mine && c !== theirs) {
        tuz[human ? 0 : 1] = c;
        pit[mine] += 3; pit[c] = 0;
      }
      return false;
    }
    function aiTurn() {
      var guard = 0;
      for (;;) {
        var best = -1e9, bi = -1;
        for (var i = HOUSES + 1; i < CPU; i++) {
          if (!pit[i]) continue;
          var save = pit.slice(), st = tuz.slice();
          var again = sow(i, false);
          var v = (pit[CPU] - save[CPU]) * 10 - (pit[MY] - save[MY]) * 8 + (again ? 15 : 0);
          pit = save; tuz = st;
          if (v > best) { best = v; bi = i; }
        }
        if (bi < 0) return;
        if (!sow(bi, false)) return;
        if (sideEmpty(false) || ++guard > 12) return;
      }
    }
    function reset() {
      pit = new Array(PITS).fill(SEEDS);
      pit[MY] = pit[CPU] = 0;
      tuz = [-1, -1];
      cur = 0; msg = null;
    }
    reset();
    return {
      key: function (k) {
        if (msg) { reset(); return; }
        if (k === 'left' && cur > 0) cur--;
        if (k === 'right' && cur < HOUSES - 1) cur++;
        if (k !== 'enter' && k !== 'space') return;
        if (!pit[cur]) return;
        if (!sow(cur, true) && !sideEmpty(true)) aiTurn();
        if (sideEmpty(true) || sideEmpty(false)) {
          var i;
          for (i = 0; i < HOUSES; i++) { pit[MY] += pit[i]; pit[i] = 0; }
          for (i = HOUSES + 1; i < CPU; i++) { pit[CPU] += pit[i]; pit[i] = 0; }
          msg = pit[MY] > pit[CPU] ? 'You win!' : pit[MY] < pit[CPU] ? 'Computer wins.' : 'Draw.';
          host.saveScore(pit[MY]);
        }
      },
      draw: function (t) {
        t.header('MANCALA', NAMES[VAR] + ' — ' + HOUSES + ' houses, ' + SEEDS + ' seeds');
        var left = Math.max(6, 42 - (HOUSES * 5) / 2);
        t.text(left - 5, 5, 'CPU', C.red, null, true);
        for (var i = CPU - 1, x = 0; i > HOUSES; i--, x++)
          t.text(left + x * 5, 5, '[' + String(pit[i]).padStart(2, ' ') + ']',
                 (tuzdik && i === tuz[0]) ? C.green : C.red);
        t.text(left - 5, 7, 'CPU store ' + pit[CPU] + '   ', C.red);
        t.text(left + HOUSES * 5, 7, 'Your store ' + pit[MY] + '   ', C.cyan);
        t.text(left - 5, 9, 'YOU', C.cyan, null, true);
        for (var j = 0; j < HOUSES; j++)
          t.text(left + j * 5, 9, '[' + String(pit[j]).padStart(2, ' ') + ']',
                 (tuzdik && j === tuz[1]) ? C.green : C.cyan,
                 j === cur ? '#2b4a6b' : null, j === cur);
        if (tuzdik) t.text(left - 5, 11, 'Green marks a claimed tuzdik.', C.dim);
        if (msg) t.center(13, msg + ' Press any key.', C.white, true);
      }
    };
  }
});

/* ---------------------------------------------------------- dots & boxes */
reg('dotsboxes', {
  title: 'Dots and Boxes', help: 'Arrows move · Tab switches edge · Enter draws',
  start: function (host, p) {
    var B = Math.max(3, Math.min(7, p.size > 0 ? p.size : 4));
    var D = B + 1;
    var h, v, own, you, cpu, cr = 0, cc = 0, horiz = true, msg = null;
    function reset() {
      h = []; for (var r = 0; r <= B; r++) h.push(new Array(B).fill(0));
      v = []; for (r = 0; r < B; r++) v.push(new Array(D).fill(0));
      own = []; for (r = 0; r < B; r++) own.push(new Array(B).fill(' '));
      you = cpu = 0; msg = null;
    }
    function closed(r, c) { return h[r][c] && h[r+1][c] && v[r][c] && v[r][c+1]; }
    function sides(r, c) { return h[r][c] + h[r+1][c] + v[r][c] + v[r][c+1]; }
    function claim(who) {
      var n = 0;
      for (var r = 0; r < B; r++) for (var c = 0; c < B; c++)
        if (own[r][c] === ' ' && closed(r, c)) {
          own[r][c] = who; n++;
          if (who === 'Y') you++; else cpu++;
        }
      return n;
    }
    function left() {
      var n = 0, r, c;
      for (r = 0; r <= B; r++) for (c = 0; c < B; c++) if (!h[r][c]) n++;
      for (r = 0; r < B; r++) for (c = 0; c <= B; c++) if (!v[r][c]) n++;
      return n;
    }
    function aiTurn() {
      for (;;) {
        var pick = null, safe = null, r, c;
        for (r = 0; r <= B && !pick; r++) for (c = 0; c < B; c++) {
          if (h[r][c]) continue;
          h[r][c] = 1;
          var gain = (r < B && closed(r, c)) || (r > 0 && closed(r-1, c));
          var risky = (r < B && sides(r, c) === 3) || (r > 0 && sides(r-1, c) === 3);
          h[r][c] = 0;
          if (gain) { pick = ['h', r, c]; break; }
          if (!risky && !safe) safe = ['h', r, c];
        }
        if (!pick) for (r = 0; r < B && !pick; r++) for (c = 0; c <= B; c++) {
          if (v[r][c]) continue;
          v[r][c] = 1;
          var g2 = (c < B && closed(r, c)) || (c > 0 && closed(r, c-1));
          var k2 = (c < B && sides(r, c) === 3) || (c > 0 && sides(r, c-1) === 3);
          v[r][c] = 0;
          if (g2) { pick = ['v', r, c]; break; }
          if (!k2 && !safe) safe = ['v', r, c];
        }
        if (!pick) pick = safe;
        if (!pick) {
          for (r = 0; r <= B && !pick; r++) for (c = 0; c < B; c++) if (!h[r][c]) { pick = ['h', r, c]; break; }
          for (r = 0; r < B && !pick; r++) for (c = 0; c <= B; c++) if (!v[r][c]) { pick = ['v', r, c]; break; }
        }
        if (!pick) return;
        if (pick[0] === 'h') h[pick[1]][pick[2]] = 1; else v[pick[1]][pick[2]] = 1;
        if (!claim('C')) return;
        if (left() === 0) return;
      }
    }
    reset();
    return {
      key: function (k) {
        if (msg) { reset(); return; }
        var maxr = horiz ? B : B - 1, maxc = horiz ? B - 1 : B;
        if (k === 'tab') { horiz = !horiz; cr = Math.min(cr, horiz ? B : B - 1); cc = Math.min(cc, horiz ? B - 1 : B); return; }
        if (k === 'up'    && cr > 0) cr--;
        if (k === 'down'  && cr < maxr) cr++;
        if (k === 'left'  && cc > 0) cc--;
        if (k === 'right' && cc < maxc) cc++;
        if (k !== 'enter' && k !== 'space') return;
        if (horiz ? h[cr][cc] : v[cr][cc]) return;
        if (horiz) h[cr][cc] = 1; else v[cr][cc] = 1;
        if (!claim('Y') && left() > 0) aiTurn();
        if (left() === 0) {
          msg = you > cpu ? 'You win!' : you < cpu ? 'Computer wins.' : 'Draw.';
          host.saveScore(you);
        }
      },
      draw: function (t) {
        t.header('DOTS AND BOXES', B + 'x' + B + ' boxes · arrows move · Tab switches edge');
        for (var r = 0; r < D; r++) {
          for (var c = 0; c < D; c++) {
            t.put(30 + c * 4, 5 + r * 2, '•', C.white);
            if (c < B) {
              var sel = horiz && r === cr && c === cc;
              t.text(31 + c * 4, 5 + r * 2, h[r][c] ? '───' : ' · ',
                     h[r][c] ? C.green : C.grey, sel ? '#3a4048' : null);
            }
          }
          if (r < B) for (var c2 = 0; c2 < D; c2++) {
            var sv = !horiz && r === cr && c2 === cc;
            t.put(30 + c2 * 4, 6 + r * 2, v[r][c2] ? '│' : '·',
                  v[r][c2] ? C.green : C.grey, sv ? '#3a4048' : null);
            if (c2 < B) t.put(32 + c2 * 4, 6 + r * 2, own[r][c2] === ' ' ? ' ' : own[r][c2],
                              own[r][c2] === 'Y' ? C.cyan : C.red, null, true);
          }
        }
        t.text(30, 6 + D * 2, 'You ' + you + '   CPU ' + cpu + '   ', C.fg);
        if (msg) t.center(8 + D * 2, msg + ' Press any key.', C.white, true);
      }
    };
  }
});

/* --------------------------------------------------------------- checkers */
reg('checkers', {
  title: 'Checkers', help: 'Arrows move · Enter selects then targets · Q quits',
  start: function (host, p) {
    var N = 8, bd, cr = 5, cc = 0, sr = -1, sc = -1, msg = null;
    var DR = [-1,-1,1,1], DC = [-1,1,-1,1];
    function mine(p)   { return p === 'x' || p === 'X'; }
    function theirs(p) { return p === 'o' || p === 'O'; }
    function king(p)   { return p === 'X' || p === 'O'; }
    function reset() {
      bd = [];
      for (var r = 0; r < N; r++) {
        var row = [];
        for (var c = 0; c < N; c++) {
          if ((r + c) % 2 === 0) row.push('.');
          else if (r < 3) row.push('o');
          else if (r > 4) row.push('x');
          else row.push('.');
        }
        bd.push(row);
      }
      sr = sc = -1; msg = null;
    }
    function gen(human) {
      var out = [], jumps = false, r, c, d;
      for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        var p = bd[r][c];
        if (human ? !mine(p) : !theirs(p)) continue;
        for (d = 0; d < 4; d++) {
          if (!king(p)) { if (human && DR[d] > 0) continue; if (!human && DR[d] < 0) continue; }
          var r1 = r + DR[d], c1 = c + DC[d], r2 = r + 2*DR[d], c2 = c + 2*DC[d];
          if (r2 >= 0 && r2 < N && c2 >= 0 && c2 < N && bd[r2][c2] === '.' &&
              (human ? theirs(bd[r1][c1]) : mine(bd[r1][c1]))) {
            out.push({ r1: r, c1: c, r2: r2, c2: c2, jump: true }); jumps = true;
          } else if (r1 >= 0 && r1 < N && c1 >= 0 && c1 < N && bd[r1][c1] === '.') {
            out.push({ r1: r, c1: c, r2: r1, c2: c1, jump: false });
          }
        }
      }
      return jumps ? out.filter(function (m) { return m.jump; }) : out;
    }
    function apply(m, human) {
      var p = bd[m.r1][m.c1];
      bd[m.r1][m.c1] = '.';
      if (m.jump) bd[(m.r1 + m.r2) >> 1][(m.c1 + m.c2) >> 1] = '.';
      if (human && m.r2 === 0) p = 'X';
      if (!human && m.r2 === N - 1) p = 'O';
      bd[m.r2][m.c2] = p;
      if (m.jump)
        return gen(human).some(function (x) { return x.jump && x.r1 === m.r2 && x.c1 === m.c2; });
      return false;
    }
    function material() {
      var s = 0;
      for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) {
        var p = bd[r][c];
        if (p === 'o') s += 10 + r; else if (p === 'O') s += 25;
        else if (p === 'x') s -= 10 + (N - 1 - r); else if (p === 'X') s -= 25;
      }
      return s;
    }
    function aiTurn() {
      for (;;) {
        var moves = gen(false);
        if (!moves.length) return;
        var best = -1e9, bi = 0;
        moves.forEach(function (m, i) {
          var save = bd.map(function (row) { return row.slice(); });
          apply(m, false);
          var v = material() + (m.jump ? 40 : 0) + rnd(3);
          bd = save;
          if (v > best) { best = v; bi = i; }
        });
        if (!apply(moves[bi], false)) return;
      }
    }
    reset();
    return {
      key: function (k) {
        if (msg) { reset(); return; }
        if (k === 'up'    && cr > 0) cr--;
        if (k === 'down'  && cr < N - 1) cr++;
        if (k === 'left'  && cc > 0) cc--;
        if (k === 'right' && cc < N - 1) cc++;
        if (k !== 'enter' && k !== 'space') return;
        var moves = gen(true);
        if (sr < 0) { if (mine(bd[cr][cc])) { sr = cr; sc = cc; } return; }
        if (sr === cr && sc === cc) { sr = sc = -1; return; }
        var found = moves.filter(function (m) {
          return m.r1 === sr && m.c1 === sc && m.r2 === cr && m.c2 === cc;
        })[0];
        if (!found) return;
        if (apply(found, true)) { sr = cr; sc = cc; return; }
        sr = sc = -1;
        aiTurn();
        var m2 = 0, t2 = 0;
        for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) {
          if (mine(bd[r][c])) m2++;
          if (theirs(bd[r][c])) t2++;
        }
        if (!t2 || !gen(false).length) { msg = 'You win!'; host.saveScore(m2 * 10); }
        else if (!m2 || !gen(true).length) msg = 'Computer wins.';
      },
      draw: function (t) {
        t.header('CHECKERS', 'Arrows move · Enter selects then targets · Q quits');
        t.text(28, 4, '  a  b  c  d  e  f  g  h', C.dim);
        for (var r = 0; r < 8; r++) {
          t.text(28, 5 + r, String(8 - r), C.dim);
          for (var c = 0; c < 8; c++) {
            var p = bd[r][c], dark = (r + c) % 2 === 1;
            var bg = (r === cr && c === cc) ? '#2b4a6b'
                   : (r === sr && c === sc) ? '#2f5a33'
                   : (dark ? '#1b1e23' : null);
            var ch = p === '.' ? (dark ? '·' : ' ') : (king(p) ? 'K' : '●');
            t.put(31 + c * 3, 5 + r, ch, mine(p) ? C.cyan : theirs(p) ? C.red : C.grey, bg, true);
          }
        }
        var mv = gen(true);
        t.text(28, 15, mv.length && mv[0].jump ? 'Capture available — you must jump.' : '                                   ', C.yellow);
        if (msg) t.center(17, msg, C.white, true);
      }
    };
  }
});

}());
