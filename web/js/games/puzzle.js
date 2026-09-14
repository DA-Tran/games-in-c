/* puzzle.js - parameterised grid-puzzle families, mirroring src/games/*.c.
 * Each start(host, p) reads the same GParams fields as its C counterpart.
 */
(function () {
'use strict';
var G = window.GIC, C = G.COL, reg = G.register, rnd = G.rnd, shuffle = G.shuffle;
function ival(v, d) { return (v && v > 0) ? v : d; }

/* ------------------------------------------------------------ minesweeper */
reg('minesweeper', {
  title: 'Minesweeper', help: 'Arrows move · Enter reveals · F flags · Q quits',
  start: function (host, p) {
    var W = Math.min(40, ival(p.width, 16)), H = Math.min(20, ival(p.height, 16));
    var MINES = Math.min(W * H - 10, ival(p.count, 40));
    var VAR = p.variant || 0;
    var VNAME = ['Classic', 'No Guess', 'Wrap', 'Knight'];
    var NCOL = [C.grey, C.blue, C.green, C.red, C.magenta, C.yellow, C.cyan, C.white, C.white];
    var mine, shown, flag, cr, cc, first, dead, won, t0;

    function neigh(r, c) {
      var out = [], KR = [-2,-2,-1,-1,1,1,2,2], KC = [-1,1,-2,2,-2,2,-1,1], i, dr, dc, a, b;
      if (VAR === 3) {
        for (i = 0; i < 8; i++) { a = r + KR[i]; b = c + KC[i]; push(a, b); }
      } else {
        for (dr = -1; dr <= 1; dr++) for (dc = -1; dc <= 1; dc++) {
          if (!dr && !dc) continue;
          push(r + dr, c + dc);
        }
      }
      function push(a2, b2) {
        if (VAR === 2) { a2 = (a2 + H) % H; b2 = (b2 + W) % W; }
        else if (a2 < 0 || a2 >= H || b2 < 0 || b2 >= W) return;
        out.push([a2, b2]);
      }
      return out;
    }
    function count(r, c) {
      return neigh(r, c).filter(function (n) { return mine[n[0]][n[1]]; }).length;
    }
    function lay(sr, sc) {
      var placed = 0;
      for (var r = 0; r < H; r++) mine[r] = new Array(W).fill(0);
      while (placed < MINES) {
        var a = rnd(H), b = rnd(W);
        if (mine[a][b]) continue;
        if (a >= sr - 1 && a <= sr + 1 && b >= sc - 1 && b <= sc + 1) continue;
        mine[a][b] = 1; placed++;
      }
    }
    function reveal(r, c) {
      if (r < 0 || r >= H || c < 0 || c >= W) return;
      if (shown[r][c] || flag[r][c]) return;
      shown[r][c] = 1;
      if (mine[r][c] || count(r, c) !== 0) return;
      neigh(r, c).forEach(function (n) { reveal(n[0], n[1]); });
    }
    /* Single-point solver, used to reject boards that need a guess. */
    function solvable(sr, sc) {
      var progress = true, r, c;
      for (r = 0; r < H; r++) { shown[r] = new Array(W).fill(0); flag[r] = new Array(W).fill(0); }
      reveal(sr, sc);
      while (progress) {
        progress = false;
        for (r = 0; r < H; r++) for (c = 0; c < W; c++) {
          if (!shown[r][c] || mine[r][c]) continue;
          var ns = neigh(r, c);
          var flags = ns.filter(function (n) { return flag[n[0]][n[1]]; }).length;
          var hid = ns.filter(function (n) { return !shown[n[0]][n[1]] && !flag[n[0]][n[1]]; });
          if (!hid.length) continue;
          var n0 = count(r, c);
          if (n0 - flags === hid.length) {
            hid.forEach(function (n) { flag[n[0]][n[1]] = 1; });
            progress = true;
          } else if (n0 === flags) {
            hid.forEach(function (n) { reveal(n[0], n[1]); });
            progress = true;
          }
        }
      }
      for (r = 0; r < H; r++) for (c = 0; c < W; c++)
        if (!mine[r][c] && !shown[r][c]) return false;
      return true;
    }
    function cleared() {
      var n = 0;
      for (var r = 0; r < H; r++) for (var c = 0; c < W; c++) if (shown[r][c]) n++;
      return n === W * H - MINES;
    }
    function reset() {
      mine = []; shown = []; flag = [];
      for (var r = 0; r < H; r++) {
        mine.push(new Array(W).fill(0));
        shown.push(new Array(W).fill(0));
        flag.push(new Array(W).fill(0));
      }
      cr = H >> 1; cc = W >> 1;
      first = true; dead = false; won = false; t0 = Date.now();
    }
    reset();
    return {
      key: function (k) {
        if (dead || won) { reset(); return; }
        if (k === 'up' && cr > 0) cr--;
        if (k === 'down' && cr < H - 1) cr++;
        if (k === 'left' && cc > 0) cc--;
        if (k === 'right' && cc < W - 1) cc++;
        if (k === 'f') { if (!shown[cr][cc]) flag[cr][cc] ^= 1; return; }
        if (k !== 'enter' && k !== 'space') return;
        if (flag[cr][cc]) return;
        if (first) {
          var tries = 0;
          do { lay(cr, cc); tries++; } while (VAR === 1 && tries < 200 && !solvable(cr, cc));
          for (var r = 0; r < H; r++) {
            shown[r] = new Array(W).fill(0);
            flag[r] = new Array(W).fill(0);
          }
          first = false;
        }
        if (mine[cr][cc]) dead = true;
        else {
          reveal(cr, cc);
          if (cleared()) { won = true; host.saveScore(Math.floor(MINES * 200 / (1 + (Date.now() - t0) / 1000))); }
        }
      },
      draw: function (t) {
        t.header('MINESWEEPER', W + 'x' + H + ' ' + VNAME[VAR] + ', ' + MINES +
                 ' mines · arrows · Enter reveals · F flags');
        var left = Math.max(1, 42 - W);
        for (var r = 0; r < H; r++) for (var c = 0; c < W; c++) {
          var sel = r === cr && c === cc, bg = sel ? '#2b4a6b' : null, ch, fg = C.grey;
          if (flag[r][c] && !shown[r][c]) { ch = 'F'; fg = C.red; }
          else if (!shown[r][c] && !dead) ch = '.';
          else if (mine[r][c]) { ch = '*'; fg = C.red; }
          else if (!shown[r][c]) ch = '.';
          else { var n = count(r, c); ch = n ? String(n) : ' '; fg = NCOL[Math.min(8, n)]; }
          t.put(left + c * 2, 4 + r, ch, fg, bg, true);
        }
        t.text(left, H + 5, 'Mines ' + MINES + '   ', C.fg);
        if (dead) t.center(H + 7, 'BOOM — press any key.', C.red, true);
        if (won) t.center(H + 7, 'Field cleared! Press any key.', C.green, true);
      }
    };
  }
});

/* ----------------------------------------------------------------- sudoku */
reg('sudoku', {
  title: 'Sudoku', help: 'Arrows move · digits enter · 0 clears · H hints · Q quits',
  start: function (host, p) {
    var N = ival(p.size, 9);
    if ([4, 6, 9, 12, 16].indexOf(N) < 0) N = 9;
    var VAR = p.variant || 0;
    if (VAR === 5 && N !== 9) VAR = 0;
    var VNAME = ['Classic', 'Diagonal', 'Even-Odd', 'Consecutive', 'Anti-Knight', 'Windoku'];
    var BW = N === 4 ? 2 : N === 6 ? 3 : N === 12 ? 4 : N === 16 ? 4 : 3;
    var BH = N === 4 ? 2 : N === 6 ? 2 : N === 12 ? 3 : N === 16 ? 4 : 3;
    var HOLE = [0, 35, 44, 50, 56, 62][Math.max(1, Math.min(5, p.difficulty || 3))];
    var sol, grid, given, cr = 0, cc = 0, hints, done, t0;

    function wreg(r, c) {
      if (N !== 9) return -1;
      if (r >= 1 && r <= 3 && c >= 1 && c <= 3) return 0;
      if (r >= 1 && r <= 3 && c >= 5 && c <= 7) return 1;
      if (r >= 5 && r <= 7 && c >= 1 && c <= 3) return 2;
      if (r >= 5 && r <= 7 && c >= 5 && c <= 7) return 3;
      return -1;
    }
    function ok(g, r, c, v) {
      var i, j;
      for (i = 0; i < N; i++) {
        if (i !== c && g[r][i] === v) return false;
        if (i !== r && g[i][c] === v) return false;
      }
      var br = Math.floor(r / BH) * BH, bc = Math.floor(c / BW) * BW;
      for (i = br; i < br + BH; i++) for (j = bc; j < bc + BW; j++)
        if ((i !== r || j !== c) && g[i][j] === v) return false;
      if (VAR === 1) {
        if (r === c) for (i = 0; i < N; i++) if (i !== r && g[i][i] === v) return false;
        if (r + c === N - 1) for (i = 0; i < N; i++) if (i !== r && g[i][N-1-i] === v) return false;
      }
      if (VAR === 4) {
        var KR = [-2,-2,-1,-1,1,1,2,2], KC = [-1,1,-2,2,-2,2,-1,1];
        for (i = 0; i < 8; i++) {
          var a = r + KR[i], b = c + KC[i];
          if (a >= 0 && a < N && b >= 0 && b < N && g[a][b] === v) return false;
        }
      }
      if (VAR === 5) {
        var reg2 = wreg(r, c);
        if (reg2 >= 0) for (i = 0; i < N; i++) for (j = 0; j < N; j++)
          if ((i !== r || j !== c) && wreg(i, j) === reg2 && g[i][j] === v) return false;
      }
      return true;
    }
    /* Backtracking fill that always expands the most constrained empty cell
     * first (minimum remaining values). Positional order sends the extra
     * constraints - diagonal especially - down enormous dead ends on the
     * bigger boards; MRV prunes those immediately. */
    var fillBudget = 0;
    function fill(g) {
      if (--fillBudget <= 0) return false;
      var br = -1, bc = -1, bestn = N + 1, r, c, v;
      for (r = 0; r < N && bestn > 1; r++)
        for (c = 0; c < N; c++) {
          if (g[r][c]) continue;
          var cnt = 0;
          for (v = 1; v <= N; v++) if (ok(g, r, c, v)) cnt++;
          if (cnt === 0) return false;
          if (cnt < bestn) { bestn = cnt; br = r; bc = c; }
          if (cnt === 1) break;
        }
      if (br < 0) return true;
      var cand = [];
      for (v = 1; v <= N; v++) if (ok(g, br, bc, v)) cand.push(v);
      shuffle(cand);
      for (var i = 0; i < cand.length; i++) {
        g[br][bc] = cand[i];
        if (fill(g)) return true;
        g[br][bc] = 0;
        if (fillBudget <= 0) return false;
      }
      return false;
    }
    function fillRestarting(g) {
      for (var a = 0; a < 60; a++) {
        for (var r = 0; r < N; r++) g[r] = new Array(N).fill(0);
        fillBudget = 200000;
        if (fill(g)) return;
      }
      VAR = 0;                                  /* fall back to plain rules */
      for (var r2 = 0; r2 < N; r2++) g[r2] = new Array(N).fill(0);
      fillBudget = 3000000;
      fill(g);
    }
    function countSol(g, pos, found, budget) {
      if (--budget.n <= 0) return -1;
      if (found >= 2) return found;
      if (pos === N * N) return found + 1;
      var r = Math.floor(pos / N), c = pos % N;
      if (g[r][c]) return countSol(g, pos + 1, found, budget);
      for (var v = 1; v <= N; v++) {
        if (!ok(g, r, c, v)) continue;
        g[r][c] = v;
        found = countSol(g, pos + 1, found, budget);
        g[r][c] = 0;
        if (found < 0 || found >= 2) break;
      }
      return found;
    }
    function reset() {
      sol = []; for (var r = 0; r < N; r++) sol.push(new Array(N).fill(0));
      fillRestarting(sol);
      grid = sol.map(function (x) { return x.slice(); });
      var order = shuffle(Array.from({ length: N * N }, function (_, i) { return i; }));
      var holes = Math.floor(N * N * HOLE / 100), removed = 0;
      for (var i = 0; i < N * N && removed < holes; i++) {
        var rr = Math.floor(order[i] / N), cc2 = order[i] % N, keep = grid[rr][cc2];
        grid[rr][cc2] = 0;
        var work = grid.map(function (x) { return x.slice(); });
        /* Uniqueness proving is exponential; give the big boards a smaller
         * budget, since an inconclusive answer just keeps the clue. */
        if (countSol(work, 0, 0, { n: N > 9 ? 8000 : 40000 }) !== 1) grid[rr][cc2] = keep;
        else removed++;
      }
      given = grid.map(function (x) { return x.map(function (v) { return v !== 0; }); });
      hints = 0; done = false; t0 = Date.now();
    }
    function complete() {
      for (var r = 0; r < N; r++) for (var c = 0; c < N; c++)
        if (grid[r][c] !== sol[r][c]) return false;
      return true;
    }
    function dchar(v) { return v <= 0 ? '.' : v <= 9 ? String(v) : String.fromCharCode(55 + v); }
    reset();
    return {
      key: function (k) {
        if (done) { reset(); return; }
        if (k === 'up' && cr > 0) cr--;
        if (k === 'down' && cr < N - 1) cr++;
        if (k === 'left' && cc > 0) cc--;
        if (k === 'right' && cc < N - 1) cc++;
        if (k === 'h' && !given[cr][cc]) { grid[cr][cc] = sol[cr][cc]; hints++; }
        if (!given[cr][cc]) {
          if (k >= '1' && k <= '9' && +k <= N) grid[cr][cc] = +k;
          if (N > 9 && k.length === 1 && k >= 'a' && k <= 'g') {
            var v = k.charCodeAt(0) - 87;
            if (v <= N) grid[cr][cc] = v;
          }
          if (k === '0' || k === 'space') grid[cr][cc] = 0;
        }
        if (complete()) {
          done = true;
          host.saveScore(Math.max(0, (p.difficulty || 3) * 1000 -
                         Math.floor((Date.now() - t0) / 1000) - hints * 50));
        }
      },
      draw: function (t) {
        t.header('SUDOKU', N + 'x' + N + ' ' + VNAME[VAR] +
                 (N > 9 ? ' · 1-9 and A-G' : ' · 1-9') + ' · H hints');
        var cw = N > 9 ? 3 : 2, left = Math.max(1, 42 - (N * cw) / 2);
        for (var r = 0; r < N; r++) {
          var y = 4 + r + Math.floor(r / BH);
          for (var c = 0; c < N; c++) {
            var x = left + c * cw + Math.floor(c / BW);
            var v = grid[r][c];
            var fg = given[r][c] ? C.white : (v && !ok(grid, r, c, v)) ? C.red : C.cyan;
            var bg = (r === cr && c === cc) ? '#2b4a6b'
                   : (VAR === 2 && sol[r][c] % 2 === 0) ? '#22262c'
                   : (VAR === 5 && wreg(r, c) >= 0) ? '#22262c' : null;
            t.text(x, y, ' ' + dchar(v) + ' ', v ? fg : C.grey, bg, given[r][c]);
          }
          if (VAR === 3) for (var c2 = 0; c2 + 1 < N; c2++) {
            var d = sol[r][c2] - sol[r][c2 + 1];
            if (d === 1 || d === -1)
              t.put(left + c2 * cw + Math.floor(c2 / BW) + cw, y, '.', C.yellow);
          }
        }
        t.text(left, 6 + N + BH, 'Hints: ' + hints + '   ', C.dim);
        if (done) t.center(8 + N + BH, 'Solved! Press any key for a new puzzle.', C.green, true);
      }
    };
  }
});

/* ---------------------------------------------------------------- sliding */
reg('sliding', {
  title: 'Sliding Puzzle', help: 'Arrows slide tiles · Q quits',
  start: function (host, p) {
    var N = Math.max(3, Math.min(7, ival(p.size, 4)));
    var t4, er, ec, moves;
    function slide(dr, dc) {
      var r = er + dr, c = ec + dc;
      if (r < 0 || r >= N || c < 0 || c >= N) return false;
      t4[er][ec] = t4[r][c]; t4[r][c] = 0; er = r; ec = c;
      return true;
    }
    function solved() {
      for (var i = 0; i < N * N - 1; i++)
        if (t4[Math.floor(i / N)][i % N] !== i + 1) return false;
      return t4[N-1][N-1] === 0;
    }
    function reset() {
      t4 = [];
      for (var r = 0; r < N; r++) {
        var row = [];
        for (var c = 0; c < N; c++) row.push((r * N + c + 1) % (N * N));
        t4.push(row);
      }
      er = ec = N - 1;
      for (var i = 0; i < N * N * 30; i++) {
        var d = rnd(4);
        slide(d === 0 ? -1 : d === 1 ? 1 : 0, d === 2 ? -1 : d === 3 ? 1 : 0);
      }
      moves = 0;
    }
    reset();
    return {
      key: function (k) {
        if (solved()) { reset(); return; }
        var ok = false;
        if (k === 'up') ok = slide(1, 0);
        if (k === 'down') ok = slide(-1, 0);
        if (k === 'left') ok = slide(0, 1);
        if (k === 'right') ok = slide(0, -1);
        if (ok) moves++;
        if (solved()) host.saveScore(Math.floor(N * N * 200 / (moves + 1)));
      },
      draw: function (t) {
        t.header('SLIDING PUZZLE', N + 'x' + N + ' · arrows slide tiles into the gap');
        var w = N * N > 25 ? 4 : 3, left = Math.max(1, 42 - (N * w) / 2);
        for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) {
          var v = t4[r][c];
          var s = v ? String(v).padStart(w - 1, ' ') : ' '.repeat(w - 1);
          t.text(left + c * w, 5 + r * 2, s + ' ', v ? C.white : C.grey, v ? '#2b4a6b' : null, true);
        }
        t.text(left, N * 2 + 6, 'Moves: ' + moves + '   ', C.fg);
        if (solved()) t.center(N * 2 + 8, 'Solved! Press any key.', C.green, true);
      }
    };
  }
});

/* ------------------------------------------------------------- lights out */
reg('lightsout', {
  title: 'Lights Out', help: 'Arrows move · Enter presses · Q quits',
  start: function (host, p) {
    var N = Math.max(3, Math.min(8, ival(p.size, 5))), VAR = p.variant || 0;
    var VNAME = ['plus', 'diagonal', 'row and column'];
    var on, cr, cc, moves;
    function toggle(r, c) {
      var i, DR, DC;
      if (VAR === 2) {
        for (i = 0; i < N; i++) on[r][i] ^= 1;
        for (i = 0; i < N; i++) if (i !== r) on[i][c] ^= 1;
        return;
      }
      DR = VAR === 1 ? [0,-1,-1,1,1] : [0,-1,1,0,0];
      DC = VAR === 1 ? [0,-1,1,-1,1] : [0,0,0,-1,1];
      for (i = 0; i < 5; i++) {
        var a = r + DR[i], b = c + DC[i];
        if (a >= 0 && a < N && b >= 0 && b < N) on[a][b] ^= 1;
      }
    }
    function allOff() { return on.every(function (r) { return r.every(function (v) { return !v; }); }); }
    function reset() {
      on = []; for (var r = 0; r < N; r++) on.push(new Array(N).fill(0));
      for (var i = 0; i < N * N / 2 + 4; i++) toggle(rnd(N), rnd(N));
      if (allOff()) toggle(rnd(N), rnd(N));
      cr = N >> 1; cc = N >> 1; moves = 0;
    }
    reset();
    return {
      key: function (k) {
        if (allOff()) { reset(); return; }
        if (k === 'up' && cr > 0) cr--;
        if (k === 'down' && cr < N - 1) cr++;
        if (k === 'left' && cc > 0) cc--;
        if (k === 'right' && cc < N - 1) cc++;
        if (k === 'enter' || k === 'space') {
          toggle(cr, cc); moves++;
          if (allOff()) host.saveScore(Math.floor(N * N * 20 / (moves + 1)));
        }
      },
      draw: function (t) {
        t.header('LIGHTS OUT', N + 'x' + N + ', ' + VNAME[VAR] + ' toggle · Enter presses');
        var left = Math.max(1, 42 - (N * 3) / 2);
        for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) {
          var sel = r === cr && c === cc;
          t.put(left + c * 3, 5 + r, on[r][c] ? 'O' : '.',
                on[r][c] ? C.black : C.grey,
                on[r][c] ? C.yellow : (sel ? '#3a4048' : null), true);
          if (sel) t.put(left + c * 3 - 1, 5 + r, '[', C.cyan);
        }
        t.text(left, N + 7, 'Moves: ' + moves + '   ', C.fg);
        if (allOff()) t.center(N + 9, 'All lights out! Press any key.', C.green, true);
      }
    };
  }
});

