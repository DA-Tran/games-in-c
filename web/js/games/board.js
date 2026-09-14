/* board.js - board and strategy ports of src/games/{tictactoe,connect4,
 * reversi,gomoku,checkers,mancala,nim,dots_boxes,battleship,mastermind}.c
 */
(function () {
'use strict';
var G = window.GIC, C = G.COL, reg = G.register, rnd = G.rnd;

/* ------------------------------------------------------------- tic tac toe */
reg('tictactoe', {
  title: 'Tic Tac Toe', help: 'Arrows move · Enter places · Q quits',
  start: function (host, p) {
    var VAR = (p.variant >= 0 && p.variant <= 8) ? p.variant : 0;
    var N = Math.max(3, Math.min(6, p.size > 0 ? p.size : 3));
    var K = Math.max(3, Math.min(N, p.count > 0 ? p.count : 3));
    if (VAR === 8) { N = 3; K = 3; }
    var NINE = (VAR === 6 || VAR === 7);
    var SUB = ['Arrows move, Enter places',
               'MISERE: making a line LOSES',
               'WILD: place either mark; any line wins',
               'ORDER: make a line of either mark; Chaos stops you',
               'TOROIDAL: lines wrap around the edges',
               'NOTAKTO: both play X; making a line LOSES',
               'NINE-BOARD: a line on any board wins',
               'ULTIMATE: win small boards to claim the big one',
               'NUMERICAL: odds are yours; three summing to 15 wins'][VAR];
    var DR = [0,1,1,1], DC = [1,0,1,-1];
    var LINES = [[0,1,2],[3,4,5],[6,7,8],[0,3,6],[1,4,7],[2,5,8],[0,4,8],[2,4,6]];

    var bd, cur, pending, mynum, msg;
    var nb, meta, board, cell, forced;

    function cellAt(r, c) {
      if (VAR === 4) { r = ((r % N) + N) % N; c = ((c % N) + N) % N; }
      else if (r < 0 || r >= N || c < 0 || c >= N) return null;
      return bd[r * N + c];
    }
    function runLen(r, c, d, who) {
      var n = 1, i;
      for (i = 1; i < K; i++) { if (cellAt(r + DR[d]*i, c + DC[d]*i) !== who) break; n++; }
      for (i = 1; i < K; i++) { if (cellAt(r - DR[d]*i, c - DC[d]*i) !== who) break; n++; }
      return n;
    }
    function madeLine(idx) {
      var r = Math.floor(idx / N), c = idx % N, who = bd[idx], d;
      if (who === ' ') return false;
      for (d = 0; d < 4; d++) if (runLen(r, c, d, who) >= K) return true;
      return false;
    }
    function madeFifteen() {
      for (var i = 0; i < 8; i++) {
        var a = bd[LINES[i][0]], b = bd[LINES[i][1]], c = bd[LINES[i][2]];
        if (a === ' ' || b === ' ' || c === ' ') continue;
        if ((+a) + (+b) + (+c) === 15) return true;
      }
      return false;
    }
    function boardFull() { return bd.indexOf(' ') < 0; }
    function heur() {
      var r, c, d, s = 0;
      for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        var w = bd[r * N + c];
        if (w === ' ') continue;
        for (d = 0; d < 4; d++) { var v = Math.pow(runLen(r, c, d, w), 2); s += (w === 'O') ? v : -v; }
      }
      return s;
    }
    function search(aiturn, depth, lim, alpha, beta) {
      var i, best;
      if (depth >= lim || boardFull()) return heur();
      best = aiturn ? -1e6 : 1e6;
      for (i = 0; i < N * N; i++) {
        if (bd[i] !== ' ') continue;
        bd[i] = aiturn ? 'O' : 'X';
        var v;
        if (madeLine(i)) {
          var win = (VAR === 1 || VAR === 5) ? !aiturn : aiturn;
          v = win ? 9000 - depth : depth - 9000;
        } else v = search(!aiturn, depth + 1, lim, alpha, beta);
        bd[i] = ' ';
        if (aiturn) { if (v > best) best = v; if (best > alpha) alpha = best; }
        else        { if (v < best) best = v; if (best < beta)  beta  = best; }
        if (alpha >= beta) break;
      }
      return best;
    }
    function aiSimple() {
      var lim = N <= 3 ? 9 : N === 4 ? 5 : 4;
      var cand = ['O'], i, ci, best = -1e9, mv = -1, mk = 'O';
      if (VAR === 2 || VAR === 3) cand = ['X', 'O'];
      if (VAR === 5) cand = ['X'];
      for (i = 0; i < N * N; i++) {
        if (bd[i] !== ' ') continue;
        for (ci = 0; ci < cand.length; ci++) {
          bd[i] = cand[ci];
          var v;
          if (madeLine(i)) v = (VAR === 1 || VAR === 3 || VAR === 5) ? -9000 : 9000;
          else if (VAR === 2 || VAR === 3) v = heur() + rnd(3);
          else v = search(false, 1, lim, -1e6, 1e6);
          bd[i] = ' ';
          if (v > best) { best = v; mv = i; mk = cand[ci]; }
        }
      }
      return [mv, mk];
    }
    function aiNumerical() {
      var best = -1e9, mv = -1, n, i;
      for (n = 2; n <= 8; n += 2) {
        if (bd.indexOf(String(n)) >= 0) continue;
        for (i = 0; i < 9; i++) {
          if (bd[i] !== ' ') continue;
          bd[i] = String(n);
          var v = madeFifteen() ? 9000 : rnd(50);
          bd[i] = ' ';
          if (v > best) { best = v; mv = i * 16 + n; }
        }
      }
      return mv;
    }
    function smallWinner(b) {
      for (var i = 0; i < 8; i++)
        if (b[LINES[i][0]] !== ' ' && b[LINES[i][0]] === b[LINES[i][1]] &&
            b[LINES[i][1]] === b[LINES[i][2]]) return b[LINES[i][0]];
      return b.indexOf(' ') < 0 ? 'D' : 0;
    }
    function smallFull(b) { return b.indexOf(' ') < 0; }

    function reset() {
      msg = null; pending = 'X'; mynum = 1;
      if (NINE) {
        nb = []; meta = [];
        for (var B = 0; B < 9; B++) { nb.push(new Array(9).fill(' ')); meta.push(0); }
        board = 4; cell = 4; forced = -1;
      } else {
        bd = new Array(N * N).fill(' ');
        cur = Math.floor(N * N / 2);
      }
    }
    reset();

    function keySimple(k) {
      if (k === 'up')    cur = (cur < N) ? cur + N * (N - 1) : cur - N;
      if (k === 'down')  cur = (cur >= N * (N - 1)) ? cur - N * (N - 1) : cur + N;
      if (k === 'left')  cur = (cur % N === 0) ? cur + N - 1 : cur - 1;
      if (k === 'right') cur = (cur % N === N - 1) ? cur - N + 1 : cur + 1;
      if (k === 'tab' && (VAR === 2 || VAR === 3)) pending = pending === 'X' ? 'O' : 'X';
      if (VAR === 8 && /^[13579]$/.test(k)) mynum = +k;
      if (k !== 'enter' && k !== 'space') return;
      if (bd[cur] !== ' ') return;

      if (VAR === 8) {
        if (bd.indexOf(String(mynum)) >= 0) { msg = 'You already used that number.'; return; }
        bd[cur] = String(mynum);
        if (madeFifteen()) { msg = 'Fifteen! You win.'; host.saveScore(100); return; }
      } else {
        bd[cur] = (VAR === 5) ? 'X' : (VAR === 2 || VAR === 3) ? pending : 'X';
        if (madeLine(cur)) {
          var lose = (VAR === 1 || VAR === 5);
          msg = lose ? 'You made a line — you lose.' : 'You win!';
          if (!lose) host.saveScore(100);
          return;
        }
      }
      if (boardFull()) { msg = VAR === 3 ? 'Board full — Chaos wins.' : 'Draw.'; return; }

      if (VAR === 8) {
        var m = aiNumerical();
        if (m >= 0) {
          bd[Math.floor(m / 16)] = String(m % 16);
          if (madeFifteen()) { msg = 'Computer makes fifteen.'; return; }
        }
      } else {
        var res = aiSimple();
        if (res[0] >= 0) {
          bd[res[0]] = res[1];
          if (madeLine(res[0])) {
            var al = (VAR === 1 || VAR === 5);
            msg = al ? 'Computer made a line — you win!' : 'Computer wins.';
            if (al) host.saveScore(100);
            return;
          }
        }
      }
      if (boardFull()) msg = VAR === 3 ? 'Board full — Chaos wins.' : 'Draw.';
    }

    function keyNine(k) {
      if (k === 'up')    cell = (cell < 3) ? cell + 6 : cell - 3;
      if (k === 'down')  cell = (cell > 5) ? cell - 6 : cell + 3;
      if (k === 'left')  cell = (cell % 3 === 0) ? cell + 2 : cell - 1;
      if (k === 'right') cell = (cell % 3 === 2) ? cell - 2 : cell + 1;
      if (k === 'tab' && forced < 0) board = (board + 1) % 9;
      if (k !== 'enter' && k !== 'space') return;
      if (forced >= 0 && board !== forced) return;
      if (nb[board][cell] !== ' ') return;
      if (VAR === 7 && meta[board]) return;

      nb[board][cell] = 'X';
      var w = smallWinner(nb[board]);
      if (VAR === 6 && w === 'X') { msg = 'Line on a board — you win!'; host.saveScore(100); return; }
      if (VAR === 7) {
        if (w) meta[board] = w;
        if (smallWinner(meta) === 'X') { msg = 'You win the big board!'; host.saveScore(200); return; }
      }
      forced = cell;
      if (smallFull(nb[forced]) || (VAR === 7 && meta[forced])) forced = -1;

      var tb = forced, i;
      if (tb < 0) for (i = 0; i < 9; i++) if (!smallFull(nb[i]) && !(VAR === 7 && meta[i])) { tb = i; break; }
      if (tb < 0) { msg = 'All boards full — draw.'; return; }
      var best = -1, bc = -1;
      for (i = 0; i < 9; i++) {
        if (nb[tb][i] !== ' ') continue;
        nb[tb][i] = 'O'; var s = (smallWinner(nb[tb]) === 'O') ? 1000 : 0;
        nb[tb][i] = 'X'; if (smallWinner(nb[tb]) === 'X') s += 500;
        nb[tb][i] = ' ';
        s += (i === 4) ? 8 : (i % 2 === 0) ? 4 : 1;
        s += rnd(3);
        if (s > best) { best = s; bc = i; }
      }
      if (bc < 0) { msg = 'No moves left — draw.'; return; }
      nb[tb][bc] = 'O';
      w = smallWinner(nb[tb]);
      if (VAR === 6 && w === 'O') { msg = 'Computer made a line.'; return; }
      if (VAR === 7) {
        if (w) meta[tb] = w;
        if (smallWinner(meta) === 'O') { msg = 'Computer wins the big board.'; return; }
      }
      forced = bc;
      if (smallFull(nb[forced]) || (VAR === 7 && meta[forced])) forced = -1;
      board = (forced >= 0) ? forced : tb;
    }

    return {
      key: function (k) { if (msg) { reset(); return; } if (NINE) keyNine(k); else keySimple(k); },
      draw: function (t) {
        t.header(NINE ? 'NINE BOARD' : 'TIC TAC TOE', SUB);
        if (NINE) {
          for (var B = 0; B < 9; B++) {
            var br = Math.floor(B / 3) * 5 + 5, bc2 = (B % 3) * 14 + 18;
            var active = (forced < 0 || forced === B);
            t.text(bc2, br - 1, '#' + (B + 1), active ? C.white : C.grey, null, B === board);
            for (var r = 0; r < 3; r++) for (var c = 0; c < 3; c++) {
              var i = r * 3 + c, ch = nb[B][i], col;
              if (VAR === 7 && meta[B] && meta[B] !== 'D') col = meta[B] === 'X' ? C.cyan : C.yellow;
              else col = ch === 'X' ? C.cyan : ch === 'O' ? C.yellow : C.grey;
              t.put(bc2 + c * 2, br + r, ch === ' ' ? '.' : ch, col,
                    (B === board && i === cell) ? '#2b4a6b' : null);
            }
          }
          t.text(18, 22, msg ? msg : forced >= 0 ? 'You must play in board #' + (forced + 1) + '.   '
                                                 : 'Play in any open board.        ',
                 C.white, null, !!msg);
        } else {
          var left = Math.max(2, 40 - N * 2);
          for (var rr = 0; rr < N; rr++) for (var cc = 0; cc < N; cc++) {
            var idx = rr * N + cc, v = bd[idx];
            var col2 = v === 'X' ? C.cyan : v === 'O' ? C.yellow : v === ' ' ? C.grey : C.green;
            t.text(left + cc * 4, 5 + rr * 2, ' ' + (v === ' ' ? '.' : v) + ' ', col2,
                   idx === cur ? '#2b4a6b' : null, v !== ' ');
          }
          if (VAR === 2 || VAR === 3)
            t.text(left, 7 + N * 2, 'Placing: ' + pending + '  (Tab switches)   ', C.white);
          if (VAR === 8)
            t.text(left, 7 + N * 2, 'Your number: ' + mynum + '  (press 1,3,5,7,9)   ', C.white);
          if (msg) t.text(left, 9 + N * 2, msg + ' Press any key.', C.white, null, true);
        }
      }
    };
  }
});

/* ----------------------------------------------------------- connect four */
reg('connect', {
  title: 'Connect', help: 'Left/Right aim · Enter drops · Tab pop-out · Q quits',
  start: function (host, p) {
    var W = Math.max(4, Math.min(13, p.width  > 0 ? p.width  : 7));
    var H = Math.max(4, Math.min(10, p.height > 0 ? p.height : 6));
    var K = Math.max(3, p.count > 0 ? p.count : 4);
    if (K > W && K > H) K = Math.min(W, H);
    var POP = p.variant === 1;
    var DR = [0,1,1,1], DC = [1,0,1,-1];
    var bd, cur, popmode, msg;

    function reset() {
      bd = [];
      for (var r = 0; r < H; r++) bd.push(new Array(W).fill(' '));
      cur = Math.floor(W / 2); popmode = false; msg = null;
    }
    function drop(col, pl) {
      for (var r = H - 1; r >= 0; r--) if (bd[r][col] === ' ') { bd[r][col] = pl; return r; }
      return -1;
    }
    /* Removing your bottom disc slides the whole column down one. */
    function popout(col) {
      for (var r = H - 1; r > 0; r--) bd[r][col] = bd[r - 1][col];
      bd[0][col] = ' ';
    }
    function wins(pl) {
      for (var r = 0; r < H; r++) for (var c = 0; c < W; c++) {
        if (bd[r][c] !== pl) continue;
        for (var d = 0; d < 4; d++) {
          var rr = r + DR[d] * (K - 1), cc = c + DC[d] * (K - 1), i;
          if (rr < 0 || rr >= H || cc < 0 || cc >= W) continue;
          for (i = 1; i < K; i++) if (bd[r + DR[d]*i][c + DC[d]*i] !== pl) break;
          if (i === K) return true;
        }
      }
      return false;
    }
    function full() { for (var c = 0; c < W; c++) if (bd[0][c] === ' ') return false; return true; }
    function window4(r, c, dr, dc) {
      var me = 0, op = 0, sp = 0;
      for (var i = 0; i < K; i++) {
        var v = bd[r + dr*i][c + dc*i];
        if (v === 'O') me++; else if (v === 'X') op++; else sp++;
      }
      if (me && op) return 0;
      if (me && sp) return me * me * 4;
      if (op && sp) return -(op * op * 5);
      return 0;
    }
    function evaluate() {
      var s = 0;
      for (var r = 0; r < H; r++) for (var c = 0; c < W; c++) for (var d = 0; d < 4; d++) {
        var rr = r + DR[d] * (K - 1), cc = c + DC[d] * (K - 1);
        if (rr < 0 || rr >= H || cc < 0 || cc >= W) continue;
        s += window4(r, c, DR[d], DC[d]);
      }
      for (var r2 = 0; r2 < H; r2++)
        s += bd[r2][W >> 1] === 'O' ? 6 : bd[r2][W >> 1] === 'X' ? -6 : 0;
      return s;
    }
    function negamax(depth, alpha, beta, ai) {
      if (wins('O')) return 100000 + depth;
      if (wins('X')) return -100000 - depth;
      if (full() || depth === 0) return evaluate();
      var best = ai ? -1e6 : 1e6;
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
    function aiMove() {
      /* Bigger boards get a shallower search so the reply stays quick. */
      var depth = (W * H <= 42) ? 5 : (W * H <= 72) ? 4 : 3;
      var best = -1e9, mv = 0, c;
      for (c = 0; c < W; c++) {
        if (bd[0][c] !== ' ') continue;
        var r = drop(c, 'O');
        var v = negamax(depth - 1, -1e6, 1e6, false) - (c === (W >> 1) ? 0 : 1);
        bd[r][c] = ' ';
        if (v > best) { best = v; mv = c; }
      }
      if (POP) for (c = 0; c < W; c++) {
        if (bd[H - 1][c] !== 'O') continue;
        var save = bd.map(function (row) { return row.slice(); });
        popout(c);
        var v2 = wins('O') ? 200000 : wins('X') ? -200000 : negamax(depth - 1, -1e6, 1e6, false);
        bd = save;
        if (v2 > best) { best = v2; mv = -(c + 1); }
      }
      return mv;
    }
    reset();
    return {
      key: function (k) {
        if (msg) { reset(); return; }
        if (k === 'left'  && cur > 0)     cur--;
        if (k === 'right' && cur < W - 1) cur++;
        if (k === 'tab' && POP) popmode = !popmode;
        if (k !== 'enter' && k !== 'space') return;

        if (popmode) { if (bd[H - 1][cur] !== 'X') return; popout(cur); }
        else { if (bd[0][cur] !== ' ') return; drop(cur, 'X'); }

        /* A pop-out can complete a line for both sides at once. */
        if (wins('X') && wins('O')) { msg = 'Both complete — draw.'; return; }
        if (wins('X')) { msg = 'You win!'; host.saveScore(100); return; }
        if (wins('O')) { msg = 'Computer wins.'; return; }
        if (full() && !POP) { msg = 'Draw.'; return; }

        var m = aiMove();
        if (m < 0) popout(-m - 1); else drop(m, 'O');
        if (wins('O') && wins('X')) msg = 'Both complete — draw.';
        else if (wins('O')) msg = 'Computer wins.';
        else if (wins('X')) { msg = 'You win!'; host.saveScore(100); }
        else if (full() && !POP) msg = 'Draw.';
      },
      draw: function (t) {
        t.header('CONNECT ' + K, (POP ? 'pop-out allowed · Tab toggles · ' : '') +
                 W + 'x' + H + ' · Left/Right aim, Enter ' + (popmode ? 'POPS' : 'drops'));
        var left = Math.max(1, 40 - W);
        for (var c = 0; c < W; c++) t.put(left + c * 2, 4, ' ', C.grey);
        t.put(left + cur * 2, 4, popmode ? '^' : 'v', popmode ? C.red : C.cyan, null, true);
        for (var r = 0; r < H; r++) for (var c2 = 0; c2 < W; c2++) {
          var v = bd[r][c2];
          t.put(left + c2 * 2, 5 + r, v === ' ' ? '.' : 'O',
                v === 'X' ? C.cyan : v === 'O' ? C.yellow : C.grey, null, v !== ' ');
        }
        if (msg) t.text(left, 7 + H, msg + ' Press any key.', C.white, null, true);
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
    var VAR = (p.variant >= 0 && p.variant <= 5) ? p.variant : 0;
    var N = Math.max(8, Math.min(12, p.size > 0 ? p.size : 10));
    var LEN = [5, 4, 3, 3, 2];
    var SUB = ['Sink the enemy fleet', 'SALVO: one shot per surviving ship',
               'Sink the enemy fleet', 'MOVING: undamaged ships redeploy',
               'FOG: results stay hidden until a ship sinks',
               'Sink the enemy fleet'][VAR];
    var you, cpu, cr, cc, queue, msg, turns, shotsLeft;

    function blank() {
      var g = [];
      for (var r = 0; r < N; r++) g.push(new Array(N).fill(0));
      return g;
    }
    function place(g) {
      for (var s = 0; s < LEN.length; s++) {
        for (var guard = 0; guard < 500; guard++) {
          var horiz = rnd(2), r = rnd(N), c = rnd(N), ok = true, i;
          if (horiz ? c + LEN[s] > N : r + LEN[s] > N) continue;
          for (i = 0; i < LEN[s]; i++) if (g[r + (horiz ? 0 : i)][c + (horiz ? i : 0)]) { ok = false; break; }
          if (!ok) continue;
          for (i = 0; i < LEN[s]; i++) g[r + (horiz ? 0 : i)][c + (horiz ? i : 0)] = 1;
          break;
        }
      }
    }
    function remaining(g) {
      var n = 0;
      for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) if (g[r][c] === 1) n++;
      return n;
    }
    /* Roughly how many hulls are still afloat — salvo size comes from this. */
    function shipCount(g) {
      var n = remaining(g);
      return n > 5 ? 5 : (n > 0 ? Math.ceil(n / 3) : 0);
    }
    /* Moving-ships mode lifts every unhit hull and redeploys it. */
    function redeploy(g) {
      var intact = 0, r, c, s, i;
      for (r = 0; r < N; r++) for (c = 0; c < N; c++) if (g[r][c] === 1) { g[r][c] = 0; intact++; }
      for (s = 0; s < LEN.length && intact > 0; s++) {
        if (LEN[s] > intact) continue;
        for (var tries = 0; tries < 200; tries++) {
          var horiz = rnd(2), r2 = rnd(N), c2 = rnd(N), ok = true;
          if (horiz ? c2 + LEN[s] > N : r2 + LEN[s] > N) continue;
          for (i = 0; i < LEN[s]; i++) if (g[r2 + (horiz ? 0 : i)][c2 + (horiz ? i : 0)]) { ok = false; break; }
          if (!ok) continue;
          for (i = 0; i < LEN[s]; i++) g[r2 + (horiz ? 0 : i)][c2 + (horiz ? i : 0)] = 1;
          intact -= LEN[s];
          break;
        }
      }
    }
    function pushTarget(r, c) {
      if (r < 0 || r >= N || c < 0 || c >= N) return;
      if (queue.length < 8) queue.push([r, c]);
    }
    function cpuShot() {
      var r, c, guard = 0;
      for (;;) {
        if (++guard > 4000) return;
        if (queue.length) { var q = queue.pop(); r = q[0]; c = q[1]; }
        else {
          r = rnd(N); c = rnd(N);
          /* Parity search: nothing shorter than two hides on one colour. */
          if ((r + c) % 2) continue;
        }
        if (you[r][c] === 2 || you[r][c] === 3) continue;
        break;
      }
      if (you[r][c] === 1) {
        you[r][c] = 3;
        pushTarget(r-1, c); pushTarget(r+1, c); pushTarget(r, c-1); pushTarget(r, c+1);
      } else you[r][c] = 2;
    }
    function newVolley() { shotsLeft = (VAR === 1) ? Math.max(1, shipCount(you)) : 1; }
    function reset() {
      you = blank(); cpu = blank();
      place(you); place(cpu);
      cr = 0; cc = 0; queue = []; msg = null; turns = 0;
      newVolley();
    }
    reset();
    return {
      key: function (k) {
        if (msg) { reset(); return; }
        if (k === 'up'    && cr > 0)     cr--;
        if (k === 'down'  && cr < N - 1) cr++;
        if (k === 'left'  && cc > 0)     cc--;
        if (k === 'right' && cc < N - 1) cc++;
        if (k !== 'enter' && k !== 'space') return;
        if (cpu[cr][cc] === 2 || cpu[cr][cc] === 3) return;

        cpu[cr][cc] = (cpu[cr][cc] === 1) ? 3 : 2;
        shotsLeft--;
        if (remaining(cpu) === 0) {
          msg = 'Enemy fleet destroyed — you win!';
          host.saveScore(remaining(you) * 100);
          return;
        }
        if (shotsLeft > 0) return;

        var salvo = (VAR === 1) ? Math.max(1, shipCount(cpu)) : 1;
        for (var s = 0; s < salvo; s++) {
          cpuShot();
          if (remaining(you) === 0) break;
        }
        if (remaining(you) === 0) { msg = 'Your fleet is lost — computer wins.'; return; }

        if (VAR === 3 && ++turns % 4 === 0) { redeploy(cpu); redeploy(you); queue = []; }
        newVolley();
      },
      draw: function (t) {
        t.header('BATTLESHIP', SUB);
        var right = 10 + N * 3 + 8;
        function grid(left, g, hide) {
          var hdr = '   ', c;
          for (c = 0; c < N; c++) hdr += String(c % 10) + '  ';
          t.text(left, 4, hdr, C.dim);
          for (var r = 0; r < N; r++) {
            t.text(left, 5 + r, String.fromCharCode(65 + r) + ' ', C.dim);
            for (c = 0; c < N; c++) {
              var v = g[r][c], sel = (!hide ? false : (cr === r && cc === c));
              var ch, fg;
              if (VAR === 4 && hide && (v === 2 || v === 3)) { ch = '?'; fg = C.magenta; }
              else if (v === 3) { ch = 'X'; fg = C.red; }
              else if (v === 2) { ch = 'o'; fg = C.grey; }
              else if (v === 1 && !hide) { ch = '#'; fg = C.cyan; }
              else { ch = '.'; fg = C.blue; }
              t.text(left + 2 + c * 3, 5 + r, ' ' + ch + ' ', fg, sel ? '#2b4a6b' : null, v === 3);
            }
          }
        }
        t.text(10, 3, 'YOUR FLEET', C.cyan, null, true);
        grid(10, you, false);
        t.text(right, 3, 'ENEMY WATERS', C.red, null, true);
        grid(right, cpu, true);
        t.text(10, 6 + N, 'Ships afloat: ' + remaining(you) + '    ', C.fg);
        t.text(right, 6 + N, 'Enemy afloat: ' + remaining(cpu) + '    ', C.fg);
        if (VAR === 1) t.text(10, 7 + N, 'Shots left this volley: ' + shotsLeft + '   ', C.yellow);
        if (msg) t.center(9 + N, msg + ' Press any key.', C.white, true);
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

/* --------------------------------------------------------------- draughts */
reg('checkers', {
  title: 'Draughts', help: 'Arrows move · Enter selects then targets · Q quits',
  start: function (host, p) {
    /* back, fly, maxcap, dirs(0 diag/1 orth/2 both), sideMove, menTakeKings,
     * giveaway, promoteContinues, name */
    var RULES = [
      [0,0,0,0,0,1,0,0,'Checkers'],
      [1,1,1,0,0,1,0,0,'International Draughts'],
      [1,1,0,0,0,1,0,1,'Russian Draughts'],
      [1,1,1,0,0,1,0,0,'Brazilian Draughts'],
      [0,1,1,1,1,1,0,0,'Turkish Draughts'],
      [0,0,1,0,0,0,0,0,'Italian Draughts'],
      [0,1,1,0,0,1,0,0,'Spanish Draughts'],
      [1,1,0,0,0,1,0,0,'Pool Checkers'],
      [1,0,0,0,0,1,1,0,'Suicide Checkers'],
      [1,1,1,2,0,1,0,0,'Frisian Draughts'],
      [1,1,1,1,1,1,0,0,'Armenian Draughts'],
      [1,1,1,0,0,1,0,0,'Canadian Checkers'],
      [0,1,1,1,0,1,0,0,'Dameo']];
    var v = (p.variant >= 0 && p.variant <= 12) ? p.variant : 0;
    var R = RULES[v];
    var BACK = R[0], FLY = R[1], MAXCAP = R[2], DIRS = R[3], SIDE = R[4],
        MTK = R[5], GIVE = R[6], PROMC = R[7], RNAME = R[8];
    var N = Math.max(8, Math.min(12, p.size > 0 ? p.size : 8));
    if (N & 1) N++;

    var DR8 = [-1,-1,1,1,-1,1,0,0], DC8 = [-1,1,-1,1,0,0,-1,1];
    var dlo = DIRS === 1 ? 4 : 0, dhi = DIRS === 0 ? 4 : 8;
    var bd, cr, cc, sr, sc, msg;

    function mine(x)   { return x === 'x' || x === 'X'; }
    function theirs(x) { return x === 'o' || x === 'O'; }
    function king(x)   { return x === 'X' || x === 'O'; }
    function on(r, c)  { return r >= 0 && r < N && c >= 0 && c < N; }

    function manMay(human, d, capturing) {
      var dr = DR8[d];
      var fwd = human ? dr < 0 : dr > 0;
      if (fwd) return true;
      if (dr === 0) return !!SIDE;
      return capturing ? !!BACK : false;
    }
    function enemyAt(human, r, c) {
      var x = bd[r][c];
      if (human ? !theirs(x) : !mine(x)) return false;
      if (!MTK && king(x)) return false;      /* Italian: men spare kings */
      return true;
    }
    function capsAt(r, c, human) {
      var pc = bd[r][c], out = [], d;
      if (human ? !mine(pc) : !theirs(pc)) return out;
      var isK = king(pc);
      for (d = dlo; d < dhi; d++) {
        var dr = DR8[d], dc = DC8[d];
        if (!isK && !manMay(human, d, true)) continue;
        if (isK && FLY) {
          var i = 1;
          while (on(r + dr*i, c + dc*i) && bd[r + dr*i][c + dc*i] === '.') i++;
          var tr = r + dr*i, tc = c + dc*i;
          if (!on(tr, tc) || !enemyAt(human, tr, tc)) continue;
          i++;
          while (on(r + dr*i, c + dc*i) && bd[r + dr*i][c + dc*i] === '.') {
            out.push({r1:r,c1:c,r2:r+dr*i,c2:c+dc*i,cr:tr,cc:tc,jump:1});
            i++;
          }
        } else {
          var r1 = r+dr, c1 = c+dc, r2 = r+2*dr, c2 = c+2*dc;
          if (!on(r2, c2) || bd[r2][c2] !== '.') continue;
          if (!enemyAt(human, r1, c1)) continue;
          out.push({r1:r,c1:c,r2:r2,c2:c2,cr:r1,cc:c1,jump:1});
        }
      }
      return out;
    }
    function doMove(m, human, promote) {
      var pc = bd[m.r1][m.c1];
      bd[m.r1][m.c1] = '.';
      if (m.jump) bd[m.cr][m.cc] = '.';
      if (promote) {
        if (human && m.r2 === 0) pc = 'X';
        if (!human && m.r2 === N - 1) pc = 'O';
      }
      bd[m.r2][m.c2] = pc;
    }
    function snapshot() { return bd.map(function (x) { return x.slice(); }); }
    function chainFrom(r, c, human, depth) {
      if (depth > 12) return 0;
      var mv = capsAt(r, c, human), best = 0;
      for (var i = 0; i < mv.length; i++) {
        var save = snapshot();
        doMove(mv[i], human, false);
        var t = 1 + chainFrom(mv[i].r2, mv[i].c2, human, depth + 1);
        bd = save;
        if (t > best) best = t;
      }
      return best;
    }
    function genMoves(human) {
      var out = [], r, c, d, jumps = false;
      for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        var got = capsAt(r, c, human);
        if (got.length) { jumps = true; out = out.concat(got); }
      }
      if (jumps) {
        if (MAXCAP) {
          var len = [], best = 0;
          for (var i = 0; i < out.length; i++) {
            var save = snapshot();
            doMove(out[i], human, false);
            len[i] = 1 + chainFrom(out[i].r2, out[i].c2, human, 0);
            bd = save;
            if (len[i] > best) best = len[i];
          }
          out = out.filter(function (_, i) { return len[i] === best; });
        }
        return out;                        /* captures are compulsory */
      }
      for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        var pc = bd[r][c], isK = king(pc);
        if (human ? !mine(pc) : !theirs(pc)) continue;
        for (d = dlo; d < dhi; d++) {
          var dr = DR8[d], dc = DC8[d];
          if (!isK && !manMay(human, d, false)) continue;
          if (isK && FLY) {
            for (var i2 = 1; on(r+dr*i2, c+dc*i2) && bd[r+dr*i2][c+dc*i2] === '.'; i2++)
              out.push({r1:r,c1:c,r2:r+dr*i2,c2:c+dc*i2,jump:0});
          } else {
            var r1 = r+dr, c1 = c+dc;
            if (on(r1, c1) && bd[r1][c1] === '.') out.push({r1:r,c1:c,r2:r1,c2:c1,jump:0});
          }
        }
      }
      return out;
    }
    function applyMove(m, human) {
      var before = bd[m.r1][m.c1];
      doMove(m, human, true);
      var promoted = !king(before) && king(bd[m.r2][m.c2]);
      if (m.jump) {
        if (promoted && !PROMC) return false;
        if (capsAt(m.r2, m.c2, human).length) return true;
      }
      return false;
    }
    function material() {
      var s = 0;
      for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) {
        var x = bd[r][c];
        if (x === 'o') s += 10 + r;
        else if (x === 'O') s += 28;
        else if (x === 'x') s -= 10 + (N - 1 - r);
        else if (x === 'X') s -= 28;
      }
      return GIVE ? -s : s;                /* giveaway wants to be taken */
    }
    function aiTurn() {
      for (var guard = 0; guard < 24; guard++) {
        var mv = genMoves(false);
        if (!mv.length) return;
        var best = -1e9, bi = 0;
        for (var i = 0; i < mv.length; i++) {
          var save = snapshot();
          applyMove(mv[i], false);
          var val = material() + (mv[i].jump ? (GIVE ? -30 : 40) : 0) + rnd(3);
          bd = save;
          if (val > best) { best = val; bi = i; }
        }
        if (!applyMove(mv[bi], false)) return;
      }
    }
    function reset() {
      var rows = N === 8 ? 3 : N === 10 ? 4 : 5;
      bd = [];
      for (var r = 0; r < N; r++) {
        var row = [];
        for (var c = 0; c < N; c++) {
          /* Orthogonal games fill whole rows; diagonal games use dark squares. */
          var usable = (DIRS === 1) ? (r !== 0 && r !== N - 1) : ((r + c) % 2 === 1);
          if (!usable) row.push('.');
          else if (r < rows) row.push('o');
          else if (r >= N - rows) row.push('x');
          else row.push('.');
        }
        bd.push(row);
      }
      cr = N - 3; cc = 0; sr = -1; sc = -1; msg = null;
    }
    reset();
    return {
      key: function (k) {
        if (msg) { reset(); return; }
        var mv = genMoves(true);
        if (!mv.length) { msg = GIVE ? 'No moves left — you win!' : 'No moves — computer wins.'; return; }
        if (k === 'up'    && cr > 0)     cr--;
        if (k === 'down'  && cr < N - 1) cr++;
        if (k === 'left'  && cc > 0)     cc--;
        if (k === 'right' && cc < N - 1) cc++;
        if (k !== 'enter' && k !== 'space') return;
        if (sr < 0) { if (mine(bd[cr][cc])) { sr = cr; sc = cc; } return; }
        if (sr === cr && sc === cc) { sr = sc = -1; return; }
        var found = -1;
        for (var i = 0; i < mv.length; i++)
          if (mv[i].r1 === sr && mv[i].c1 === sc && mv[i].r2 === cr && mv[i].c2 === cc) found = i;
        if (found < 0) return;
        if (applyMove(mv[found], true)) { sr = cr; sc = cc; return; }
        sr = sc = -1;
        aiTurn();
        var m2 = 0, t2 = 0;
        for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) {
          if (mine(bd[r][c])) m2++;
          if (theirs(bd[r][c])) t2++;
        }
        if (GIVE) {
          if (m2 === 0) { msg = 'All gone — you win!'; host.saveScore(100); }
          else if (t2 === 0) msg = 'Computer shed everything first.';
        } else {
          if (t2 === 0 || !genMoves(false).length) { msg = 'You win!'; host.saveScore(100); }
          else if (m2 === 0) msg = 'Computer wins.';
        }
      },
      draw: function (t) {
        t.header('DRAUGHTS', RNAME + ' — ' +
                 (DIRS === 1 ? 'orthogonal' : DIRS === 2 ? 'orth+diagonal' : 'diagonal') +
                 (FLY ? ', flying kings' : '') + (MAXCAP ? ', must take the most' : '') +
                 (GIVE ? ', LOSE everything to win' : ''));
        var left = Math.max(2, 40 - (N * 3) / 2);
        for (var r = 0; r < N; r++) {
          t.text(left - 4, 4 + r, String(N - r).padStart(2, ' '), C.dim);
          for (var c = 0; c < N; c++) {
            var pc = bd[r][c], dark = ((r + c) % 2) === 1;
            var bg = (r === cr && c === cc) ? '#2b4a6b'
                   : (r === sr && c === sc) ? '#2f6b2f'
                   : (dark ? '#1c1f24' : null);
            var fg = mine(pc) ? C.cyan : theirs(pc) ? C.red : C.grey;
            var g = pc === '.' ? (dark ? '.' : ' ') : (king(pc) ? 'K' : 'o');
            t.text(left + c * 3, 4 + r, ' ' + g + ' ', fg, bg, king(pc));
          }
        }
        if (msg) t.text(left - 4, 6 + N, msg + ' Press any key.', C.white, null, true);
      }
    };
  }
});

})();