/* ---------------------------------------------------------------- flood it */
reg('floodit', {
  title: 'Flood It', help: 'Left/Right pick colour · Enter floods · Q quits',
  start: function (host, p) {
    var N = Math.max(4, Math.min(18, ival(p.size, 12)));
    var COLOURS = Math.max(3, Math.min(8, ival(p.count, 6)));
    var MOVES = ival(p.level, 22);
    var CC = [C.red, C.green, C.blue, C.yellow, C.magenta, C.cyan, C.white, C.grey];
    var g, cur, left, over;
    function reset() {
      g = [];
      for (var r = 0; r < N; r++) {
        var row = [];
        for (var c = 0; c < N; c++) row.push(rnd(COLOURS));
        g.push(row);
      }
      cur = 0; left = MOVES; over = null;
    }
    function flood(from, to) {
      var stack = [[0, 0]];
      while (stack.length) {
        var q = stack.pop(), y = q[0], x = q[1];
        if (y < 0 || y >= N || x < 0 || x >= N || g[y][x] !== from) continue;
        g[y][x] = to;
        stack.push([y+1,x],[y-1,x],[y,x+1],[y,x-1]);
      }
    }
    function owned() {
      var seen = [], stack = [[0,0]], n = 0, col = g[0][0];
      for (var r = 0; r < N; r++) seen.push(new Array(N).fill(0));
      seen[0][0] = 1;
      while (stack.length) {
        var q = stack.pop(), y = q[0], x = q[1];
        n++;
        [[y+1,x],[y-1,x],[y,x+1],[y,x-1]].forEach(function (z) {
          if (z[0] < 0 || z[0] >= N || z[1] < 0 || z[1] >= N) return;
          if (seen[z[0]][z[1]] || g[z[0]][z[1]] !== col) return;
          seen[z[0]][z[1]] = 1; stack.push(z);
        });
      }
      return n;
    }
    reset();
    return {
      key: function (k) {
        if (over) { reset(); return; }
        if (k === 'left') cur = (cur + COLOURS - 1) % COLOURS;
        if (k === 'right') cur = (cur + 1) % COLOURS;
        if (k !== 'enter' && k !== 'space') return;
        if (cur === g[0][0]) return;
        flood(g[0][0], cur);
        left--;
        if (owned() === N * N) { over = 'Board flooded — you win!'; host.saveScore(left * 50); }
        else if (left === 0) over = 'Out of moves.';
      },
      draw: function (t) {
        t.header('FLOOD IT', N + 'x' + N + ', ' + COLOURS + ' colours, ' + MOVES + ' moves');
        var left0 = Math.max(1, 42 - N);
        for (var r = 0; r < N; r++) for (var c = 0; c < N; c++)
          t.text(left0 + c * 2, 4 + r, '##', CC[g[r][c]]);
        t.text(left0, N + 5, 'Colour: ', C.fg);
        for (var i = 0; i < COLOURS; i++)
          t.text(left0 + 8 + i * 3, N + 5, '##', CC[i], i === cur ? '#4a5058' : null);
        t.text(left0, N + 6, 'Moves left: ' + left + '   Filled: ' + owned() + '/' + (N * N) + '   ', C.fg);
        if (over) t.center(N + 8, over + ' Press any key.', C.white, true);
      }
    };
  }
});

/* ------------------------------------------------------------------ merge */
reg('merge', {
  title: 'Merge', help: 'Arrows slide · Q quits',
  start: function (host, p) {
    var N = Math.max(3, Math.min(8, ival(p.size, 4))), VAR = p.variant || 0;
    var VNAME = ['doubling', 'tripling', 'Fibonacci', 'Threes'];
    var g, score, over;
    function combine(a, b) {
      if (!a || !b) return 0;
      if (VAR === 1) return 0;
      if (VAR === 2) {
        var x = Math.min(a, b), y = Math.max(a, b), f0 = 1, f1 = 1;
        while (f1 < x) { var tt = f0 + f1; f0 = f1; f1 = tt; }
        if (f1 !== x) return 0;
        if (f0 + f1 === y || (x === y && x === 1)) return x + y;
        return 0;
      }
      if (VAR === 3) {
        if ((a === 1 && b === 2) || (a === 2 && b === 1)) return 3;
        if (a === b && a >= 3) return a + b;
        return 0;
      }
      return a === b ? a + b : 0;
    }
    function slideLine(line) {
      var tmp = line.filter(function (v) { return v; }), moved = false, i;
      while (tmp.length < N) tmp.push(0);
      if (VAR === 1) {
        for (i = 0; i + 2 < N; i++)
          if (tmp[i] && tmp[i] === tmp[i+1] && tmp[i+1] === tmp[i+2]) {
            tmp[i] *= 3; score += tmp[i];
            tmp.splice(i + 1, 2); tmp.push(0, 0);
          }
      } else {
        for (i = 0; i + 1 < N; i++) {
          var m = combine(tmp[i], tmp[i+1]);
          if (m) { tmp[i] = m; score += m; tmp.splice(i + 1, 1); tmp.push(0); }
        }
      }
      for (i = 0; i < N; i++) { if (line[i] !== tmp[i]) moved = true; line[i] = tmp[i]; }
      return moved;
    }
    function move(dir) {
      var moved = false, i, j, line;
      for (i = 0; i < N; i++) {
        line = [];
        for (j = 0; j < N; j++)
          line.push(dir === 0 ? g[i][j] : dir === 1 ? g[i][N-1-j] : dir === 2 ? g[j][i] : g[N-1-j][i]);
        if (slideLine(line)) moved = true;
        for (j = 0; j < N; j++) {
          if (dir === 0) g[i][j] = line[j];
          else if (dir === 1) g[i][N-1-j] = line[j];
          else if (dir === 2) g[j][i] = line[j];
          else g[N-1-j][i] = line[j];
        }
      }
      return moved;
    }
    function spawn() {
      var free = [];
      for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) if (!g[r][c]) free.push([r, c]);
      if (!free.length) return;
      var q = free[rnd(free.length)];
      g[q[0]][q[1]] = VAR === 3 ? rnd(3) + 1 : VAR === 2 ? 1 : VAR === 1 ? 3 : (rnd(10) === 0 ? 4 : 2);
    }
    function canMove() {
      for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) if (!g[r][c]) return true;
      var save = g.map(function (x) { return x.slice(); }), d, m;
      for (d = 0; d < 4; d++) {
        g = save.map(function (x) { return x.slice(); });
        m = move(d);
        if (m) { g = save; return true; }
      }
      g = save;
      return false;
    }
    function colour(v) {
      return v <= 2 ? C.white : v <= 4 ? C.cyan : v <= 8 ? C.green : v <= 16 ? C.yellow
           : v <= 32 ? C.magenta : v <= 64 ? C.red : C.yellow;
    }
    function reset() {
      g = []; for (var r = 0; r < N; r++) g.push(new Array(N).fill(0));
      score = 0; over = false; spawn(); spawn();
    }
    reset();
    return {
      key: function (k) {
        if (over) { reset(); return; }
        var moved = false;
        if (k === 'left') moved = move(0);
        if (k === 'right') moved = move(1);
        if (k === 'up') moved = move(2);
        if (k === 'down') moved = move(3);
        if (moved) spawn();
        if (!canMove()) { over = true; host.saveScore(score); }
      },
      draw: function (t) {
        t.header('MERGE', N + 'x' + N + ', ' + VNAME[VAR] + ' merges · arrows slide');
        var left = Math.max(1, 42 - (N * 7) / 2);
        for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) {
          var v = g[r][c];
          t.text(left + c * 7, 5 + r * 2, v ? String(v).padStart(6, ' ') : '     .',
                 v ? colour(v) : C.grey, null, v >= 64);
        }
        t.text(left, N * 2 + 6, 'Score: ' + score + '   Best: ' + host.best() + '   ', C.fg);
        if (over) t.center(N * 2 + 8, 'No moves left. Press any key.', C.red, true);
      }
    };
  }
});

/* ----------------------------------------------------------------- memory */
reg('memory', {
  title: 'Memory Match', help: 'Arrows move · Enter flips · Q quits',
  start: function (host, p) {
    var PAIRS = Math.max(4, Math.min(32, ival(p.count, 12)));
    var SYM = 'ABCDEFGHJKLMNPRSTUVWXYZ23456789'.split('');
    var SC = [C.red, C.green, C.yellow, C.blue, C.magenta, C.cyan, C.white, C.green];
    var COLS = 8, ROWS;
    for (var c0 = 8; c0 >= 4; c0--) if ((PAIRS * 2) % c0 === 0) { COLS = c0; break; }
    ROWS = Math.ceil(PAIRS * 2 / COLS);
    var card, found, cr, cc, first, second, turns, matched;
    function reset() {
      var deck = [];
      for (var i = 0; i < PAIRS * 2; i++) deck.push(i >> 1);
      shuffle(deck);
      card = deck; found = new Array(PAIRS * 2).fill(0);
      cr = cc = 0; first = second = -1; turns = 0; matched = 0;
    }
    reset();
    return {
      key: function (k) {
        if (matched === PAIRS) { reset(); return; }
        if (second >= 0) {
          if (card[first] === card[second]) {
            found[first] = found[second] = 1;
            matched++;
            if (matched === PAIRS) host.saveScore(Math.floor(PAIRS * 200 / turns));
          }
          first = second = -1;
          return;
        }
        if (k === 'up' && cr > 0) cr--;
        if (k === 'down' && cr < ROWS - 1) cr++;
        if (k === 'left' && cc > 0) cc--;
        if (k === 'right' && cc < COLS - 1) cc++;
        if (k !== 'enter' && k !== 'space') return;
        var idx = cr * COLS + cc;
        if (idx >= PAIRS * 2 || found[idx] || first === idx) return;
        if (first < 0) first = idx; else { second = idx; turns++; }
      },
      draw: function (t) {
        t.header('MEMORY MATCH', PAIRS + ' pairs · arrows move · Enter flips');
        var left = Math.max(1, 42 - COLS * 2);
        for (var r = 0; r < ROWS; r++) for (var c = 0; c < COLS; c++) {
          var idx = r * COLS + c;
          if (idx >= PAIRS * 2) continue;
          var face = found[idx] || idx === first || idx === second;
          t.put(left + c * 4, 4 + r * 2, face ? SYM[card[idx]] : '.',
                face ? SC[card[idx] % 8] : C.grey,
                (r === cr && c === cc) ? '#2b4a6b' : null, true);
        }
        t.text(left, ROWS * 2 + 5, 'Turns: ' + turns + '   Pairs: ' + matched + '/' + PAIRS + '   ', C.fg);
        if (second >= 0) t.center(ROWS * 2 + 7, 'Press any key to continue', C.dim);
        if (matched === PAIRS) t.center(ROWS * 2 + 7, 'All pairs found! Press any key.', C.green, true);
      }
    };
  }
});

/* ------------------------------------------------------------------ hanoi */
reg('hanoi', {
  title: 'Towers of Hanoi', help: 'Left/Right pick peg · Enter lifts or drops',
  start: function (host, p) {
    var D = Math.max(3, Math.min(10, ival(p.size, 5)));
    var peg, cur, from, moves;
    function reset() {
      peg = [[], [], []];
      for (var i = D; i >= 1; i--) peg[0].push(i);
      cur = 0; from = -1; moves = 0;
    }
    reset();
    return {
      key: function (k) {
        if (peg[2].length === D) { reset(); return; }
        if (k === 'left') cur = (cur + 2) % 3;
        if (k === 'right') cur = (cur + 1) % 3;
        if (k !== 'enter' && k !== 'space') return;
        if (from < 0) { if (peg[cur].length) from = cur; return; }
        if (from === cur) { from = -1; return; }
        var d = peg[from][peg[from].length - 1];
        var top = peg[cur][peg[cur].length - 1];
        if (top && top < d) return;
        peg[cur].push(peg[from].pop());
        moves++; from = -1;
        if (peg[2].length === D) host.saveScore(Math.floor(D * 1000 / (moves + 1)));
      },
      draw: function (t) {
        t.header('TOWERS OF HANOI', D + ' discs · optimal ' + ((1 << D) - 1) + ' moves');
        var base = 5 + D;
        for (var pg = 0; pg < 3; pg++) {
          var cx = 20 + pg * 22;
          for (var i = 0; i < D; i++) {
            var y = base - 1 - i, w = peg[pg][i] || 0;
            if (w) t.text(cx - w, y, '='.repeat(w * 2 - 1), w % 2 ? C.cyan : C.magenta);
            else t.put(cx, y, '|', C.grey);
          }
          t.text(cx - 1, base, ' ' + String.fromCharCode(65 + pg) + ' ',
                 pg === cur ? C.white : C.dim, pg === cur ? '#3a4048' : null, pg === cur);
        }
        t.text(20, base + 2, 'Moves: ' + moves + '   ', C.fg);
        t.text(20, base + 3, from >= 0 ? 'Holding a disc from peg ' + String.fromCharCode(65 + from) + '  '
                                       : '                                 ', C.yellow);
        if (peg[2].length === D) t.center(base + 5, 'Solved in ' + moves + ' moves! Press any key.', C.green, true);
      }
    };
  }
});

/* --------------------------------------------------------------- nonogram */
reg('nonogram', {
  title: 'Nonogram', help: 'Enter paints · X crosses · Q quits',
  start: function (host, p) {
    var N = Math.max(5, Math.min(30, ival(p.size, 10))), VAR = p.variant ? 1 : 0;
    var NC = 3, PAINT = [C.grey, C.white, C.cyan, C.yellow];
    var sol, mark, rowclue, colclue, cr, cc, pen, done, t0;
    function runs(line) {
      var out = [], run = 0, col = 0;
      line.forEach(function (v) {
        if (v && v === col) run++;
        else { if (run) out.push([run, col]); run = v ? 1 : 0; col = v; }
      });
      if (run) out.push([run, col]);
      return out.length ? out : [[0, 1]];
    }
    function reset() {
      sol = [];
      for (var r = 0; r < N; r++) {
        var row = [];
        for (var c = 0; c < N; c++) row.push(rnd(100) < 55 ? (VAR ? 1 + rnd(NC) : 1) : 0);
        sol.push(row);
      }
      rowclue = sol.map(runs);
      colclue = [];
      for (var c2 = 0; c2 < N; c2++)
        colclue.push(runs(sol.map(function (row) { return row[c2]; })));
      mark = []; for (var r2 = 0; r2 < N; r2++) mark.push(new Array(N).fill(0));
      cr = cc = 0; pen = 1; done = false; t0 = Date.now();
    }
    function correct() {
      for (var r = 0; r < N; r++) for (var c = 0; c < N; c++)
        if (sol[r][c] !== (mark[r][c] > 0 ? mark[r][c] : 0)) return false;
      return true;
    }
    reset();
    return {
      key: function (k) {
        if (done) { reset(); return; }
        if (k === 'up' && cr > 0) cr--;
        if (k === 'down' && cr < N - 1) cr++;
        if (k === 'left' && cc > 0) cc--;
        if (k === 'right' && cc < N - 1) cc++;
        if (VAR && k >= '1' && k <= String(NC)) pen = +k;
        if (k === 'enter' || k === 'space') mark[cr][cc] = mark[cr][cc] === pen ? 0 : pen;
        if (k === 'x') mark[cr][cc] = mark[cr][cc] < 0 ? 0 : -1;
        if (correct()) { done = true; host.saveScore(Math.max(0, N * N * 4 - Math.floor((Date.now() - t0) / 1000))); }
      },
      draw: function (t) {
        t.header('NONOGRAM', N + 'x' + N + ' ' + (VAR ? 'colour' : 'mono') +
                 ' · Enter paints · X crosses' + (VAR ? ' · 1-3 colour' : ''));
        var maxcol = 0, maxrow = 0, i;
        for (i = 0; i < N; i++) {
          maxcol = Math.max(maxcol, colclue[i].length);
          maxrow = Math.max(maxrow, rowclue[i].map(function (q) { return String(q[0]); }).join(' ').length);
        }
        var left = maxrow + 2, top = 4 + maxcol;
        for (var k2 = 0; k2 < maxcol; k2++)
          for (var j = 0; j < N; j++) {
            var idx = k2 - (maxcol - colclue[j].length);
            if (idx < 0) continue;
            t.text(left + j * 2, 4 + k2, String(colclue[j][idx][0]).padStart(2, ' '),
                   PAINT[VAR ? colclue[j][idx][1] : 1]);
          }
        for (var r = 0; r < N; r++) {
          var s = rowclue[r].map(function (q) { return q[0]; }).join(' ');
          t.text(left - s.length - 1, top + r, s, C.cyan);
          for (var c = 0; c < N; c++) {
            var m = mark[r][c], sel = r === cr && c === cc;
            t.text(left + c * 2, top + r, m > 0 ? '##' : m < 0 ? ' x' : ' .',
                   m > 0 ? PAINT[m] : m < 0 ? C.red : C.grey, sel ? '#2b4a6b' : null);
          }
        }
        if (VAR) {
          t.text(left, top + N + 1, 'Pen: ', C.fg);
          for (i = 1; i <= NC; i++)
            t.text(left + 5 + (i - 1) * 3, top + N + 1, '##', PAINT[i], i === pen ? '#4a5058' : null);
        }
        if (done) t.center(top + N + 3, 'Picture complete! Press any key.', C.green, true);
      }
    };
  }
});

/* ---------------------------------------------------------------- sokoban */
reg('sokoban', {
  title: 'Sokoban', help: 'Arrows push · U undo · R restart · Q quits',
  start: function (host, p) {
    var LEVELS = [
      ["    #####          ","    #   #          ","    #$  #          ",
       "  ###  $##         ","  #  $ $ #         ","### # ## #   ######",
       "#   # ## #####  ..#","# $  $          ..#","##### ### #@##  ..#",
       "    #     #########","    #######        "],
      ["############  ","#..  #     ###","#..  # $  $  #","#..  #$####  #",
       "#..    @ ##  #","#..  # #  $ ##","###### ##$ $ #","  # $  $ $ $ #",
       "  #    #     #","  ############"],
      ["        ######## ","        #     @# ","        # $#$ ## ","        # $  $#  ",
       "        ##$ $ #  ","######### $ # ###","#....  ## $  $  #","##...    $  $   #",
       "#....  ##########","########         "],
      ["  ####       ","  #  ###     ","  #    #     ","  # $$ #     ",
       "### ## ##### ","#  $ $     # ","# @ $ $$$  # ","#### ..... # ",
       "   # ..... # ","   #########"],
      ["#####    ","#   #####","# $ #   #","# $$# $ #","#  .. $ #","##..@####",
       " #..$#   "," #####   "],
      ["####################","#..    #           #","#..    # $  $      #",
       "#..    #$###    ## #","#..      @ ##   $  #","#..    # #  $ $ #  #",
       "#..    # #  $   #  #","#....### ###$#### ##","#      #        #  #",
       "####################"]
    ];
    var lv = Math.max(0, Math.min(LEVELS.length - 1, p.level || 0));
    var map, box, pr, pc, rows, cols, moves, undo, cleared;
    function load() {
      var L = LEVELS[lv];
      rows = L.length; cols = 0;
      map = []; box = [];
      for (var r = 0; r < rows; r++) {
        cols = Math.max(cols, L[r].length);
        map.push([]); box.push([]);
        for (var c = 0; c < L[r].length; c++) {
          var ch = L[r][c];
          map[r].push(ch === '#' ? '#' : (ch === '.' || ch === '*' || ch === '+') ? '.' : ' ');
          box[r].push(ch === '$' || ch === '*' ? '$' : ' ');
          if (ch === '@' || ch === '+') { pr = r; pc = c; }
        }
      }
      moves = 0; undo = []; cleared = false;
    }
    function at(g, r, c) { return (g[r] && g[r][c]) || ' '; }
    function solved() {
      for (var r = 0; r < rows; r++) for (var c = 0; c < box[r].length; c++)
        if (box[r][c] === '$' && map[r][c] !== '.') return false;
      return true;
    }
    function step(dr, dc) {
      var nr = pr + dr, nc = pc + dc;
      if (at(map, nr, nc) === '#') return;
      if (at(box, nr, nc) === '$') {
        var br = nr + dr, bc = nc + dc;
        if (at(map, br, bc) === '#' || at(box, br, bc) === '$') return;
        undo.push({ box: box.map(function (x) { return x.slice(); }), pr: pr, pc: pc });
        box[nr][nc] = ' '; box[br][bc] = '$';
      } else undo.push({ box: box.map(function (x) { return x.slice(); }), pr: pr, pc: pc });
      pr = nr; pc = nc; moves++;
    }
    load();
    return {
      key: function (k) {
        if (cleared) { lv = (lv + 1) % LEVELS.length; load(); return; }
        if (k === 'r') { load(); return; }
        if (k === 'u') {
          var u = undo.pop();
          if (u) { box = u.box; pr = u.pr; pc = u.pc; if (moves) moves--; }
          return;
        }
        if (k === 'up') step(-1, 0);
        if (k === 'down') step(1, 0);
        if (k === 'left') step(0, -1);
        if (k === 'right') step(0, 1);
        if (solved()) { cleared = true; host.saveScore((lv + 1) * 200 - moves); }
      },
      draw: function (t) {
        t.header('SOKOBAN', 'Level ' + (lv + 1) + ' of ' + LEVELS.length +
                 ' · arrows push · U undo · R restart');
        for (var r = 0; r < rows; r++) for (var c = 0; c < cols; c++) {
          var ch = ' ', fg = C.grey;
          if (r === pr && c === pc) { ch = '@'; fg = C.cyan; }
          else if (at(box, r, c) === '$') {
            ch = at(map, r, c) === '.' ? '*' : '$';
            fg = at(map, r, c) === '.' ? C.green : C.yellow;
          } else if (at(map, r, c) === '#') { ch = '#'; fg = C.blue; }
          else if (at(map, r, c) === '.') { ch = '.'; fg = C.green; }
          t.put(30 + c, 4 + r, ch, fg, null, ch === '@' || ch === '*');
        }
        t.text(30, rows + 5, 'Moves: ' + moves + '   ', C.fg);
        if (cleared) t.center(rows + 7, 'Level complete! Press any key for the next.', C.green, true);
      }
    };
  }
});

}());
